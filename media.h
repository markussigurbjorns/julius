#ifndef __MEDIA_JULIUS__
#define __MEDIA_JULIUS__

#include <glib.h>
#include "sdp_builder.h"

gchar *get_invite_sdp();
void process_offer(const gchar *offer);
void process_answer(const gchar *answer);

#endif

