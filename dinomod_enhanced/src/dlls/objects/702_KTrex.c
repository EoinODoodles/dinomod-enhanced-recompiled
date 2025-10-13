#include "modding.h"

#include "common.h"
#include "game/gamebits.h"

#include "sys/map_enums.h"
#include "sys/objtype.h"
#include "sys/main.h"

#include "recomp/dlls/_asm/702_recomp.h"

typedef struct {
    u8 unk0[0x25B - 0x0];
    u8 unk25B;
    u8 unk25C[0x273 - 0x25C];
    s8 unk273;
    u8 unk274[0x33D - 0x274];
    u8 unk33D;
    u8 unk33E[0x574 - 0x33E];
} DLL702_Data;

/** Plays the defeat cutscene, and makes sure the necessary Object Groups are loaded in Walled City for the follow-up cutscene (originally by MusicalProgrammer) */
RECOMP_PATCH s32 dll_702_func_2D98(Object* self, DLL702_Data* objData, s32 arg2) {
    if (objData->unk273 != 0) {
        main_set_bits(BIT_564, 1);
        gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_WALLED_CITY, 4, 1);
        gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_WALLED_CITY, 5, 1);
    }
    return 0;
}
