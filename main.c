#include <math.h>
#include <spa/param/audio/format-utils.h>
#include <pipewire/pipewire.h>
#include <stdint.h>
#include <stdio.h>
#include <raylib.h>
#include <complex.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#define FRAMES_PER_BUFFER 1024
#define RING_BUFFER_SIZE (FRAMES_PER_BUFFER * 1+1)

/* ATOMIC RINGBUFFER*/

typedef struct {
    float *buffer;
    int size;
    atomic_int readPos;
    atomic_int writePos;
} AtomicRingBuffer ;

static void initAtomicRingBuffer(AtomicRingBuffer *rb, int size) {
    rb->buffer = (float*)calloc(size, sizeof(float));
    rb->size = size;
    atomic_init(&rb->readPos, 0);
    atomic_init(&rb->writePos, 0);
}

static void freeAtomicRingBuffer(AtomicRingBuffer *rb) {
    free(rb->buffer);
}

static bool writeAtomicRingBuffer(AtomicRingBuffer *rb, float *data, int numSamples) {
    int readPos = atomic_load_explicit(&rb->readPos, memory_order_acquire);
    int writePos = atomic_load_explicit(&rb->writePos, memory_order_relaxed);
    
    int available = (readPos > writePos) ? (readPos - writePos) : (rb->size - writePos + readPos);

    if (available < numSamples) return false;

    int spaceToEnd = rb->size - writePos;

    if (spaceToEnd >= numSamples) {
        memcpy(&rb->buffer[writePos], data, numSamples * sizeof(float));
    } else {
        memcpy(&rb->buffer[writePos], data, spaceToEnd * sizeof(float));
        memcpy(&rb->buffer[0], &data[spaceToEnd], (numSamples - spaceToEnd) * sizeof(float));
    }

    atomic_store_explicit(&rb->writePos, (writePos + numSamples) % rb->size, memory_order_release);
    return true;
}

static bool readAtomicRingBuffer(AtomicRingBuffer *rb, float *data, int numSamples) {
    int readPos = atomic_load_explicit(&rb->readPos, memory_order_relaxed);
    int writePos = atomic_load_explicit(&rb->writePos, memory_order_acquire);

    int available = (writePos >= readPos) ? (writePos - readPos) : (rb->size -readPos + writePos);

    if (available < numSamples) return false;

    int spaceToEnd = rb->size - readPos;

    if (spaceToEnd >= numSamples) {
        memcpy(data, &rb->buffer[readPos], numSamples * sizeof(float));
    } else {
        memcpy(data, &rb->buffer[readPos], spaceToEnd * sizeof(float));
        memcpy(&data[spaceToEnd], &rb->buffer[0], (numSamples - spaceToEnd) * sizeof(float));
    }

    atomic_store_explicit(&rb->readPos, (readPos + numSamples) % rb->size, memory_order_release);
    return true;
}

AtomicRingBuffer rb;

/* ATOMIC RINGBUFFER END*/

/* FFT  */

void fft(float* x, size_t n, size_t s, float complex* X){
    if (n == 1) {
        X[0] = x[0];
    } else {
        fft(x, n/2, 2*s, X);
        fft(x+s, n/2, 2*s, X + n/2);

        for (size_t k = 0; k<n/2; k++) {
            float complex tmp = X[k];
            float complex t = cexp(-2.0f * I * (float)M_PI * k / n) * X[k + n/2];
            X[k] = tmp + t;
            X[k + n/2] = tmp - t;
        }
    }
}

/* FFT END*/


typedef struct {
    struct pw_main_loop* loop;
    struct pw_stream* stream;

    struct spa_audio_info format;
} Data;

static void on_process(void* userdata){
    Data* data = (Data*)userdata;
    struct pw_buffer* b;
    struct spa_buffer *buf;
    float *samples, max;
    uint32_t c, n, n_channels, n_samples, peak;
    float *mono_buffer = malloc(FRAMES_PER_BUFFER * sizeof(float));


    if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL) {
        pw_log_warn("out of buffers: %m");
        return;
    }
    buf = b->buffer;
    if ((samples = buf->datas[0].data) == NULL) {
        return;
    }

    n_channels = data->format.info.raw.channels;
    n_samples = buf->datas[0].chunk->size / sizeof(float);
    
    uint32_t mono_sample_count = n_samples / n_channels;

    for (c = 0; c<mono_sample_count;c++ ) {
        mono_buffer[c] = samples[c * n_channels];
    }
    writeAtomicRingBuffer(&rb, mono_buffer, FRAMES_PER_BUFFER);
    pw_stream_queue_buffer(data->stream, b);

}

