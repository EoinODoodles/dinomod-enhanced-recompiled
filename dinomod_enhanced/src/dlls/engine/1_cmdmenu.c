#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "recomp/dlls/engine/1_cmdmenu_recomp.h"
#include "dlls/engine/1_ui.h"
#include "game/objects/inventory_items.h"

#include "common.h"

extern s16 _data_20;
extern InventoryItem _data_128[37];
extern InventoryItem _data_2E4[37];
extern InventoryItem _data_4A0[];
extern InventoryItem _data_4E8[];
extern InventoryItem _data_530[];
extern InventoryItem _data_5E4[];
extern InventoryItem _data_698[];
extern EnergyBar* _bss_90;

static u32 useExtraDescriptions;

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

/** Adds new text for inventory items that didn't have any Gametext string (originally by LaminGaming)
    Note that this patch depends on the extra lines LaminGaming appended to file Gametext_3
    (This gametext file has 107 lines by default, so any indices beyond that number won't render without the text patch)
*/
static void add_extra_descriptions() {
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
}

/** Resets extra description text back to default */
static void remove_extra_descriptions() {
    InventoryItem *bank;
    InventoryItem *item;
    u32 bankID;
    u32 index;
    u32 end;

    for (bankID = 0; bankID < 2; bankID++){
        bank = bankID == 0 ? _data_128 : _data_2E4;
        end = bankID == 0 ? ARRAYCOUNT(_data_128) : ARRAYCOUNT(_data_2E4);
        for (index = 0; index < end; index++){
            item = &bank[index];

            //Check if description text in bank 0 (gametext3) and beyond original last line
            if ((item->textID & 0xF00) == 0 && item->textID > 107){
                item->textID = -1;
                
            //Check if description text in bank 1 (gametext568) and beyond original last line
            } else if ((item->textID & 0xF00) == 0x100 && item->textID > 3){
                item->textID = -1;
            }
        }
    }
}

/** Adding new text for inventory items that didn't have any Gametext string (originally by LaminGaming)
    Note that this patch depends on the extra lines LaminGaming appended to file Gametext_3
    (This gametext file has 107 lines by default, so any indices beyond that number won't render without the text patch)
*/
RECOMP_HOOK_DLL(dll_1_ctor) void dll_1_ctor_hook_item_edits_extra_text() {
    _data_698[INVENTORY_SPELL_2_GRENADE].textID = 21; //Change from "Randorn" to "Fire Spell" (unused text that was overwritten with "Grenade")

    useExtraDescriptions = recomp_get_config_u32("lamingaming_extra_description_text");
    if (!useExtraDescriptions)
        return;

    add_extra_descriptions();
}

/** Check if extra text user config has changed */
RECOMP_CALLBACK("*", recomp_on_game_tick_start) void updateExtraTextInventory() {
    u32 setting = recomp_get_config_u32("lamingaming_extra_description_text");
    if (useExtraDescriptions == setting){
        return;
    }

    useExtraDescriptions = setting;

    if (useExtraDescriptions){
        add_extra_descriptions();
    } else {
        remove_extra_descriptions();
    }
}

// TODO: replace with full match from the decomp
RECOMP_PATCH s32 dll_1_func_F5C(Object **arg0, s32 arg1, u8 arg2, s32 arg3, f32 arg4) {
    s32 _pad[2];
    f32 temp_fa0;
    f32 temp_fa1;
    f32 temp_fv0;
    Camera* temp_s2;
    f32 sp9C;
    f32 sp98;
    f32 sp94;
    s32 _sp8C_pad[2];
    s32 sp88;
    s32 sp84;
    Object* temp_s0;
    Object** temp_v0;
    Object* temp_a0;
    Object* temp_v1;
    s32 var_a3;
    s32 var_s1;
    s32 var_s4;
    s32 var_v1;

    set_camera_selector(0);
    temp_s2 = get_main_camera();
    temp_v0 = get_world_objects(&sp84, &sp88);
    var_s4 = 0;
    for (var_s1 = sp84; var_s1 < sp88; var_s1++) {
        temp_s0 = temp_v0[var_s1];
        // @recomp: Allow transparent objects to be targeted. Allows CCFirecrystal to be picked up.
        //          (Original patch by MusicalProgrammer)
        if ((temp_s0->def->unk40 != NULL) && /*(temp_s0->opacity == 0xFF)*/(temp_s0->opacity >= 32) && !(temp_s0->unkAF & 8) && 
                (temp_s0->def->unk40->unk10 & arg2) && (var_s4 < arg1) && (arg3 & 1)) {
            get_object_child_position(temp_s0, &sp9C, &sp98, &sp94);
            temp_fa0 = sp9C - temp_s2->srt.transl.x;
            temp_fv0 = sp98 - temp_s2->srt.transl.y;
            temp_fa1 = sp94 - temp_s2->srt.transl.z;
            if ((SQ(temp_fa0) + SQ(temp_fv0) + SQ(temp_fa1)) < SQ(arg4)) {
                var_v1 = temp_s2->srt.yaw - ((0x4000 - arctan2_f(temp_fa0, temp_fa1)) & 0xFFFF);
                CIRCLE_WRAP(var_v1);
                if ((var_v1 < -0x2710) && (var_v1 > -0x55F0)) {
                    arg0[var_s4] = temp_s0;
                    var_s4 += 1;
                }
            }
        }
    }

    if (var_s4 > 0) {
        do {
            var_a3 = 1;
            for (var_s1 = 0; var_s1 < (var_s4 - 1); var_s1++) {
                temp_v1 = arg0[var_s1];
                temp_a0 = arg0[var_s1 + 1];
                if ((s32)temp_v1 < (s32)temp_a0) {
                    arg0[var_s1] = temp_a0;
                    arg0[var_s1 + 1] = temp_v1;
                    var_a3 = 0;
                }
            }
        } while (var_a3 == 0);
    }
    
    return var_s4;
}
