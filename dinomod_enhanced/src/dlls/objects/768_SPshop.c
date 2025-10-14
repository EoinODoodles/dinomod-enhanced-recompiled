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
