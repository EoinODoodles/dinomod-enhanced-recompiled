#include "dll_util.h"
#include "modding.h"
#include "recomputils.h"

#include "dlls/engine/3_animation.h"
#include "dlls/objects/210_player.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/gfx/animseq.h"
#include "sys/memory.h"
#include "sys/menu.h"
#include "sys/objects.h"
#include "sys/segment_1460.h"
#include "dll.h"

#include "recomp/dlls/engine/3_ANIM_recomp.h"

#define DEBUG_ANIM

// Maximum number of active object sequences
#define MAX_SEQSLOTS 45

// Some names inferred from default.dol
enum AnimEventType {
    ANIM_EVT_SETDURATION = -1,
    ANIM_EVT_SETTIME = 0,
    ANIM_EVT_MOVEMODE = 1,
    ANIM_EVT_ANIM = 2,
    ANIM_EVT_OVERRIDE = 3,
    ANIM_EVT_VTXANIM = 4,
    ANIM_EVT_SOFTWARE = 5,
    ANIM_EVT_SFX = 6,
    ANIM_EVT_GROUND_MODE = 7,
    ANIM_EVT_TUNE = 8,
    ANIM_EVT_ANGLE_MODE = 9,
    ANIM_EVT_LOOK_AT = 10,
    ANIM_EVT_CODE = 11,
    ANIM_EVT_SPEECH = 12,
    ANIM_EVT_ENVFX = 13, // TODO: "ENVFX" is presumably the official name but maybe we can do better?
    ANIM_EVT_STORYBOARD = 14,
    ANIM_EVT_SFX_WITH_DURATION = 15
};

// Subtypes for ANIM_EVT_ENVFX
enum AnimEnvFxEventType {
    ANIM_EVT_ENVFX_SET_MUSIC = 0,
    ANIM_EVT_ENVFX_APPLY = 2,
    ANIM_EVT_ENVFX_PARTFX = 3,
    ANIM_EVT_ENVFX_4 = 4, // noop
    ANIM_EVT_ENVFX_PROJGFX = 5,
    ANIM_EVT_ENVFX_WARP = 6,
    ANIM_EVT_ENVFX_SFX = 7,
    ANIM_EVT_ENVFX_BLINK = 8,
    ANIM_EVT_ENVFX_SCREEN_FX = 9,
    ANIM_EVT_ENVFX_SUBTITLES = 10,
    ANIM_EVT_ENVFX_SET_BIT = 11,
    ANIM_EVT_ENVFX_CLEAR_BIT = 12,
    ANIM_EVT_ENVFX_CMDMENU_BUTTON_OVERRIDE = 13,
    ANIM_EVT_ENVFX_EYELID_R = 14,
    ANIM_EVT_ENVFX_EYELID_L = 15
};

// Subtypes for ANIM_EVT_CODE
enum AnimCodeEventType {
    ANIM_CODE_EVT_JUMPTOTIME = 1,
    ANIM_CODE_EVT_SET = 2,
    ANIM_CODE_EVT_COUNTER_ADD = 3,
    ANIM_CODE_EVT_PAUSE = 4,
    ANIM_CODE_EVT_CONTINUE = 5,
    ANIM_CODE_EVT_6 = 6,
    ANIM_CODE_EVT_MESSAGE = 7,
    ANIM_CODE_EVT_DECISION = 8,
    ANIM_CODE_EVT_JUMPTARGET = 9,
    ANIM_CODE_EVT_JUMPTOLABEL = 10
};

// Subtypes for ANIM_CODE_EVT_SET
enum AnimSetCodeEventType {
    ANIM_CODE_EVT_SET_MESSAGE = 0,
    ANIM_CODE_EVT_SET_COUNTER = 1,
    ANIM_CODE_EVT_SET_ANIMCOUNT1 = 3,
    ANIM_CODE_EVT_SET_ANIMCOUNT2 = 4,
    ANIM_CODE_EVT_SET_FLAGS = 5,
    ANIM_CODE_EVT_SET_BIT = 6
};

// Subtypes for ANIM_CODE_EVT_COUNTER_ADD
enum AnimCounterAddCodeEventType {
    ANIM_CODE_EVT_COUNTER_ADD_ENABLED = 0,
    ANIM_CODE_EVT_COUNTER_ADD_DISABLED = 1
};

