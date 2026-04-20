#include "game/objects/interaction_arrow.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "common.h"
#include "game/objects/inventory_items.h"
#include "game/gametexts_ui.h"
#include "sys/main.h"
#include "sys/objmsg.h"
#include "dlls/engine/1_cmdmenu.h"

#include "recomp/dlls/engine/1_cmdmenu_recomp.h"

#include "engine/1_cmdmenu.h"
#include "engine/59_minimap.h"

#define MAX_LOADED_ITEMS 64
#define MAX_OPACITY 0xFF
#define MAX_OPACITY_F 255.0f

#define NO_GAMETEXT -1
#define NO_TEXTURE -1
#define NO_PAGE -1
#define NO_ITEM -1

#define SLOT_OCCUPIED 0
#define SLOT_PADDED 1

#define NONE 0xFFFF
#define EXIT -1

/* UI COORD MACROS (TO-DO: move to separate header?) */

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

//Change these to move the UI elements in the top-left of screen (character icon, health, magic)
#define UI_TOP_LEFT_X 0
#define UI_TOP_LEFT_Y 0

//Change these to move the UI elements in the top-right of screen (C-buttons, inventory scroll)
#define UI_TOP_RIGHT_X 0
#define UI_TOP_RIGHT_Y 0

//Change these to move the UI elements in the bottom-left of screen (item info pop-up, minimap)
#define UI_BOTTOM_LEFT_X 0
#define UI_BOTTOM_LEFT_Y 0

//Change these to move the UI elements in the bottom-right of screen (Scarab counter, active Spell/Command)
#define UI_BOTTOM_RIGHT_X 0
#define UI_BOTTOM_RIGHT_Y 0

/* UI TOP-LEFT */

//Character icon
#define CHARACTER_ICON_X 20
#define CHARACTER_ICON_Y 10

//Health
#define HEALTH_ICONS_X 60
#define HEALTH_ICONS_Y 20
#define APPLES_SPACING_X 10
#define APPLES_SPACING_Y 10
#define APPLES_ROW_1 7
#define APPLES_ROW_2 6
#define APPLES_ROW_1_OFFSET_X 0
#define APPLES_ROW_2_OFFSET_X 5
#define APPLES_ROW_3_OFFSET_X 0

#define APPLES_ROW_2_IDX (APPLES_ROW_1)
#define APPLES_ROW_3_IDX (APPLES_ROW_1 + APPLES_ROW_2)

//Magic
#define MAGIC_UNITS_PER_BAR 25
#define MAGIC_BARS_WIDTH 66
#define MAGIC_BARS_HEIGHT 14
#define MAGIC_BARS_X 23
#define MAGIC_BARS_Y 60
#define MAGIC_BARS_SPACING_Y 12
#define MAGIC_BARS_ZERO_POINT_X 13

/* UI TOP-RIGHT */

//Inventory page icon
#define PAGE_ICON_X 261
#define PAGE_ICON_Y 10

//Inventory item icons
#define MENU_ITEM_X 262
#define MENU_ITEM_Y 59
#define MENU_ITEM_WIDTH 32
#define MENU_ITEM_HEIGHT 24
#define MENU_ITEM_QUANTITY_OFFSET_X 14
#define MENU_ITEM_QUANTITY_OFFSET_Y 9

#define MENU_ITEM_QUANTITY_X (MENU_ITEM_X + MENU_ITEM_QUANTITY_OFFSET_X)

//Inventory scroll
#define MENU_HEIGHT_OPEN 72

#define MENU_SCROLL_WIDTH 40
#define MENU_SCROLL_HEIGHT 8

#define MENU_SCROLL_X (MENU_ITEM_X - (MENU_SCROLL_WIDTH - MENU_ITEM_WIDTH)/2)
#define MENU_SCROLL_TOP_Y (MENU_ITEM_Y - MENU_SCROLL_HEIGHT)
#define MENU_SCROLL_BOTTOM_Y (MENU_ITEM_Y)

#define MENU_SCROLL_CENTRE_Y (MENU_ITEM_Y + (MENU_ITEM_HEIGHT + MENU_ITEM_HEIGHT/2)) //Screen Y-coord in the middle of the inventory's 3 tiles

//Inventory item selection highlight
#define ITEM_HL_WIDTH 8
#define ITEM_HL_HEIGHT 6
#define ITEM_HL_MARGIN 4

#define ITEM_HL_X1 (MENU_ITEM_X + ITEM_HL_MARGIN)
#define ITEM_HL_Y1 (MENU_ITEM_Y + (MENU_HEIGHT_OPEN - MENU_ITEM_HEIGHT)/2 - 1)
#define ITEM_HL_X2 (MENU_ITEM_X + MENU_ITEM_WIDTH - ITEM_HL_WIDTH - ITEM_HL_MARGIN)
#define ITEM_HL_Y2 (ITEM_HL_Y1 + MENU_ITEM_HEIGHT - ITEM_HL_MARGIN)

//Sidekick meter
#define SIDEKICK_METER_X 250
#define SIDEKICK_METER_Y 21
#define SIDEKICK_METER_SPACING_X 9
#define SIDEKICK_METER_SPACING_Y 8
#define SIDEKICK_METER_ICONS_PER_COLUMN 4

//C buttons
#define C_BUTTONS_X 245
#define C_BUTTONS_Y 17

#define C_BUTTONS_LEFT_EMPTY_X (C_BUTTONS_X + 1)
#define C_BUTTONS_LEFT_EMPTY_Y (C_BUTTONS_Y + 9)

#define C_BUTTONS_DOWN_EMPTY_X (C_BUTTONS_X + 7)
#define C_BUTTONS_DOWN_EMPTY_Y (C_BUTTONS_Y + 26)

#define C_BUTTONS_RIGHT_EMPTY_X (C_BUTTONS_X + 29)
#define C_BUTTONS_RIGHT_EMPTY_Y (C_BUTTONS_Y + 17)

#define C_BUTTONS_LEFT_DOWN_BOOK_SIDEKICK_X (C_BUTTONS_X + 0)
#define C_BUTTONS_LEFT_DOWN_BOOK_SIDEKICK_Y (C_BUTTONS_Y + 0)

#define C_BUTTONS_RIGHT_BAG_X (C_BUTTONS_X + 30)
#define C_BUTTONS_RIGHT_BAG_Y (C_BUTTONS_Y + 8)

/* UI BOTTOM-LEFT */

//Item info pop-up
#define INFO_POPUP_X 20
#define INFO_POPUP_Y 175
#define INFO_POPUP_EDGE_WIDTH 16

#define INFO_POPUP_L_X (INFO_POPUP_X + 0)
#define INFO_POPUP_M_X (INFO_POPUP_L_X + INFO_POPUP_EDGE_WIDTH)
#define INFO_POPUP_R_X (INFO_POPUP_M_X + MENU_ITEM_WIDTH)
#define INFO_POPUP_SHADOW_X (INFO_POPUP_X + 2)
#define INFO_POPUP_SHADOW_Y (INFO_POPUP_Y + 1)
#define INFO_POPUP_QUANTITY_X (INFO_POPUP_M_X + 16)
#define INFO_POPUP_QUANTITY_Y (INFO_POPUP_Y + 16)

