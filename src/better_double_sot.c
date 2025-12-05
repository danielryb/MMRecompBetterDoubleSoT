#include "better_double_sot.h"

void dsot_load_day_number_texture(PlayState* play, s32 day);

static void dsot_rain_fix(PlayState* play);
static void dsot_bgm_fix(PlayState* play);
static void dsot_actor_fixes(PlayState* play);

int skip_dsot_cutscene = -1;

// @mod_export void dsot_set_skip_dsot_cutscene(bool new_val): Set whether the DSoT cutscene should be skipped.
RECOMP_EXPORT void dsot_set_skip_dsot_cutscene(bool new_val) {
    skip_dsot_cutscene = new_val;
}

void set_time(PlayState* play, s32 day, u16 time);

RECOMP_EXPORT void dsot_set_time(PlayState* play, s32 day, u16 time) {
    set_time(play, day, time);
}

bool custom_blacklisted;

RECOMP_EXPORT void dsot_set_blacklist_true(void) {
    custom_blacklisted = true;
}

RECOMP_DECLARE_EVENT(dsot_on_blacklist_check(s16 actor_id))

static u8 choiceHour;

void dsot_init_hour_selection(PlayState* play) {
    choiceHour = (u16)(TIME_TO_HOURS_F(CURRENT_TIME - 1)) + 1;
    if (choiceHour >= 24) {
        choiceHour = 0;
    }
    if (choiceHour == 6) {
        if (gSaveContext.save.day == 3) {
            choiceHour = 5;
        } else {
            dsot_load_day_number_texture(play, gSaveContext.save.day + 1);
        }
    }
}

#define TIMER_INIT 10
#define TIMER_STEP_RESET 0

void dsot_handle_hour_selection(PlayState* play) {
    static s8 sInputTimer = TIMER_INIT;
    static s8 sPrevInputVector = 0;
    static bool sAnalogStickHeld = false;
    Input* input = CONTROLLER1(&play->state);

    s8 inputVector = (
        (CFG_ANALOG_INPUT_ALLOWED && (input->rel.stick_x >= 30)) ||
        (CFG_DPAD_INPUT_ALLOWED && CHECK_BTN_ANY(input->cur.button, BTN_DRIGHT)) ||
        (CFG_ZR_INPUT_ALLOWED && CHECK_BTN_ANY(input->cur.button, BTN_R))
    );
    inputVector -= (
        (CFG_ANALOG_INPUT_ALLOWED && (input->rel.stick_x <= -30)) ||
        (CFG_DPAD_INPUT_ALLOWED && CHECK_BTN_ANY(input->cur.button, BTN_DLEFT)) ||
        (CFG_ZR_INPUT_ALLOWED && CHECK_BTN_ANY(input->cur.button, BTN_Z))
    );

    if (inputVector > 0 && !sAnalogStickHeld) {
        u8 prevHour = choiceHour;
        sAnalogStickHeld = true;

        choiceHour++;
        if (choiceHour >= 24) {
            choiceHour = 0;
        }

        if ((CURRENT_DAY == 3) && (prevHour <= 5) && (choiceHour > 5)) {
            choiceHour = 5;
        } else if ((prevHour <= 6) && (choiceHour > 6)) {
            choiceHour = 6;
        } else {
            Audio_PlaySfx(NA_SE_SY_CURSOR);
        }

        if ((choiceHour == 6) && (choiceHour > prevHour)) {
            dsot_load_day_number_texture(play, gSaveContext.save.day + 1);
        }
    } else if (inputVector < 0 && !sAnalogStickHeld) {
        u8 prevHour = choiceHour;
        sAnalogStickHeld = true;

        choiceHour--;
        if (choiceHour > 24) {
            choiceHour = 23;
        }

        if (((CLOCK_TIME(choiceHour, 0) <= CURRENT_TIME) && (CLOCK_TIME(prevHour, 0) > CURRENT_TIME))
        || ((choiceHour > prevHour) && (CURRENT_TIME >= CLOCK_TIME(23, 0)))
        || ((choiceHour > prevHour) && (CURRENT_TIME < CLOCK_TIME(1, 0)))) {
            choiceHour = prevHour;
        }
        else {
            Audio_PlaySfx(NA_SE_SY_CURSOR);
        }

        if ((prevHour == 6) && (choiceHour < prevHour)) {
            dsot_load_day_number_texture(play, gSaveContext.save.day);
        }
    } else if (inputVector == 0) {
        sAnalogStickHeld = false;
        sInputTimer = TIMER_INIT;
    }

    if (inputVector != 0) {
        if (!DECR(sInputTimer)) {
            sAnalogStickHeld = false;
            sInputTimer = TIMER_STEP_RESET;
        }
    }

    if (sPrevInputVector * inputVector < 0) {
        sAnalogStickHeld = false;
        sInputTimer = TIMER_INIT;
    }

    sPrevInputVector = inputVector;
}