static void on_stream_param_changed(void* userdata, uint32_t id, const struct spa_pod* param) {
    Data* data = (Data*)userdata;

    if (param == NULL || id != SPA_PARAM_Format) {
        return;
    }

    if (spa_format_parse(param, &data->format.media_type, &data->format.media_subtype) < 0) {
        return;
    }

    if (data->format.media_type != SPA_MEDIA_TYPE_audio || 
        data->format.media_subtype != SPA_MEDIA_SUBTYPE_raw) {
            return;
        }
    spa_format_audio_raw_parse(param, &data->format.info.raw);
    
    fprintf(stdout, "capturing rate:%d channels:%d\n",
                        data->format.info.raw.rate, data->format.info.raw.channels);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .param_changed = on_stream_param_changed,
    .process = on_process,
};

static void do_quit(void* userdata, int signal_number) {
    (void)signal_number;
    Data* data = (Data*)userdata;
    pw_main_loop_quit(data->loop);
}

void* run_audio_loop(void *userdata) {

    (void)userdata;
    Data data = {0,};

    const struct spa_pod *params[1];
    uint8_t buffer[FRAMES_PER_BUFFER];
    struct pw_properties* props;
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    data.loop = pw_main_loop_new(NULL);

    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGINT, do_quit, &data);
    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGTERM, do_quit, &data);

    props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                              PW_KEY_CONFIG_NAME, "client-rt.conf",
                              PW_KEY_MEDIA_CATEGORY, "Capture",
                              PW_KEY_MEDIA_ROLE, "Music",
                              NULL);

    data.stream = pw_stream_new_simple(pw_main_loop_get_loop(data.loop), 
                                       "audio-capture", 
                                       props, 
                                       &stream_events, 
                                       &data);

    
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
                                           &SPA_AUDIO_INFO_RAW_INIT(
                                           .format = SPA_AUDIO_FORMAT_F32));    
    pw_stream_connect(data.stream, 
                      PW_DIRECTION_INPUT, 
                      PW_ID_ANY, 
                      PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS, 
                      params,
                      1);
    pw_main_loop_run(data.loop);

    pw_stream_destroy(data.stream);
    pw_main_loop_destroy(data.loop);
    pw_deinit();
    return NULL;
}

int main(int argc, char *argv[]) {

    float buffer[FRAMES_PER_BUFFER];
    float complex spectral_out[FRAMES_PER_BUFFER];

    initAtomicRingBuffer(&rb, RING_BUFFER_SIZE);

    pw_init(&argc, &argv);

    pthread_t audioThread;
    pthread_create(&audioThread, NULL, run_audio_loop, NULL);
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    Vector2* points = (Vector2*)malloc(FRAMES_PER_BUFFER/2*sizeof(Vector2));
    int x = FRAMES_PER_BUFFER;
    int y = 600;

    InitWindow(x, y, "julius - Audio Analyzing and Visulizing");
    
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        int currentScreenWidth = GetScreenWidth();
        int currentScreenHeight = GetScreenHeight();        
        Rectangle audioWidget = {
            .x = 50,
            .y = 50,
            .width = currentScreenWidth - 100,
            .height = currentScreenHeight - 100
        };

        readAtomicRingBuffer(&rb, buffer, FRAMES_PER_BUFFER);
        fft(buffer, FRAMES_PER_BUFFER, 1, spectral_out);

        float maxMagnitude = 0.0f;

        // First pass: determine the maximum magnitude for normalization
        for (int i = 0; i < FRAMES_PER_BUFFER/2; i++) {
            float mag = cabsf(spectral_out[i]);
            if (mag > maxMagnitude) {
                maxMagnitude = mag;
            }
        }

        for (int i = 0; i<FRAMES_PER_BUFFER/2; i++) {
            float normalizedX = (float)i / ((FRAMES_PER_BUFFER/2.0f) - 1);
            points[i].x = audioWidget.x + normalizedX * audioWidget.width;

            //float normalizedY = (buffer[i] + 1.0f) / 2.0f;
            //float magnitude = 20 * log10f(cabsf(spectral_out[i]) + 1e-6f);
            float magnitude = cabsf(spectral_out[i]);
            float normalizedMagnitude = (maxMagnitude > 0) ? magnitude / maxMagnitude : 0;
            points[i].y = audioWidget.y + (1.0f - normalizedMagnitude) * audioWidget.height;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawFPS(0, 0);
        DrawRectangleLines(audioWidget.x, audioWidget.y, audioWidget.width, audioWidget.height, LIGHTGRAY);
        DrawLineStrip(points, FRAMES_PER_BUFFER/2, RED);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
