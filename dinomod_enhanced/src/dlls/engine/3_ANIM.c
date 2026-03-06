#include "modding.h"
#include "dll_util.h"

#include "dlls/engine/3_animation.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/214_animobj.h"
#include "game/objects/object.h"
#include "sys/memory.h"
#include "sys/menu.h"
#include "sys/objects.h"
#include "dll.h"
#include "functions.h"

#include "recomp/dlls/engine/3_ANIM_recomp.h"

extern s8 _bss_A4[];
extern s8 _bss_198[ANIMCURVES_SCENES_MAX];
extern u8 _bss_3A8[];

extern void dll_3_func_5698(AnimObj_Data* objData, Object* arg1);
extern void dll_3_func_65EC(s32 arg0, s32 arg1, s32 arg2, s32 arg3);

typedef s32 (*Export17)(s32 objectSeqIndex, Object* object, s32 arg2);
static Export17 export17_func; 
static s32 export17_hijack(s32 objectSeqIndex, Object* object, s32 arg2);

RECOMP_HOOK_DLL(dll_3_ctor) void anim_ctor_hook(DLLFile *dll) {
    export17_func = dinomod_hijack_dll_export(dll, 17, export17_hijack);
}

RECOMP_HOOK_RETURN_DLL(dll_3_dtor) void anim_dtor_hook() {
    export17_func = NULL;
}

static s32 export17_hijack(s32 objectSeqIndex, Object* object, s32 arg2) {
    if (object->def != NULL) {
        // @recomp: Ignore if the given index is out of bounds. The actual function has a bug in this
        //          specific case where it will acquire a sequence slot but never free it when it sees
        //          that the index is out of bounds. Over time, this will make it impossible for any
        //          object sequence to play for the rest of the session.
        if (objectSeqIndex < 0 || objectSeqIndex >= object->def->numSequences || object->def->pSeq == NULL) {
            return -1;
        }
        // @recomp: Ignore if the ObjDef sequence ID is -1. This is new behavior required by dinomod.
        if (object->def->pSeq[objectSeqIndex] == -1) {
            return -1;
        }
    }

    return export17_func(objectSeqIndex, object, arg2);
}