void dsot_cancel_hour_selection(PlayState* play) {
    if (choiceHour == 6) {
        dsot_load_day_number_texture(play, gSaveContext.save.day);
    }
}

// Loads day number texture from week_static for the three-day clock.
void dsot_load_day_number_texture(PlayState* play, s32 day) {
    s16 i = day - 1;

    if ((i < 0) || (i >= 3)) {
        i = 0;
    }

    DmaMgr_RequestSync(play->interfaceCtx.doActionSegment + DO_ACTION_OFFSET_DAY_NUMBER,
                       SEGMENT_ROM_START(week_static) + i * WEEK_STATIC_TEX_SIZE, WEEK_STATIC_TEX_SIZE);

}

#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"

void Interface_DrawClock(PlayState* play);

// Wrapper for Interface_DrawClock to display selected hour.
void dsot_draw_clock(PlayState* play) {
    MessageContext *msgCtx = &play->msgCtx;

    u8 prev_msgMode = msgCtx->msgMode;
    msgCtx->msgMode = MSGMODE_NONE;

    PlayState* state = (PlayState*)&play->state;
    s32 prev_frameAdvanceCtx = state->frameAdvCtx.enabled;
    state->frameAdvCtx.enabled = 0;

    u16 prev_pauseCtx_state = play->pauseCtx.state;
    play->pauseCtx.state = PAUSE_STATE_OFF;

    u16 prev_pauseCtx_debugEditor = play->pauseCtx.state;
    play->pauseCtx.debugEditor = DEBUG_EDITOR_NONE;

    bool flag_modified = false;
    if (!(play->actorCtx.flags & ACTORCTX_FLAG_TELESCOPE_ON)) {
        play->actorCtx.flags ^= ACTORCTX_FLAG_TELESCOPE_ON;
        flag_modified = true;
    }

    u16 prev_time = gSaveContext.save.time;
    gSaveContext.save.time = CLOCK_TIME(choiceHour, 0);

    Interface_DrawClock(play);

    gSaveContext.save.time = prev_time;

    if (flag_modified) {
        play->actorCtx.flags ^= ACTORCTX_FLAG_TELESCOPE_ON;
    }

    play->pauseCtx.debugEditor = prev_pauseCtx_debugEditor;
    play->pauseCtx.state = prev_pauseCtx_state;
    state->frameAdvCtx.enabled = prev_frameAdvanceCtx;
    msgCtx->msgMode = prev_msgMode;
}

Actor* Actor_Delete(ActorContext* actorCtx, Actor* actor, PlayState* play);
void Actor_Destroy(Actor* actor, PlayState* play);

void Actor_InitHalfDaysBit(ActorContext* actorCtx);
void Actor_KillAllOnHalfDayChange(PlayState* play, ActorContext* actorCtx);
Actor* Actor_SpawnEntry(ActorContext* actorCtx, ActorEntry* actorEntry, PlayState* play);

bool is_on_respawn_blacklist(s16 actor_id) {
    switch (actor_id) {
        case ACTOR_PLAYER:
        case ACTOR_EN_TEST3:

        case ACTOR_EN_DOOR:

        case ACTOR_EN_TEST4:
        case ACTOR_OBJ_TOKEI_STEP:
        case ACTOR_OBJ_TOKEIDAI:
            return true;
    }

    custom_blacklisted = false;
    dsot_on_blacklist_check(actor_id);

    return custom_blacklisted;
}

