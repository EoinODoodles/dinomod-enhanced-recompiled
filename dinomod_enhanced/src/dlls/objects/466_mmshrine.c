#include "modding.h"

#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/214_animobj.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "sys/joypad.h"
#include "sys/gfx/texture.h"
#include "sys/gfx/model.h"
#include "sys/gfx/modgfx.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/objmsg.h"
#include "functions.h"
#include "types.h"
#include "dll.h"

#include "recomp/dlls/objects/466_mmshrine_recomp.h"

typedef struct {
/*00*/ s16 unk0;
/*02*/ s16 unk2;
/*04*/ s16 unk4;
/*06*/ s16 unk6;
/*08*/ s16 unk8;
/*0A*/ s16 unkA;
/*0C*/ s16 unkC;
/*0E*/ s8 unkE;
/*0F*/ u8 unkF;
/*10*/ u8 unk10;
/*11*/ s8 unk11;
/*12*/ u8 unk12;
/*13*/ u8 unk13;
} MMShrine_Data;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 unk18;
/*1A*/ s16 unk1A;
} Shrine_Setup;

/*0x0*/ extern Texture* _data_0;
/*0x4*/ extern u8 _data_4;

extern void MMshrine_func_1140(Object *arg0);

RECOMP_PATCH void MMshrine_control(Object* arg0) {
    MMShrine_Data* objdata;
    Object* sp48;
    DLL_IModgfx* temp_v0_5;
    Object* temp_v0_4;
    f32 sp3C;
    f32 sp30;
    s32 var_v0;
    f32 var_fv0;

    objdata = (MMShrine_Data*)arg0->data;
    sp48 = get_player();
    sp3C = 1000.0f;
    if (_data_4 != 0) {
        arg0->positionMirror.x = arg0->srt.transl.x;
        arg0->positionMirror.y = arg0->srt.transl.y;
        arg0->positionMirror.z = arg0->srt.transl.z;
        gDLL_5_AMSEQ->vtbl->func5(2, 0x2B, 0x50, 1, 0);
        main_set_bits(0x127, 1);
        _data_4 = 0;
    }
    MMshrine_func_1140(arg0);
    if (objdata->unk6 != 0) {
        objdata->unk4 = (s16) (objdata->unk4 + objdata->unk6);
        if (objdata->unk4 < 0xD) {
            objdata->unk4 = 0xC;
            objdata->unk6 = 0;
        } else if (objdata->unk4 >= 0x46) {
            objdata->unk4 = 0x46;
            objdata->unk6 = 0;
        }
        gDLL_5_AMSEQ->vtbl->func13(2, objdata->unk4);
    }
    if (objdata->unkA != 0) {
        objdata->unk8 = (s16) (objdata->unk8 + objdata->unkA);
        if ((objdata->unk8 < 2) && (objdata->unkA <= 0)) {
            objdata->unk8 = 1;
            objdata->unkA = 0;
        } else if ((objdata->unk8 >= 0x46) && (objdata->unkA >= 0)) {
            objdata->unk8 = 0x46;
            objdata->unkA = 0;
        }
        gDLL_5_AMSEQ->vtbl->func13(3, objdata->unk8);
    }
    if (objdata->unk2 > 0) {
        objdata->unk2 = (s16) (objdata->unk2 - gUpdateRate);
        if (objdata->unk2 <= 0) {
            objdata->unk2 = 0;
            if (objdata->unk12 == 0) {
                gDLL_5_AMSEQ->vtbl->func5(3, 0x2C, 0x50, objdata->unk8, 0);
                objdata->unk12 = 1;
            }
        }
        if ((objdata->unkF == 2) && (objdata->unk2 < 0x29) && (objdata->unk13 == 0)) {
            objdata->unk13 = 1;
        }
    } else {
        temp_v0_4 = obj_get_nearest_type_to(0x10, sp48, &sp3C);
        if ((temp_v0_4 != NULL) && (sp3C < 300.0f) && (sp3C > 100.0f)) {
            var_fv0 = temp_v0_4->srt.transl.z - sp48->srt.transl.z;
            if (var_fv0 <= 0.0f) {
                if (var_fv0 < 0.0f) {
                    var_fv0 *= -1.0f;
                }
                sp30 = var_fv0 - 100.0f;
                if (objdata->unk8 != 0x1E) {
                    objdata->unk8 = 0x1E;
                }
                var_v0 = (s16) (s32) ((f32) objdata->unk8 * (sp30 / 200.0f));
                if (var_v0 <= 0) {
                    var_v0 = 1;
                }
                gDLL_5_AMSEQ->vtbl->func13(3, var_v0);
                var_v0 = (s16) (s32) ((f32) objdata->unk4 * ((200.0f - sp30) / 200.0f));
                if (var_v0 <= 0) {
                    var_v0 = 1;
                }
                gDLL_5_AMSEQ->vtbl->func13(2, var_v0);
            }
        }
        switch (objdata->unkF) {
        case 0:
            if (vec3_distance(&arg0->positionMirror, &sp48->positionMirror) < (f32) objdata->unk0) {
                objdata->unkF = 1;
                main_set_bits(0x129, 0);
                gDLL_3_Animation->vtbl->func19(0x5E, 0, 0, 0);
                gDLL_3_Animation->vtbl->func17(0, arg0, -1);
                temp_v0_5 = dll_load_deferred(0x102B, 1);
                temp_v0_5->vtbl->func0(arg0, 1, 0, 1, -1, 0);
                dll_unload(temp_v0_5);
                temp_v0_5 = dll_load_deferred(0x102C, 1);
                temp_v0_5->vtbl->func0(arg0, 0, 0, 1, -1, 0);
                dll_unload(temp_v0_5);
                main_set_bits(0x126, 0);
                gDLL_14_Modgfx->vtbl->func7(&objdata->unkC);
            }
        default:
            return;
        case 1:
            if (objdata->unk10 == 1) {
                objdata->unkF = 2;
                objdata->unk2 = 0;
                return;
            }
            break;
        case 2:
            gDLL_3_Animation->vtbl->func17(3, arg0, -1);
            objdata->unkF = 8;
            return;
        case 8:
            gDLL_3_Animation->vtbl->func17(5, arg0, -1);
            objdata->unkF = 4;
            return;
        case 7:
            gDLL_3_Animation->vtbl->func17(4, arg0, -1);
            objdata->unkF = 9;
            objdata->unk2 = 0;
            gDLL_5_AMSEQ->vtbl->func5(3, 0x35, 0x50, (s16) (u8) objdata->unk8, 0);
            objdata->unkA = 1;
            return;
        case 4:
            if (main_get_bits(0x12A) != 0) {
                objdata->unk8 = 1;
                gDLL_5_AMSEQ->vtbl->func5(3, 0x2C, 0x50, (s16) (u8) objdata->unk8, 0);
                objdata->unkA = 1;
                main_set_bits(0x129, 1);
                objdata->unkF = 6;
                return;
            }
            main_set_bits(0x126, 0);
            gDLL_5_AMSEQ->vtbl->func5(3, 0x2C, 0x50, (s16) (u8) objdata->unk8, 0);
            objdata->unkA = 1;
            gDLL_3_Animation->vtbl->func17(1, arg0, -1);
            break;
        case 5:
            if (main_get_bits(0xFD) == 0) {
                main_set_bits(0xFD, 1);
            }
            main_set_bits(0x12B, 0);
            main_set_bits(0x127, 0);
            main_set_bits(0x129, 1);
            objdata->unkF = 6;
            main_set_bits(0x126, 1);
            // @recomp: Prevent Test of Fear from unnecessarily setting bit 0x12A (MMP Spirit Deposited) early
            //          (original patch by jeebs2kx)
            //main_set_bits(0x12A, 1);
            gDLL_29_Gplay->vtbl->set_map_setup(0xB, 4);
            break;
        case 9:
            objdata->unkF = 0;
            objdata->unk10 = 0;
            objdata->unk2 = 0x190;
            main_set_bits(0x129, 1);
            main_set_bits(0x12B, 0);
            main_set_bits(0x126, 1);
            main_set_bits(0x127, 1);
            temp_v0_5 = dll_load_deferred(0x1012, 1);
            objdata->unkC = temp_v0_5->vtbl->func0(arg0, 0, 0, 0x402, -1, 0);
            dll_unload(temp_v0_5);
            break;
        }
    }
}

