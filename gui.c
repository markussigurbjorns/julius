#include <glib.h>
#include "gui.h"
#include "lvgl.h"
#include "drivers/sdl/lv_sdl_mouse.h"
#include "drivers/sdl/lv_sdl_mousewheel.h"
#include "drivers/sdl/lv_sdl_keyboard.h"
#include "drivers/sdl/lv_sdl_window.h"
#include "ua.h"

#define SDL_HOR_RES  800
#define SDL_VER_RES  480

static lv_obj_t *kb = NULL; //global keyboard

static void call_btn_cb(lv_event_t * e)
{
    lv_obj_t * ta = lv_event_get_user_data(e);

    const char * to = lv_textarea_get_text(ta);

    g_print("Input field says: \"%s\"\n", to ? to : "(empty)");
    ua_invite(to);
}

static void reg_btn_cb(lv_event_t *e)
{
    g_print("Register button pressed – do SIP register here\n");
    ua_register();
}

static void ta_focus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_FOCUSED)
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    else if(code == LV_EVENT_DEFOCUSED)
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

void setup_gui(char *host_ip) {
    lv_display_t *lvDisplay = lv_sdl_window_create(SDL_HOR_RES, SDL_VER_RES);
    lv_indev_t *lvMouse = lv_sdl_mouse_create();
    lv_indev_t *lvMouseWheel = lv_sdl_mousewheel_create();
    lv_indev_t *lvKeyboard = lv_sdl_keyboard_create();

    /* ---------------- Status label (top-left) ------------------------ */
    lv_obj_t *status = lv_label_create(lv_scr_act());
    lv_label_set_text(status, host_ip);
    lv_obj_align(status, LV_ALIGN_TOP_LEFT, 6, 4);     /* 6 px from edges */

    /* --- Row container ------------------------------------------------ */

    lv_obj_t *row = lv_obj_create(lv_scr_act());
    lv_obj_set_size(row, LV_SIZE_CONTENT, 70);
    lv_obj_set_style_pad_row(row, 0, 0);
    lv_obj_set_style_pad_column(row, 8, 0);     /* 8 px gap between items */
    lv_obj_center(row);                         /* centre of the screen   */
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);

    /* --- Textarea ----------------------------------------------------- */
    lv_obj_t *ta = lv_textarea_create(row);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, "Type here...");
    lv_obj_set_width(ta, 300);

    /* show/hide keyboard when textarea gains/loses focus */
    lv_obj_add_event_cb(ta, ta_focus_cb, LV_EVENT_FOCUSED,   NULL);
    lv_obj_add_event_cb(ta, ta_focus_cb, LV_EVENT_DEFOCUSED, NULL);

    /* --- OK button ---------------------------------------------------- */
    lv_obj_t *ok_btn = lv_btn_create(row);
    lv_obj_set_size(ok_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_t *ok_lbl = lv_label_create(ok_btn);
    lv_label_set_text(ok_lbl, "Call");
    lv_obj_add_event_cb(ok_btn, call_btn_cb, LV_EVENT_CLICKED, ta);

    /* --- Register button --------------------------------------------- */
    lv_obj_t *reg_btn = lv_btn_create(row);
    lv_obj_set_size(reg_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_t *reg_lbl = lv_label_create(reg_btn);
    lv_label_set_text(reg_lbl, "Register");
    lv_obj_add_event_cb(reg_btn, reg_btn_cb, LV_EVENT_CLICKED, NULL);

    /* --- On-screen keyboard ------------------------------------------ */
    kb = lv_keyboard_create(lv_scr_act());
    lv_obj_set_size(kb, 320, 180);                /* tweak as needed */
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);      /* start hidden    */
    lv_keyboard_set_textarea(kb, ta);
}
