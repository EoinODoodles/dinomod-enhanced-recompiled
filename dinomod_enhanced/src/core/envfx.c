#include "modding.h"

#include "common.h"
#include "sys/map_enums.h"
#include "sys/objects.h"

/** Stop snow from reactivating outside of SnowHorn Wastes */
RECOMP_PATCH s32 envfxAction(Object *calledBy, Object *target, u16 actionIndex, s32 arg3) {
    EnvFxAction *envFx;

    //@recomp: don't allow snow envFxAction to be reapplied outside of snowy areas
    if (actionIndex == 42) { //Seems to be set while on Ice Mountain, and persists for ages!
        Object* player = objGetPlayer();
        if (player) {
            int mapID = mapWorldXZToMapID(
                player->globalPosition.x, 
                player->globalPosition.z
            );

            switch (mapID) {
            case MAP_ICE_MOUNTAIN_1:
            case MAP_ICE_MOUNTAIN_2:
            case MAP_ICE_MOUNTAIN_3:
            case MAP_SNOWHORN_WASTES:
            case MAP_DARK_ICE_MINES_1:
                break;
            default:
                return 0;
            }
        }
    }

    envFx = mmAlloc(sizeof(EnvFxAction), COLOUR_TAG_WHITE, ALLOC_NAME("envfx:action1"));
    assetRomLoadSection((void **)envFx, ENVFXACT_BIN, actionIndex * sizeof(EnvFxAction), sizeof(EnvFxAction));
    if (envFx != NULL) {
        if ((envFx->type < 3) || (envFx->type == 4)) {
            gDLL_9_Newclouds->vtbl->func0(calledBy, target, envFx, arg3);
        } else if (envFx->type == 3) {
            gDLL_8_newfog->vtbl->func0(calledBy, target, envFx, arg3, actionIndex);
        } else if (envFx->type == 5)	{
            gDLL_7_Newday->vtbl->func0(calledBy, target, envFx, arg3);
		} else if (envFx->type == 6)	{
            gDLL_12_Minic->vtbl->func0(calledBy, target, envFx, arg3, actionIndex);
        }
    }

    mmFree(envFx);
    return 0;
}
