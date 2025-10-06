#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "recomp/dlls/engine/1_cmdmenu_recomp.h"
#include "dlls/engine/1_ui.h"
#include "game/objects/inventory_items.h"

#include "PR/ultratypes.h"
#include "sys/gfx/texture.h"
#include "sys/memory.h"
#include "dll.h"
#include "types.h"
#include "functions.h"

extern s16 _data_20;
extern InventoryItem _data_128[];
extern InventoryItem _data_2E4[];
extern InventoryItem _data_4A0[];
extern InventoryItem _data_4E8[];
extern InventoryItem _data_530[];
extern InventoryItem _data_5E4[];
extern InventoryItem _data_698[];
extern EnergyBar* _bss_90;

/** Prevents a crash when trying to leave CloudRunner Fortress' racetrack (originally by MusicalProgrammer, 25th February 2024) */
RECOMP_PATCH void dll_1_func_7550(void) {
    EnergyBar* fuelGauge;

    fuelGauge = _bss_90;

    if (!fuelGauge)
        return;

    fuelGauge->unk14 = 0;
    texture_destroy(fuelGauge->unk18);
    texture_destroy(fuelGauge->unk30);
    mmFree(_bss_90);
    _bss_90 = NULL;
}

#define INVENTORY_TEXT(bankID, lineID) ((bankID << 8) + (lineID & 0xFF))

/** Inventory item changes (originally by jeebs2kx, LaminGaming, MusicalProgrammer)
  * These data edits change the flags associated with certain inventory items, allowing them to appear/disappear from the inventory at appropriate times.
  * The icons for certain items were also changed, for example making the SpellStones use their activated icon when in that form
  * Items without description strings (or with mismapped ones) were also assigned appropriate text, which already existed in the game but was left unused
  */
RECOMP_HOOK_DLL(dll_1_ctor) void dll_1_ctor_hook_item_edits() {
    _data_20 = 0x50; //Don't know what this does!

    //Flag edits (obtaining items)
    _data_2E4[INVENTORY_ITEM_SABRE_0_NW_GATE_KEY].flagObtained = 0x7cc; //turns this into an activated version of Dragon Rock's SpellStone 

    //Flag edits (hiding items once used)
    _data_128[INVENTORY_ITEM_KRYSTAL_31_PRISON_KEY_CRF].flagHide = 0x453;
    _data_2E4[INVENTORY_ITEM_SABRE_11_DIM_BELINA_TE_CELL_KEY].flagHide = 0x219;
    _data_2E4[INVENTORY_ITEM_SABRE_12_DIM_TRICKY_CELL_KEY].flagHide = 0x208;
    _data_2E4[INVENTORY_ITEM_SABRE_13_WM_WARP_CRYSTAL].flagHide = 0x899;
    _data_2E4[INVENTORY_ITEM_SABRE_14_DIM_DOOR_KEY_1].flagHide = 0x24b;
    _data_2E4[INVENTORY_ITEM_SABRE_15_DIM_DOOR_KEY_2].flagHide = 0x285;
    _data_2E4[INVENTORY_ITEM_SABRE_25_WC_SILVER_TOOTH].flagHide = 0x25b;
    _data_2E4[INVENTORY_ITEM_SABRE_26_WC_GOLD_TOOTH].flagHide = 0x25a;
    _data_2E4[INVENTORY_ITEM_SABRE_31_SPELLSTONE_DIM_ACTIVATED].flagHide = 0x877;
    _data_2E4[INVENTORY_ITEM_SABRE_32_SPELLSTONE_WC_ACTIVATED].flagHide = 0x63c;

    //TextureID edits
    _data_128[INVENTORY_ITEM_KRYSTAL_23_SPELLSTONE_CRF_ACTIVATED].textureID = 0x563; //using the activated SpellStone icon
    _data_128[INVENTORY_ITEM_KRYSTAL_24_SPELLSTONE_BWC_ACTIVATED].textureID = 0x563; //using the activated SpellStone icon
    _data_128[INVENTORY_ITEM_KRYSTAL_25_SPELLSTONE_KP_ACTIVATED].textureID = 0x563;  //using the activated SpellStone icon
    _data_2E4[INVENTORY_ITEM_SABRE_0_NW_GATE_KEY].textureID = 0x563; //turns this unused item into an activated version of Dragon Rock's SpellStone
    _data_2E4[INVENTORY_ITEM_SABRE_32_SPELLSTONE_WC_ACTIVATED].textureID = 0x563; //using the activated SpellStone icon
    _data_2E4[INVENTORY_ITEM_SABRE_33_SPELLSTONE_DR_ACTIVATED].textureID = 0x562; //turns this into in inactive version of Dragon Rock's SpellStone
    
    //Description text edits
    _data_128[INVENTORY_ITEM_KRYSTAL_19_GOLD_NUGGET_1_GP].textID = INVENTORY_TEXT(1, 3);
    _data_128[INVENTORY_ITEM_KRYSTAL_20_GOLD_NUGGET_2_LFV].textID = INVENTORY_TEXT(1, 3);
    _data_128[INVENTORY_ITEM_KRYSTAL_24_SPELLSTONE_BWC_ACTIVATED].textID = 85;
    _data_128[INVENTORY_ITEM_KRYSTAL_27_HORN_OF_TRUTH].textID = 26;   
    _data_128[INVENTORY_ITEM_KRYSTAL_28_CRF_TREASURE_CHEST_KEY].textID = 99;
    _data_128[INVENTORY_ITEM_KRYSTAL_33_MOONSEEDS].textID = 98;
    //_data_2E4[INVENTORY_ITEM_SABRE_22_CORRUPT_FORCEFIELD_SPELL].textID = 26; //Patched in Dinomod, but not necessary since unused
    _data_2E4[INVENTORY_ITEM_SABRE_23_HORN_OF_TRUTH].textID = 26;
}