/* UI BOTTOM-RIGHT */

//Scarabs counter
#define SCARABS_ICON_X 252
#define SCARABS_ICON_Y 198
#define SCARABS_ICON_WIDTH 16
#define SCARABS_ICON_HEIGTH 16
#define SCARABS_NUMBER_X (SCARABS_ICON_X + 18)
#define SCARABS_NUMBER_Y (SCARABS_ICON_Y + 4)

//Active Spell
#define ACTIVE_SPELL_X 253
#define ACTIVE_SPELL_Y 169
#define ACTIVE_SPELL_ICON_OFFSET_X 9
#define ACTIVE_SPELL_ICON_OFFSET_Y 11

#define ACTIVE_SPELL_ICON_X (ACTIVE_SPELL_X + ACTIVE_SPELL_ICON_OFFSET_X)
#define ACTIVE_SPELL_ICON_Y (ACTIVE_SPELL_Y + ACTIVE_SPELL_ICON_OFFSET_Y)

//Active Sidekick Command
#define ACTIVE_SIDECOMMAND_X 253
#define ACTIVE_SIDECOMMAND_Y 121
#define ACTIVE_SIDECOMMAND_ICON_OFFSET_X 9
#define ACTIVE_SIDECOMMAND_ICON_OFFSET_Y 11

#define ACTIVE_SIDECOMMAND_ICON_X (ACTIVE_SIDECOMMAND_X + ACTIVE_SIDECOMMAND_ICON_OFFSET_X)
#define ACTIVE_SIDECOMMAND_ICON_Y (ACTIVE_SIDECOMMAND_Y + ACTIVE_SIDECOMMAND_ICON_OFFSET_Y)

/* CENTRED UI */

//Info scroll
#define INFO_SCROLL_WIDTH 120
#define INFO_SCROLL_HEIGHT 50 //When open
#define INFO_SCROLL_TEXT_Y 3 //Top margin for text printed inside the tutorial box
#define INFO_SCROLL_LINE_HEIGHT 16 //Text lines' vertical spacing
#define INFO_SCROLL_X (SCREEN_WIDTH/2)
#define INFO_SCROLL_Y 30
#define INFO_SCROLL_Y_INITIAL (INFO_SCROLL_Y - 10)
#define INFO_SCROLL_OPACITY_MAX 160
#define INFO_SCROLL_OPACITY_SPEED 32

//Info scroll texture dimensions
#define INFO_SCROLL_PAGE_EDGE_WIDTH 16
#define INFO_SCROLL_PAGE_SHADOW_HEIGHT 8
#define INFO_SCROLL_ROLL_HEIGHT 16
#define INFO_SCROLL_ROLL_TOP_Y_OFFSET 11
#define INFO_SCROLL_ROLL_BOTTOM_Y_OFFSET 4 //NOTE: causes 1px gap, may be intentional as dark shadow
#define INFO_SCROLL_HANDLE_WIDTH 16
#define INFO_SCROLL_HANDLE_HEIGHT 16

//Tutorial textbox
#define TUTORIAL_BOX_WIDTH 240
#define TUTORIAL_BOX_HEIGHT 80
#define TUTORIAL_BOX_TEXT_Y 3 //Top margin for text printed inside the tutorial box
#define TUTORIAL_BOX_LINE_HEIGHT 16 //Text lines' vertical spacing
#define TUTORIAL_BOX_X (SCREEN_WIDTH/2)
#define TUTORIAL_BOX_Y 20
#define TUTORIAL_BOX_OPACITY_MAX 160
#define TUTORIAL_BOX_OPACITY_SPEED 8

//Tutorial textbox texture dimensions
#define TUTORIAL_BOX_PAGE_EDGE_WIDTH 16
#define TUTORIAL_BOX_PAGE_SHADOW_HEIGHT 8
#define TUTORIAL_BOX_ROLL_HEIGHT 16
#define TUTORIAL_BOX_ROLL_TOP_Y_OFFSET 11
#define TUTORIAL_BOX_ROLL_BOTTOM_Y_OFFSET 4 //NOTE: causes 1px gap, may be intentional as dark shadow
#define TUTORIAL_BOX_HANDLE_WIDTH 16
#define TUTORIAL_BOX_HANDLE_HEIGHT 16

#define TUTORIAL_BOX_A_BUTTON_WIDTH 24
#define TUTORIAL_BOX_A_BUTTON_HEIGHT 24
#define TUTORIAL_BOX_A_BUTTON_OFFSET_X 8
#define TUTORIAL_BOX_A_BUTTON_OFFSET_Y 0

//Energy bar
#define ENERGY_BAR_X (SCREEN_WIDTH / 2)
#define ENERGY_BAR_Y (SCREEN_HEIGHT - 10)

//Aiming reticle
#define AIMING_RETICLE_WIDTH 32
#define AIMING_RETICLE_HEIGHT 32
#define AIMING_RETICLE_OPACITY 150

