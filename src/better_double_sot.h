#ifndef __DSOT_H__
#define __DSOT_H__

#include "global.h"
#include "rt64_extended_gbi.h"

void dsot_init_hour_selection(PlayState* play);
void dsot_handle_hour_selection(PlayState* play);
void dsot_cancel_hour_selection(PlayState* play);
void dsot_advance_hour(PlayState* play);
void dsot_draw_clock(PlayState* play);

extern bool skip_dsot_cutscene;

#endif /* __DSOT_H__ */