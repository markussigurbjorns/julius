#include <stdio.h>
#include <glib.h>
#include <gst/gst.h>
#include <sofia-sip/su.h>
#include <sofia-sip/su_glib.h>
#include "display/lv_display_private.h"
#include "lv_types.h"
#include "ua.h"                
#include "gui.h"
#include "network.h"
#include "lvgl.h"


#ifdef JULIUS_USE_SDL  
#include <SDL2/SDL.h>
#define SDL_MAIN_HANDLED        /*To fix SDL's "undefined reference to WinMain" issue*/
#endif


/* ---------- GLib wrappers ---------- */
static gboolean lv_tick_cb(gpointer user_data)      /* 1 ms tick */
{
    lv_tick_inc(1);
    return G_SOURCE_CONTINUE;
}

static gboolean lv_timer_cb(gpointer user_data)     /* 5 ms handler */
{
    lv_timer_handler();
    return G_SOURCE_CONTINUE;
}
/* ---------- GLib wrappers END---------- */

int main(int argc, char *argv[])
{
    printf("launch program\n");
    /* --- subsystem init ------------------------------------------------- */
    gst_init(&argc, &argv);
    su_init();                          

    printf("after gst init\n");

#ifdef JULIUS_USE_SDL
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }
#endif

    lv_init();             
    printf("after lv init\n");

    char* host_ip = get_host_ipv4();

    printf("after getting host ip\n");
    setup_gui(host_ip);
    printf("after setting up gui\n");

    su_root_t *root = ua_init();          

    /* --- main loop (GLib) ---------------------------------------------- */
    GMainLoop *loop   = g_main_loop_new(NULL, FALSE);
    GSource   *source = su_glib_root_gsource(root); 
    g_source_attach(source, g_main_loop_get_context(loop));

    g_timeout_add_full(G_PRIORITY_HIGH, 1,  lv_tick_cb,  NULL, NULL);
    g_timeout_add_full(G_PRIORITY_DEFAULT, 5, lv_timer_cb, NULL, NULL);


    printf("running loop\n");
    g_main_loop_run(loop);

    /* --- clean-up ------------------------------------------------------- */
    g_source_destroy(source);
    g_main_loop_unref(loop);
    ua_deinit();
    su_deinit();

#ifdef JULIUS_USE_SDL
    SDL_Quit();
#endif
    free(host_ip);
    return 0;
}
