#ifndef __UA_JULIUS__
#define __UA_JULIUS__

#include "sofia-sip/nua.h"
#include <glib.h>

typedef struct app_ctx {
    su_home_t home[1];
    su_root_t *root;
    nua_t *nua;

} app_ctx;

typedef union op_ctx {
    nua_handle_t *handle;
    
    struct {
        nua_handle_t *handle;
        // more call related things
    } j_call;

    struct {
        nua_handle_t *handle;
        // more register related things
    } j_register;

    struct {
        nua_handle_t *hanlde;
        // more subscribtion related things
    } j_subscribtion;

} op_ctx;

su_root_t* ua_init(void);
void ua_deinit(void);

void ua_register(void);
void ua_invite(const gchar *to);

#endif
