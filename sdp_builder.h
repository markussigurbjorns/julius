#ifndef __SDP_BUILDER_H_JULIUS__
#define __SDP_BUILDER_H_JULIUS__

#include <gst/sdp/gstsdpmessage.h>
#include <glib.h>


typedef struct _SdpBuilder SdpBuilder;

SdpBuilder *sdp_builder_new(void);
void        sdp_builder_free(SdpBuilder *b);

int sdp_builder_set_origin       (SdpBuilder *b, const gchar *username,
                                  const gchar *addr);
int sdp_builder_set_session_name (SdpBuilder *b, const gchar *name);
int sdp_builder_set_bandwidth    (SdpBuilder *b, const gchar *bwtype,
                                  guint32 bandwidth);
int sdp_builder_set_connection   (SdpBuilder *b,
                                  const gchar *addr,
                                  guint       ttl,
                                  guint       addrs);
int sdp_builder_add_attribute    (SdpBuilder *b, const gchar *key,
                                  const gchar *value); 

/* Media helpers */
/* Returns media index (0‑based) or negative on error */
int sdp_builder_add_media(SdpBuilder *b, const gchar *media,
                          guint port, guint n_ports,
                          const gchar *proto,
                          const guint8 *payload_types,
                          guint n_payloads);

int sdp_builder_add_media_bandwidth(SdpBuilder *b,
                                    guint       media_idx,
                                    const char *bwtype,
                                    guint32     bandwidth);

int sdp_builder_add_media_attribute(SdpBuilder *b, guint media_index,
                                    const gchar *key, const gchar *value);

GstSDPMessage *sdp_builder_get_message(SdpBuilder *b); /* Borrowed; do not free */
gchar        *sdp_builder_to_string (SdpBuilder *b);  /* g_free when done */


#endif /* SDP_BUILDER_H */

/* == Implementation section – include from ONE compilation unit ========= */
#ifdef SDP_BUILDER_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

struct _SdpBuilder {
    GstSDPMessage *msg;
};

SdpBuilder *sdp_builder_new(void)
{
    SdpBuilder *b = g_new0(SdpBuilder, 1);
    gst_sdp_message_new(&b->msg);
    gst_sdp_message_set_version(b->msg, "0");
    return b;
}

void sdp_builder_free(SdpBuilder *b)
{
    if (!b) return;
    if (b->msg) gst_sdp_message_free(b->msg);
    g_free(b);
}

static gchar *generate_session_id(void)
{
    guint64 now = (guint64)g_get_real_time();
    return g_strdup_printf("%" G_GUINT64_FORMAT, now);
}

int sdp_builder_set_origin(SdpBuilder *b, const gchar *username,
                           const gchar *addr)
{
    g_return_val_if_fail(b && username && addr, -1);

    gchar *sid = generate_session_id();
    /* Network type: IN, AddrType: IP4 or IP6 – detect simple */
    const gchar *addr_type = strchr(addr, ':') ? "IP6" : "IP4";

    int ret = gst_sdp_message_set_origin(b->msg,
                                         username,    /* username */
                                         sid,         /* sess‑id  */
                                         "1",        /* sess‑version */
                                         "IN",       /* nettype */
                                         addr_type,   /* addr‑type */
                                         addr);       /* unicast‑address */
    g_free(sid);
    return ret;
}

int sdp_builder_set_session_name(SdpBuilder *b, const gchar *name)
{
    g_return_val_if_fail(b && name, -1);
    return gst_sdp_message_set_session_name(b->msg, name);
}

int sdp_builder_set_bandwidth(SdpBuilder *b, const gchar *bwtype,
                                guint32 bandwidth)
{
    g_return_val_if_fail (b != NULL, -1);
    g_return_val_if_fail (bwtype != NULL && *bwtype != '\0', -1);

    return gst_sdp_message_add_bandwidth (b->msg, bwtype, bandwidth);
}

int sdp_builder_set_connection   (SdpBuilder *b,
                                  const gchar *addr,
                                  guint       ttl,
                                  guint       addrs)
{
    g_return_val_if_fail (b != NULL,-1);
    g_return_val_if_fail (addr != NULL && *addr != '\0',-1);

    return gst_sdp_message_set_connection (b->msg,
                                            "IN",                    
                                            strchr (addr, ':')   
                                                ? "IP6" : "IP4",   
                                            addr,
                                            ttl,
                                            addrs);  

}

int sdp_builder_add_attribute(SdpBuilder *b, const gchar *key,
                              const gchar *value)
{
    g_return_val_if_fail(b && key, -1);
    return gst_sdp_message_add_attribute(b->msg, key, value);
}

int sdp_builder_add_media(SdpBuilder *b, const gchar *media,
                          guint port, guint n_ports,
                          const gchar *proto,
                          const guint8 *payload_types,
                          guint n_payloads)
{
    g_return_val_if_fail(b && media && proto && payload_types && n_payloads > 0, -1);

    GstSDPMedia *m;
    gst_sdp_media_new(&m);

    int ret = gst_sdp_media_set_media(m, media);
    if (ret) goto err;
    ret = gst_sdp_media_set_port_info(m, port, n_ports);
    if (ret) goto err;
    ret = gst_sdp_media_set_proto(m, proto);
    if (ret) goto err;

    for (guint i = 0; i < n_payloads; ++i) {
        ret = gst_sdp_media_add_format(m, g_strdup_printf("%u", payload_types[i]));
        if (ret) goto err;
    }

    ret = gst_sdp_message_add_media(b->msg, m);
    if (ret) goto err;

    /* gst_sdp_message_add_media copies, so clear */
    gst_sdp_media_free(m);

    return gst_sdp_message_medias_len(b->msg) - 1; /* index */
err:
    gst_sdp_media_free(m);
    return -1;
}

int sdp_builder_add_media_bandwidth(SdpBuilder *b,
                                    guint       media_idx,
                                    const char *bwtype,
                                    guint32     bandwidth)
{
    g_return_val_if_fail (b != NULL, -1);
    g_return_val_if_fail (bwtype != NULL && *bwtype != '\0', -1);

    GstSDPMedia *media = (GstSDPMedia *)
                         gst_sdp_message_get_media (b->msg, media_idx);
    g_return_val_if_fail (media != NULL, -1);

    return gst_sdp_media_add_bandwidth (media, bwtype, bandwidth);
}

int sdp_builder_add_media_attribute(SdpBuilder *b, guint media_index,
                                    const gchar *key, const gchar *value)
{
    g_return_val_if_fail(b && key, -1);

    GstSDPMedia *m = (GstSDPMedia *)gst_sdp_message_get_media(b->msg, media_index);
    if (!m) return -1;
    return gst_sdp_media_add_attribute(m, key, value);
}

GstSDPMessage *sdp_builder_get_message(SdpBuilder *b)
{
    return b ? b->msg : NULL;
}

gchar *sdp_builder_to_string(SdpBuilder *b)
{
    if (!b) return NULL;
    gchar *out = gst_sdp_message_as_text(b->msg);
    return out; /* caller g_free */
}

#endif /* SDP_BUILDER_IMPLEMENTATION */