enum CmdMenuTextures {
    CMDMENU_TEX_00_Scroll_BG = 0,
    CMDMENU_TEX_01_Scroll_Bottom = 1,
    CMDMENU_TEX_02_Scroll_Top = 2,
    CMDMENU_TEX_03_InfoScroll_Roll_End = 3,
    CMDMENU_TEX_04_InfoScroll_Roll = 4,
    CMDMENU_TEX_05_InfoScroll_Side = 5,
    CMDMENU_TEX_06_InfoScroll_BG = 6,
    CMDMENU_TEX_07_InfoScroll_SelfShadow = 7,
    CMDMENU_TEX_08_Apple_0_Pct = 8,
    CMDMENU_TEX_09_Apple_25_Pct = 9,
    CMDMENU_TEX_10_Apple_50_Pct = 10,
    CMDMENU_TEX_11_Apple_75_Pct = 11,
    CMDMENU_TEX_12_Mushroom_Blue_Full = 12,
    CMDMENU_TEX_13_Grub_Blue_Full = 13,
    CMDMENU_TEX_14_Unk_Circle_Glow = 14,
    CMDMENU_TEX_15_Unk_Circle_Blue = 15,
    CMDMENU_TEX_16_Grub_Blue_Half = 16,
    CMDMENU_TEX_17_Apple_100_Pct = 17,
    CMDMENU_TEX_18_Scarab = 18,
    CMDMENU_TEX_19_Scarab_Flutter_Frame1 = 19,
    CMDMENU_TEX_20_Scarab_Flutter_Frame2 = 20,
    CMDMENU_TEX_21_Scarab_Flutter_Frame3 = 21,
    CMDMENU_TEX_22_Scarab_Spin_Frame1 = 22,
    CMDMENU_TEX_23_Scarab_Spin_Frame2 = 23,
    CMDMENU_TEX_24_Scarab_Spin_Frame3 = 24,
    CMDMENU_TEX_25_Scarab_Spin_Frame4 = 25,
    CMDMENU_TEX_26_Scarab_Spin_Frame5 = 26,
    CMDMENU_TEX_27_Scarab_Spin_Frame6 = 27,
    CMDMENU_TEX_28_Scarab_Spin_Frame7 = 28,
    CMDMENU_TEX_29_Page_Torn_Left = 29,
    CMDMENU_TEX_30_Page_Torn_Right = 30,
    CMDMENU_TEX_31_Highlight_Corner_Top_Left = 31,
    CMDMENU_TEX_32_Highlight_Corner_Top_Right = 32,
    CMDMENU_TEX_33_Highlight_Corner_Bottom_Left = 33,
    CMDMENU_TEX_34_Highlight_Corner_Bottom_Right = 34,
    CMDMENU_TEX_35_MagicBar_Empty = 35,
    CMDMENU_TEX_36_MagicBar_Full = 36,
    CMDMENU_TEX_37_C_Down = 37,
    CMDMENU_TEX_38_LeftDownButtons_SpellBook_With_Kyte = 38,
    CMDMENU_TEX_39_C_Left = 39,
    CMDMENU_TEX_40_Sabre = 40,
    CMDMENU_TEX_41_C_Right = 41,
    CMDMENU_TEX_42_Tricky = 42,
    CMDMENU_TEX_43_LeftDownButtons_SpellBook_With_Tricky = 43,
    CMDMENU_TEX_44_Mushroom_Empty = 44,
    CMDMENU_TEX_45_Mushroom_Red_Full = 45,
    CMDMENU_TEX_46_Mushroom_Red_Half = 46,
    CMDMENU_TEX_47_RightButton_With_Bag = 47,
    CMDMENU_TEX_48_LeftDownButtons_SpellBook_NoSidekick = 48,
    CMDMENU_TEX_49_MagicBook = 49,
    CMDMENU_TEX_50_Bag = 50,
    CMDMENU_TEX_51_Mushroom_Blue_Half = 51,
    CMDMENU_TEX_52_Page_Torn_Shadow = 52,
    CMDMENU_TEX_53_Krystal = 53,
    CMDMENU_TEX_54_Kyte = 54,
    CMDMENU_TEX_55_Grub_Empty = 55,
    CMDMENU_TEX_56_Grub_Red_Full = 56,
    CMDMENU_TEX_57_Grub_Red_Half = 57
};

extern s8 dInventoryShow;
extern s8 sInventoryScrollOffset;
extern s8 dInventoryMoveSpeed;
extern s16 dInventoryUnrollMax;
extern s16 dInventoryOpacity;
extern s16 dOpacitySidekickMeter;
extern s16 dSelectedItemTextID;
extern s8 dInfoScrollShow;
extern s16 dInfoScrollWidthHalf;
extern s16 dInfoScrollUnrollMax;
extern s16 dInfoScrollY;
extern s16 dInfoScrollX;
extern s16 dInfoScrollOpacity;
extern char* dInfoScrollStrings[];
extern s16 dInfoScrollTextID;
extern s8 dTutorialBoxShow;
extern s16 dTutorialBoxHalfWidth;
extern s16 dTutorialBoxHeight;
extern s16 dTutorialBoxY;
extern s16 dTutorialBoxX;
extern s16 dTutorialBoxOpacity;
extern s16 dTutorialBoxTextOpacity;
extern Texture* dInventoryPageIcon;
extern s8 sJoyButtonMask;
extern s16 dInventoryMovesQueued;
extern u8 dInventoryIsScrolling;
extern u8 sForceStatsDisplay;
extern s16 dInfoScrollDisabled;
extern s16 dSpellGamebits[];
extern s16 dSpellTextableIDs[];
extern s16 dCommandTextableIDs[];
extern CmdmenuPlayerStatsChangeSounds dStatChangeSounds;
extern s8 dInventoryFrameTop;
extern s8 dInventoryFrameBottom;
extern s8 dInfoScrollFrameTop;
extern s8 dInfoScrollFrameBottom;

extern InventoryItem dPage0ItemsKrystal[37];
extern InventoryItem dPage1ItemsSabre[37];
extern InventoryItem dPage2FoodActionsKrystal[];
extern InventoryItem dPage3FoodActionsSabre[];
extern InventoryItem dPage4FoodItemsKrystal[];
extern InventoryItem dPage5FoodItemsSabre[];
extern InventoryItem dPage6MagicSpells[];
extern InventoryCommand dPage7CommandsKyte[];
extern InventoryCommand sPage8CommandsTricky[];
extern InventoryItem dPage9FoodActionsKyte[];
extern InventoryItem dPage10FoodActionsTricky[];
extern InventoryItem dPage11FoodItemsKyte[];
extern InventoryItem dPage12FoodItemsTricky[];
extern CmdmenuPage dCmdmenuPages[];
extern s8 dPageCategory;
extern s8 dNextPageCategory;
extern s16 dTextableIDs[];

