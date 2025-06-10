#include <glib.h>
#include <gst/gst.h>
#include "glib-object.h"
#include "gst/gstcaps.h"
#include "gst/gstelement.h"
#include "gst/gstelementfactory.h"
#include "network.h"
#include "media.h"


static void handle_message (GstBus *bus, GstMessage *msg, gpointer user_data)
{
  GError *err  = NULL;
  gchar  *debug = NULL;

  switch (GST_MESSAGE_TYPE (msg)) {
  case GST_MESSAGE_ERROR:
    gst_message_parse_error (msg, &err, &debug);
    g_printerr ("ERROR: %s\n", err->message);
    g_error_free (err);
    g_free (debug);
    gst_element_set_state (GST_ELEMENT (user_data), GST_STATE_NULL);
    break;
  case GST_MESSAGE_EOS:
    g_print ("Reached end-of-stream\n");
    gst_element_set_state (GST_ELEMENT (user_data), GST_STATE_NULL);
    break;
  default:
    break;
  }
}

gchar *get_invite_sdp() {
    SdpBuilder *b = sdp_builder_new();

    // TODO: HANDLE THE IP PROPERLY THIS IS A MEMORY LEAK
    sdp_builder_set_origin(b, "markus-sip", get_host_ipv4());
    sdp_builder_set_session_name(b, "Julius Call");
    sdp_builder_set_connection(b, get_host_ipv4(),0 ,0);
    sdp_builder_set_bandwidth(b, "TIAS", 6128000);
    
    /* audioline */
    const guint8 opus_pt = 98;
    sdp_builder_add_media(b, "audio", 55000, 1, "RTP/AVP", &opus_pt, 1);
    sdp_builder_add_media_bandwidth(b, 0, "TIAS", 128000);
    sdp_builder_add_media_attribute(b, 0, "rtpmap", "98 opus/48000/2");
    sdp_builder_add_media_attribute(b, 0, "fmtp", "114 maxaveragebitrate=128000;stereo=1");

    /* videoline */
    const guint8 h264_pt = 97;
    sdp_builder_add_media(b, "video", 55002, 1, "RTP/AVP", &h264_pt, 1);
    sdp_builder_add_media_bandwidth(b, 1, "TIAS", 6000000);
    sdp_builder_add_media_attribute(b, 1, "rtpmap", "97 H264/90000");
    sdp_builder_add_media_attribute(b, 1, "fmtp", "97 packetization-mode=1;profile-level-id=428014;max-br=2500;max-mbps=245000;max-fs=8160;max-dpb=16320;max-smbps=245000;max-fps=3000");

    return sdp_builder_to_string(b);
}

void process_offer(const gchar *offer) {

}

int process_answer(const gchar* answer) {

    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstElement *src = gst_element_factory_make("udpsrc", "netsrc");
    GstElement *caps = gst_element_factory_make("capsfilter", "caps");
    GstElement *depay = gst_element_factory_make("rtph264depay", "depayloader");
    GstElement *parse = gst_element_factory_make("h264parse", "parse");
    GstElement *decode = gst_element_factory_make("openh264dec", "decode");
    GstElement *convert = gst_element_factory_make("videoconvert", "convert");
    GstElement *sink = gst_element_factory_make("autovideosink", "blackhole");

    if (!pipeline || !src || !sink) {
        g_printerr ("Failed to create one or more elements\n");
        return -1;
    }


    GstCaps *caps_filter = gst_caps_from_string("application/x-rtp, media=video, payload=96, clock-rate=90000, encoding-name=H264");

    g_object_set (src,  "port", 55002, NULL);
    g_object_set (caps, "caps", caps_filter, NULL);
    //g_object_set (sink, "sync", FALSE, 
    //                   "dump", TRUE,  NULL);

    gst_bin_add_many (GST_BIN (pipeline), src, caps, depay, parse, decode, convert, sink, NULL);
    if (!gst_element_link (src, caps)) {
      g_printerr ("Elements could not be linked\n");
      gst_object_unref (pipeline);
      return -1;
    }
    if (!gst_element_link (caps, depay)) {
      g_printerr ("Elements could not be linked\n");
      gst_object_unref (pipeline);
      return -1;
    }
    if (!gst_element_link (depay, parse)) {
      g_printerr ("Elements could not be linked\n");
      gst_object_unref (pipeline);
      return -1;
    }
    if (!gst_element_link (parse, decode)) {
      g_printerr ("Elements could not be linked\n");
      gst_object_unref (pipeline);
      return -1;
    }
    if (!gst_element_link (decode, convert)) {
      g_printerr ("Elements could not be linked\n");
      gst_object_unref (pipeline);
      return -1;
    }
    if (!gst_element_link (convert, sink)) {
      g_printerr ("Elements could not be linked\n");
      gst_object_unref (pipeline);
      return -1;
    }

    gst_element_set_state (pipeline, GST_STATE_PLAYING);

    GstBus *bus = gst_element_get_bus (pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message", G_CALLBACK (handle_message), pipeline);

    g_print("PIPELINE SET UP\n");
    return 0;

}