RECOMP_PATCH int dll_466_func_C50(Object* self, Object *arg1, AnimObj_Data* arg2, s8 arg3) {
    MMShrine_Data* objdata;
    Object* player;
    s32 i;
    u8 temp;

    objdata = self->data;
    player = get_player();
    
    arg2->unk7A = -1;
    arg2->unk62 = 0;
    
    if (objdata->unkA) {
        objdata->unk8 += objdata->unkA;
        if (objdata->unk8 < 2 && objdata->unkA <= 0) {
            objdata->unk8 = 1;
            objdata->unkA = 0;
        } else if (objdata->unk8 >= 0x46 && objdata->unkA >= 0) {
            objdata->unk8 = 0x46;
            objdata->unkA = 0;
        }
        gDLL_5_AMSEQ->vtbl->func13(3, objdata->unk8);
    }

    for (i = 0; i < arg2->unk98; i++){
        temp = arg2->unk8E[i];
        if (temp != 0) {
            switch (temp) {
                case 1:
                    func_80000860(self, self, 0xC7, 0);
                    break;
                case 2:
                    if (D_80092A7C == -1) {
                        func_80000860(self, self, 0x14, 0);
                    } else {
                        func_80000860(self, self, D_80092A7C, 0);
                    }
                    break;
                case 3:
                    objdata->unk10 = 1;
                    break;
                case 4:
                    objdata->unk2 = 0;
                    break;
                case 5:
                    objdata->unkF = 5;
                    objdata->unk10 = 2;
                    main_set_bits(BIT_DB_Entered_Shrine_3, 1);
                    break;
                case 6:
                    objdata->unk10 = 3;
                    main_set_bits(BIT_DB_Entered_Shrine_3, 1);
                    break;
                case 7:
                    main_set_bits(BIT_MMP_GP_Shrine_Spirit_Light_Beams, 1);
                    break;
                case 8:
                    main_set_bits(BIT_MMP_GP_Shrine_Spirit_Light_Beams, 0);
                    objdata->unkA = -3;
                    break;
                case 10:
                    main_set_bits(BIT_DB_Triggered_In_Shrine_Spirit_Cutscene, 1);
                    if (_data_0 == NULL)
                        _data_0 = func_8004A1E8(1);
                    break;
                case 9:
                    // @recomp: Beating ToF will set flag allowing 3rd Spirit be deposited when WM is reached. 
                    //          (original patch by jeebs2kx)
                    main_set_bits(BIT_Spirit_Bits, 0x4);
                    // TODO: is this ok to keep?
                    main_set_bits(BIT_DB_Entered_Shrine_2, 1);
                    break;
                case 11:
                    objdata->unk8 = 0x64;
                    gDLL_5_AMSEQ->vtbl->func5(3, 0x2f, 0x50, (u8)objdata->unk8, 0);
                    break;
                case 12:
                    func_80000860(self, self, 0xCE, 0);
                    main_set_bits(BIT_Test_of_Fear_Particles, 1);
                    gDLL_6_AMSFX->vtbl->play_sound(NULL, SOUND_342_Low_Whoosh, MAX_VOLUME, 0, 0, 0, 0);
                    break;
                case 13:
                    if (D_80092A7C == -1) {
                        func_80000860(self, self, 0x14, 0);
                    } else {
                        func_80000860(self, self, D_80092A7C, 0);
                    }
                    main_set_bits(BIT_Test_of_Fear_Particles, 0);
                    break;
            }
        }
        arg2->unk8E[i] = 0;
    }
    
    if (objdata->unkF == 8) {
        if (vec3_distance(&self->positionMirror, &player->positionMirror) > 10.0f) {
            gDLL_3_Animation->vtbl->func18(arg2->unk63);
            objdata->unkF = 7;
        } else if (joy_get_buttons(0)) {
            gDLL_3_Animation->vtbl->func18(arg2->unk63);
            objdata->unkF = 7;
        }
    }
    
    return 0;
}
