#include "modding.h"
#include "better_double_sot.h"

// Draw DSoT hour selection clock.
RECOMP_HOOK("Message_DrawTextBox") void on_Message_DrawTextBox(PlayState* play, Gfx** gfxP) {
    if (play->msgCtx.ocarinaMode == OCARINA_MODE_PROCESS_DOUBLE_TIME) {
        dsot_draw_clock(play);
    }
}