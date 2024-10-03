#ifndef __DSOT_H__
#define __DSOT_H__

#include "global.h"
#include "rt64_extended_gbi.h"

// Macros and enums from a newer decompilation commit.
#define SEGMENT_ROM_START_OFFSET(segment, offset) ((uintptr_t) (( _ ## segment ## SegmentRomStart ) + (offset)))
#define CURRENT_TIME ((void)0, gSaveContext.save.time)
#define EVENTINF_HAS_DAYTIME_TRANSITION_CS 0x52
#define ACTORCTX_FLAG_TELESCOPE_ON (1 << 1)
#define WEATHER_MODE_RAIN WEATHER_MODE_1
typedef enum ThreeDayDaytime {
    /* 0 */ THREEDAY_DAYTIME_NIGHT,
    /* 1 */ THREEDAY_DAYTIME_DAY,
    /* 2 */ THREEDAY_DAYTIME_MAX
} ThreeDayDaytime;

void dsot_init_hour_selection(PlayState* play);
void dsot_handle_hour_selection(PlayState* play);
void dsot_cancel_hour_selection(PlayState* play);
void dsot_advance_hour(PlayState* play);
void dsot_draw_clock(PlayState* play);

#endif /* __DSOT_H__ */