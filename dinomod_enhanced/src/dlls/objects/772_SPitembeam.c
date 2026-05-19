#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "game/objects/object_id.h"
#include "dll.h"
#include "dlls/objects/768_SPshop.h"

#include "recomp/dlls/objects/772_SPitembeam_recomp.h"

typedef struct {
    ObjSetup base;
    s16 unused18;
    s16 itemIndex; //The kind of shop item being highlighted (see `ShopItemIndices`)
} SPitembeam_Setup;

/** 
  * Fix a null pointer crash that could happen after being kicked out of the shop: 
  * SPitembeam could run its control before the SPshop Object has loaded. 
  * Could also be encountered by no-clipping and fall resetting in the shop.
  */
RECOMP_PATCH void SPitembeam_control(Object* self) {
    s32 pad[2];
    Object* shop;
    SPitembeam_Setup* objSetup;
    TextureAnimator* texAnim;

    objSetup = (SPitembeam_Setup*)self->setup;
    shop = (Object*)self->unkDC;

    //@recomp: return if SPShop object isn't loaded, avoiding a crash
    if (!shop) {
        return;
    }

    //Check if the beam's item isn't visible (and shouldn't be spotlighted)
    if ((((DLL_768_SPShop*)shop->dll)->vtbl->is_item_shown(shop, objSetup->itemIndex) == FALSE) ||
        (((DLL_768_SPShop*)shop->dll)->vtbl->is_item_hidden(shop, objSetup->itemIndex))
    ) {
        self->srt.flags |= OBJFLAG_INVISIBLE;
        self->stateFlags |= OBJSTATE_CONTROL_DISABLED;
    }

    //Scroll the light beam texture
    texAnim = func_800348A0(self, 0, 0);
    if (texAnim != NULL) {
        texAnim->positionU += 8;
        if (texAnim->positionU > 0x400) {
            texAnim->positionU -= 0x400;
        }
    }
}
