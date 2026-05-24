#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "configs.h"

#include "common.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/objects.h"

#include "recomp/dlls/objects/734_DR_PushCart_recomp.h"

typedef struct {
    UnkCurvesStruct unk0;
    Object* unk108;
    f32 unk10C;
    f32 unk110;
    f32 unk114;
    f32 unk118;
    s8 unk11C[0x14C - 0x11C];
    u8 unk14C;
    s8 unk14D[0x15C - 0x14D];
    u32 unk15C_31 : 1;
    u32 unk15C_30 : 1;
    u32 unk15C_29 : 1;
    u32 unk15C_28 : 4;
    u32 unk15C_24 : 1;
    u32 unk15C_23 : 1;
    u32 unk15C_22 : 1;
    u32 unk15C_21 : 1;
} DRPushCart_Data;

extern void dll_734_func_133C(Object* self, DRPushCart_Data* objData);

/**
  * Stop DR_PushCart from opening mine door too early (originally by MusicalProgrammer)
  */
RECOMP_PATCH s32 dll_734_func_DD8(Object* self, u8 arg1, u8 arg2, s32* arg3) {
    DRPushCart_Data* objData;
    Object* player;
    f32 temp;

    objData = self->data;
    player = get_player();
    
    *arg3 = -1;

    switch (arg1) {
    case 1:
        dll_734_func_133C(self, objData);
    default:
        break;
    case 2:
        break;
    case 3:
        if (!(objData->unk15C_30) && !(objData->unk10C <= 0.0f) && ((objData->unk15C_31) == 0)) {
            dll_734_func_133C(self, objData);
        } else {
            break;
        }
        return 1;
    case 4:
        if (!(objData->unk10C <= 0.0f)) {
            if (objData->unk15C_30) {
                // main_set_bits(BIT_660, 1); //@recomp: Stop DR_PushCart from opening mine door too early
            } else {
                main_get_bits(BIT_661);
                if (main_get_bits(BIT_661) == 0) {
                    main_set_bits(BIT_788, 1);
                    objData->unk15C_28 = 1;
                    objData->unk118 = 0.0f;
                } else {
                    if (objData->unk118 < 0.0f) {
                        temp = -2.0f;
                    } else {
                        temp = 2.0f;
                    }
                    objData->unk110 += temp;
                }
            }
        }
        break;
    case 9:
        if (!(objData->unk10C >= 0.0f)) {
            if (main_get_bits(BIT_661) == 0) {
                objData->unk118 = 0.0f;
                objData->unk15C_28 = 1;
            } else {
                if (objData->unk118 < 0.0f) {
                    temp = -2.0f;
                } else {
                    temp = 2.0f;
                }
                objData->unk110 += temp;
            }
        }
        break;
    case 5:
        if (!(objData->unk15C_30)) {
            objData->unk15C_28 = 2;
        }
        break;
    case 6:
        if (!(objData->unk15C_30)) {
            if (objData->unk118 < 0.0f) {
                temp = -3.0f;
            } else {
                temp = 3.0f;
            }
            objData->unk110 += temp;
        }
        break;
    case 7:
        if (objData->unk118 <= 0.0f) {
            objData->unk118 = 0.0f;
            objData->unk15C_28 = 3;
        }
        break;        
    case 10:
        if ((objData->unk15C_22) && (main_get_bits(BIT_689) == 0)) {
            main_set_bits(BIT_689, 1);
        }
        break;
    case 11:
        if ((objData->unk15C_22) && (self == player->parent)) {
            main_set_bits(BIT_68A, 1);
        }
        break;
    case 12:
        if ((objData->unk15C_22) && (self == player->parent)) {
            main_set_bits(BIT_68B, 1);
        }
        break;
    case 13:
        if ((main_get_bits(BIT_68A) != 0) && (objData->unk118 >= 0.0f)) {
            dll_734_func_133C(self, objData);
        }
        break;
    case 14:
        if ((objData->unk15C_22) && (objData->unk118 <= 0.0f)) {
            dll_734_func_133C(self, objData);
        }
        break;
    case 15:
        if (!(objData->unk15C_30)) {
            main_set_bits(BIT_788, 1);
        }
        break;
    case 16:
        if (objData->unk118 >= 0) {
            temp = objData->unk118;
        } else {
            temp = -objData->unk118;
        }
        
        if (temp == 2.0f) {
            objData->unk118 *= 0.5f;
        } else {
            objData->unk118 *= 2.0f;
        }
        break;
    }

    if (arg2 != 2) {
        if (arg2 == 8) {
            if (main_get_bits(BIT_67F)) {
                *arg3 = 0;
            } else {
                *arg3 = 1;
            }
        }
    } else {
        main_set_bits(BIT_DR_Minecart_Track_Entrance_Demolished, 1);
    }
    
    return 1;
}