// TODO: replace with clean match from the decomp, when available
RECOMP_PATCH s32 dll_3_func_6620(Object *arg0, Object *arg1, AnimObj_Data *arg2, s32 arg3, s8 arg4) {
    s32 sp54;
    s32 sp4C[2];
    Object *temp_v0_3;
    f32 temp_fv0;
    f32 var_fa0;

    sp54 = (u8)(arg3 >> 8);
    switch ((u8)arg3) {                              /* switch 1 */
    case 2:                                         /* switch 1 */
        if (arg4 != 0) {
            break;
        }
        sp4C[0] = 0x19;
        sp4C[1] = 0x15;
        if (arg2->unk28 < 0) {
            // @fake
            //if (&sp54) {}
            arg2->unk28 = gDLL_26_Curves->vtbl->func_1E4(arg0->srt.transl.x, arg0->srt.transl.y, arg0->srt.transl.z, sp4C, 2, sp54);
            if (arg2->unk28 >= 0) {
                if (arg2->unk2C != NULL) {
                    mmFree(arg2->unk2C);
                    arg2->unk2C = NULL;
                }
                arg2->unk2C = mmAlloc(0x2C, 0x11, NULL);
                if (arg2->unk2C != NULL) {
                    dll_3_func_5698(arg2->unk2C, (Object *) arg2->unk28);
                } else {
                    arg2->unk28 = -1;
                }
            }
        }
        break;
    case 9:                                         /* switch 1 */
        if (arg4 != 0) {
            break;
        }
        arg2->unk8C |= 1;
        break;
    case 18:                                        /* switch 1 */
        if (arg4 != 0) {
            break;
        }
        if (_bss_3A8[arg2->unk63] & 0x10) {
            _bss_3A8[arg2->unk63] &= ~0x10;
        } else {
            _bss_3A8[arg2->unk63] |= 0x10;
        }
        break;
    case 14:                                        /* switch 1 */
        if (arg4 != 0) {
            break;
        }
        if (_bss_198[arg2->unk63] == 0) {
            gDLL_28_ScreenFade->vtbl->fade(sp54, 1);
        }
        break;
    case 15:                                        /* switch 1 */
        if (arg4 != 0) {
            break;
        }
        if (_bss_198[arg2->unk63] == 0) {
            gDLL_28_ScreenFade->vtbl->fade_reversed(sp54, 1);
        }
        break;
    case 20:                                        /* switch 1 */
        dll_3_func_65EC(0x59, sp54 & 0x7F, 1, 0x78);
        break;
    case 23:                                        /* switch 1 */
        if (arg4 != 0) {
            break;
        }
        if (sp54 < arg1->def->numModels) {
            if ((arg1->group == 1) && (arg1->modelInstIdx == 2)) {
                return 1;
            }
            func_80023A18(arg1, sp54);
        }
        break;
    case 24:                                        /* switch 1 */
        if (arg1->group == 1) {
            ((DLL_210_Player*)arg1->dll)->vtbl->func28(arg1, sp54);
        }
        break;
    case 25:                                        /* switch 1 */
        if (arg1->group == 1) {
            ((DLL_210_Player*)arg1->dll)->vtbl->func29(arg1, sp54);
        }
        break;
    case 26:                                        /* switch 1 */
        dll_3_func_65EC(0x54, 4, 0, 0);
        break;
    case 33:                                        /* switch 1 */
        arg2->unk7A |= 0x400;
        arg2->unk142_4 = sp54;
        break;
    case 34:                                        /* switch 1 */
        arg2->unk7A &= ~0x400;
        arg2->unk142_4 = 0;
        break;
    case 35:                                        /* switch 1 */
        gDLL_29_Gplay->vtbl->checkpoint(&arg1->srt.transl, arg1->srt.yaw, 0, map_get_layer());
        break;
    case 36:                                        /* switch 1 */
        gDLL_29_Gplay->vtbl->checkpoint(NULL, 0, 1, map_get_layer());
        break;
    case 37:                                        /* switch 1 */
        ((DLL_210_Player*)get_player()->dll)->vtbl->func69(get_player(), sp54);
        break;
    default:                                        /* switch 1 */
        break;
    }

    switch ((u8)arg3) {                             /* switch 2 */
    case 0:                                     /* switch 2 */
        *_bss_A4 = 1;
        return 0;
    case 5:                                     /* switch 2 */
        gDLL_6_AMSFX->vtbl->func_480(arg1);
        break;
    case 6:                                     /* switch 2 */
        gDLL_6_AMSFX->vtbl->func_480(NULL);
        break;
    case 7:                                     /* switch 2 */
        if (arg4 == 0) {
            camera_enable_y_offset();
            temp_v0_3 = get_player();
            if (temp_v0_3 != NULL) {
                temp_fv0 = vec3_distance_xz(&temp_v0_3->positionMirror, &arg0->positionMirror);
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
    case 10:                                    /* switch 2 */
        // @recomp: Replace with new credits subcommand (original patch by MusicalProgrammer)
        //func_8000F64C(0x12, sp54);
        menu_set(MENU_15);
        break;
    case 11:                                    /* switch 2 */
        // @recomp: Replace with new collision toggle subcommand (original patch by MusicalProgrammer)
        //func_8000F64C(0x11, sp54);
        arg1->objhitInfo->unk5B = sp54;
        break;
    case 12:                                    /* switch 2 */
        func_8000F6CC();
        break;
    case 13:                                    /* switch 2 */
        gDLL_6_AMSFX->vtbl->func_A6C(arg1);
        break;
    case 16:                                    /* switch 2 */
        arg2->unk89 = sp54;
        break;
    case 23:                                    /* switch 2 */
        if ((arg4 == 0) && (sp54 < arg1->def->numModels)) {
            func_80023A18(arg1, sp54);
        }
        break;
    case 27:                                    /* switch 2 */
        gDLL_29_Gplay->vtbl->set_obj_group_status((s32) arg1->mapID, sp54, 1);
        break;
    case 28:                                    /* switch 2 */
        gDLL_29_Gplay->vtbl->set_obj_group_status((s32) arg1->mapID, sp54, 0);
        break;
    case 29:                                    /* switch 2 */
        gDLL_29_Gplay->vtbl->set_map_setup((s32) arg1->mapID, sp54);
        break;
    case 19:                                    /* switch 2 */
        if (arg4 == 0) {
            _bss_3A8[arg2->unk63] &= ~0x10;
        }
        break;
    case 30:                                    /* switch 2 */
        if (arg4 == 0) {
            _bss_3A8[arg2->unk63] |= 0x10;
        }
        break;
    case 31:                                    /* switch 2 */
        gDLL_29_Gplay->vtbl->restart_clear();
        break;
    case 32:                                    /* switch 2 */
        gDLL_29_Gplay->vtbl->restart_goto();
        break;
    }
    return 1;
}