void kill_all_setup_actors(PlayState* play, ActorContext* actorCtx) {
    s32 category;

    for (category = 0; category < ACTORCAT_MAX; category++) {
        Actor* actor = actorCtx->actorLists[category].first;

        while (actor != NULL) {
            if (!is_on_respawn_blacklist(actor->id)) {
                func_80123590(play, actor);

                if (!actor->isDrawn) {
                    actor = Actor_Delete(actorCtx, actor, play);
                } else {
                    Actor_Kill(actor);
                    Actor_Destroy(actor, play);
                    actor = actor->next;
                }
            } else {
                actor = actor->next;
            }
        }
    }

    CollisionCheck_ClearContext(play, &play->colChkCtx);
    play->msgCtx.unk_12030 = 0;
}

void respawn_setup_actors(PlayState* play, ActorContext* actorCtx) {
    if (play->numSetupActors < 0) {
        play->numSetupActors = -play->numSetupActors;
    }

    ActorEntry* actorEntry = play->setupActorList;
    s32 prevHalfDaysBitValue = actorCtx->halfDaysBit;
    s32 actorEntryHalfDayBit;
    s32 i;

    Actor_InitHalfDaysBit(actorCtx);
    // Actor_KillAllOnHalfDayChange(play, &play->actorCtx);
    kill_all_setup_actors(play, &play->actorCtx);

    for (i = 0; i < play->numSetupActors; i++) {
        actorEntryHalfDayBit = ((actorEntry->rot.x & 7) << 7) | (actorEntry->rot.z & 0x7F);
        if (actorEntryHalfDayBit == 0) {
            actorEntryHalfDayBit = HALFDAYBIT_ALL;
        }

        if (!is_on_respawn_blacklist(actorEntry->id) &&
            (actorEntryHalfDayBit & actorCtx->halfDaysBit) &&
            (!CHECK_EVENTINF(EVENTINF_17) || !(actorEntryHalfDayBit & prevHalfDaysBitValue) ||
            !(actorEntry->id & 0x800))) {

            Actor_SpawnEntry(&play->actorCtx, actorEntry, play);
        }
        actorEntry++;
    }

    // Prevents re-spawning the setup actors
    play->numSetupActors = -play->numSetupActors;
}

Actor* Actor_RemoveFromCategory(PlayState* play, ActorContext* actorCtx, Actor* actorToRemove);
void Actor_AddToCategory(ActorContext* actorCtx, Actor* actor, u8 actorCategory);

void update_actor_categories(PlayState*play, ActorContext* actorCtx) {
    s32 category;
    Actor* actor;
    Player* player = GET_PLAYER(play);
    u32* categoryFreezeMaskP;
    s32 newCategory;
    Actor* next;
    ActorListEntry* entry;

    // Move actors to a different actorList if it has changed categories.
    for (category = 0, entry = actorCtx->actorLists; category < ACTORCAT_MAX; entry++, category++) {
        if (entry->categoryChanged) {
            actor = entry->first;

            while (actor != NULL) {
                if (actor->category == category) {
                    // The actor category matches the list category. No change needed.
                    actor = actor->next;
                    continue;
                }

                // The actor category does not match the list category and needs to be moved.
                next = actor->next;
                newCategory = actor->category;
                actor->category = category;
                Actor_RemoveFromCategory(play, actorCtx, actor);
                Actor_AddToCategory(actorCtx, actor, newCategory);
                actor = next;
            }
            entry->categoryChanged = false;
        }
    }
}

void Actor_SpawnSetupActors(PlayState* play, ActorContext* actorCtx);

void set_time(PlayState* play, s32 day, u16 time) {
    bool prevIsNight = gSaveContext.save.isNight;

    gSaveContext.save.time = time;
    gSaveContext.skyboxTime = CURRENT_TIME;
    gSaveContext.save.isNight = ((CURRENT_TIME < CLOCK_TIME(6, 0)) || (CURRENT_TIME > CLOCK_TIME(18, 0)));

    if (CURRENT_DAY != day) {
        Sram_IncrementDay();
        gSaveContext.save.day = day;
        gSaveContext.save.eventDayCount = gSaveContext.save.day;

        Interface_NewDay(play, CURRENT_DAY);
        Environment_NewDay(&play->envCtx);

        respawn_setup_actors(play, &play->actorCtx);
        update_actor_categories(play, &play->actorCtx);
        DynaPoly_UpdateBgActorTransforms(play, &play->colCtx.dyna);
    } else if (prevIsNight != gSaveContext.save.isNight) {
        if (play->numSetupActors < 0) {
            play->numSetupActors = -play->numSetupActors;
        }
    }

    dsot_rain_fix(play);
    dsot_bgm_fix(play);
    dsot_actor_fixes(play);
}

