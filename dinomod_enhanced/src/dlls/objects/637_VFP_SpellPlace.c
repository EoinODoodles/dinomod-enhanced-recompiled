#include "modding.h"

#include "PR/ultratypes.h"
#include "game/objects/object.h"
#include "recomputils.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "dll.h"

#include "recomp/dlls/objects/637_VFP_SpellPlace_recomp.h"

// size:0x6
typedef struct {
    s16 unk0;
    s16 unk2;
    u8 unk4;
} VFP_SpellPlace_Data;

RECOMP_PATCH void VFP_SpellPlace_do_act1(Object* self) {
    VFP_SpellPlace_Data* objdata;
    s16 bits2;
    s16 bits1;

    objdata = (VFP_SpellPlace_Data*)self->data;
    
    bits2 = main_get_bits(objdata->unk2);
    bits1 = main_get_bits(objdata->unk0);
    
    if ((bits1 == 0) && (bits2 != 0)) {
        self->unkAF &= ~0x8;
        
        // @recomp: Accept DIM's activated SpellStone instead of the unactivated one
        if ((bits2 != 0) && (gDLL_1_UI->vtbl->func7(0x22B) != 0)) {
            main_set_bits(objdata->unk0, 1);
            objdata->unk4 = 1;
            self->unkAF |= 8;
        }
    }
}
