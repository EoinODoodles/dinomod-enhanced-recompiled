#include "modding.h"

#include "dlls/engine/6_amsfx.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "game/objects/setups/51_Generic.h"
#include "game/objects/setups/57_ObjCreator.h"
#include "game/objects/setups/93_PickObj.h"
#include "sys/dll.h"
#include "sys/gfx/modgfx.h"
#include "sys/main.h"
#include "sys/objprint.h"
#include "dll.h"
#include "macros.h"

#include "recomp/dlls/objects/473_GPSH_ObjCreator_recomp.h"

typedef struct {
/*0*/ s16 timer;
/*2*/ s16 timerRate;
/*4*/ u8 type;
/*5*/ u8 spawned;
} GPSH_ObjCreator_Data;

/*0x0*/ extern s16 data_0[6];

RECOMP_PATCH void GPSH_ObjCreator_control(Object* self) {
    ObjCreator_Setup* setup;
    PickObj_Setup* pickObjSetup;
    Flybaddie_Setup* flybaddieSetup;
    GPSH_ObjCreator_Data* objdata;
    s32 _pad;
    DLL_IModgfx* modgfx;

    setup = (ObjCreator_Setup*)self->setup;
    objdata = self->data;
    // @recomp: Don't let reset bit affect flybaddie creators. The spawn flag for the flybaddies is never cleared by
    //          the shrine so when the reset bit gets set the creator will spawn a flybaddie every game tick causing a crash.
    //          The flybaddies shouldn't respawn anyway.
    if (mainGetBits(BIT_5AF) != 0 && objdata->type != 6) {
        // Reset
        self->unkE0 = 0;
        objdata->timer = 100;
        objdata->timerRate = 0;
        objdata->spawned = FALSE;
        self->opacityWithFade = 0xFF;
        self->opacity = 0xFF;
    }
    if (!objdata->spawned) {
        if ((objdata->type == 6) && (self->unkE0 == 0) && (mainGetBits(BIT_5AC) != 0)) {
            // Start flybaddie spawn
            modgfx = dllLoad(DLL_ID_146, 1);
            modgfx->vtbl->Spawn(self, 3, 0, 1, -1, 0);
            modgfx->vtbl->Spawn(self, 4, 0, 1, -1, 0);
            dll_amSfx->Play(NULL, SOUND_303, MAX_VOLUME, NULL, NULL, 0, NULL);
            dllFree(modgfx);
            objdata->timerRate = 1;
            self->unkE0 = 1;
        } else if ((self->unkE0 == 0) && (mainGetBits(BIT_148) != 0)) {
            // Start pickup spawn
            modgfx = dllLoad(DLL_ID_146, 1);
            modgfx->vtbl->Spawn(self, 3, 0, 1, -1, 0);
            modgfx->vtbl->Spawn(self, 4, 0, 1, -1, 0);
            dll_amSfx->Play(NULL, SOUND_303, MAX_VOLUME, NULL, NULL, 0, NULL);
            dllFree(modgfx);
            objdata->timerRate = 1;
            self->unkE0 = 1;
        }
        if (objdata->timerRate != 0) {
            objdata->timer -= (objdata->timerRate * gUpdateRate);
        }
        if (objdata->type != 6) {
            if (objdata->timer <= 0) {
                // Spawn pickup
                pickObjSetup = objAllocSetup(sizeof(PickObj_Setup), objdata->type + OBJ_GPSHpickobjroot);
                objdata->spawned = TRUE;
                pickObjSetup->base.fadeDistance = 0xFF;
                pickObjSetup->base.loadFlags = OBJSETUP_LOAD_FLAG20;
                pickObjSetup->base.fadeFlags = OBJSETUP_FADE_MAIN;
                pickObjSetup->base.x = self->srt.transl.x;
                pickObjSetup->base.y = self->srt.transl.y;
                pickObjSetup->base.z = self->srt.transl.z;
                pickObjSetup->base.objId = objdata->type + OBJ_GPSHpickobjroot;
                pickObjSetup->yaw = self->srt.yaw >> 8;
                pickObjSetup->pickupParam = data_0[objdata->type];
                objSetupObject(&pickObjSetup->base, OBJINIT_FLAG4 | OBJINIT_STANDALONE, self->mapID, -1, self->parent)
                    ->modelInstIdx = 0;
                objdata->timer = 100;
                objdata->timerRate = 0;
            }
        } else {
            if (objdata->timer <= 0) {
                // Spawn flybaddie
                flybaddieSetup = objAllocSetup(sizeof(Flybaddie_Setup), OBJ_GPSH_flybaddie);
                flybaddieSetup->base.x = setup->base.x;
                flybaddieSetup->base.y = setup->base.y + 50.0f;
                flybaddieSetup->base.z = setup->base.z;
                flybaddieSetup->base.objId = OBJ_GPSH_flybaddie;
                flybaddieSetup->base.uID = -1;
                flybaddieSetup->base.loadFlags = setup->base.loadFlags;
                flybaddieSetup->base.byte5 = setup->base.byte5;
                flybaddieSetup->base.byte6 = setup->base.byte6;
                flybaddieSetup->base.fadeDistance = setup->base.fadeDistance;
                flybaddieSetup->unk1A = -0x3C;
                objSetupObject(&flybaddieSetup->base, OBJINIT_FLAG4 | OBJINIT_STANDALONE, self->mapID, -1, self->parent);
                objdata->timer = 100;
                objdata->timerRate = 0;
            }
        }
    }
}