// Subtypes for ANIM_CODE_EVT_6
enum Anim6CodeEventType {
    ANIM_CODE_EVT_6_END = 0, // signal end of seq for the current object
    ANIM_CODE_EVT_6_2 = 2, // curve related
    ANIM_CODE_EVT_6_5 = 5, // sfx related, noop
    ANIM_CODE_EVT_6_6 = 6, // sfx related, noop
    ANIM_CODE_EVT_6_CAMERA_SHAKE = 7,
    ANIM_CODE_EVT_6_9 = 9,
    ANIM_CODE_EVT_6_COUNTUP_TIMER = 10,
    ANIM_CODE_EVT_6_COUNTDOWN_TIMER = 11,
    ANIM_CODE_EVT_6_COUNTDOWN_TIMER_SFX = 12,
    ANIM_CODE_EVT_6_SFX_STOP = 13,
    ANIM_CODE_EVT_6_14 = 14,
    ANIM_CODE_EVT_6_15 = 15,
    ANIM_CODE_EVT_6_16 = 16,
    ANIM_CODE_EVT_6_TOGGLE_LETTERBOX = 18,
    ANIM_CODE_EVT_6_ENABLE_LETTERBOX = 19,
    ANIM_CODE_EVT_6_STATIC_CAMERA = 20,
    ANIM_CODE_EVT_6_SET_MODEL = 23,
    ANIM_CODE_EVT_6_24 = 24,
    ANIM_CODE_EVT_6_25 = 25,
    ANIM_CODE_EVT_6_NORMAL_CAMERA = 26,
    ANIM_CODE_EVT_6_ENABLE_OBJ_GROUP = 27,
    ANIM_CODE_EVT_6_DISABLE_OBJ_GROUP = 28,
    ANIM_CODE_EVT_6_SET_ACT = 29,
    ANIM_CODE_EVT_6_DISABLE_LETTERBOX = 30,
    ANIM_CODE_EVT_6_RESTART_CLEAR = 31,
    ANIM_CODE_EVT_6_RESTART_GOTO = 32,
    ANIM_CODE_EVT_6_33 = 33,
    ANIM_CODE_EVT_6_34 = 34,
    ANIM_CODE_EVT_6_SAVEPOINT = 35,
    ANIM_CODE_EVT_6_SAVEPOINT_NO_LOCATION = 36,
    ANIM_CODE_EVT_6_TOGGLE_PLAYER_CONTROL = 37
};

enum AnimCurvesKeyframeChannels {
/*00*/ CHANNEL_headRotateZ = 0,
/*01*/ CHANNEL_headRotateX = 1,
/*02*/ CHANNEL_headRotateY = 2,
/*03*/ CHANNEL_opacity = 3,
/*04*/ CHANNEL_dayTime = 4,
/*05*/ CHANNEL_scale = 5,
/*06*/ CHANNEL_rotateZ = 6,
/*07*/ CHANNEL_rotateY = 7,
/*08*/ CHANNEL_rotateX = 8,
/*09*/ CHANNEL_animSpeed = 9,
/*0A*/ CHANNEL_animBlendSpeed = 10,
/*0B*/ CHANNEL_translateZ = 11,
/*0C*/ CHANNEL_translateY = 12,
/*0D*/ CHANNEL_translateX = 13,
/*0E*/ CHANNEL_fieldOfView = 14,
/*0F*/ CHANNEL_eyeX = 15,
/*10*/ CHANNEL_eyeY = 16,
/*11*/ CHANNEL_jaw = 17,
/*12*/ CHANNEL_soundVolume = 18
};

enum KeyframeInterpolationType {
    KF_INTERP_Bezier = 0,
    KF_INTERP_Linear = 1,
    KF_INTERP_Stepped = 2
};

enum ActorSettings {
    ACTORSETTING_UNK4000 = 0x4000, // look for existing obj to override
    ACTORSETTING_UNK8000 = 0x8000
};

// TODO: better name
enum ActorUpperSettings {
    ACTORUSETTING_DONT_OVERRIDE_POS = 0x1,
    ACTORUSETTING_DONT_OVERRIDE_ROT = 0x2,
    ACTORUSETTING_ZERO_YAW = 0x4,
    ACTORUSETTING_SKIPPABLE = 0x8, // whether the seq can be skipped with L trigger
    ACTORUSETTING_NO_LETTERBOX = 0x10,
    ACTORUSETTING_UNK20 = 0x20
};

