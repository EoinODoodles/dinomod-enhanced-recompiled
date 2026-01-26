#include "modding.h"

#include "PR/ultratypes.h"
#include "PR/gbi.h"
#include "dlls/objects/214_animobj.h"
#include "game/gamebits.h"
#include "sys/dll.h"
#include "sys/gfx/modgfx.h"
#include "sys/main.h"
#include "sys/objmsg.h"
#include "sys/objtype.h"
#include "sys/rand.h"
#include "dll.h"
#include "functions.h"

#include "recomp/dlls/objects/481_NWSH_Shrine_recomp.h"

typedef struct {
    s16 unk0;
    s16 unk2;
    s16 unk4;
    s16 unk6;
    s16 unk8;
    s16 unkA;
    s16 unkC;
    s16 unkE;
    s16 unk10;
    u8 unk12;
    u8 unk13;
    u8 unk14;
    u8 unk15;
    u8 unk16;
    u8 unk17;
} NWSH_Shrine_Data;

typedef struct {
    ObjSetup base;
    u8 _unk18[0x1A - 0x18];
    s16 unk1A;
} NWSH_Shrine_Setup;

extern int dll_481_func_C10(Object *self, Object *a1, AnimObj_Data *a2, s8 a3);
extern void dll_481_func_1084(Object* self);

RECOMP_PATCH void dll_481_setup(Object *self, NWSH_Shrine_Setup *setup, s32 arg2) {
    NWSH_Shrine_Data *objdata;
    DLL_IModgfx *sp30;

    objdata = self->data;
    self->srt.yaw = 0;
    objdata->unk0 = 0xA;
    if (setup->unk1A > 0) {
        objdata->unk0 = (s16) (setup->unk1A >> 2);
    }
    objdata->unk12 = 0;
    objdata->unk13 = 0;
    objdata->unk2 = 0;
    self->animCallback = dll_481_func_C10;
    obj_init_mesg_queue(self, 4);
    // @recomp: Fix falling rocks crash (original patch by MusicalProgrammer)
    main_set_bits(BIT_176, 0);
    main_set_bits(BIT_DB_Entered_Shrine_3, 1);
    main_set_bits(BIT_15F, 0);
    main_set_bits(BIT_DB_Entered_Shrine_1, 1);
    main_set_bits(BIT_DB_Entered_Shrine_2, 1);
    main_set_bits(BIT_DB_Triggered_In_Shrine_Spirit_Cutscene, 0);
    objdata->unk17 = 0;
    objdata->unk4 = 0xC;
    objdata->unk8 = 0x1E;
    objdata->unk2 = 0xC8;
    gDLL_5_AMSEQ->vtbl->play_ex(2, 0x2B, 0x50, 1, 0);
    objdata->unk6 = 0;
    objdata->unkA = 0;
    objdata->unk15 = 0;
    objdata->unk16 = 0;
    objdata->unkE = 0x12C;
    objdata->unk10 = 0x514;
    sp30 = dll_load_deferred(DLL_ID_122, 1);
    objdata->unkC = sp30->vtbl->func0(self, 3, 0, 0x402, -1, 0);
    dll_unload(sp30);
}