void dsot_advance_hour(PlayState* play) {
    set_time(play, CURRENT_DAY, CLOCK_TIME(choiceHour, 0));
}

static void dsot_rain_fix(PlayState* play) {
    if ((CURRENT_DAY == 2) && (Environment_GetStormState(play) != STORM_STATE_OFF)) {
        if ((CURRENT_TIME >= CLOCK_TIME(8, 0)) && (CURRENT_TIME < CLOCK_TIME(18, 00))) {
            gWeatherMode = WEATHER_MODE_RAIN;
            play->envCtx.lightningState = LIGHTNING_ON;
            play->envCtx.precipitation[PRECIP_RAIN_MAX] = 60;
            play->envCtx.precipitation[PRECIP_RAIN_CUR] = 60;
            Environment_PlayStormNatureAmbience(play);
        } else {
            gWeatherMode = WEATHER_MODE_CLEAR;
            play->envCtx.lightningState = LIGHTNING_OFF;
            play->envCtx.precipitation[PRECIP_RAIN_MAX] = 0;
            play->envCtx.precipitation[PRECIP_RAIN_CUR] = 0;
            Environment_StopStormNatureAmbience(play);
        }
    }
}

static void dsot_bgm_fix(PlayState* play) {
    play->envCtx.timeSeqState = TIMESEQ_FADE_DAY_BGM;

    if ((CURRENT_TIME >= CLOCK_TIME(18, 0)) || (CURRENT_TIME <= CLOCK_TIME(6,0))) {
        SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0);
        play->envCtx.timeSeqState = TIMESEQ_NIGHT_BEGIN_SFX;
    } else if (CURRENT_TIME > CLOCK_TIME(17, 10)) {
        SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 240);
        play->envCtx.timeSeqState = TIMESEQ_NIGHT_BEGIN_SFX;
    }

    if ((CURRENT_TIME >= CLOCK_TIME(18, 0)) || (CURRENT_TIME <= CLOCK_TIME(6, 0))) {
        Audio_PlayAmbience(play->sceneSequences.ambienceId);
        Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_0, 1, 1);
        play->envCtx.timeSeqState = TIMESEQ_NIGHT_DELAY;
    }

    if ((CURRENT_TIME >= CLOCK_TIME(19, 0)) || (CURRENT_TIME <= CLOCK_TIME(6, 0))) {
        Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_0, 1, 0);
        Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_1 << 4 | AMBIENCE_CHANNEL_CRITTER_3, 1, 1);
        play->envCtx.timeSeqState = TIMESEQ_DAY_BEGIN_SFX;
    }

    if ((CURRENT_TIME >= CLOCK_TIME(5, 0)) && (CURRENT_TIME <= CLOCK_TIME(6, 0))) {
        Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_1 << 4 | AMBIENCE_CHANNEL_CRITTER_3, 1, 0);
        Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_CRITTER_4 << 4 | AMBIENCE_CHANNEL_CRITTER_5, 1, 1);
        play->envCtx.timeSeqState = TIMESEQ_DAY_DELAY;
    }

    // BGM fix for instant day skip
    u8 dayMinusOne = ((void)0, gSaveContext.save.day) - 1;
    if ((CURRENT_TIME >= CLOCK_TIME(6, 0)) && (CURRENT_TIME <= CLOCK_TIME(17, 10))) {
        Audio_PlaySceneSequence(play->sceneSequences.seqId, dayMinusOne);
        play->envCtx.timeSeqState = TIMESEQ_FADE_DAY_BGM;
    }
}

#include "overlays/actors/ovl_En_Test4/z_en_test4.h"
#include "overlays/actors/ovl_Obj_Tokei_Step/z_obj_tokei_step.h"
#include "overlays/actors/ovl_Obj_Tokeidai/z_obj_tokeidai.h"