extern s8 sSeqEnded;
extern s8 _bss_198[MAX_SEQSLOTS];
extern u8 _bss_3A8[MAX_SEQSLOTS];

void anim_func_5698(UnkAnimStruct* arg0, s32 arg1);
void anim_set_camera_module(s32 module, s32 arg1, s32 arg2, s32 arg3);

typedef s32 (*StartObjSequenceFunc)(s32 objectSeqIndex, Object* object, s32 enabledActors);
static StartObjSequenceFunc start_obj_sequence_orig; 
static s32 start_obj_sequence_hijack(s32 objectSeqIndex, Object* object, s32 enabledActors);

RECOMP_HOOK_DLL(anim_ctor) void anim_ctor_hook(DLLFile *dll) {
    start_obj_sequence_orig = dinomod_hijack_dll_export(dll, 17, start_obj_sequence_hijack);
}

RECOMP_HOOK_RETURN_DLL(anim_dtor) void anim_dtor_hook(void) {
    start_obj_sequence_orig = NULL;
}

static s32 start_obj_sequence_hijack(s32 objectSeqIndex, Object* object, s32 enabledActors) {
    if (object->def != NULL) {
        // @recomp: Ignore if the given index is out of bounds. The actual function has a bug in this
        //          specific case where it will acquire a sequence slot but never free it when it sees
        //          that the index is out of bounds. Over time, this will make it impossible for any
        //          object sequence to play for the rest of the session.
        //          Fun fact: This issue is actually fixed in default.dol!
        if (objectSeqIndex < 0 || objectSeqIndex >= object->def->numSequences || object->def->pSeq == NULL) {
            return -1;
        }
        // @recomp: Ignore if the ObjDef sequence ID is -1. This is new behavior required by dinomod.
        if (object->def->pSeq[objectSeqIndex] == -1) {
            return -1;
        }
    }

    return start_obj_sequence_orig(objectSeqIndex, object, enabledActors);
}

// size:0x8
typedef struct {
    u32 uid;
    u16 settings;
    u16 objID;
} Actor;

typedef struct {
    Vec3f coord; 
    s8 unkC;
} CameraFunc15Unk_unk74; //Related to CameraAction and Unk_DLL2_Func888? TO-DO: figure out

typedef struct {
    Object *obj;
    s32 preemptTime;
} PreemptTime;

typedef struct{
    Object* object;
    Object* overrideObject;
} ANIMActorOverride;

typedef struct {
    s16 seqSlot;
    s16 startTime;
    u16 numActors;
} Activate;

typedef struct {
    Object* actor;
    s16 value;
    s8 type;
} QueuedEnvFx;

typedef struct {
    s32* events; // pointer to list of code events
    s16 numEvents;
    s16 time; // timestamp of code event
} CodeEventList;

extern f32 anim_channel_value(AnimObj_Data* st, s32 channel, s32 time);
extern Object* anim_toggle_override(Object* animObj, AnimObj_Data* st, AnimObj_Setup* setup);

extern s32 sCodeEvtQueueCount;
extern CodeEventList sCodeEvtQueue[20];
extern QueuedEnvFx sEnvFxQueue[10];
extern s8 sEnvFxQueueCount;
extern s8 _bss_89;
extern s8 _bss_8A;
extern s8 _bss_8B;

extern s8 anim_get_free_sfx_slot(AnimObj_Data* st);