extern f32 sOpacityHealth;  //Opacity of player health UI
extern f32 sOpacityScarabs; //Opacity of Scarab counter UI
extern f32 sOpacityMagic;   //Opacity of player magic bar UI
extern CmdmenuPlayerSidekickData sStats;
extern CmdmenuPlayerSidekickData sPrevStats;
extern CmdmenuPlayerSidekickDataChangeTimers sStatsChangeTimers;
extern u8 sPlayerStatsFlags;
extern u8 sAnimFrameScarab;        //Frame offset for the Scarab (an ID offset in practice, since Scarab's animation frames are stored as separate textures)
extern u8 sAnimScarabFlutterTimer; //Plays Scarab flutter animation during last ticks of countdown
extern u8 sAnimScarabSpin;         //Plays Scarab spin animation when nonzero (used as frame offset)
extern f32 sOpacityR;              //Opacity of icons on right side of screen (C-buttons, menu page image, etc.)
extern EnergyBar* sEnergyBar; 
extern Texture* sMenuItemTextures[MAX_LOADED_ITEMS];          //Inventory icon texture pointers for the current menu page's loaded items
extern Texture* sMenuItemTexturesSidekick[MAX_LOADED_ITEMS];  //Sidekick command inventory icon textures for the current menu page's loaded items (unused aside from loading the textures)
extern s16 sMenuItemTextureIDs[MAX_LOADED_ITEMS];             //TextableIDs for the current menu page's loaded items
extern s32 sMenuItemGamebits[MAX_LOADED_ITEMS];               //GamebitIDs for the current menu page's loaded items (or for sidekick commands: the command's index)
extern s16 sMenuItemTextIDs[MAX_LOADED_ITEMS];                //Gametext lineIDs for the current menu page's loaded items
extern s8 sMenuItemOpenPageIDs[MAX_LOADED_ITEMS];             //The menu page ID opened by each of the current menu page's loaded items (or -1 if it closes the inventory on use)
extern u8 sMenuItemUseSounds[MAX_LOADED_ITEMS];               //The sound types (see `CmdMenuItemSounds`) used by each of the current menu page's loaded items
extern u8 sMenuItemVisibilities[MAX_LOADED_ITEMS];            //Visibility Booleans for each of the current menu page's loaded items (Spells are the only kind of inventory item that still load while their gamebitHidden is set, however they're drawn as an empty tile when hidden)
extern u8 sMenuItemQuantities[MAX_LOADED_ITEMS];              //Item quantities for each of the current menu page's loaded items
extern Texture* sActiveSpellIcon;             //Icon in bottom-right of screen, showing the Spell currently in use
extern Texture* sActiveSpellRing;             //Icon in bottom-right of screen, circling the Spell currently in use
extern Texture* sAButtonAnimTex;              //Animated A button icon, shown on the tutorial textbox
extern s16 sPrevActiveSpellGamebit;           //The gamebitID of the mostly recently-used Spell (used to check if the active Spell changed)
extern Texture* sActiveSidekickCommandIcon;   //Icon in bottom-right of screen, showing the Sidekick Command currently in use
extern Texture* sActiveSidekickCommandRing;   //Icon in bottom-right of screen, circling the Sidekick Command currently in use
extern s16 sPrevSidekickCommandIndex;         //The index of the mostly recently-used Sidekick Command (used to check if the active Sidekick Command changed)
extern s32 sAButtonAnimRenderFlags;
extern s32 sCrosshairAnimRenderFlags;
extern s32 sAButtonAnimProgress;
extern s32 sCrosshairAnimProgress;
extern Texture* sTextures[];
extern Texture* sInventoryStackNumbersTex;
extern TextureTile sTextureTiles[][2];
extern s16 sInventoryUnrollY;  //How far the inventory scroll has opened (0 when fully closed)
extern s16 sInfoScrollUnrollY; //How far the R-button info scroll has opened (0 when fully closed)
extern s16 sTutorialBoxHeight;
extern s16 sTutorialBoxStringIndex;
extern GameTextChunk* sTutorialBoxGametext;
extern Texture* sCrosshairTex;
extern s16 sUsedItemGamebitID;  //The gamebitID associated with the used item
extern s16 sSubmenuGamebitID;   //The gamebitID associated with the item that opened a menu subpage (e.g. a foodbag's gamebit)
extern s8 sUsedItemSoundType;   //Set to 0 when item selection successful (item given to character, etc.)
extern s8 sUsedItemPageID;      //The pageID associated with the used item (see `CmdMenuPages`)
extern s8 sInventoryPageID;     //The pageID currently open (see `CmdMenuPages`)
extern s16 sMenuSelectedItemIdx; //Display index of the item currently selected in the menu page 
extern s32 sDisplayedItemCount;  //The number of items displayed on the current page (while drawing the inventory icons, this number is updated to be at least the number of slots in the tile strip)
extern s8 sShouldOverrideJoypadButtons;   //Whether to fully override the player's UI control with simulated button presses
extern s32 sInventoryFrameCounter;        //Counts how many times `cmdmenu_update2` has run (clamped from 0-2, and resets to 0 upon closing the inventory)
extern s32 sJoyPressedButtons;            //Joypad button bitfield
extern s32 sJoyPressedButtonsOverride;    //Joypad button bitfield (for simulated presses, used during inventory tutorials)
extern s32 sJoyHeldButtons;               //Joypad button bitfield
extern TextureTile sTempIcon[2];
extern s8 sAutoSelectItemGamebit;     //Always -1, but seems intended to auto-select a specific item when opening the inventory
extern s16 sAutoSelectItemIdx;        //Index of the auto-selected item (unused in practice)
extern s16 sInfoScrollOverrideTextID; //Causes the info scroll to open automatically (@bug: initialises at 0 instead of NO_GAMETEXT. This causes the info box to display for the first few frames of gameplay, leaving a beige smear in the top-left of the framebuffer.)
extern s16 sInfoScrollOverrideX;      //Custom screen position for the info scroll when auto-shown
extern s16 sInfoScrollOverrideY;      //Custom screen position for the info scroll when auto-shown
extern CmdmenuInfoPopup sInfoPopup;   //Item info pop-up that appears after collecting certain items (e.g. Kyte's grubs)

extern void cmdmenu_tick_tutorial_textbox(void);
extern void cmdmenu_draw_tutorial_textbox(Gfx** gdl, Mtx** mtxs, Vertex** vtxs);
extern void cmdmenu_tick_inventory_page(void);
extern void cmdmenu_draw_main(Gfx** gdl, Mtx** mtxs, Vertex** vtxs);
extern s32 cmdmenu_page_load_items(InventoryItem* items, s8 isSidekickMenu);
extern s32 cmdmenu_page_count_shown_items(InventoryItem* menuItems, s8 isSidekickMenu);
extern void cmdmenu_store_loaded_item_metadata(InventoryItem* items, s32 loadedItemIndex, s32 itemIndex);
extern void cmdmenu_gfx_set_texture(Gfx** gdl, Texture* tex, s32 frame);
extern int cmdmenu_is_inventory_open(void);
extern int cmdmenu_is_inventory_closed(void);
extern void cmdmenu_close_inventory(void);
extern void cmdmenu_open_inventory(void);
extern void cmdmenu_inventory_animate(void);
extern void cmdmenu_draw_c_buttons_and_sidekick_meter(Gfx** gdl, Mtx** mtxs, Vertex** vtxs);
extern void cmdmenu_gfx_set_scroll_scissor(Gfx** gdl);
extern void cmdmenu_gfx_set_screen_scissor(Gfx** gdl);
extern int cmdmenu_is_info_scroll_closed(void);
extern void cmdmenu_close_info_scroll(void);
extern void cmdmenu_open_info_scroll(void);
extern void cmdmenu_info_scroll_animate(void);
extern void cmdmenu_draw_info_scroll(Gfx** gdl, Mtx** mtxs, Vertex** vtxs);
extern void cmdmenu_update_stats(void);
extern void cmdmenu_draw_player_stats(Gfx** gdl, Mtx** mtxs, Vertex** vtxs);
extern void cmdmenu_info_hide(CmdmenuInfoPopup* info);
extern void cmdmenu_info_draw(Gfx** gdl, CmdmenuInfoPopup* box);
extern void cmdmenu_draw_energy_bar(Gfx** gdl);
extern void cmdmenu_energy_bar_free(void);

static u32 useExtraDescriptions;

/** Prevents a crash when trying to leave CloudRunner Fortress' racetrack (originally by MusicalProgrammer, 25th February 2024) */
void cmdmenu_energy_bar_free(void) {
    EnergyBar* enbar;

    /* default.dol
    if (sEnergyBar == NULL) {
        STUBBED_PRINTF(" WARNING : cmdmenu Energy bar alreadby freed \n");
    }
    */

    STUBBED_PRINTF(" Killing Bar ");
    enbar = sEnergyBar;

    //@recomp: null pointer return
    if (!enbar) {
        return;
    }

    enbar->alpha = 0;
    tex_free(enbar->fullbarTex[0].tex);
    tex_free(enbar->emptybarTex[0].tex);
    mmFree(sEnergyBar);
    sEnergyBar = NULL;
}

#define INVENTORY_TEXT(bankID, lineID) ((bankID << 8) + (lineID & 0xFF))

