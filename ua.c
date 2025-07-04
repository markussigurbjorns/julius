#include "ua.h"
#include "glib.h"
#include "sofia-sip/nua.h"
#include "sofia-sip/nua_tag.h"
#include "sofia-sip/sip.h"
#include "sofia-sip/sip_tag.h"
#include "sofia-sip/su.h"
#include "sofia-sip/su_alloc.h"
#include "sofia-sip/su_glib.h"
#include "sofia-sip/su_tag.h"
#include "media.h"
#include <stdlib.h>

static app_ctx *ctx; 

static void ua_event_callback(nua_event_t   event,
                  int           status,
                  char const   *phrase,
                  nua_t        *nua,
                  nua_magic_t  *ctx,
                  nua_handle_t *nh,
                  nua_hmagic_t *op_ctx,
                  sip_t const  *sip,
                  tagi_t        tags[]) {

    g_print("Event: %s (%d %s)\n", nua_event_name(event), status, phrase);

    switch (event) {
        case nua_i_state: {
            g_print("in nua_i_state");
            const gchar *remote_sdp = NULL;
            gint is_offer = 0, is_answer = 0;
            tl_gets(tags,
                SOATAG_REMOTE_SDP_STR_REF(remote_sdp),
                NUTAG_OFFER_RECV_REF(is_offer),
                NUTAG_ANSWER_RECV_REF(is_answer),
                TAG_END());

            if (remote_sdp) {
                if (is_offer) {
                    g_message("----- remote SDP offer -----\n%s", remote_sdp);
                    process_offer(remote_sdp);
                } else if (is_answer) {
                    g_message("----- remote SDP answer -----\n%s", remote_sdp);
                    process_answer(remote_sdp);
                }
            }
            break;
        }
        case nua_r_register: {
            if (status == 200) {
                g_print("Registration successful!\n");
            } else {
                g_print("Registration failed: %d %s\n", status, phrase);
            }
            break;
        }
        case nua_r_invite: {
            if (status == 200) {
                g_print("Invite Successful\n");
                //nua_ack(nh, TAG_NULL()); SOA should send ACK
            } else {
                g_print("response to INVITE: %03d %s\n", status, phrase);
            }
            break;
        }
        case nua_i_invite: {
            g_print("incoming INVITE: %03d %s\n", status, phrase);
            break;
        }
        case nua_r_set_params: {
            g_print("set params\n");
            break;
        }

        default: {
            /* unknown event */
            if (status > 100) {
                printf("unknown event %d: %03d %s\n",
                    event,
                    status,
                    phrase);
            } else {
                printf("unknown event %d\n", event);
            }
            break;
        }
    }
}

su_root_t* ua_init(void) {
    ctx = malloc(sizeof(app_ctx));
    if (ctx == NULL) {
        g_error("failed to create ctx\n");
        exit(1);
    }

    su_home_init(ctx->home);
    ctx->root = su_glib_root_create(ctx);
    if (ctx->root == NULL) {
        g_error("failed to create su root\n");
        exit(1);
    }

    url_t *registrar = url_make(ctx->home,
                                    "sips:213.239.88.94:5061;transport=tls");

    url_t *bindurl = url_make(ctx->home, "sips:*:5061;transport=tls");

    ctx->nua = nua_create(ctx->root, ua_event_callback, ctx, 
                          NUTAG_PROXY(registrar),
                          NUTAG_REGISTRAR(registrar),
                          SIPTAG_FROM_STR("Markus <sips:markus@testdomain.com>"),
                          NUTAG_SIPS_URL(bindurl),
                          SIPTAG_USER_AGENT_STR("markus-sip/0.1"),
                          NUTAG_CERTIFICATE_DIR("/opt/certs"),
                          TAG_NULL());

    if (ctx->nua == NULL) {
        g_error("failed to create nua\n");
        exit(1);
    }

    nua_set_params(ctx->nua,
                   SIPTAG_ALLOW_STR("INVITE,CANCEL,BYE,ACK"),
                   TAG_NULL());

    return ctx->root;
}

void ua_deinit(void) {
    nua_destroy(ctx->nua);
    su_root_destroy(ctx->root);
    su_home_deinit(ctx->home);
}

void ua_register(void) {
    op_ctx *op = su_zalloc(ctx->home, sizeof(*op));
    op->handle = nua_handle(ctx->nua, op,
                            TAG_NULL()); 
    
    if (op->handle == NULL) {
        g_error("cannot create operation handle in register\n");
        exit(1);
    }

    nua_register(op->handle,
                SIPTAG_CONTACT_STR("sips:markus@testdomain.com;transport=tls"),
                NUTAG_M_USERNAME("markus"),
                SIPTAG_EXPIRES_STR("3600"),
                TAG_NULL()); 
}

void ua_invite(const gchar *to) {
    op_ctx *op = su_zalloc(ctx->home, sizeof(*op));

    op->handle = nua_handle(ctx->nua, op, SIPTAG_TO_STR(to), TAG_NULL());

    if (op->handle == NULL) {
        g_error("cannot create operation handle in invite\n");
        exit(1);
    }

    gchar *sdp_invite = get_invite_sdp();

    g_print("SDP\n%s", sdp_invite);

    nua_invite(op->handle,
               SOATAG_USER_SDP_STR(sdp_invite),
               SIPTAG_CONTACT_STR("sips:markus@testdomain.com;transport=tls"),
               TAG_NULL());

    g_free(sdp_invite);
}
