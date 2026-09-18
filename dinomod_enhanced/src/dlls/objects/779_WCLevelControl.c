#include "modding.h"
#include "recomputils.h"

#include "game/gamebits.h"
#include "game/objects/object.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "dll.h"

#include "recomp/dlls/objects/779_WCLevelControl_recomp.h"

typedef struct {
    f32 timer;
    u8 state;
    u8 flags;
    u8 previousState;
} WCLevelControl_Data;

extern void WCLevelControl_handleAct1(Object *self, WCLevelControl_Data *objdata);
extern void WCLevelControl_handleAct2(Object *self, WCLevelControl_Data *objdata);

/**
  * Fix a bug where you suddenly get blinded by intense fog when visiting Walled City (originally by MusicalProgrammer)
  *
  * TODO: An alternative fix could be to edit the EnvFxAction that has the bugged fog distances (0x149)
  */
RECOMP_PATCH void WCLevelControl_obj_Control(Object *self) {
    WCLevelControl_Data *objdata = self->data;
    f32 time;
    u8 act;

    //Set up environment/lighting effects
    if (self->unkDC == 0) {
        envfxAction(self, self, 0x1FB, 0);
        envfxAction(self, self, 0x1FC, 0);
        // envfxAction(self, self, 0x149, 0); //@recomp: stop blinding fog
        lfxAction(self, self, 0x97, 0, 0, 0);
        lfxAction(self, self, 0x24F, 0, 0, 0);
        self->unkDC = 1;
    }

    //Handle acts
    act = gDLL_29_Gplay->vtbl->get_act(self->mapID);
    if ((act == 1) || (act != 2)) {
        WCLevelControl_handleAct1(self, objdata);
    } else {
        WCLevelControl_handleAct2(self, objdata);
    }

    //Check if night-time
    if (gDLL_7_Newday->vtbl->func8(&time)) {
        mainSetBits(BIT_WC_Is_Nighttime, TRUE);
        mainSetBits(BIT_WC_Is_Daytime, FALSE);
    } else {
        mainSetBits(BIT_WC_Is_Nighttime, FALSE);
        mainSetBits(BIT_WC_Is_Daytime, TRUE);
    }
}