void dsot_ObjEnTest4_fix(EnTest4* this, PlayState* play);
void dsot_ObjTokeiStep_fix(ObjTokeiStep* this, PlayState* play);
void dsot_ObjTokeidai_fix(ObjTokeidai* this, PlayState* play);

// Calls actor specific fixes.
static void dsot_actor_fixes(PlayState* play) {
    s32 category;
    Actor* actor;
    Player* player = GET_PLAYER(play);
    ActorListEntry* entry;

    ActorContext* actorCtx = &play->actorCtx;

    for (category = 0, entry = actorCtx->actorLists; category < ACTORCAT_MAX;
         entry++, category++) {
        actor = entry->first;

        for (actor = entry->first; actor != NULL; actor = actor->next) {
            switch(actor->id) {
                case ACTOR_EN_TEST4:
                    dsot_ObjEnTest4_fix((EnTest4*)actor, play);
                    break;
                case ACTOR_OBJ_TOKEI_STEP:
                    dsot_ObjTokeiStep_fix((ObjTokeiStep*)actor, play);
                    break;
                case ACTOR_OBJ_TOKEIDAI:
                    dsot_ObjTokeidai_fix((ObjTokeidai*)actor, play);
                    break;
            }
        }
    }
}

#define PAST_MIDNIGHT ((CURRENT_DAY == 3) && (CURRENT_TIME < CLOCK_TIME(6, 0)) && (CURRENT_TIME > CLOCK_TIME(0, 0)))

// z_obj_en_test_4

void EnTest4_GetBellTimeOnDay3(EnTest4* this);

void dsot_ObjEnTest4_fix(EnTest4* this, PlayState* play) {
    this->prevTime = CURRENT_TIME;
    this->prevBellTime = CURRENT_TIME;

    // Change daytime to night manually if necessary.
    if (((this->daytimeIndex = THREEDAY_DAYTIME_DAY) && (CURRENT_TIME > CLOCK_TIME(18, 0))) || (CURRENT_TIME <= CLOCK_TIME(6, 0))) {
        this->daytimeIndex = THREEDAY_DAYTIME_NIGHT;
    }

    // Setup next bell time.
    if (CURRENT_DAY == 3) {
        gSaveContext.save.time--;
        EnTest4_GetBellTimeOnDay3(this);
        gSaveContext.save.time++;
    }
}

// z_obj_tokei_step

void ObjTokeiStep_InitStepsOpen(ObjTokeiStep* this);
void ObjTokeiStep_SetupDoNothingOpen(ObjTokeiStep* this);
void ObjTokeiStep_DrawOpen(Actor* thisx, PlayState* play);

void dsot_ObjTokeiStep_fix(ObjTokeiStep* this, PlayState* play) {
    if (PAST_MIDNIGHT) {
        this->dyna.actor.draw = ObjTokeiStep_DrawOpen;
        ObjTokeiStep_InitStepsOpen(this);
        ObjTokeiStep_SetupDoNothingOpen(this);
        DynaPoly_DisableCollision(play, &play->colCtx.dyna, this->dyna.bgId);
    }
}

// z_obj_tokeidai

#define GET_CLOCK_FACE_ROTATION(currentClockHour) (TRUNCF_BINANG(currentClockHour * (0x10000 / 24.0f)))
#define GET_MINUTE_RING_OR_EXTERIOR_GEAR_ROTATION(currentClockMinute) \
    (TRUNCF_BINANG(currentClockMinute * (0x10000 * 12.0f / 360)))

s32 ObjTokeidai_GetTargetSunMoonPanelRotation(void);
void ObjTokeidai_Init(Actor* thisx, PlayState* play);
void ObjTokeidai_Draw(Actor* thisx, PlayState* play);
void ObjTokeidai_ExteriorGear_Draw(Actor* thisx, PlayState* play);
void ObjTokeidai_Clock_Draw(Actor* thisx, PlayState* play);

void ObjTokeidai_Counterweight_Idle(ObjTokeidai* this, PlayState* play);
void ObjTokeidai_ExteriorGear_Idle(ObjTokeidai* this, PlayState* play);
void ObjTokeidai_TowerClock_Idle(ObjTokeidai* this, PlayState* play);