/** Inventory item changes (originally by jeebs2kx, LaminGaming, MusicalProgrammer)
  * These data edits change the flags associated with certain inventory items, allowing them to appear/disappear from the inventory at appropriate times.
  * The icons for certain items were also changed, for example making the SpellStones use their activated icon when in that form
  * Items without description strings (or with mismapped ones) were also assigned appropriate text, which already existed in the game but was left unused
  */
RECOMP_HOOK_DLL(cmdmenu_ctor) void cmdmenu_ctor_hook_item_edits() {
    dInfoScrollWidthHalf = 80; //Increase info scroll width to 160

    //Gamebit edits (obtaining items)
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_0_NW_GATE_KEY].gamebitObtained = BIT_7CC; //turns this into an activated version of Dragon Rock's SpellStone 

    //Gamebit edits (hiding items once used)
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_31_PRISON_KEY_CRF].gamebitHide = BIT_453;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_11_DIM_BELINA_TE_CELL_KEY].gamebitHide = BIT_219;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_12_DIM_TRICKY_CELL_KEY].gamebitHide = BIT_208;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_13_WM_WARP_CRYSTAL].gamebitHide = BIT_WM_Sabre_Transporter_Visible;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_14_DIM_DOOR_KEY_1].gamebitHide = BIT_24B;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_15_DIM_DOOR_KEY_2].gamebitHide = BIT_285;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_25_WC_SILVER_TOOTH].gamebitHide = BIT_25B;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_26_WC_GOLD_TOOTH].gamebitHide = BIT_25A;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_31_SPELLSTONE_DIM_ACTIVATED].gamebitHide = BIT_877;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_32_SPELLSTONE_WC_ACTIVATED].gamebitHide = BIT_DB_Unlock_Act_Three;

    //TextureID edits
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_23_SPELLSTONE_CRF_ACTIVATED].textureID = TEXTABLE_563; //using the activated SpellStone icon
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_24_SPELLSTONE_BWC_ACTIVATED].textureID = TEXTABLE_563; //using the activated SpellStone icon
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_25_SPELLSTONE_KP_ACTIVATED].textureID = TEXTABLE_563;  //using the activated SpellStone icon
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_0_NW_GATE_KEY].textureID = TEXTABLE_563; //turns this unused item into an activated version of Dragon Rock's SpellStone
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_32_SPELLSTONE_WC_ACTIVATED].textureID = TEXTABLE_563; //using the activated SpellStone icon
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_33_SPELLSTONE_DR_ACTIVATED].textureID = TEXTABLE_562; //turns this into an inactive version of Dragon Rock's SpellStone
    
    //Description text edits
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_19_GOLD_NUGGET_1_GP].textID = INVENTORY_TEXT(1, GAMETEXT_UI_B_03_Shiney_Nugget);
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_20_GOLD_NUGGET_2_LFV].textID = INVENTORY_TEXT(1, GAMETEXT_UI_B_03_Shiney_Nugget);
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_24_SPELLSTONE_BWC_ACTIVATED].textID = GAMETEXT_UI_55_Ice_Mountain_Map; //replacing Ice Mountain Map description (TODO: add a new string for this, and restore Ice Mountain line?)
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_27_HORN_OF_TRUTH].textID = GAMETEXT_UI_1A_Horn_Of_Truth;
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_28_CRF_TREASURE_CHEST_KEY].textID = GAMETEXT_UI_63_Treasure_Chest_Key;
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_33_MOONSEEDS].textID = GAMETEXT_UI_62_Moon_Seeds;
    //dPage1ItemsSabre[INVENTORY_ITEM_SABRE_22_CORRUPT_FORCEFIELD_SPELL].textID = 26; //Patched in Dinomod, but not necessary since unused
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_23_HORN_OF_TRUTH].textID = GAMETEXT_UI_1A_Horn_Of_Truth;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_25_WC_SILVER_TOOTH].textID = GAMETEXT_UI_5C_Silver_Trex_Tooth;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_26_WC_GOLD_TOOTH].textID = GAMETEXT_UI_5D_Gold_Trex_Tooth;
}

/** Adds new text for inventory items that didn't have any Gametext string (originally by LaminGaming)
    Note that this patch depends on the extra lines LaminGaming appended to file Gametext_3
    (This gametext file has 107 lines by default, so any indices beyond that number won't render without the text patch)
*/
static void add_extra_descriptions() {
    //Krystal's items
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_9_DIM_GEAR_1].textID = 116;
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_10_DIM_GEAR_2].textID = 122;
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_11_DIM_GEAR_3].textID = 123;
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_12_DIM_GEAR_4].textID = 124;
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_23_SPELLSTONE_CRF_ACTIVATED].textID = 109;
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_25_SPELLSTONE_KP_ACTIVATED].textID = 111;
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_26_KRAZOA_TRANSLATOR].textID = 195;
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_31_PRISON_KEY_CRF].textID = 118;

    //Sabre's items
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_0_NW_GATE_KEY].textID = 115;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_7_DIM_GEAR_1].textID = 116;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_8_DIM_GEAR_2].textID = 122;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_9_DIM_GEAR_3].textID = 123;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_10_DIM_GEAR_4].textID = 124;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_11_DIM_BELINA_TE_CELL_KEY].textID = 119;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_14_DIM_DOOR_KEY_1].textID = 120;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_15_DIM_DOOR_KEY_2].textID = 121;
    dPage1ItemsSabre[INVENTORY_ITEM_SABRE_32_SPELLSTONE_WC_ACTIVATED].textID = 113;
}

/** Resets extra description text back to default */
static void remove_extra_descriptions() {
    InventoryItem *bank;
    InventoryItem *item;
    u32 bankID;
    u32 index;
    u32 end;

    for (bankID = 0; bankID < 2; bankID++){
        bank = bankID == 0 ? dPage0ItemsKrystal : dPage1ItemsSabre;
        end = bankID == 0 ? ARRAYCOUNT(dPage0ItemsKrystal) : ARRAYCOUNT(dPage1ItemsSabre);
        for (index = 0; index < end; index++){
            item = &bank[index];

            //Check if description text in bank 0 (gametext3) and beyond original last line
            if ((item->textID & 0xF00) == 0 && item->textID > GAMETEXT_UI_6B_Warp_Podium){
                item->textID = -1;
                
            //Check if description text in bank 1 (gametext568) and beyond original last line
            } else if ((item->textID & 0xF00) == 0x100 && item->textID > GAMETEXT_UI_B_03_Shiney_Nugget){
                item->textID = -1;
            }
        }
    }
}

