#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "dll.h"
#include "dlls/engine/29_gplay.h"
#include "dlls/engine/6_amsfx.h"
#include "functions.h"
#include "game/gamebits.h"
#include "dll_util.h"

#include "game/objects/object_id.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/768_SPshop.h"

#include "recomp/dlls/objects/768_SPshop_recomp.h"

extern ShopItem shopItemData[];

typedef struct {
    s8 unk0;
    u8 itemIndex;
    s8 unk2;
    s8 unk3;
    s8 unk4;
    //@recomp: extended struct
    s32 transitionTimer;    //frames until player is warped away when leaving shop
    s8 doWarp;              //Boolean tracking whether the exit transition has started
} Extended_SPShop_Data;

/** Shop item data edits (originally by jeebs2kx) 
    Allows Krystal and Sabre's Lanterns to be purchased separately
    Allows Sabre's Small Food Bag to be purchased
    Allows the Krazoa Translator to be purchased
*/
RECOMP_HOOK_DLL(SPShop_ctor) void hook_edit_shop_item_data() {
    //Firefly Lantern
    shopItemData[SPItem_Firefly_Lantern].sabre.hide = BIT_5D6;
    shopItemData[SPItem_Firefly_Lantern].krystal.hide = BIT_13D;
    
    //Small Player Food Bag
    shopItemData[SPItem_Small_Player_Foodbag].sabre.show = BIT_190;
    
    //Krazoa Translator (repurposing Large Player Foodbag slot)
    shopItemData[SPItem_Large_Player_Foodbag].krystal.show = BIT_ALWAYS_1;
    shopItemData[SPItem_Large_Player_Foodbag].krystal.hide = BIT_7BF;
}

/** Adds a fade-to-black when leaving the shop (Banjeoin) */
RECOMP_PATCH void SPShop_control(Object* self) {
    Extended_SPShop_Data* objData; 
    Object* player;

    objData = self->data; //@recomp: get objData
    player = get_player();

    if (self->unk0xdc == 0) {
        gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, 0, 1);
        gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, 5, 1);

        if (player->id == OBJ_Krystal) {
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, 6, 1);
        } else {
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, 7, 1);
        }

        func_80000860(self, self, 0x1C8, 0);
        func_80000860(self, self, 0x1CB, 0);
        func_80000450(self, self, 0x22F, 0, 0, 0);
        func_80000450(self, self, 0x231, 0, 0, 0);
        main_set_bits(BIT_SP_Entered_Shop, 1);
        gDLL_5_AMSEQ2->vtbl->func0(NULL, 0xF3, 0, 0, 0);
        func_8001EBD0(1);
        self->unk0xdc = 1;
    }

    //@recomp: update transition timer
    if (objData->doWarp) {
        objData->transitionTimer -= delayByte;
        if (objData->transitionTimer <= 0){
            objData->transitionTimer = 0;
        }
    }

    //@recomp: instead of immediately warping away when leaving shop, fade to black then warp
    if (!objData->doWarp && main_get_bits(BIT_SP_Exiting_Shop)) {
        gDLL_28_ScreenFade->vtbl->fade(20, SCREEN_FADE_BLACK);
        objData->transitionTimer = 35;
        objData->doWarp = TRUE;
        //TO-DO: lock player control until warp is finished
    }

    //@recomp: handle warping away once transition timer finished
    if (objData->doWarp && objData->transitionTimer <= 0) {
        //Fade from black after warping
        gDLL_28_ScreenFade->vtbl->fade_reversed(20, SCREEN_FADE_BLACK);

        if (player->id == OBJ_Sabre) {
            warpPlayer(WARP_SH_ROCKY_PODIUM, FALSE);
        } else {
            warpPlayer(WARP_SC_RUBBLE_PODIUM, FALSE);
        }
    }
}

//@recomp: extend object's Data struct
RECOMP_PATCH u32 SPShop_get_data_size(Object *self, u32 a1) {
    return sizeof(Extended_SPShop_Data);
}
