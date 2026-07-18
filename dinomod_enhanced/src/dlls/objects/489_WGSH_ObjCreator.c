#include "modding.h"

#include "dlls/engine/6_amsfx.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/dll.h"
#include "sys/gfx/modgfx.h"
#include "sys/main.h"
#include "sys/objprint.h"
#include "dll.h"

typedef struct {
    ObjSetup base;
    s16 unk18;
    s16 unk1A;
} WGSH_flybaddie_Setup;

#include "recomp/dlls/objects/489_WGSH_ObjCreator_recomp.h"

typedef struct {
    ObjSetup base;
    u8 _unk18[0x1E - 0x18];
    s8 rotation;
    s8 disableSpawner;
} WGSH_ObjCreator_Setup;

typedef struct {
    s32 unk0;
    s16 timer;
    s16 countdownRate;
} WGSH_ObjCreator_Data;

RECOMP_PATCH void WGSH_ObjCreator_control(Object *self) {
    WGSH_ObjCreator_Setup *setup;
    WGSH_ObjCreator_Data *objdata;
    ObjSetup *flybaddieSetup;
    DLL_IModgfx *sp38;

    setup = (WGSH_ObjCreator_Setup*)self->setup;
    objdata = self->data;
    if ((self->unkE0 != 0) && (mainGetBits(BIT_1D4) != 0)) {
        self->unkE0 = 0;
    }
    if ((self->unkE0 == 0) && (mainGetBits(BIT_1D3) != 0)) {
        sp38 = dllLoad(DLL_ID_146, 1);
        sp38->vtbl->func0(self, 0, 0, 1, -1, 0);
        sp38->vtbl->func0(self, 1, 0, 1, -1, 0);
        dll_amSfx->Play(NULL, SOUND_303, MAX_VOLUME, NULL, NULL, 0, NULL);
        dllFree(sp38);
        objdata->countdownRate = 1;
        self->unkE0 = 1;
    }
    if (objdata->countdownRate != 0) {
        objdata->timer -= (objdata->countdownRate * gUpdateRate);
    }
    if ((objdata->timer <= 0) && !setup->disableSpawner) {
        // @recomp: Fix setup allocation size for flybaddie. This fixes the behavior of the flybaddie,
        //          where in vanilla the undefined behavior causes them to fly out of bounds.
        flybaddieSetup = objAllocSetup(sizeof(WGSH_flybaddie_Setup), OBJ_WGSH_flybaddie);
        flybaddieSetup->x = setup->base.x;
        flybaddieSetup->y = setup->base.y + 50.0f;
        flybaddieSetup->z = setup->base.z;
        flybaddieSetup->objId = OBJ_WGSH_flybaddie;
        flybaddieSetup->uID = -1;
        flybaddieSetup->loadFlags = setup->base.loadFlags;
        flybaddieSetup->byte5 = setup->base.byte5;
        flybaddieSetup->byte6 = setup->base.byte6;
        flybaddieSetup->fadeDistance = setup->base.fadeDistance;
        objSetupObject(flybaddieSetup, OBJINIT_STANDALONE | OBJINIT_FLAG4, self->mapID, -1, self->parent);
        objdata->timer = 100;
        objdata->countdownRate = 0;
    }
}
