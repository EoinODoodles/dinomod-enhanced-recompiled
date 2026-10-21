#include "modding.h"

#include "game/gamebits.h"
#include "game/objects/interaction_arrow.h"
#include "sys/gfx/animseq.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objmsg.h"
#include "sys/objprint.h"
#include "dll.h"

#include "recomp/dlls/objects/371_CFPowerBase_recomp.h"

typedef struct {
    ObjSetup base;
    s8 yaw8;
    u8 _unk19[0x1E - 0x19];
    s16 gamebit;
} CFPowerBase_Setup;

typedef struct {
    s16 gamebit;
    s16 preempted;
    s16 crystalBit;
    s8 seqno;
    // @recomp: new fields
    s8 preemptNeeded;
} CFPowerBase_Data;

RECOMP_HOOK_DLL(CFPowerBase_obj_Setup) void CFPowerBase_obj_Setup_hook(Object* self, CFPowerBase_Setup* setup, s32 reset) {
    CFPowerBase_Data* objdata = self->data;

    if (mainGetBits(setup->gamebit) != 0) {
        objdata->preemptNeeded = TRUE;
    }
}

RECOMP_PATCH void CFPowerBase_obj_Control(Object* self) {
    CFPowerBase_Data* objdata = self->data;
    
    switch (objdata->gamebit) {
    case BIT_CRF_Red_PowerBase:
        objdata->crystalBit = BIT_CRF_Red_Power_Crystal;
        break;
    case BIT_CRF_Green_PowerBase:
        objdata->crystalBit = BIT_CRF_Green_Power_Crystal;
        break;
    case BIT_CRF_Blue_PowerBase:
        objdata->crystalBit = BIT_CRF_Blue_Power_Crystal;
        break;
    }
    if (mainGetBits(objdata->crystalBit) != 0) {
        self->unkAF &= ~ARROW_FLAG_10_Greyed_Out;
    } else {
        self->unkAF |= ARROW_FLAG_10_Greyed_Out;
    }
    // @recomp: Don't preempt if it's not actually needed. Normally, the seq will reset the power base Y-coord so that
    //          this can preempt the seq again right after and raise it back up (to avoid raising it twice). This results
    //          in the base being at the wrong height for one frame tho and looks pretty bad. The seq was changed to
    //          no longer reset the height.
    if (objdata->preemptNeeded && !objdata->preempted && (mainGetBits(objdata->gamebit) != 0)) {
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
        objdata->preempted = TRUE;
        gDLL_3_Animation->vtbl->preempt_sequence_time(self, 478);
        gDLL_3_Animation->vtbl->start_obj_sequence(objdata->seqno, self, 3);
    }
    if ((self->unkAF & ARROW_FLAG_1_Interacted) && (gDLL_1_cmdmenu->vtbl->was_this_item_used(objdata->crystalBit) != 0)) {
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
        mainSetBits(objdata->crystalBit, 0);
        gDLL_3_Animation->vtbl->start_obj_sequence(objdata->seqno, self, -1);
    }
}

RECOMP_PATCH u32 CFPowerBase_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(CFPowerBase_Data); // @recomp: new size
}