s32 dsot_ObjTokeidai_get_target_sun_moon_panel_rotation(void) {
    if (CURRENT_TIME >= CLOCK_TIME(18, 0) || CURRENT_TIME < CLOCK_TIME(6, 0)) {
        return 0x8000;
    }
    return 0;
}

void dsot_ObjTokeidai_update_clock(ObjTokeidai* this, u16 currentHour, u16 currentMinute) {
    this->clockTime = CURRENT_TIME;

    // Instantly switch to desired hour.
    u16 clockHour = currentHour;
    if (currentMinute == 0) {
        clockHour--;
        if (clockHour > 23) {
            clockHour = 23;
        }
    }

    this->clockFaceRotation = GET_CLOCK_FACE_ROTATION(clockHour);
    this->clockHour = clockHour;
    this->clockFaceAngularVelocity = 0;
    this->clockFaceRotationTimer = 0;

    // Instantly switch to desired minute.
    u16 clockMinute = currentMinute - 1;
    if (clockMinute > 59) {
        clockMinute = 59;
    }

    this->minuteRingOrExteriorGearRotation = GET_MINUTE_RING_OR_EXTERIOR_GEAR_ROTATION(clockMinute);
    this->clockMinute = clockMinute;
    this->minuteRingOrExteriorGearAngularVelocity = 0x5A;
    this->minuteRingOrExteriorGearRotationTimer = 0;

    // Switch the medalion rotation immediately.
    if (((currentHour != 6) && (currentHour != 18)) || (currentMinute != 0)) {
        this->sunMoonPanelRotation = dsot_ObjTokeidai_get_target_sun_moon_panel_rotation();
        this->sunMoonPanelAngularVelocity = 0;
    }
}

void dsot_ObjTokeidai_fix(ObjTokeidai* this, PlayState* play) {
    s32 type = OBJ_TOKEIDAI_TYPE(&this->actor);
    u16 currentHour = TIME_TO_HOURS_F(CURRENT_TIME);
    u16 currentMinute = TIME_TO_MINUTES_F(CURRENT_TIME);

    switch(type) {
        case OBJ_TOKEIDAI_TYPE_COUNTERWEIGHT_TERMINA_FIELD:
        case OBJ_TOKEIDAI_TYPE_COUNTERWEIGHT_CLOCK_TOWN:
            if (PAST_MIDNIGHT && (this->actionFunc == ObjTokeidai_Counterweight_Idle)) {
                this->actor.shape.rot.y = 0;
                ObjTokeidai_Init((Actor*)this, play);
            }
            break;

        case OBJ_TOKEIDAI_TYPE_EXTERIOR_GEAR_TERMINA_FIELD:
        case OBJ_TOKEIDAI_TYPE_EXTERIOR_GEAR_CLOCK_TOWN:
            if (PAST_MIDNIGHT && (this->actionFunc == ObjTokeidai_ExteriorGear_Idle)) {
                dsot_ObjTokeidai_update_clock(this, 0, 0);
                this->actor.draw = ObjTokeidai_ExteriorGear_Draw;
                ObjTokeidai_Init((Actor*)this, play);
            } else {
                dsot_ObjTokeidai_update_clock(this, currentHour, currentMinute);
            }
            break;

        case OBJ_TOKEIDAI_TYPE_TOWER_CLOCK_TERMINA_FIELD:
        case OBJ_TOKEIDAI_TYPE_TOWER_CLOCK_CLOCK_TOWN:
            if (PAST_MIDNIGHT && (this->actionFunc == ObjTokeidai_TowerClock_Idle)) {
                dsot_ObjTokeidai_update_clock(this, 0, 0);
                this->actor.draw = ObjTokeidai_Clock_Draw;
                ObjTokeidai_Init((Actor*)this, play);
            } else {
                dsot_ObjTokeidai_update_clock(this, currentHour, currentMinute);
            }
            break;
        case OBJ_TOKEIDAI_TYPE_STAIRCASE_TO_ROOFTOP:
            if (PAST_MIDNIGHT) {
                this->actor.draw = ObjTokeidai_Draw;
            }
            break;
        case OBJ_TOKEIDAI_TYPE_WALL_CLOCK:
        case OBJ_TOKEIDAI_TYPE_SMALL_WALL_CLOCK:
            dsot_ObjTokeidai_update_clock(this, currentHour, currentMinute);
            break;
    }
}