/** Adding new text for inventory items that didn't have any Gametext string (originally by LaminGaming)
    Note that this patch depends on the extra lines LaminGaming appended to file Gametext_3
    (This gametext file has 107 lines by default, so any indices beyond that number won't render without the text patch)
*/
RECOMP_HOOK_DLL(cmdmenu_ctor) void cmdmenu_ctor_hook_item_edits_extra_text() {
    dPage6MagicSpells[INVENTORY_SPELL_2_GRENADE].textID = GAMETEXT_UI_15_Fire_Spell; //Change from "Randorn" to "Fire Spell" (unused text that was overwritten with "Grenade") (TODO: restore "Fire Spell" string and append a new string for "Grenade Spell"?)

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

/**
  * Fix a visual "pop" when scrolling through inventory items:
  *
  * The item icon strip would jump too far (especially when wrapping from top-to-bottom),
  * causing a visual judder and occasionally exposing an empty gap at the top of the icon strip.
  *
  * Rare seem to have accidentally used the tiles' width instead of height to calculate the jump size 
  * (possibly leftover from the horizontally-scrolling design) - changing it to the height fixes it!
  */
RECOMP_PATCH void cmdmenu_tick_inventory_page(void) {
    s16* pageSelectionIndex;
    Object* player;
    InventoryItem *pageItems;
    u32 pageMsg;
    s32 pageBtnMask;
    s32 usedGamebit;
    s8 isSidekickMenu;

    player = get_player();

    isSidekickMenu = FALSE;

    //Do nothing if the player's unloaded
    if (player == NULL) {
        return;
    }

    //Lock/unlock accessing the C-button scroll menu
    if (player->unkB0 & 0x1000) {
        joy_set_button_mask(0, L_CBUTTONS | R_CBUTTONS | D_CBUTTONS);
    } else if (sJoyButtonMask != 0) {
        joy_set_button_mask(0, sJoyButtonMask);
    }

    //Get button presses (or simulated ones, for tutorial sequences)
    if (sShouldOverrideJoypadButtons) {
        sJoyPressedButtons = sJoyPressedButtonsOverride;
    } else {
        sJoyPressedButtons = joy_get_pressed(0);
        if ((player->unkB0 & 0x1000) || (sJoyButtonMask != 0)) {
            sJoyPressedButtons |= B_BUTTON;
        }
    }

    //Play item use sound if needed
    switch (sUsedItemSoundType) {
    case CMDMENU_SOUND_NONE:
        break;
    case CMDMENU_SOUND_ITEM:
        gDLL_6_AMSFX->vtbl->play_sound(NULL, SOUND_79C_Cmdmenu_CantUse, MAX_VOLUME, NULL, NULL, 0, NULL);
        break;
    case CMDMENU_SOUND_PAGE:
        gDLL_6_AMSFX->vtbl->play_sound(NULL, SOUND_814_Cmdmenu_OpenSubMenu, MAX_VOLUME, NULL, NULL, 0, NULL);
        break;
    }

    sUsedItemGamebitID = NO_GAMEBIT;
    sUsedItemSoundType = CMDMENU_SOUND_NONE;
    sUsedItemPageID = NO_PAGE;

    pageItems = dCmdmenuPages[sInventoryPageID].items;
    pageSelectionIndex = &dCmdmenuPages[sInventoryPageID].selectedIndex;
    pageMsg = dCmdmenuPages[sInventoryPageID].mesgID;
    pageBtnMask = dCmdmenuPages[sInventoryPageID].btnMask;

    //Check if the current page is a sidekick command page
    if ((sInventoryPageID == CMDMENU_PAGE_7_Sidekick_Kyte) || 
        (sInventoryPageID == CMDMENU_PAGE_8_Sidekick_Tricky)
    ) {
        isSidekickMenu = TRUE;
    }

    sMenuSelectedItemIdx = *pageSelectionIndex;
    sDisplayedItemCount = cmdmenu_page_load_items(pageItems, isSidekickMenu);

    //If the current page has no visible items, close the inventory
    if (sDisplayedItemCount == 0) {
        dPageCategory = 0;
        cmdmenu_close_inventory();
        return;
    }

    if (sMenuSelectedItemIdx >= sDisplayedItemCount) {
        sMenuSelectedItemIdx = 0;
    }

    dSelectedItemTextID = sMenuItemTextIDs[sMenuSelectedItemIdx];

    //Handle scrolling through the inventory
    if (cmdmenu_is_inventory_open()) {
        //If the page's scroll button was pressed
        if ((sJoyPressedButtons & pageBtnMask) && (sInventoryScrollOffset < 8) && (dInventoryIsScrolling == FALSE)) {
            gDLL_6_AMSFX->vtbl->play_sound(NULL, SOUND_28A_Cmdmenu_MoveSelection, MAX_VOLUME, NULL, NULL, 0, NULL);
            dInventoryMovesQueued++;

            if (sInventoryScrollOffset != 0) {
                dInventoryIsScrolling = TRUE;
            }
        }

        //Limit the number of moves
        if (dInventoryMovesQueued > 255) {
            dInventoryMovesQueued = 255;
        }

        //Auto-select a specific item (unused feature)
        if (sAutoSelectItemIdx != NO_ITEM) {
            sMenuSelectedItemIdx = sAutoSelectItemIdx;
        }

        //Move between items
        if ((dInventoryMovesQueued > 0) && (sInventoryScrollOffset == 0)) {
            dInventoryMovesQueued--;

            if (sDisplayedItemCount > 1) {
                //When wrapping to top, skip over a gap in the inventory in specific cases (2 or 4 items)
                if (((sDisplayedItemCount == 2) && (sMenuSelectedItemIdx == 1)) || 
                    ((sDisplayedItemCount == 4) && (sMenuSelectedItemIdx == 3))
                ) {
                    //@bug: causes visual pop: should be MENU_ITEM_HEIGHT
                    // sInventoryScrollOffset = 2 * MENU_ITEM_WIDTH;
                    sInventoryScrollOffset = 2 * MENU_ITEM_HEIGHT; //@recomp: fix
                    dInventoryMoveSpeed = 2;
                    dInventoryIsScrolling = FALSE;
                } else {
                    //@bug: causes visual pop: should be MENU_ITEM_HEIGHT
                    // sInventoryScrollOffset = MENU_ITEM_WIDTH;
                    sInventoryScrollOffset = MENU_ITEM_HEIGHT; //@recomp: fix
                    dInventoryMoveSpeed = 2;
                    dInventoryIsScrolling = FALSE;
                }

                //Select next item
                sMenuSelectedItemIdx++;

                //Wrap selection back to top of item list
                if (sMenuSelectedItemIdx >= sDisplayedItemCount) {
                    sMenuSelectedItemIdx = 0;
                }
            }

        //Close the inventory with the B button
        } else if (sJoyPressedButtons & B_BUTTON) {
            gDLL_6_AMSFX->vtbl->play_sound(NULL, SOUND_28C_Cmdmenu_Close, MAX_VOLUME, NULL, NULL, 0, NULL);
            cmdmenu_close_inventory();

        //Use an inventory item with the A button
        } else if ((sJoyPressedButtons & A_BUTTON) && cmdmenu_is_inventory_open()) {
            usedGamebit = sMenuItemGamebits[sMenuSelectedItemIdx];
            
            //Items/Spells
            if (isSidekickMenu == FALSE) {
                obj_send_mesg(player, pageMsg, NULL, (void*)usedGamebit);
                
                sUsedItemGamebitID = usedGamebit;
                sUsedItemSoundType = sMenuItemUseSounds[sMenuSelectedItemIdx];
                sUsedItemPageID = sMenuItemOpenPageIDs[sMenuSelectedItemIdx];
                gDLL_6_AMSFX->vtbl->play_sound(NULL, SOUND_28B_Cmdmenu_Use, MAX_VOLUME, NULL, NULL, 0, NULL);
                cmdmenu_close_inventory();
                
            //Sidekick commands
            } else {
                if (sMenuItemVisibilities[sMenuSelectedItemIdx]) {
                    gDLL_6_AMSFX->vtbl->play_sound(NULL, SOUND_28B_Cmdmenu_Use, MAX_VOLUME, NULL, NULL, 0, NULL);
                    cmdmenu_close_inventory();
                    sUsedItemGamebitID = usedGamebit;
                    sUsedItemSoundType = CMDMENU_SOUND_NONE;
                } else {
                    /*  If the item's gamebitHidden is set, play a refusal sound and keep the menu open.
                        
                        This feature is unused in practice, because loaded Sidekick Commands 
                        always have their sMenuItemVisibilities set to TRUE.
                        
                        Possibly intended for Spells instead, since they're the only kind of 
                        inventory item that can have their sMenuItemVisibilities set to FALSE?
                    */
                    gDLL_6_AMSFX->vtbl->play_sound(NULL, SOUND_A0_Cmdmenu_Item_Locked, MAX_VOLUME, NULL, NULL, 0, NULL);
                    sUsedItemGamebitID = NO_GAMEBIT;
                    sUsedItemSoundType = CMDMENU_SOUND_NONE;
                }
            }
        }
    }

    if (cmdmenu_is_inventory_closed()) {
        dPageCategory = 0;
        sInventoryFrameCounter = 0;
        dInventoryMovesQueued = 0;
    } else {
        joy_set_button_mask(0, A_BUTTON | B_BUTTON);
    }

    *pageSelectionIndex = sMenuSelectedItemIdx;
}

/**
  * Let LockIcon appear over low-opacity Objects; allows CCFirecrystal to be collected. 
  * (Original patch by MusicalProgrammer)
  */
RECOMP_PATCH s32 cmdmenu_get_target_objects(Object **targetObjects, s32 maxObjects, u8 lockFlag, s32 arg3, f32 range) {
    s32 _pad1[2];
    f32 dx;
    f32 dz;
    f32 dy;
    Camera* camera;
    f32 objX;
    f32 objY;
    f32 objZ;
    Object* obj;
    Object** objects;
    s32 count;
    s32 index;
    s32 _pad2[2];
    s32 isSorted;
    s32 i;
    s32 targetCount;
    s32 yaw;

    set_camera_selector(0);
    camera = get_main_camera();
    objects = get_world_objects(&index, &count);

    //Get the subset of Objects that can be targetted
    for (targetCount = 0, i = index; i < count; i++) {
        obj = objects[i];

        if ((obj->def->lockdata != NULL) && 
            /*(obj->opacity == OBJECT_OPACITY_MAX)*/ (obj->opacity >= 32) && //@recomp: opacity condition changed
            ((obj->unkAF & ARROW_FLAG_8_No_Targetting) == FALSE) && 
            (obj->def->lockdata->flags & lockFlag) && 
            (targetCount < maxObjects) && 
            (arg3 & 1)
        ) {
            get_object_child_position(obj, &objX, &objY, &objZ);
            dx = objX - camera->srt.transl.x;
            dy = objY - camera->srt.transl.y;
            dz = objZ - camera->srt.transl.z;
            if ((SQ(dx) + SQ(dy) + SQ(dz)) < SQ(range)) {
                yaw = camera->srt.yaw - (u16)(M_90_DEGREES - arctan2_f(dx, dz));
                CIRCLE_WRAP(yaw);
                if (yaw < -10000 && yaw > -22000) {
                    targetObjects[targetCount++] = obj;
                }
            }
        }
    }

    //Sort the targettable Objects by address
    if (targetCount > 0) {
        do {
            isSorted = TRUE;
            for (i = 0; i < (targetCount - 1); i++) {
                if ((s32)targetObjects[i] < (s32)targetObjects[i + 1]) {
                    obj = targetObjects[i];
                    targetObjects[i] = targetObjects[i + 1];
                    targetObjects[i + 1] = obj;
                    isSorted = FALSE;
                }
            }
        } while (isSorted == FALSE);
    }
    
    return targetCount;
}

static s8 sInfoPopupUnroll = 0;

/**
  * Fix issues when repeatedly collecting the same item: 
  * just reset timer and show count increasing, instead of repeatedly fading in from 0 opacity. 
  */
RECOMP_PATCH void cmdmenu_info_show(s16 itemGamebit, s32 displayDuration, s32 itemCount) {
    InventoryItem *items;
    CmdmenuPage *inventoryPage;
    Texture* tex = NULL;

    inventoryPage = dCmdmenuPages;
    STUBBED_PRINTF("qInfoShow\n");
    // sInfoPopup.texture = NULL;

    //Find the item's TEXTABLE textureID, using the item's collection gamebitID
    while (inventoryPage->items != NULL) {
        items = inventoryPage->items;
        while (items->gamebitObtained != NO_GAMEBIT) {
            if (itemGamebit == items->gamebitObtained) {
                tex = tex_load_deferred(items->textureID);
                break;
            }
            items++;
        }
        inventoryPage++;
    }

    //@recomp: if the texture is the same as the one the pop-up is already showing, just keep it visible
    if (recomp_get_config_u32("cmdmenu_info_popup_fix") &&
        (sInfoPopup.texture == tex) && (sInfoPopup.timer > 0)
    ) {
        sInfoPopup.timer = displayDuration;
        sInfoPopup.count = itemCount;
    } else {
        //Assign the rest of the info box's parameters, and set it up to fade in
        sInfoPopup.texture = tex;
        sInfoPopup.timer = displayDuration;
        sInfoPopup.count = itemCount;
        sInfoPopup.opacity = 0.0f;
        sInfoPopupUnroll = 0; //@recomp: close up scroll
    }
}

#define POPUP_FIX_X 20
#define POPUP_FIX_Y 182

#define POPUP_FIX_TOP_X (POPUP_FIX_X + 0)
#define POPUP_FIX_TOP_Y (POPUP_FIX_Y + -1)

#define POPUP_FIX_BOTTOM_X (POPUP_FIX_X + 0)
#define POPUP_FIX_BOTTOM_Y (POPUP_FIX_Y + 31)

#define POPUP_FIX_ICON_X (POPUP_FIX_X + 4)
#define POPUP_FIX_ICON_Y (POPUP_FIX_Y + 7)

#define POPUP_FIX_ICON_CENTRE_Y (POPUP_FIX_ICON_Y + 12)

#define POPUP_FIX_COUNT_X (POPUP_FIX_X + 18)
#define POPUP_FIX_COUNT_Y (POPUP_FIX_Y + 16)

static void cmdmenu_gfx_set_info_popup_scissor(Gfx **gdl) {
    gDPSetScissor((*gdl)++, G_SC_NON_INTERLACE, 
        POPUP_FIX_ICON_X, 
        POPUP_FIX_ICON_CENTRE_Y - sInfoPopupUnroll, 
        POPUP_FIX_ICON_X + MENU_ITEM_WIDTH, 
        POPUP_FIX_ICON_CENTRE_Y + sInfoPopupUnroll
    );
}

/**
  * An edited version of the info pop-up's draw function, using the December 2000 inventory design.
  */
void cmdmenu_info_draw_custom(Gfx** gdl, CmdmenuInfoPopup* box) {
    //Do nothing after item's timer finished
    if (box->timer < 0) {
        return;
    }

    //@recomp: do nothing if the minimap is still visible
    if (minimap_get_opacity() > 0) {
        return;
    }

    //Decrement box's timer
    box->timer -= gUpdateRate;
    if (box->timer < 0) {
        tex_free(box->texture);
        box->texture = NULL;
        return;
    }

    //Update box's opacity
    if (box->timer < 30.0f) {
        //Fade out box during last half-second on-screen
        box->opacity = (box->timer * MAX_OPACITY_F) / 30.0f;

        //@recomp: roll up scroll
        sInfoPopupUnroll -= gUpdateRate;
        if (sInfoPopupUnroll < 0) {
            sInfoPopupUnroll = 0;
        }
    } else {
        //Otherwise, fade the box in if it's not fully visible
        if (box->opacity != MAX_OPACITY_F) {
            box->opacity += 8.5f * (f32) gUpdateRate;
            if (box->opacity > MAX_OPACITY_F) {
                box->opacity = MAX_OPACITY_F;
            }
        }

        //@recomp: unroll scroll
        if (box->opacity >= 64.0f) {
            sInfoPopupUnroll += gUpdateRate;
            if (sInfoPopupUnroll > MENU_ITEM_HEIGHT/2) {
                sInfoPopupUnroll = MENU_ITEM_HEIGHT/2;
            }
        }
    }

    //@recomp: draw the top of the scroll
    rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_02_Scroll_Top], 
        POPUP_FIX_TOP_X,
        POPUP_FIX_TOP_Y + 12 - sInfoPopupUnroll,
        0xFF, 
        0xFF, 
        0xFF, 
        box->opacity
    );        

    //@recomp: draw the bottom of the scroll
    rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_01_Scroll_Bottom], 
        POPUP_FIX_BOTTOM_X,
        POPUP_FIX_BOTTOM_Y - 12 + sInfoPopupUnroll,
        0xFF, 
        0xFF, 
        0xFF, 
        box->opacity
    );        

    //@recomp: set scissor for inner scroll
    cmdmenu_gfx_set_info_popup_scissor(gdl);

    //Draw the item's inventory icon (including embedded page background)
    bzero(sTempIcon, sizeof(TextureTile));
    sTempIcon->tex = box->texture;
    sTempIcon[1].tex = NULL;
    rcp_tile_write(gdl, sTempIcon, 
        POPUP_FIX_ICON_X, //@recomp: different coord
        POPUP_FIX_ICON_Y, //@recomp: different coord 
        0xFF, 
        0xFF, 
        0xFF, 
        box->opacity
    );

    //Draw item count (NOTE: can't draw values under 2 or over 10, because no icons provided)
    if (box->count > 1) {
        sTempIcon->tex = sInventoryStackNumbersTex;
        sTempIcon->animProgress = (box->count - 2) << 8;
        rcp_tile_write(gdl, sTempIcon, 
            POPUP_FIX_COUNT_X, //@recomp: different coord
            POPUP_FIX_COUNT_Y, //@recomp: different coord
            0xFF, 
            0xFF, 
            0xFF, 
            box->opacity
        );
    }

    //@recomp: restore full-screen scissor
    cmdmenu_gfx_set_screen_scissor(gdl);
}