RECOMP_PATCH void dll_481_control(Object *self) {
    NWSH_Shrine_Data *objdata;
    Object *player;
    Object *temp_v0_4;
    f32 var_fv0;
    f32 sp3C;
    s16 var_v0;

    objdata = self->data;
    player = get_player();
    sp3C = 1000.0f;
    self->positionMirror.x = self->srt.transl.x;
    self->positionMirror.y = self->srt.transl.y;
    self->positionMirror.z = self->srt.transl.z;
    dll_481_func_1084(self);
    main_set_bits(BIT_DB_Entered_Shrine_2, 1);
    if (objdata->unk6 != 0) {
        objdata->unk4 += objdata->unk6;
        if (objdata->unk4 < 0xD) {
            objdata->unk4 = 0xC;
            objdata->unk6 = 0;
        } else if (objdata->unk4 >= 0x46) {
            objdata->unk4 = 0x46;
            objdata->unk6 = 0;
        }
        gDLL_5_AMSEQ->vtbl->set_volume(2, objdata->unk4);
    }
    if (objdata->unkA != 0) {
        objdata->unk8 += objdata->unkA;
        if ((objdata->unk8 < 2) && (objdata->unkA <= 0)) {
            objdata->unk8 = 1;
            objdata->unkA = 0;
        } else if ((objdata->unk8 >= 0x46) && (objdata->unkA >= 0)) {
            objdata->unk8 = 0x46;
            objdata->unkA = 0;
        }
        gDLL_5_AMSEQ->vtbl->set_volume(3, objdata->unk8);
    }
    if (objdata->unk2 > 0) {
        objdata->unk2 -= gUpdateRate;
        if (objdata->unk2 <= 0) {
            objdata->unk2 = 0;
            if (objdata->unk15 == 0) {
                gDLL_5_AMSEQ->vtbl->play_ex(3, 0x34, 0x50, objdata->unk8, 0);
                objdata->unk15 = 1;
            }
        }
        if ((objdata->unk12 == 2) && (objdata->unk2 < 0x29) && (objdata->unk16 == 0)) {
            objdata->unk16 = 1;
        }
    } else {
        temp_v0_4 = obj_get_nearest_type_to(0x10, player, &sp3C);
        if ((temp_v0_4 != NULL) && (sp3C < 300.0f) && (sp3C > 100.0f)) {
            var_fv0 = temp_v0_4->srt.transl.z - player->srt.transl.z;
            if (var_fv0 <= 0.0f) {
                if (var_fv0 < 0.0f) {
                    var_fv0 *= -1.0f;
                }
                if (objdata->unk8 != 0x1E) {
                    objdata->unk8 = 0x1E;
                }
                var_v0 = ((f32) objdata->unk8 * ((var_fv0 - 100.0f) / 200.0f));
                if (var_v0 <= 0) {
                    var_v0 = 1;
                }
                gDLL_5_AMSEQ->vtbl->set_volume(3, var_v0);
                var_v0 = ((f32) objdata->unk4 * ((200.0f - (var_fv0 - 100.0f)) / 200.0f));
                if (var_v0 <= 0) {
                    var_v0 = 1;
                }
                gDLL_5_AMSEQ->vtbl->set_volume(2, var_v0);
            }
        }
        switch (objdata->unk12) {
        case 0:
            if (objdata->unkE <= 0) {
                main_set_bits(BIT_176, 1);
                func_80003B70(1.0f);
                objdata->unkE = rand_next(0x64, 0x96);
                objdata->unk10 = 0x91;
            } else {
                if (objdata->unk10 != -0x3E7) {
                    if (objdata->unk10 < 0) {
                        gDLL_6_AMSFX->vtbl->play_sound(NULL, 0x3B9, 0x46, NULL, NULL, 0, NULL);
                        objdata->unk10 = -0x3E7;
                    } else {
                        objdata->unk10 -= gUpdateRate;
                    }
                }
                main_set_bits(BIT_176, 0);
                objdata->unkE -= gUpdateRate;
            }
            if (vec3_distance(&self->positionMirror, &player->positionMirror) < (f32) objdata->unk0) {
                main_set_bits(BIT_5C6, 1);
                objdata->unk12 = 1;
                main_set_bits(BIT_DB_Entered_Shrine_3, 0);
                objdata->unk13 = 1;
                gDLL_14_Modgfx->vtbl->func7(&objdata->unkC);
            }
        default:
            return;
        case 1:
            if (objdata->unk13 == 1) {
                objdata->unk12 = 2;
                return;
            }
            break;
        case 2:
            gDLL_3_Animation->vtbl->func17(3, self, -1);
            objdata->unk12 = 3;
            return;
        case 7:
            gDLL_3_Animation->vtbl->func17(4, self, -1);
            break;
        case 8:
            gDLL_3_Animation->vtbl->func17(5, self, -1);
            gDLL_5_AMSEQ->vtbl->play_ex(3, 0x35, 0x50, (s16) (u8) objdata->unk8, 0);
            main_set_bits(BIT_15F, 0);
            main_set_bits(BIT_DB_Entered_Shrine_2, 0);
            main_set_bits(BIT_DB_Entered_Shrine_3, 1);
            main_set_bits(BIT_5BE, 0);
            main_set_bits(BIT_1CB, 0);
            main_set_bits(BIT_5C6, 0);
            objdata->unk12 = 6;
            return;
        case 4:
            if (main_get_bits(BIT_SP_Replay_Disk_WM) != 0) {
                objdata->unk8 = 1;
                gDLL_5_AMSEQ->vtbl->play_ex(3, 0x34, 0x50, (s16) (u8) objdata->unk8, 0);
                objdata->unkA = 1;
                main_set_bits(BIT_DB_Entered_Shrine_3, 1);
                objdata->unk12 = 6;
                return;
            }
            main_set_bits(BIT_DB_Entered_Shrine_1, 0);
            gDLL_5_AMSEQ->vtbl->play_ex(3, 0x34, 0x50, (s16) (u8) objdata->unk8, 0);
            objdata->unkA = 1;
            gDLL_3_Animation->vtbl->func17(1, self, -1);
            objdata->unk12 = 5;
            return;
        case 5:
            if (main_get_bits(BIT_FD) == 0) {
                main_set_bits(BIT_FD, 1);
            }
            main_set_bits(BIT_15F, 0);
            main_set_bits(BIT_DB_Entered_Shrine_2, 0);
            main_set_bits(BIT_DB_Entered_Shrine_3, 1);
            objdata->unk12 = 6;
            main_set_bits(BIT_DB_Entered_Shrine_1, 1);
            main_set_bits(BIT_SP_Replay_Disk_WM, 1);
            // @recomp: Set WM setup to 8 instead (original patch by MusicalProgrammer)
            gDLL_29_Gplay->vtbl->set_map_setup(MAP_WARLOCK_MOUNTAIN, 8);
            break;
        case 6:
            if (main_get_bits(BIT_5C2) == 0) {
                main_set_bits(BIT_5C2, 1);
            }
            main_set_bits(BIT_5C6, 0);
            break;
        }
    }
}