/** Adding new text for inventory items that didn't have any Gametext string (originally by LaminGaming)
    Note that this patch depends on the extra lines LaminGaming appended to file Gametext_3
    (This gametext file has 107 lines by default, so any indices beyond that number won't render without the text patch)
*/
RECOMP_HOOK_DLL(dll_1_ctor) void dll_1_ctor_hook_item_edits_extra_text() {
    s32 useExtraText = recomp_get_config_u32("lamingaming_extra_description_text");
    if (!useExtraText)
        return;

    //Krystal's items
    _data_128[INVENTORY_ITEM_KRYSTAL_9_DIM_GEAR_1].textID = 116;
    _data_128[INVENTORY_ITEM_KRYSTAL_10_DIM_GEAR_2].textID = 122;
    _data_128[INVENTORY_ITEM_KRYSTAL_11_DIM_GEAR_3].textID = 123;
    _data_128[INVENTORY_ITEM_KRYSTAL_12_DIM_GEAR_4].textID = 124;
    _data_128[INVENTORY_ITEM_KRYSTAL_23_SPELLSTONE_CRF_ACTIVATED].textID = 109;
    _data_128[INVENTORY_ITEM_KRYSTAL_25_SPELLSTONE_KP_ACTIVATED].textID = 111;
    _data_128[INVENTORY_ITEM_KRYSTAL_26_KRAZOA_TRANSLATOR].textID = 195;
    _data_128[INVENTORY_ITEM_KRYSTAL_31_PRISON_KEY_CRF].textID = 118;

    //Sabre's items
    _data_2E4[INVENTORY_ITEM_SABRE_0_NW_GATE_KEY].textID = 115;
    _data_2E4[INVENTORY_ITEM_SABRE_7_DIM_GEAR_1].textID = 116;
    _data_2E4[INVENTORY_ITEM_SABRE_8_DIM_GEAR_2].textID = 122;
    _data_2E4[INVENTORY_ITEM_SABRE_9_DIM_GEAR_3].textID = 123;
    _data_2E4[INVENTORY_ITEM_SABRE_10_DIM_GEAR_4].textID = 124;
    _data_2E4[INVENTORY_ITEM_SABRE_11_DIM_BELINA_TE_CELL_KEY].textID = 119;
    _data_2E4[INVENTORY_ITEM_SABRE_14_DIM_DOOR_KEY_1].textID = 120;
    _data_2E4[INVENTORY_ITEM_SABRE_15_DIM_DOOR_KEY_2].textID = 121;
    _data_2E4[INVENTORY_ITEM_SABRE_25_WC_SILVER_TOOTH].textID = 92;
    _data_2E4[INVENTORY_ITEM_SABRE_26_WC_GOLD_TOOTH].textID = 93;
    _data_2E4[INVENTORY_ITEM_SABRE_32_SPELLSTONE_WC_ACTIVATED].textID = 113;

    //Spells
    _data_698[INVENTORY_SPELL_2_GRENADE].textID = 21; //Change from "Randorn" to "Fire Spell" (unused text that was overwritten with "Grenade")
}
