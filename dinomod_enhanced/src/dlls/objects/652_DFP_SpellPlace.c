#include "modding.h"

#include "common.h"
#include "dlls/objects/214_animobj.h"
#include "sys/objanim.h"

#include "recomp/dlls/objects/652_DFP_SpellPlace_recomp.h"

typedef struct {
    s16 unk0;
    s16 unk2;
    u8 unk4;
} DLL652_Data;

RECOMP_PATCH void dll_652_func_364(Object* self) {
    DLL652_Data* objdata;
    s16 bit2Val;
    s16 bit1Val;

    objdata = (DLL652_Data*)self->data;
    bit2Val = main_get_bits(objdata->unk2);
    bit1Val = main_get_bits(objdata->unk0);
    if ((bit1Val == 0) && (bit2Val != 0) && (objdata->unk4 == 0)) {
        self->unkAF &= ~8;
        // @recomp: Accept the correct spellstone (original patch by jeebs2kx)
        if ((bit2Val != 0) && (gDLL_1_UI->vtbl->func7(0x83A) != 0)) {
            gDLL_3_Animation->vtbl->func17(1, self, -1);
            objdata->unk4 = 1;
            self->unkAF |= 8;
        }
    }
}
