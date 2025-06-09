#include <glib.h>
#include "network.h"
#include "media.h"


gchar *get_invite_sdp() {
    SdpBuilder *b = sdp_builder_new();

    sdp_builder_set_origin(b, "markus-sip", get_host_ipv4());
    sdp_builder_set_session_name(b, "Julius Call");
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

void process_answer(const gchar* answer) {

}