RECOMP_PATCH s32 anim_process_event(Object* animObj, ModelInstance* animObjModelInst, AnimCurvesEvent** events, s8 arg3, s32* arg4) {
    AnimState* temp_v1;
    f32 var_fv0;
    f32 var_fv1;
    ModelInstanceBlendshape *blendShape;
    s32 var_v0;
    s32 var_a0;
    Object* actor;
    s32 pad;
    AnimObj_Data* st;
    AnimObj_Setup* setup;
    s8 var_t0;
    s8 arg3_8;
    s8 sp30;
    s32 pad2;
    AnimCurvesEvent* evt; // sp3C

    evt = *events;
    sp30 = arg3 & 1;
    var_t0 = arg3 & 2;
    arg3_8 = arg3 & 8;
    if (sp30 == 0) {
        var_t0 = 1;
    }
    st = animObj->data;
    setup = (AnimObj_Setup*)animObj->setup;
    actor = st->actor;
    if (actor == NULL) {
        actor = animObj;
    }
    switch (evt->type) {
    case ANIM_EVT_ANIM:
        if (arg3_8) { break; }
        st->modAnimIdx = (s16) (evt->params & 0xFFF); // move
        st->modAnimStartFrame = (u8) ((evt->params >> 8) & 0xF0); // startframe
        if (animObjModelInst == NULL) {
            break;
        }
        temp_v1 = animObjModelInst->animState0;
        if (actor->curModAnimId == st->modAnimIdx) {
            if (temp_v1->unk60[0] != 0) {
                var_v0 = 0;
            } else {
                var_v0 = 1;
            }
        } else {
            var_v0 = 1;
        }
        temp_v1 = animObjModelInst->animState0;
        if ((var_t0 != 0) && (var_v0 != 0) && (animObjModelInst != NULL)) {
            temp_v1->curAnimationFrame[0] = temp_v1->totalAnimationFrames[0] * actor->animProgress;
            if (st->channelTotalKeys[CHANNEL_animBlendSpeed] != 0) {
                var_fv1 = anim_channel_value(st, CHANNEL_animBlendSpeed, st->time - 1);
            } else {
                var_fv1 = 8.0f;
            }
            if (var_fv1 < 1.0f) {
                var_v0 = 1;
            } else {
                var_v0 = 0;
            }
            func_80023D30(actor, st->modAnimIdx, st->modAnimStartFrame * 0.00390625f, var_v0);
            st->unk20 = 1.0f;
        }
        break;
    case ANIM_EVT_MOVEMODE:
        if (arg3_8) { break; }
        if ((st->unk87 != 0) && (_bss_198[st->seqSlot] != 0)) {
            st->unk84 = 0;
        } else {
            st->unk84 = 1 - st->unk84;
        }
        break;
    case ANIM_EVT_GROUND_MODE:
        st->groundMode = 1 - st->groundMode;
        break;
    case ANIM_EVT_OVERRIDE:
        if (arg3_8) { break; }

        if (!(arg3 & 4)) {
            actor = anim_toggle_override(animObj, st, setup);
            actor->curModAnimIdLayered = -1;
        }
        break;
    case ANIM_EVT_CODE:
        if (sCodeEvtQueueCount >= 20) {
            STUBBED_PRINTF("CODE OVERFLOW\n");
        }
        if ((var_t0 != 0) && (evt->params > 0) && (sCodeEvtQueueCount < 20)) {
            sCodeEvtQueue[sCodeEvtQueueCount].events = (s32*)(evt + 1);
            sCodeEvtQueue[sCodeEvtQueueCount].time = st->time;
            sCodeEvtQueue[sCodeEvtQueueCount].numEvents = evt->params;
            sCodeEvtQueueCount++;
        }
        st->eventIdx += evt->params;
        break;
    case ANIM_EVT_VTXANIM:
        if ((arg3_8 == 0) && (var_t0 != 0) && (animObjModelInst != NULL)) {
            if (animObjModelInst->model->blendshapes != NULL) {
                // (evt->params & 0xFF) == "move"
                var_fv0 = (evt->params >> 8) & 0xFF; // vel
                if (var_fv0 != 0.0f) {
                    var_fv0 = 1.0f / var_fv0;
                } else {
                    var_fv0 = 1.0f;
                }
                if ((animObjModelInst->model->unk71 & 1) && ((evt->params & 0xFF) < 0xF)) {
                    blendShape = animObjModelInst->blendshapes;
                    blendShape += 2;
                    func_8001AF04(animObjModelInst, blendShape->id, (evt->params & 0xFF) - 1, var_fv0, 2, 0);
                } else {
                    blendShape = animObjModelInst->blendshapes;
                    func_8001AF04(animObjModelInst, blendShape->id, (evt->params & 0xFF) - 1, var_fv0, 0, 0);
                }
            }
        }
        break;
    case ANIM_EVT_STORYBOARD:
        if (arg3_8) { break; }
        gDLL_1_cmdmenu->vtbl->open_tutorial_textbox(evt->params, 160, 140);
        break;
    case ANIM_EVT_ENVFX:
        if ((sp30 == 0) && (((evt->params >> 0xC) & 0xF) != ANIM_EVT_ENVFX_BLINK) && (sEnvFxQueueCount < 10)) {
            sEnvFxQueue[sEnvFxQueueCount].actor = actor;
            sEnvFxQueue[sEnvFxQueueCount].type = (evt->params >> 0xC) & 0xF;
            if ((sEnvFxQueue[sEnvFxQueueCount].type == ANIM_EVT_ENVFX_SET_BIT) || (sEnvFxQueue[sEnvFxQueueCount].type == ANIM_EVT_ENVFX_CLEAR_BIT)) {
                // Gamebit IDs are 16-bit, so it's stored in the next event slot
                sEnvFxQueue[sEnvFxQueueCount].value = (evt + 1)->params;
                sEnvFxQueueCount += 1;
            } else {
                sEnvFxQueue[sEnvFxQueueCount].value = evt->params & 0xFFF;
                sEnvFxQueueCount += 1;
            }
        }
        break;
    }
    if (sp30 != 0) {
        return 0;
    }
    if ((_bss_89 != 0) || (_bss_8A != 0)) {
        if (evt->type == ANIM_EVT_ENVFX) {
            switch ((evt->params >> 0xC) & 0xF) {
            case ANIM_EVT_ENVFX_APPLY:
                func_80000860(actor, actor, evt->params & 0xFFF, 0);
                break;
            case ANIM_EVT_ENVFX_WARP:
                warpPlayer(evt->params & 0xFFF, 0);
                break;
            }
        }
        return 0;
    }
    switch (evt->type) {
    case ANIM_EVT_GROUND_MODE:
    case ANIM_EVT_TUNE:
    case ANIM_EVT_ANGLE_MODE:
    case ANIM_EVT_LOOK_AT:
    case ANIM_EVT_CODE:
    case ANIM_EVT_SPEECH:
    case ANIM_EVT_STORYBOARD:
        break;
    case ANIM_EVT_SFX:
#ifdef DEBUG_ANIM
            recomp_printf("SOUND: %x\n", ((evt->params & 0xFFF) + 1));
#endif
    if (arg3_8) { break; }
    if (((evt->params >> 0xC) & 0xF) != 0xF) {
            gDLL_6_AMSFX->vtbl->play(animObj, 
                                     ((evt->params & 0xFFF) + 1), 
                                     ((((evt->params >> 0xC) & 0xF) * 7) + 0x16), 
                                     NULL, 
                                     NULL, 0, NULL);
        } else {
            if (gDLL_6_AMSFX->vtbl->is_playing(st->sfxHandles[3]) != 0) {
                gDLL_6_AMSFX->vtbl->stop(st->sfxHandles[3]);
            }
            st->sfxTimer[3] = 32000;
            gDLL_6_AMSFX->vtbl->play(animObj, 
                                     ((evt->params & 0xFFF) + 1), 
                                     (s32) anim_channel_value(st, CHANNEL_soundVolume, st->time), 
                                     &st->sfxHandles[3], 
                                     NULL, 0, NULL);
        }
        break;
    case ANIM_EVT_ENVFX:
        switch ((evt->params >> 0xC) & 0xF) {
        case ANIM_EVT_ENVFX_SET_MUSIC:
            if (arg3_8) { break; }
            gDLL_5_AMSEQ2->vtbl->set(animObj, (evt->params & 0xFFF) + 1, STUBBED_STR("anim.c"), 0, STUBBED_STR("(e->val&0xfff)+1"));
            break;
        case ANIM_EVT_ENVFX_APPLY:
            func_80000860(actor, actor, evt->params & 0xFFF, 0);
            break;
        case ANIM_EVT_ENVFX_WARP:
            if (arg3_8) { break; }
            warpPlayer(evt->params & 0xFFF, 0);
            break;
        case ANIM_EVT_ENVFX_SFX:
            if (arg3_8) { break; }
            if (st->unk30 != 0) {
                gDLL_6_AMSFX->vtbl->stop(st->unk30);
            }
            st->unk30 = 0;
            gDLL_6_AMSFX->vtbl->play(animObj, 
                                     ((evt->params & 0xFFF) + 1), 
                                     ((((evt->params >> 0xC) & 0xF) * 7) + 0x16), 
                                     &st->unk30, NULL, 0, NULL);
            break;
        case ANIM_EVT_ENVFX_BLINK:
            if (arg3_8) { break; }
            st->blinkFrameR = evt->params;
            st->blinkFrameL = st->blinkFrameR & 0xFFF;
            break;
        case ANIM_EVT_ENVFX_EYELID_R:
            if (arg3_8) { break; }
            st->blinkFrameR = evt->params & 0xFFF;
            break;
        case ANIM_EVT_ENVFX_EYELID_L:
            if (arg3_8) { break; }
            st->blinkFrameL = evt->params & 0xFFF;
            break;
        }
        break;
    case ANIM_EVT_SFX_WITH_DURATION:
        if (arg3_8) { break; }
        anim_get_free_sfx_slot(st);
        if (((evt->params >> 0xC) & 0xF) != 0xF) {
            gDLL_6_AMSFX->vtbl->play(animObj, 
                                     ((evt->params & 0xFFF) + 1), 
                                     ((((evt->params >> 0xC) & 0xF) * 7) + 0x16), 
                                     &st->sfxHandles[st->sfxNextSlot], 
                                     NULL, 0, NULL);
            var_a0 = st->sfxNextSlot;
            st->sfxNextSlot++;
            if (st->sfxNextSlot >= 3) {
                st->sfxNextSlot = 0;
            }
        } else {
            if (gDLL_6_AMSFX->vtbl->is_playing(st->sfxHandles[3]) != 0) {
                gDLL_6_AMSFX->vtbl->stop(st->sfxHandles[3]);
            }
            gDLL_6_AMSFX->vtbl->play(animObj, 
                                     ((evt->params & 0xFFF) + 1), 
                                     (s32) anim_channel_value(st, CHANNEL_soundVolume, st->time), 
                                     &st->sfxHandles[3], NULL, 0, NULL);
            var_a0 = 3;
        }
        evt->delay = (evt + 1)->delay;
        (evt + 1)->type = 0x63;
        st->sfxTimer[var_a0] = (evt + 1)->params;
        break;
    }
    return 0;
}

