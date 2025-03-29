#ifndef __DSOT_H__
#define __DSOT_H__

#include "global.h"
#include "rt64_extended_gbi.h"

#include "modding.h"

void dsot_init_hour_selection(PlayState* play);
void dsot_handle_hour_selection(PlayState* play);
void dsot_cancel_hour_selection(PlayState* play);
void dsot_advance_hour(PlayState* play);
void dsot_draw_clock(PlayState* play);

extern int skip_dsot_cutscene;

RECOMP_IMPORT("*", u32 recomp_get_config_u32(const char* key));

#define CFG_ANALOG_INPUT_ALLOWED !(bool)recomp_get_config_u32("accept_analog_input")
#define CFG_DPAD_INPUT_ALLOWED !(bool)recomp_get_config_u32("accept_dpad_input")
#define CFG_ZR_INPUT_ALLOWED !(bool)recomp_get_config_u32("accept_zr_input")

#define CGF_SKIP_DSOT_CUTSCENE !(bool)recomp_get_config_u32("skip_dsot_cutscene")

#endif /* __DSOT_H__ */