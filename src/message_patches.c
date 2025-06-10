#include "modding.h"
#include "better_double_sot.h"

RECOMP_IMPORT("mm_recomp_message_hooks", void mh_Message_Update_set_return_flag(void))

extern u8 D_801C6A70;

extern s32 sCharTexSize;
extern s32 sCharTexScale;
extern s32 D_801F6B08;

extern s16 sLastPlayedSong;

extern s16 sTextboxUpperYPositions[];
extern s16 sTextboxLowerYPositions[];
extern s16 sTextboxMidYPositions[];
extern s16 sTextboxXPositions[TEXTBOX_TYPE_MAX];

extern s16 D_801D0464[];
extern s16 D_801D045C[];

void func_80150A84(PlayState* play);
void Message_GrowTextbox(PlayState* play);
void Message_Decode(PlayState* play);
s32 Message_BombersNotebookProcessEventQueue(PlayState* play);
void Message_HandleChoiceSelection(PlayState* play, u8 numChoices);
bool Message_ShouldAdvanceSilent(PlayState* play);

void ShrinkWindow_Letterbox_SetSizeTarget(s32 target);
s32 ShrinkWindow_Letterbox_GetSizeTarget(void);


RECOMP_CALLBACK("mm_recomp_message_hooks", mh_on_Message_Update_TEXT_DONE) void on_Message_Update_TEXT_DONE(PlayState* play) {
    MessageContext* msgCtx = &play->msgCtx;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    if ((msgCtx->textboxEndType == TEXTBOX_ENDTYPE_TWO_CHOICE) &&
        (play->msgCtx.ocarinaMode == OCARINA_MODE_PROCESS_DOUBLE_TIME)) {
        Message_HandleChoiceSelection(play, 1);

        // Replace DSoT functionality.
        dsot_handle_hour_selection(play);

        if (Message_ShouldAdvance(play)) {
            if (msgCtx->choiceIndex == 0) {
                Audio_PlaySfx_MessageDecide();

                // @mod_use_export_var skip_dsot_cutscene: Skip DSoT cutscene if other mods enable it.
                if ((skip_dsot_cutscene > 0) || ((skip_dsot_cutscene == -1) && CGF_SKIP_DSOT_CUTSCENE)) {
                    play->msgCtx.ocarinaMode = OCARINA_MODE_END;
                    dsot_advance_hour(play);
                } else {
                    play->msgCtx.ocarinaMode = OCARINA_MODE_APPLY_DOUBLE_SOT;
                }
                gSaveContext.timerStates[TIMER_ID_MOON_CRASH] = TIMER_STATE_OFF;
            } else {
                Audio_PlaySfx_MessageCancel();
                play->msgCtx.ocarinaMode = OCARINA_MODE_END;

                dsot_cancel_hour_selection(play);
            }
            Message_CloseTextbox(play);
        }

        mh_Message_Update_set_return_flag();
    }
}

RECOMP_CALLBACK("mm_recomp_message_hooks", mh_on_Message_Update_TEXT_CLOSING) void on_Message_Update_TEXT_CLOSING(PlayState* play) {
    MessageContext* msgCtx = &play->msgCtx;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    if ((msgCtx->stateTimer == 1) &&
        (msgCtx->ocarinaAction != OCARINA_ACTION_CHECK_NOTIME_DONE) &&
        (sLastPlayedSong == OCARINA_SONG_DOUBLE_TIME)) {
        if (interfaceCtx->restrictions.songOfDoubleTime == 0) {
            // Adjust variables to match vanilla behaviour
            msgCtx->stateTimer--;

            if ((play->csCtx.state == CS_STATE_IDLE) && (gSaveContext.save.cutsceneIndex < 0xFFF0) &&
                ((play->activeCamId == CAM_ID_MAIN) ||
                ((play->transitionTrigger == TRANS_TRIGGER_OFF) && (play->transitionMode == TRANS_MODE_OFF))) &&
                (play->msgCtx.ocarinaMode == OCARINA_MODE_END)) {
                if (((u32)gSaveContext.prevHudVisibility == HUD_VISIBILITY_IDLE) ||
                    (gSaveContext.prevHudVisibility == HUD_VISIBILITY_NONE) ||
                    (gSaveContext.prevHudVisibility == HUD_VISIBILITY_NONE_ALT)) {
                    gSaveContext.prevHudVisibility = HUD_VISIBILITY_ALL;
                }
                gSaveContext.hudVisibility = HUD_VISIBILITY_IDLE;
            }

            msgCtx->msgLength = 0;
            msgCtx->msgMode = MSGMODE_NONE;
            msgCtx->currentTextId = 0;
            msgCtx->stateTimer = 0;
            XREG(31) = 0;

            msgCtx->textboxEndType = TEXTBOX_ENDTYPE_DEFAULT;

            // Replace DSoT functionality.
            if ((CURRENT_DAY != 3) || (CURRENT_TIME < CLOCK_TIME(5, 0)) || (CURRENT_TIME >= CLOCK_TIME(6, 0))) {
                Message_StartTextbox(play, D_801D0464[0], NULL);

                // Replace message text.
                char *buf = play->msgCtx.font.msgBuf.schar;
                buf[25] = ' ';
                buf[26] = 1;
                buf[27] = 'S';
                buf[28] = 'e';
                buf[29] = 'l';
                buf[30] = 'e';
                buf[31] = 'c';
                buf[32] = 't';
                buf[33] = 'e';
                buf[34] = 'd';
                buf[35] = ' ';
                buf[36] = 'H';
                buf[37] = 'o';
                buf[38] = 'u';
                buf[39] = 'r';
                buf[40] = 0;
                buf[41] = '?';
                buf[42] = 17;
                buf[43] = 17;
                buf[44] = 2;
                buf[45] = -62;
                buf[46] = 'Y';
                buf[47] = 'e';
                buf[48] = 's';
                buf[49] = 17;
                buf[50] = 'N';
                buf[51] = 'o';
                buf[52] = -65;

                play->msgCtx.ocarinaMode = OCARINA_MODE_PROCESS_DOUBLE_TIME;

                dsot_init_hour_selection(play);
            } else {
                Message_StartTextbox(play, 0x1B94, NULL);
                play->msgCtx.ocarinaMode = OCARINA_MODE_END;
            }

            mh_Message_Update_set_return_flag();
        } else {
            // sLastPlayedSong = 0xFF;
            Message_StartTextbox(play, 0x1B95, NULL);
            play->msgCtx.ocarinaMode = OCARINA_MODE_PROCESS_RESTRICTED_SONG;
        }
        sLastPlayedSong = 0xFF;
    }
}