#ifdef DEBUG_ANIM
static char anim_command_names[][64] = {
    "0", // 0, // remove override?
    "1", // 1,
    "2", // 2, // curve related
    "3", // 3,
    "4", // 4,
    "5", // 5, // sfx related, noop
    "6", // 6, // sfx related, noop
    "CAMERA_SHAKE", // 7,
    "8", // 8,
    "9", // 9,
    "COUNTUP_TIMER", // 10,
    "COUNTDOWN_TIMER", // 11,
    "COUNTDOWN_TIMER_SFX", // 12,
    "SFX_STOP", // 13,
    "14", // 14,
    "15", // 15,
    "16", // 16,
    "17", // 17,
    "TOGGLE_LETTERBOX", // 18,
    "ENABLE_LETTERBOX", // 19,
    "STATIC_CAMERA", // 20,
    "21", // 21,
    "22", // 22,
    "SET_MODEL", // 23,
    "24", // 24,
    "25", // 25,
    "NORMAL_CAMERA", // 26,
    "ENABLE_OBJ_GROUP", // 27,
    "DISABLE_OBJ_GROUP", // 28,
    "SET_ACT", // 29,
    "DISABLE_LETTERBOX", // 30,
    "RESTART_CLEAR", // 31,
    "RESTART_GOTO", // 32,
    "33", // 33,
    "34", // 34,
    "CHECKPOINT", // 35,
    "CHECKPOINT_NO_LOCATION", // 36,
    "TOGGLE_PLAYER_CONTROL" // 37
};

