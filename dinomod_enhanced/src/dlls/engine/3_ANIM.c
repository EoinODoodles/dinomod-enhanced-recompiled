#include "modding.h"
#include "dll_util.h"

#include "dlls/engine/3_animation.h"
#include "dlls/objects/210_player.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/gfx/animseq.h"
#include "sys/memory.h"
#include "sys/menu.h"
#include "sys/objects.h"
#include "dll.h"

#include "recomp/dlls/engine/3_ANIM_recomp.h"

// Maximum number of active object sequences
#define MAX_SEQSLOTS 45

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

RECOMP_PATCH s32 anim_do_code_event_6(Object *animObj, Object *actor, AnimObj_Data *st, s32 arg3, s8 arg4) {
    s32 sp54;
    s32 sp4C[2];
    Object *player;
    f32 temp_fv0;
    f32 var_fa0;

    sp54 = (u8)(arg3 >> 8);
    arg3 = arg3 & 0xFF;
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
            player = get_player();
            if (player != NULL) {
                temp_fv0 = vec3_distance_xz(&player->globalPosition, &animObj->globalPosition);
                var_fa0 = (2.0f * (sp54 - 7)) + 1.0f;
                if (temp_fv0 < 200.0f) {
                    if (temp_fv0 > 50.0f) {
                        temp_fv0 = (temp_fv0 - 50.0f) / 150.0f;
                        var_fa0 *= 1.0f - temp_fv0;
                    }
                    camera_set_shake_offset(var_fa0);
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
