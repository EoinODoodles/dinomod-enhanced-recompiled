#include "modding.h"

#include "game/gamebits.h"
#include "sys/dll.h"
#include "sys/gfx/animseq.h"
#include "sys/gfx/modgfx.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "sys/objmsg.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "sys/segment_1460.h"
#include "dll.h"

#include "recomp/dlls/objects/486_CCSH_Shrine_recomp.h"

typedef struct {
    ObjSetup base;
    u8 _unk18[0x1A - 0x18];
    s16 unk1A;
} CCSH_Shrine_Setup;

typedef struct {
    s16 unk0;
    s16 unk2;
    s16 unk4;
    s16 unk6;
    s16 unk8;
    s16 unkA;
    s16 unkC;
    u8 unkE;
    u8 unkF;
    u8 unk10;
    u8 unk11;
    u8 unk12;
} CCSH_Shrine_Data;

extern void CCSH_Shrine_process_obj_messages(Object* self);

RECOMP_PATCH void CCSH_Shrine_control(Object* self) {
    CCSH_Shrine_Data* objdata = self->data;
    Object* player;
    Object* obj;
    DLL_IModgfx* modgfx;
    f32 doorDist;
    f32 dist;
    f32 sp2C;
    s16 volume;
    
    player = get_player();
    dist = 1000.0f;
    self->globalPosition.x = self->srt.transl.x;
    self->globalPosition.y = self->srt.transl.y;
    self->globalPosition.z = self->srt.transl.z;
    CCSH_Shrine_process_obj_messages(self);
    main_set_bits(BIT_DB_Entered_Shrine_2, 1);
    if (objdata->unk6 != 0) {
        objdata->unk4 += objdata->unk6;
        if (objdata->unk4 <= 12) {
            objdata->unk4 = 12;
            objdata->unk6 = 0;
        } else if (objdata->unk4 >= 70) {
            objdata->unk4 = 70;
            objdata->unk6 = 0;
        }
        gDLL_5_AMSEQ->vtbl->set_volume(2, objdata->unk4);
    }
    if (objdata->unkA != 0) {
        objdata->unk8 += objdata->unkA;
        if ((objdata->unk8 <= 1) && (objdata->unkA <= 0)) {
            objdata->unk8 = 1;
            objdata->unkA = 0;
        } else if ((objdata->unk8 >= 70) && (objdata->unkA >= 0)) {
            objdata->unk8 = 70;
            objdata->unkA = 0;
        }
        gDLL_5_AMSEQ->vtbl->set_volume(3, objdata->unk8);
    }
    if (objdata->unk2 > 0) {
        objdata->unk2 -= gUpdateRate;
        if (objdata->unk2 <= 0) {
            objdata->unk2 = 0;
            if (objdata->unk12 == 0) {
                gDLL_5_AMSEQ->vtbl->play_ex(3, 0x2C, 0x50, objdata->unk8, 0);
                objdata->unk12 = 1;
            }
        }
    } else {
        obj = obj_get_nearest_type_to(OBJTYPE_Door, player, &dist);
        if ((obj != NULL) && (dist < 300.0f) && (dist > 100.0f)) {
            doorDist = obj->srt.transl.z - player->srt.transl.z;
            if (doorDist <= 0.0f) {
                if (doorDist < 0.0f) {
                    doorDist *= -1.0f;
                }
                sp2C = doorDist - 100.0f;
                if (objdata->unk8 != 0x1E) {
                    objdata->unk8 = 0x1E;
                }
                volume = (s16) ((f32) objdata->unk8 * (sp2C / 200.0f));
                if (volume <= 0) {
                    volume = 1;
                }
                gDLL_5_AMSEQ->vtbl->set_volume(3, volume);
                volume = (s16) ((f32) objdata->unk4 * ((200.0f - sp2C) / 200.0f));
                if (volume <= 0) {
                    volume = 1;
                }
                gDLL_5_AMSEQ->vtbl->set_volume(2, volume);
            }
        }
        switch (objdata->unkF) {
        case 0:
            if ((main_get_bits(BIT_5B5) == 0) && (main_get_bits(BIT_594) != 0)) {
                main_set_bits(BIT_5B5, 1);
            }
            main_set_bits(BIT_5B9, 0);
            if (vec3_distance(&self->globalPosition, &player->globalPosition) < (f32) objdata->unk0) {
                objdata->unkF = 1;
                main_set_bits(BIT_DB_Entered_Shrine_3, 0);
                gDLL_3_Animation->vtbl->start_obj_sequence(0, self, -1);
                modgfx = dll_load_deferred(DLL_ID_147, 1);
                modgfx->vtbl->func0(self, 0, 0, 1, -1, 0);
                dll_unload(modgfx);
                modgfx = dll_load_deferred(DLL_ID_148, 1);
                modgfx->vtbl->func0(self, 0, 0, 1, -1, 0);
                dll_unload(modgfx);
                main_set_bits(BIT_DB_Entered_Shrine_1, 0);
                gDLL_14_Modgfx->vtbl->func7(&objdata->unkC);
                // @recomp: Shut door while test is active (normally the trigger planes will clear this bit but the
                //          way they are positioned makes it possible to get the door stuck open.
                main_set_bits(BIT_5B6, 0);
            }
        default:
            return;
        case 1:
            if (objdata->unk10 == 1) {
                objdata->unkF = 2;
                objdata->unk2 = 0xA0;
                return;
            }
            break;
        case 2:
            if ((objdata->unkE == 0) && (main_get_bits(BIT_1CD) == 0)) {
                main_set_bits(BIT_1CD, 1);
            }
            if (main_get_bits(BIT_5B2) != 0) {
                objdata->unkE++;
                objdata->unk2 = 0x64;
                if (objdata->unkE == 1) {
                    gDLL_3_Animation->vtbl->start_obj_sequence(3, self, -1);
                }
            }
            break;
        case 7:
            gDLL_3_Animation->vtbl->start_obj_sequence(5, self, -1);
            objdata->unkF = 3;
            objdata->unk2 = 0;
            objdata->unkA = -3;
            return;
        case 8:
            gDLL_3_Animation->vtbl->start_obj_sequence(4, self, -1);
            objdata->unkF = 6;
            objdata->unk2 = 0;
            objdata->unkA = -3;
            return;
        case 6:
            gDLL_5_AMSEQ->vtbl->play_ex(3, 0x35, 0x50, (u8) objdata->unk8, 0);
            objdata->unkA = 1;
            gDLL_3_Animation->vtbl->start_obj_sequence(2, self, -1);
            dist = 10000.0f;
            obj = obj_get_nearest_type_to(OBJTYPE_Baddie, self, &dist);
            if (obj != NULL) {
                obj_destroy_object(obj);
            }
            objdata->unkF = 0;
            objdata->unk2 = 0x190;
            main_set_bits(BIT_DB_Entered_Shrine_3, 1);
            main_set_bits(BIT_DB_Entered_Shrine_1, 1);
            main_set_bits(BIT_DB_Entered_Shrine_2, 1);
            main_set_bits(BIT_5B2, 0);
            main_set_bits(BIT_5B9, 1);
            modgfx = dll_load_deferred(DLL_ID_122, 1);
            objdata->unkC = modgfx->vtbl->func0(self, 0, 0, 0x402, -1, 0);
            dll_unload(modgfx);
            main_set_bits(BIT_1CD, 0);
            objdata->unkE = 0;
            objdata->unk10 = 0;
            return;
        case 3:
            dist = 10000.0f;
            obj = obj_get_nearest_type_to(OBJTYPE_Baddie, self, &dist);
            if (obj != NULL) {
                obj_destroy_object(obj);
            }
            if (main_get_bits(BIT_1CE) != 0) {
                objdata->unk8 = 1;
                gDLL_5_AMSEQ->vtbl->play_ex(3, 0x2C, 0x50, (u8) objdata->unk8, 0);
                objdata->unkA = 1;
                main_set_bits(BIT_DB_Entered_Shrine_3, 1);
                objdata->unkF = 5;
                return;
            }
            main_set_bits(BIT_DB_Entered_Shrine_1, 0);
            gDLL_5_AMSEQ->vtbl->play_ex(3, 0x2A, 0x50, (u8) objdata->unk8, 0);
            objdata->unkA = 1;
            gDLL_3_Animation->vtbl->start_obj_sequence(1, self, -1);
            break;
        case 4:
            if (main_get_bits(BIT_Shrine_Do_Exit_Warp) == 0) {
                main_set_bits(BIT_Shrine_Do_Exit_Warp, 1);
            }
            main_set_bits(BIT_1CF, 0);
            main_set_bits(BIT_DB_Entered_Shrine_2, 0);
            objdata->unkF = 5;
            gDLL_5_AMSEQ->vtbl->play_ex(3, 0x2C, 0x50, (u8) objdata->unk8, 0);
            main_set_bits(BIT_1CE, 1);
            gDLL_29_Gplay->vtbl->set_act(MAP_WARLOCK_MOUNTAIN, 6);
            break;
        }
    }
}