static char* get_anim_event_6_name(s32 commandID) {
    return anim_command_names[commandID];
}
#endif

RECOMP_PATCH s32 anim_do_code_event_6(Object *animObj, Object *actor, AnimObj_Data *st, s32 arg3, s8 arg4) {
    s32 sp54;
    s32 sp4C[2];
    Object *player;
    f32 playerDistance;
    f32 amplitude;

    sp54 = (u8)(arg3 >> 8);
    arg3 = arg3 & 0xFF;

    #ifdef DEBUG_ANIM
    recomp_printf("[%s] Anim cmd: %s | Arg: %d | arg4: %d\n", actor->def->name, get_anim_event_6_name(arg3), sp54, arg4);
    #endif

    switch (arg3) {
    case ANIM_CODE_EVT_6_2: 
        if (arg4 != 0) {
            break;
        }
        sp4C[0] = 0x19;
        sp4C[1] = 0x15;
        // if ((s32)&sp54) {}// @fake
        if (st->unk28 < 0) {
            st->unk28 = gDLL_26_Curves->vtbl->func_1E4(animObj->srt.transl.x, animObj->srt.transl.y, animObj->srt.transl.z, sp4C, 2, sp54);
            if (st->unk28 >= 0) {
                if (st->unk2C != NULL) {
                    mmFree(st->unk2C);
                    st->unk2C = NULL;
                }
                st->unk2C = mmAlloc(sizeof(UnkAnimStruct), ALLOC_TAG_ANIMSEQ_COL, ALLOC_NAME("anim:curvedata"));
                if (st->unk2C != NULL) {
                    anim_func_5698(st->unk2C, st->unk28);
                } else {
                    st->unk28 = -1;
                }
            }
        }
        break;
    case ANIM_CODE_EVT_6_9: 
        if (arg4 != 0) {
            break;
        }
        st->unk8C |= 1;
        break;
    case ANIM_CODE_EVT_6_TOGGLE_LETTERBOX:
        if (arg4 != 0) {
            break;
        }
        if (_bss_3A8[st->seqSlot] & ACTORUSETTING_NO_LETTERBOX) {
            _bss_3A8[st->seqSlot] &= ~ACTORUSETTING_NO_LETTERBOX;
        } else {
            _bss_3A8[st->seqSlot] |= ACTORUSETTING_NO_LETTERBOX;
        }
        break;
    case ANIM_CODE_EVT_6_14:
        if (arg4 != 0) {
            break;
        }
        if (_bss_198[st->seqSlot] == 0) {
            gDLL_28_ScreenFade->vtbl->fade(sp54, SCREEN_FADE_BLACK);
        }
        break;
    case ANIM_CODE_EVT_6_15:
        if (arg4 != 0) {
            break;
        }
        if (_bss_198[st->seqSlot] == 0) {
            gDLL_28_ScreenFade->vtbl->fade_reversed(sp54, SCREEN_FADE_BLACK);
        }
        break;
    case ANIM_CODE_EVT_6_STATIC_CAMERA:
        anim_set_camera_module(DLL_ID_CAMSTATIC, sp54 & 0x7F, 1, 0x78);
        break;
    case ANIM_CODE_EVT_6_SET_MODEL:
        if (arg4 != 0) {
            break;
        }
        if (sp54 < actor->def->numModels) {
            if ((actor->controlNo == OBJCONTROL_Player) && (actor->modelInstIdx == 2)) {
                return 1;
            }
            STUBBED_PRINTF(" MODEL NO %i \n", actor->modelInstIdx);
            obj_set_model(actor, sp54);
        }
        break;
    case ANIM_CODE_EVT_6_24:
        if (actor->controlNo == OBJCONTROL_Player) {
            ((DLL_210_Player*)actor->dll)->vtbl->func28(actor, sp54);
        }
        break;
    case ANIM_CODE_EVT_6_25:
        if (actor->controlNo == OBJCONTROL_Player) {
            ((DLL_210_Player*)actor->dll)->vtbl->func29(actor, sp54);
        }
        break;
    case ANIM_CODE_EVT_6_NORMAL_CAMERA:
        anim_set_camera_module(DLL_ID_CAMNORMAL, 4, 0, 0);
        break;
    case ANIM_CODE_EVT_6_33:
        st->unk7A |= ANIM7AFLAG_UNK400;
        st->unk142_4 = sp54;
        break;
    case ANIM_CODE_EVT_6_34:
        st->unk7A &= ~ANIM7AFLAG_UNK400;
        st->unk142_4 = 0;
        break;
    case ANIM_CODE_EVT_6_SAVEPOINT:
        gDLL_29_Gplay->vtbl->savepoint(&actor->srt.transl, actor->srt.yaw, 0, map_get_layer());
        break;
    case ANIM_CODE_EVT_6_SAVEPOINT_NO_LOCATION:
        gDLL_29_Gplay->vtbl->savepoint(NULL, 0, GPLAY_SAVEPOINT_SkipMapSave, map_get_layer());
        break;
    case ANIM_CODE_EVT_6_TOGGLE_PLAYER_CONTROL:
        ((DLL_210_Player*)get_player()->dll)->vtbl->func69(get_player(), sp54);
        break;
    default:
        break;
    }

    switch (arg3) {
    case ANIM_CODE_EVT_6_END: 
        sSeqEnded = TRUE;
        return 0;
    case ANIM_CODE_EVT_6_5: 
        gDLL_6_AMSFX->vtbl->func_480(actor);
        break;
    case ANIM_CODE_EVT_6_6: 
        gDLL_6_AMSFX->vtbl->func_480(NULL);
        break;
    case ANIM_CODE_EVT_6_CAMERA_SHAKE: 
        if (arg4 == 0) {
            camera_enable_y_offset();

            //@recomp: ignore player distance check if the param's uppermost bit is set
            //(no existing sequence uses have it set)
            if (sp54 & 0x80) {
                amplitude = (2.0f * ((sp54 & 0x7F) - 7)) + 1.0f;
                camera_set_shake_offset(amplitude);
                break;
            }

            player = get_player();
            if (player != NULL) {
                playerDistance = vec3_distance_xz(&player->globalPosition, &animObj->globalPosition);
                amplitude = (2.0f * (sp54 - 7)) + 1.0f;

                if (playerDistance < 200.0f) {
                    if (playerDistance > 50.0f) {
                        playerDistance = (playerDistance - 50.0f) / 150.0f;
                        amplitude *= 1.0f - playerDistance;
                    }
                    camera_set_shake_offset(amplitude);
                }
            }
        }
        break;
    case ANIM_CODE_EVT_6_COUNTUP_TIMER:
        // @recomp: Replace with new credits subcommand (original patch by MusicalProgrammer)
        //func_8000F64C(0x12, sp54);
        switch (sp54) {
        case 1: //restore gameplay menu (for skipping credits)
            menu_set(MENU_GAMEPLAY);
            break;
        default: //play credits
            menu_set(MENU_CREDITS);
            break;
        }
        break;
    case ANIM_CODE_EVT_6_COUNTDOWN_TIMER:
        // @recomp: Replace with new collision toggle subcommand (original patch by MusicalProgrammer)
        //func_8000F64C(0x11, sp54);
        actor->objhitInfo->unk5B = sp54;
        break;
    case ANIM_CODE_EVT_6_COUNTDOWN_TIMER_SFX:
        func_8000F6CC();
        break;
    case ANIM_CODE_EVT_6_SFX_STOP:
        gDLL_6_AMSFX->vtbl->stop_object(actor);
        break;
    case ANIM_CODE_EVT_6_16:
        st->unk89 = sp54;
        break;
    case ANIM_CODE_EVT_6_SET_MODEL:
        if ((arg4 == 0) && (sp54 < actor->def->numModels)) {
            obj_set_model(actor, sp54);
        }
        break;
    case ANIM_CODE_EVT_6_ENABLE_OBJ_GROUP:
        gDLL_29_Gplay->vtbl->set_obj_group_status(actor->mapID, sp54, 1);
        break;
    case ANIM_CODE_EVT_6_DISABLE_OBJ_GROUP:
        gDLL_29_Gplay->vtbl->set_obj_group_status(actor->mapID, sp54, 0);
        break;
    case ANIM_CODE_EVT_6_SET_ACT:
        gDLL_29_Gplay->vtbl->set_act(actor->mapID, sp54);
        break;
    case ANIM_CODE_EVT_6_ENABLE_LETTERBOX:
        if (arg4 == 0) {
            _bss_3A8[st->seqSlot] &= ~ACTORUSETTING_NO_LETTERBOX;
        } 
        else { } // @fake
        break;
    case ANIM_CODE_EVT_6_DISABLE_LETTERBOX:
        if (arg4 == 0) {
            _bss_3A8[st->seqSlot] |= ACTORUSETTING_NO_LETTERBOX;
        }
        break;
    case ANIM_CODE_EVT_6_RESTART_CLEAR:
        gDLL_29_Gplay->vtbl->restart_clear();
        break;
    case ANIM_CODE_EVT_6_RESTART_GOTO:
        gDLL_29_Gplay->vtbl->restart_goto();
        break;
    }
    return 1;
}