/**
  * Fix up the item info pop-up to better match December 2000's inventory design.
  *
  * By default it looked quite broken, since it seemed to expect the older/taller item
  * icons from the horizontally scrolling version of the inventory seen in early footage.
  */
RECOMP_PATCH void cmdmenu_info_draw(Gfx** gdl, CmdmenuInfoPopup* box) {
    //@recomp: use the fixed-up info pop-up instead, if wanted
    if (recomp_get_config_u32("cmdmenu_info_popup_fix")) {
        return cmdmenu_info_draw_custom(gdl, box);
    }

    //Do nothing after item's timer finished
    if (box->timer < 0) {
        return;
    }

    //Decrement box's timer
    box->timer -= gUpdateRate;
    if (box->timer < 0) {
        tex_free(box->texture);
        box->texture = NULL;
        return;
    }

    //Update box's opacity
    if (box->timer < 30.0f) {
        //Fade out box during last half-second on-screen
        box->opacity = (box->timer * MAX_OPACITY_F) / 30.0f;
    } else {
        //Otherwise, fade the box in if it's not fully visible
        if (box->opacity != MAX_OPACITY_F) {
            box->opacity += 8.5f * (f32) gUpdateRate;
            if (box->opacity > MAX_OPACITY_F) {
                box->opacity = MAX_OPACITY_F;
            }
        }
    }

    //Draw the box's shadow
    rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_52_Page_Torn_Shadow], 
        INFO_POPUP_SHADOW_X, 
        INFO_POPUP_SHADOW_Y, 
        0xFF, 
        0xFF, 
        0xFF, 
        box->opacity
    );

    //Draw the item's inventory icon (including embedded page background)
    bzero(sTempIcon, sizeof(TextureTile));
    sTempIcon->tex = box->texture;
    sTempIcon[1].tex = NULL;
    rcp_tile_write(gdl, sTempIcon, 
        INFO_POPUP_M_X, 
        INFO_POPUP_Y, 
        0xFF, 
        0xFF, 
        0xFF, 
        box->opacity
    );

    //Draw tattered left edge of box (@unfinished: outdated page design, mismatched with icons' newer BG)
    rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_29_Page_Torn_Left], 
        INFO_POPUP_L_X, 
        INFO_POPUP_Y, 
        0xFF, 
        0xFF, 
        0xFF, 
        box->opacity
    );

    //Draw tattered right edge of box (@unfinished: outdated page design, mismatched with icons' newer BG)
    rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_30_Page_Torn_Right], 
        INFO_POPUP_R_X,
        INFO_POPUP_Y, 
        0xFF, 
        0xFF, 
        0xFF, 
        box->opacity
    );

    //Draw item count (NOTE: can't draw values under 2 or over 10, because no icons provided)
    if (box->count > 1) {
        sTempIcon->tex = sInventoryStackNumbersTex;
        sTempIcon->animProgress = (box->count - 2) << 8;
        rcp_tile_write(gdl, sTempIcon, 
            INFO_POPUP_QUANTITY_X, 
            INFO_POPUP_QUANTITY_Y, 
            0xFF, 
            0xFF, 
            0xFF, 
            box->opacity
        );
    }
}

/** 
  * Custom pseudo-export, so the minimap can check if the cmdmenu's item info pop-up should appear.
  * (To avoid visual clash between minimap and item info pop-up). 
  */
s16 cmdmenu_info_popup_is_visible() {
    return (sInfoPopup.timer > 0);
}
