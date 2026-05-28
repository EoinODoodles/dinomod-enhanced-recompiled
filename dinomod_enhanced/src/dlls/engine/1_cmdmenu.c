#include "configs.h"
#include "custom_textable_ids.h"
#include "dll_util.h"
#include "math_util.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "rt64_extended_gbi.h"

#include "PR/os.h"
#include "common.h"
#include "gbi_extra.h"
#include "game/gamebits.h"
#include "game/gametexts.h"
#include "game/gametexts_ui.h"
#include "game/objects/interaction_arrow.h"
#include "game/objects/inventory_items.h"
#include "game/objects/object_id.h"
#include "sys/camera.h"
#include "sys/fonts.h"
#include "sys/gfx/model.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objmsg.h"
#include "sys/print.h"
#include "dll.h"
#include "dlls/engine/29_gplay.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/engine/1_cmdmenu.h"
#include "dlls/objects/common/sidekick.h"
#include "dlls/objects/210_player.h"

#include "recomp/dlls/engine/1_cmdmenu_recomp.h"
#include "recomp/dlls/engine/2_camcontrol_recomp.h"

#include "engine/1_cmdmenu.h"
#include "engine/59_minimap.h"

extern s32 D_8008C890;

/* RECOMP VARS */

static f32 rsLerpActiveSpell = 0;               //Lerp tValue for (optionally) moving the Active Spell icon
static f32 rsLerpActiveSideCommand = 0;         //Lerp tValue for (optionally) moving the Active Sidekick Command icon
static s16 rsOpacityActiveSpell = 0;            //Opacity value for (optionally) fading in/out the Active Spell icon
static s16 rsOpacityActiveSideCommand = 0;      //Opacity value for (optionally) fading in/out the Active Sidekick Command icon
static u8 rsShowActiveSpell = FALSE;            //Whether to show/hide the Active Spell icon (used when optionally fading)
static u8 rsShowActiveSideCommand = FALSE;      //Whether to show/hide the Active Spell icon (used when optionally fading)

/* RECOMP CMDMENU MACROS */

#define DEBUG_INVENTORY_SCROLLING FALSE
#define DEBUG_SIDEKICK_METER FALSE

#define INVENTORY_LERP_SPEED (1.0f/((f32)(MENU_HEIGHT_OPEN >> 2)))

//Fixes a bug where the inventory strip doesn't draw the item/command icons in the top row of pixels (noticeable when scrolling)
#define FIX_ICON_STRIP_PIXEL_ROW_ONE

/* DECOMP CMDMENU MACROS */

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

typedef enum {
    CIcon_FLAG_None = 0,
    CIcon_FLAG_Have_Spells = 1,
    CIcon_FLAG_Have_Items = 2,
    CIcon_FLAG_Have_Sidekick_Commands = 4
} CmdMenuCIconFlags;

typedef enum {
    PlayerStats_FLAG_None = 0,
    PlayerStats_FLAG_Update_Snapshot = 1
} CmdMenuPlayerStatsFlags;

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
extern s8 sJoyDisabledButtons;
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
extern s16 cmdmenu_get_spell_textable(s32 spellGamebit);
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

static void cmdmenu_kiosk_icons_update(void);

/**
  * Fixes a bug where the info scroll (the box that appears when holding R in the inventory, etc.)
  * ended up accidentally drawing during the first few frames of gameplay due to an incorrectly initialised variable.
  * The bug caused a beige smear to appear in the top-left corner of the screen's framebuffer.
  * This fix is more relevant for N64/emulators than recomp.
  */
RECOMP_HOOK_DLL(cmdmenu_ctor) void cmdmenu_fix_game_start_framebuffer_smear() {
    sInfoScrollOverrideTextID = NO_GAMETEXT;
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
    dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_20_GOLD_NUGGET_2_LFV].gamebitHide = BIT_CC_Bribed_GuardClaw; //fix issue where it used the "Rescued Kyte" gamebit instead of "Bribed GuardClaw", unlike the other Shiny Nuggets
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
    dPage11FoodItemsKyte[3].textureID = TEXTABLE_1A5;   //use Blue Mushroom icon instead of Old Apple icon (TODO: update index with enum)
    dPage12FoodItemsTricky[3].textureID = TEXTABLE_1A5; //use Blue Mushroom icon instead of Old Apple icon (TODO: update index with enum)
    
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

    //Icon patches
    cmdmenu_kiosk_icons_update();
}

/** 
  * Handles adding/removing custom icons for DIM's Gold/Silver keys, sourced from SFA Kiosk leftovers. 
  */
static void kiosk_icons_gold_silver_keys() {
    static s8 rsKioskIconsStateKeys = -1;
    u8 enabled = recomp_get_config_u32("cmdmenu_icons_gold_silver_keys");

    //Check if the setting changed
    if (rsKioskIconsStateKeys != enabled) {
        rsKioskIconsStateKeys = enabled;
    } else {
        return;
    }

    //Toggle on/off
    if (enabled) {
        dPage1ItemsSabre[INVENTORY_ITEM_SABRE_11_DIM_BELINA_TE_CELL_KEY].textureID = TEXTABLE_25C;
        dPage1ItemsSabre[INVENTORY_ITEM_SABRE_12_DIM_TRICKY_CELL_KEY].textureID = TEXTABLE_25D;
    } else {
        dPage1ItemsSabre[INVENTORY_ITEM_SABRE_11_DIM_BELINA_TE_CELL_KEY].textureID = TEXTABLE_175;
        dPage1ItemsSabre[INVENTORY_ITEM_SABRE_12_DIM_TRICKY_CELL_KEY].textureID = TEXTABLE_175;
    }
}

/** 
  * Handles adding/removing a custom icon for the Firefly Lantern, sourced from SFA Kiosk leftovers. 
  */
static void kiosk_icons_firefly() {
    static s8 rsKioskIconsStateFirefly = -1;
    u8 enabled = recomp_get_config_u32("cmdmenu_icons_firefly");

    //Check if the setting changed
    if (rsKioskIconsStateFirefly != enabled) {
        rsKioskIconsStateFirefly = enabled;
    } else {
        return;
    }

    //Toggle on/off
    if (enabled) {
        dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_30_FIREFLY_LANTERN].textureID = TEXTABLE_25E;
        dPage1ItemsSabre[INVENTORY_ITEM_SABRE_24_FIREFLY_LANTERN].textureID = TEXTABLE_25E;
    } else {
        dPage0ItemsKrystal[INVENTORY_ITEM_KRYSTAL_30_FIREFLY_LANTERN].textureID = TEXTABLE_46D;
        dPage1ItemsSabre[INVENTORY_ITEM_SABRE_24_FIREFLY_LANTERN].textureID = TEXTABLE_46D;
    }
}

/** 
  * Handles adding/removing custom icons for the Energy Eggs, replacing the icons Rare had yet to update. 
  */
static void custom_icons_energy_eggs() {
    static s8 rsKioskIconsStateEnergyEggs = -1;
    u8 enabled = recomp_get_config_u32("cmdmenu_icons_energy_eggs");

    //Check if the setting changed
    if (rsKioskIconsStateEnergyEggs != enabled) {
        rsKioskIconsStateEnergyEggs = enabled;
    } else {
        return;
    }

    //Toggle on/off
    if (enabled) {
        dPage4FoodItemsKrystal[INVENTORY_FOOD_5_Dino_Egg].textureID     = TEXTABLE_260_Energy_Egg_Icon;
        dPage4FoodItemsKrystal[INVENTORY_FOOD_6_Moldy_Meat].textureID   = TEXTABLE_261_Energy_Egg_Moldy_Icon;
        dPage5FoodItemsSabre[INVENTORY_FOOD_5_Dino_Egg].textureID       = TEXTABLE_260_Energy_Egg_Icon;
        dPage5FoodItemsSabre[INVENTORY_FOOD_6_Moldy_Meat].textureID     = TEXTABLE_261_Energy_Egg_Moldy_Icon;
    } else {
        dPage4FoodItemsKrystal[INVENTORY_FOOD_5_Dino_Egg].textureID     = TEXTABLE_2C4;
        dPage4FoodItemsKrystal[INVENTORY_FOOD_6_Moldy_Meat].textureID   = TEXTABLE_2C5;
        dPage5FoodItemsSabre[INVENTORY_FOOD_5_Dino_Egg].textureID       = TEXTABLE_2C4;
        dPage5FoodItemsSabre[INVENTORY_FOOD_6_Moldy_Meat].textureID     = TEXTABLE_2C5;
    }
}

/** Updates configurable Cmdmenu items */
static void cmdmenu_kiosk_icons_update(void) {
    kiosk_icons_gold_silver_keys();
    kiosk_icons_firefly();
    custom_icons_energy_eggs();
}

RECOMP_CALLBACK("*", recomp_on_game_tick_start) void cmdmenu_callback_kiosk_icons_update() {
    cmdmenu_kiosk_icons_update();
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
RECOMP_CALLBACK("*", recomp_on_game_tick_start) void update_extra_text_inventory() {
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

typedef enum {
    Illusion_Icon_Sabre,
    Illusion_Icon_Fox
} SabreIllusionSpellIcons;

#define ILLUSION_ICON_SWAP_SPEED 16

/**
  * - Duplicates Shinx's base recomp widescreen HUD patches, allowing more patches to be added on top.
  * - Fixes framerate dependent behaviour in the Scarab counter.
  * - Hides the Scarab counter while the Active Spell/Sidekick Command icons are overlapping it.
  * - Option to show a Fox portrait icon while Sabre's using Illusion Spell (sourced from unused Kiosk files)
  */
static void cmdmenu_draw_player_stats_custom(Gfx** gdl, Mtx** mtxs, Vertex** vtxs) {
    f32 goalOpacity;
    u32 pad;
    u8 i;
    u8 statsOpacity;
    s8 offsetX;
    s8 offsetY;
    s32 temp;
    Gfx* dl;
    Object* player = get_player();
    u8 texIdx;
    static char playerScarabCountText[4] = "   ";
    /* RECOMP */
    u8 rConfigActiveIconOverlapFix = recomp_get_config_u32("cmdmenu_active_icon_overlap");

    dl = *gdl;
    temp = vi_get_current_size();
    
    gDPSetScissor(dl++, G_SC_NON_INTERLACE, 0, 0, (u16)GET_VIDEO_WIDTH(temp) - 1, SCREEN_HEIGHT - 1);

    // @recomp: Align stats to left
    #ifndef DINOMOD_ROM_PATCH
    gEXSetViewportAlign(dl++, G_EX_ORIGIN_LEFT, 0, 0);
    gEXSetRectAlign(dl++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, 0, 0, 0, 0);
    #endif
    
    //Draw player health
    {
        if ((sStatsChangeTimers.playerHealth >= 0.0f) || 
            (sStatsChangeTimers.playerHealthMax >= 0.0f) || 
            (sStatsChangeTimers.unk14 >= 0.0f) //Shows health when pressing R
        ) {
            goalOpacity = MAX_OPACITY_F;
        } else {
            goalOpacity = 0.0f;
        }

        if (sOpacityHealth < goalOpacity) {
            sOpacityHealth += 8.5f * gUpdateRateF;
            if (sOpacityHealth > MAX_OPACITY_F) {
                sOpacityHealth = MAX_OPACITY_F;
            }
        } else if (goalOpacity < sOpacityHealth) {
            sOpacityHealth -= 8.5f * gUpdateRateF;
            if (sOpacityHealth < 0.0f) {
                sOpacityHealth = 0.0f;
            }
        }
        
        statsOpacity = sOpacityHealth;
        if (statsOpacity) {
            for (i = 0; i < (sStats.playerHealthMax >> 2); i++) {
                s32 appleCount;

                //Get apple coords (3 rows)
                if (i >= APPLES_ROW_3_IDX) {
                    //Row 3
                    offsetX = (i * APPLES_SPACING_X) + (APPLES_ROW_3_OFFSET_X - (APPLES_ROW_3_IDX * APPLES_SPACING_X));
                    offsetY = 2 * APPLES_SPACING_Y;
                } else if (i >= APPLES_ROW_2_IDX) {
                    //Row 2
                    offsetX = (i * APPLES_SPACING_X) + (APPLES_ROW_2_OFFSET_X - (APPLES_ROW_2_IDX * APPLES_SPACING_Y));
                    offsetY = APPLES_SPACING_Y;
                } else {
                    //Row 1
                    offsetX = (i * APPLES_SPACING_X) + (APPLES_ROW_1_OFFSET_X);
                    offsetY = 0;
                }

                //Pick which texture to use for this apple
                appleCount = sStats.playerHealth >> 2;
                if (i < appleCount) {
                    //Full apple
                    texIdx = CMDMENU_TEX_17_Apple_100_Pct;
                } else if (appleCount < i) {
                    //Empty apple
                    texIdx = CMDMENU_TEX_08_Apple_0_Pct;
                } else {
                    //Portion of an apple
                    texIdx = CMDMENU_TEX_08_Apple_0_Pct + (sStats.playerHealth & 3);
                }
                
                rcp_tile_write(
                    &dl,
                    sTextureTiles[texIdx], 
                    HEALTH_ICONS_X + offsetX, 
                    HEALTH_ICONS_Y + offsetY,
                    0xFF,0xFF, 0xFF, statsOpacity
                );
            }
        }
    }

    //Draw player magic
    {
        //Get opacity goal
        if ((sStatsChangeTimers.playerMagic >= 0.0f) || 
            (sStatsChangeTimers.unk14 >= 0.0f) || 
            (((DLL_210_Player*)player->dll)->vtbl->func50(player) != -1)
        ) {
            goalOpacity = MAX_OPACITY_F;
        } else {
            goalOpacity = 0.0f;
        }

        //Animate magic bar's opacity towards goal opacity
        if (sOpacityMagic < goalOpacity) {
            sOpacityMagic += 8.5f * gUpdateRateF;
            if (sOpacityMagic > MAX_OPACITY_F) {
                sOpacityMagic = MAX_OPACITY_F;
            }
        } else if (goalOpacity < sOpacityMagic) {
            sOpacityMagic -= 8.5f * gUpdateRateF;
            if (sOpacityMagic < 0.0f) {
                sOpacityMagic = 0.0f;
            }
        }

        //Draw magic bar(s)
        statsOpacity = sOpacityMagic;
        if (statsOpacity) {
            //Draw a magic bar for every 25 units of the player's max magic (just 1 bar initially)
            for (i = 0; i < (sStats.playerMagicMax / MAGIC_UNITS_PER_BAR); i++) {
                if (i < (sStats.playerMagic / MAGIC_UNITS_PER_BAR)) {
                    //Full bar
                    temp = MAGIC_BARS_WIDTH;
                } else if ((sStats.playerMagic / MAGIC_UNITS_PER_BAR) < i) {
                    //Empty bar
                    temp = 0;
                } else {
                    //Partial bar
                    temp = MAGIC_BARS_ZERO_POINT_X + ((sStats.playerMagic % MAGIC_UNITS_PER_BAR) * 2);
                }

                //Draw the filled part of the bar
                rcp_tile_write_x(
                    &dl,
                    sTextureTiles[CMDMENU_TEX_36_MagicBar_Full],
                    MAGIC_BARS_X,
                    MAGIC_BARS_Y + (i * MAGIC_BARS_SPACING_Y),
                    temp,
                    MAGIC_BARS_HEIGHT,
                    0, 0,
                    1.0f, 1.0f,
                    statsOpacity | ~0xFF,
                    TILE_WRITE_TRANSLUCENT | TILE_WRITE_POINT_FILT
                );

                //Draw the empty part of the bar
                rcp_tile_write_x(
                    &dl, sTextureTiles[CMDMENU_TEX_35_MagicBar_Empty],
                    MAGIC_BARS_X + temp,
                    MAGIC_BARS_Y + (i * MAGIC_BARS_SPACING_Y),
                    (MAGIC_BARS_WIDTH - temp), 
                    MAGIC_BARS_HEIGHT,
                    temp << 5, 0, 
                    1.0f, 1.0f, 
                    statsOpacity | ~0xFF, 
                    TILE_WRITE_TRANSLUCENT | TILE_WRITE_POINT_FILT
                );
            }
        }
    }

    //Draw character icon
    {
        _Bool rConfigFoxIcon = recomp_get_config_u32("cmdmenu_icons_fox");
        static s16 rsIllusionOpacity;
        static u8 rsIllusionIcon = 0;
        ModelInstance* modelInst;
        u8 showFoxIcon = FALSE;
        u8 opacity;

        //@recomp: Check if the Illusion Spell is showing the Fox model
        if (rConfigFoxIcon && (player->id == OBJ_Sabre)) {
            if ((player->modelInstIdx == 2) && 
                (modelInst = player->modelInsts[player->modelInstIdx]) && 
                ((modelInst->model->modelId == 9) || (modelInst->model->modelId == 10)) //Check if it's either Fox model
            ) {
                showFoxIcon = TRUE;
            } else {
                showFoxIcon = FALSE;
            }
        }

        statsOpacity = ((u8)sOpacityHealth < (u8)sOpacityMagic) ? (u8)sOpacityMagic : (u8)sOpacityHealth;

        //@recomp: if the character portrait's currently hidden and it needs to be swapped, swap it immediately off-screen
        if (rConfigFoxIcon && (statsOpacity < 20)) {
            rsIllusionOpacity = MAX_OPACITY;

            if (showFoxIcon && (rsIllusionIcon == Illusion_Icon_Sabre)) {
                rsIllusionIcon = Illusion_Icon_Fox;
            } else if (!showFoxIcon && (rsIllusionIcon == Illusion_Icon_Fox)) {
                rsIllusionIcon = Illusion_Icon_Sabre;
            }
        }

        if (statsOpacity) {
            if (player->id == OBJ_Krystal) {
                offsetX = 0;
                offsetY = 0;
                texIdx = CMDMENU_TEX_53_Krystal;
            } else {
                offsetX = 2;
                offsetY = -1;
                texIdx = CMDMENU_TEX_40_Sabre;
            }

            //@recomp: handle optional Sabre icon behaviour
            if (rConfigFoxIcon && (player->id == OBJ_Sabre)) {
                //Change to Fox icon when Sabre's using the Illusion Spell
                if (showFoxIcon) {
                    if (rsIllusionIcon == Illusion_Icon_Sabre) {
                        //Fade out Sabre icon
                        rsIllusionOpacity -= gUpdateRate * ILLUSION_ICON_SWAP_SPEED;
                        if (rsIllusionOpacity < 0) {
                            rsIllusionOpacity = 0;
                            rsIllusionIcon = Illusion_Icon_Fox;
                        }
                        dInventoryPageIcon = tex_load_deferred(dTextableIDs[texIdx]);
                    } else {
                        //Fade in Fox icon
                        if (rsIllusionOpacity < MAX_OPACITY) {
                            rsIllusionOpacity += gUpdateRate * ILLUSION_ICON_SWAP_SPEED;
                            if (rsIllusionOpacity > MAX_OPACITY) {
                                rsIllusionOpacity = MAX_OPACITY;
                            }
                        }
                        dInventoryPageIcon = tex_load_deferred(TEXTABLE_266_Kiosk_Fox_Icon);
                    }
                } else {
                    if (rsIllusionIcon == Illusion_Icon_Fox) {
                        //Fade out Fox icon
                        rsIllusionOpacity -= gUpdateRate * ILLUSION_ICON_SWAP_SPEED;
                        if (rsIllusionOpacity < 0) {
                            rsIllusionOpacity = 0;
                            rsIllusionIcon = Illusion_Icon_Sabre;
                        }
                        dInventoryPageIcon = tex_load_deferred(TEXTABLE_266_Kiosk_Fox_Icon);
                    } else {
                        //Fade in Sabre icon
                        if (rsIllusionOpacity < MAX_OPACITY) {
                            rsIllusionOpacity += gUpdateRate * ILLUSION_ICON_SWAP_SPEED;
                            if (rsIllusionOpacity > MAX_OPACITY) {
                                rsIllusionOpacity = MAX_OPACITY;
                            }
                        }
                        dInventoryPageIcon = tex_load_deferred(dTextableIDs[texIdx]);
                    }
                }

                //Compound opacity
                if (rsIllusionOpacity >= MAX_OPACITY) {
                    opacity = statsOpacity;
                } else if (rsIllusionOpacity <= 0) {
                    opacity = 0;
                } else {
                    opacity = statsOpacity * (((f32)rsIllusionOpacity)/MAX_OPACITY_F);
                }

                //Draw icon
                rcp_screen_full_write(
                    &dl,
                    dInventoryPageIcon,
                    CHARACTER_ICON_X + offsetX + (rsIllusionIcon == Illusion_Icon_Fox ? 1 : 0), //shift over by 1px when using Fox icon
                    CHARACTER_ICON_Y + offsetY,
                    0,
                    0,
                    opacity,
                    SCREEN_WRITE_TRANSLUCENT
                );

                tex_free(dInventoryPageIcon);
            } else {
                //Unmodified behaviour
                dInventoryPageIcon = tex_load_deferred(dTextableIDs[texIdx]);

                rcp_screen_full_write(
                    &dl,
                    dInventoryPageIcon,
                    CHARACTER_ICON_X + offsetX,
                    CHARACTER_ICON_Y + offsetY,
                    0,
                    0,
                    statsOpacity,
                    SCREEN_WRITE_TRANSLUCENT
                );

                //TODO: for console, it might be a good idea to keep the character icon loaded until it changes?
                tex_free(dInventoryPageIcon);
            }
        }
    }

    // @recomp: Align scarab counter to right
    #ifndef DINOMOD_ROM_PATCH
    gEXSetViewportAlign(dl++, G_EX_ORIGIN_RIGHT, -SCREEN_WIDTH * 4, 0);
    gEXSetRectAlign(dl++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, -SCREEN_WIDTH * 4, 0, -SCREEN_WIDTH * 4, 0);
    #endif

    //Draw Scarab counter
    {
        if ((sStatsChangeTimers.playerScarabCount >= 0.0f) || (sStatsChangeTimers.unk14 >= 0.0f)) {
            goalOpacity = MAX_OPACITY_F;
        } else {
            goalOpacity = 0.0f;
        }

        //@recomp: hide Scarab counter if it overlaps with the Active Spell / Sidekick Command icons
        switch (rConfigActiveIconOverlapFix) {
        case ACTIVEICON_DEFAULT:
        case ACTIVEICON_HIDE:
            if (rsOpacityActiveSpell) {
                goalOpacity = 0.0f;
            }
            break;
        case ACTIVEICON_MOVE:
            //Hide if the Sidekick Command icon is shifted down over the Scarab counter
            if (sActiveSidekickCommandIcon && (rsLerpActiveSideCommand > 0)) {
                goalOpacity = 0.0f;
                break;
            }

            //Hide if the Spell icon is over the Scarab counter
            if (sActiveSpellIcon && (rsLerpActiveSideCommand < 1)) {
                goalOpacity = 0.0f;
                break;
            }
            break;
        }

        if (sOpacityScarabs < goalOpacity) {
            sOpacityScarabs += 8.5f * gUpdateRateF;
            if (sOpacityScarabs > MAX_OPACITY_F) {
                sOpacityScarabs = MAX_OPACITY_F;
            }
        } else if (goalOpacity < sOpacityScarabs) {
            sOpacityScarabs -= 8.5f * gUpdateRateF;
            if (sOpacityScarabs < 0.0f) {
                sOpacityScarabs = 0.0f;
            }
        }

        statsOpacity = sOpacityScarabs;
        if (statsOpacity && main_get_bits(BIT_UI_Scarab_Counter_Enabled)) {
            sAnimFrameScarab = 0;
            if (statsOpacity == MAX_OPACITY) {
                if (sAnimScarabSpin) {
                    //Animate Scarab spinning (upon collecting Scarabs)
                    sAnimFrameScarab = 11 - (sAnimScarabSpin/2); //@recomp: play scarab spin anim on 2s

                    //@recomp: fix framerate dependency
                    if (sAnimScarabSpin >= gUpdateRate) {
                        sAnimScarabSpin -= gUpdateRate;
                    } else {
                        sAnimScarabSpin = 0;
                    }

                    if (sAnimScarabSpin == 0) {
                        sAnimScarabFlutterTimer = 80 * 2; //@recomp: doubled, assuming average gUpdateRate of 2 on N64
                    }
                } else {
                    //Animate Scarab fluttering wings
                    if (sAnimScarabFlutterTimer) {
                       //@recomp: fix framerate dependency
                        if (sAnimScarabFlutterTimer >= gUpdateRate) {
                            sAnimScarabFlutterTimer -= gUpdateRate;
                        } else {
                            sAnimScarabFlutterTimer = 0;
                        }

                        //Change the Scarab's frame during last few ticks of the timer 
                        if (sAnimScarabFlutterTimer < (6 * 2)) { //@recomp: doubled, for framerate dependency fix
                            sAnimFrameScarab = 3 - (sAnimScarabFlutterTimer / 4); //@recomp: playing flutter on 4s
                        }
                    } else {
                        sAnimScarabFlutterTimer = rand_next(20, 255);
                    }
                }
            }

            //@recomp: make sure Scarab frame offset is in range, just in case
            if (sAnimFrameScarab > 10) {
                sAnimFrameScarab = 0;
            }

            rcp_tile_write_x(
                &dl, 
                sTextureTiles[CMDMENU_TEX_18_Scarab + sAnimFrameScarab], 
                SCARABS_ICON_X, 
                SCARABS_ICON_Y, 
                SCARABS_ICON_WIDTH, 
                SCARABS_ICON_HEIGTH, 
                0, 0, 
                1.0f, 1.0f, 
                statsOpacity | ~0xFF, 
                TILE_WRITE_TRANSLUCENT | TILE_WRITE_POINT_FILT
            );

            sprintf(playerScarabCountText, "%d", (int)sStats.playerScarabCount);
            font_window_set_coords(3, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            font_window_use_font(3, FONT_DINO_SUBTITLE_FONT_1);
            font_window_set_bg_colour(3, 0, 0, 0, 0);
            font_window_flush_strings(3);
            font_window_set_text_colour(3, 0xFF, 0xFF, 0xFF, 0xFF, statsOpacity);
            font_window_add_string_xy(3, SCARABS_NUMBER_X, SCARABS_NUMBER_Y, playerScarabCountText, 1, ALIGN_TOP_LEFT);
            font_window_set_text_colour(3, 0x14, 0x14, 0x14, 0xFF, 0xFF);
            font_window_use_font(3, FONT_DINO_SUBTITLE_FONT_1);
            font_window_draw(&dl, mtxs, vtxs, 3);
        }
    }

    // @recomp: Reset alignment
    #ifndef DINOMOD_ROM_PATCH
    gEXSetRectAlign(dl++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
    gEXSetViewportAlign(dl++, G_EX_ORIGIN_NONE, 0, 0);
    #endif

    *gdl = dl;
}

// Fix Scarab spin anim's framerate dependency, and play on 2s
RECOMP_PATCH void cmdmenu_update_stats(void) {
    Object* player;
    Object* sidekick;
    CmdmenuPlayerSidekickData stats;
    f32 timer;
    u8 scarabFrameOffset;
    u8 i;

    player = get_player();
    sidekick = get_sidekick();
    scarabFrameOffset = 0;

    stats.playerHealth = ((DLL_210_Player*)player->dll)->vtbl->get_health(player);
    stats.playerHealthMax = ((DLL_210_Player*)player->dll)->vtbl->get_health_max(player);

    if (sidekick != NULL) {
        stats.sidekickBlueFood = ((DLL_ISidekick*)sidekick->dll)->vtbl->get_blue_food_count(sidekick);
        stats.sidekickRedFood = ((DLL_ISidekick*)sidekick->dll)->vtbl->get_red_food_count(sidekick);
        stats.sidekickMaxFood = 8;
    } else {
        stats.sidekickBlueFood = 0;
        stats.sidekickRedFood = 0;
        stats.sidekickMaxFood = 0;
    }

    stats.playerMagic = ((DLL_210_Player*)player->dll)->vtbl->get_magic(player);
    stats.playerMagicMax = ((DLL_210_Player*)player->dll)->vtbl->get_magic_max(player);
    stats.playerScarabCount = ((DLL_210_Player*)player->dll)->vtbl->get_scarabs(player);

    //Check if the player collected a Scarab
    if (((DLL_210_Player*)player->dll)->vtbl->get_scarabs_largest_recently_collected(player) != 0) {
        scarabFrameOffset = 7 * 2; //@recomp: doubled for playing Scarab anim on 2s when framerate independent
    }
    if (sAnimScarabSpin < scarabFrameOffset) {
        sAnimScarabSpin = scarabFrameOffset;
    }

    //Play sound when pressing R to show HUD
    if (sJoyPressedButtons & R_TRIG) {
        gDLL_6_AMSFX->vtbl->play(NULL, SOUND_5EA_Cmdmenu_ShowHUD, MAX_VOLUME, NULL, NULL, 0, NULL);
    }

    //Increment stat.unk14 when holding R, or when there's a target Object, or when stats are auto-shown
    //(Causes health/magic to fade in and stay on-screen a little while longer than C buttons)
    if ((sJoyHeldButtons & R_TRIG) || 
        (gDLL_2_Camera->vtbl->get_target_object() != NULL) || 
        (sForceStatsDisplay && (camera_get_letterbox() == 0))
    ) {
        stats.unk14 = sPrevStats.unk14 + 1;
    } else {
        stats.unk14 = sPrevStats.unk14;
    }

    //Fade in C buttons when holding R
    if (sJoyHeldButtons & R_TRIG) {
        sOpacityR += 8.5f * gUpdateRateF;
        if (sOpacityR > MAX_OPACITY_F) {
            sOpacityR = MAX_OPACITY_F;
        }
    } else {
        sOpacityR -= 8.5f * gUpdateRateF;
        if (sOpacityR < 0.0f) {
            sOpacityR = 0.0f;
        }
    }

    //Right side's opacity shouldn't exceed health UI's opacity
    sOpacityR = (sOpacityR < sOpacityHealth) ? sOpacityR : sOpacityHealth;

    stats.unk18 = 0;

    //Update the player stats snapshot when requested
    if (sPlayerStatsFlags & PlayerStats_FLAG_Update_Snapshot) {
        sPlayerStatsFlags &= ~PlayerStats_FLAG_Update_Snapshot;
        for (i = 0; i < CMDMENU_TRACKED_PLAYER_STATS_COUNT; i++) {
            sStats.items[i] = stats.items[i];
            sPrevStats.items[i] = stats.items[i];
            sStatsChangeTimers.items[i] = -30.0f;
        }
        sOpacityHealth = 0.0f;
        return;
    }

    //Compare the current player stats and the stored player stats
    for (i = 0; i < CMDMENU_TRACKED_PLAYER_STATS_COUNT; i++) {
        //Decrease the stat's timer
        timer = sStatsChangeTimers.items[i];
        sStatsChangeTimers.items[i] = timer - gUpdateRateF;

        if (timer > 60.0f) {
            if (sStatsChangeTimers.items[i] <= 60.0f) {
                //Optionally play a sound when the stat increases/decreases (unused)
                if (sStats.items[i] < stats.items[i]) {
                    if (dStatChangeSounds.items[i].increased != NO_SOUND) {
                        gDLL_6_AMSFX->vtbl->play(NULL, dStatChangeSounds.items[i].increased, MAX_VOLUME, NULL, NULL, 0, NULL);
                    }
                } else {
                    if (dStatChangeSounds.items[i].decreased != NO_SOUND) {
                        gDLL_6_AMSFX->vtbl->play(NULL, dStatChangeSounds.items[i].decreased, MAX_VOLUME, NULL, NULL, 0, NULL);
                    }
                }

                sStats.items[i] = stats.items[i];
            }
        }

        //Check if the stat changed
        if (stats.items[i] != sPrevStats.items[i]) {
            sPrevStats.items[i] = stats.items[i];
            if (sStatsChangeTimers.items[i] <= 60.0f) {
                sStatsChangeTimers.items[i] = 90.0f - gUpdateRateF;
            }
        }

        if (sStatsChangeTimers.items[i] < -30.0f) {
            sStatsChangeTimers.items[i] = -30.0f;
        }
    }
}

#define SIDEKICK_FOOD_MAX 16

/** Handle attempting to feed the sidekick while the meter is already full:
  * If the meter is totally one colour (say blue) and the food being fed is also that colour, refuse it.
  * Otherwise, discard enough of the opposite colour (say red) to make room for what's being fed.
  */
static int sidekick_meter_handle_full(int isBlueEnergy, s8 energyAdded) {
    Object* highlighted; //Object with LockIcon over it
    Object* sidekick;
    SidekickStats* dinoStats;
    s32 blueSlotsOccupied;
    s32 redSlotsOccupied;
    u8* statToIncrease;
    u8* statToDecrease;
    s8 newValue;
    s8 returnValue = 0;

    sidekick = get_sidekick();
    highlighted = gDLL_2_Camera->vtbl->get_highlighted_object();

    //Return if the sidekick isn't who the food's being given to
    if (highlighted != sidekick) {
        return 0;
    }

    dinoStats = gDLL_29_Gplay->vtbl->get_sidekick_stats();
    if (!dinoStats) {
        return 0;
    }

    if (isBlueEnergy) {
        statToIncrease = &dinoStats->blueFood;
        statToDecrease = &dinoStats->redFood;
    } else {
        statToIncrease = &dinoStats->redFood;
        statToDecrease = &dinoStats->blueFood;
    }

    #if DEBUG_SIDEKICK_METER
    recomp_printf("\nBLUE: %d\tRED: %d (Before)\n", dinoStats->blueFood, dinoStats->redFood);
    #endif

    //Only showing Blue energy (ignoring Red energy)
    if (recomp_get_config_u32("cmdmenu_sidekick_meter_hide_red")) {
        if (*statToIncrease >= SIDEKICK_FOOD_MAX) {
            *statToIncrease = SIDEKICK_FOOD_MAX;

            //Bark
            gDLL_6_AMSFX->vtbl->play(sidekick, 0x4B8, MAX_VOLUME, NULL, NULL, 0, NULL);
            
            returnValue = 1;
        }
    } else {
    //Increasing Blue energy while Red energy is visible
        blueSlotsOccupied = (dinoStats->blueFood/2) + (dinoStats->blueFood % 2);
        redSlotsOccupied = (dinoStats->redFood/2) + (dinoStats->redFood % 2);

        //Check if all slots are already occupied
        if ((blueSlotsOccupied + redSlotsOccupied) >= SIDEKICK_FOOD_MAX/2) {

            //Refuse food if the meter's already fully filled with that colour
            if (*statToIncrease >= SIDEKICK_FOOD_MAX) {
                gDLL_6_AMSFX->vtbl->play(sidekick, 0x4B8, MAX_VOLUME, NULL, NULL, 0, NULL);
                returnValue = 1;
            } else {
            //Otherwise: discard enough of the opposite-colour food icon to make space
               newValue = *statToDecrease;
                if (newValue % 2) {
                    newValue -= 1 + (energyAdded - 2); //remove half-filled red slot
                } else {
                    newValue -= 2 + (energyAdded - 2); //remove filled red slot
                }
                newValue = MAX(0, newValue);
                *statToDecrease = newValue;
            }
        }
    }

    #if DEBUG_SIDEKICK_METER
    recomp_printf("BLUE: %d\tRED: %d (After)\n", dinoStats->blueFood, dinoStats->redFood);
    #endif

    return returnValue;
}

static int cmdmenu_handle_item_use_special_cases(s32 itemGamebitID) {
    switch (itemGamebitID) {
    case BIT_Inventory_Blue_Mushrooms:
        return sidekick_meter_handle_full(TRUE, 2);
    case BIT_Dino_Bag_Old_Mushrooms:
        return sidekick_meter_handle_full(TRUE, 1);
    case BIT_Dino_Bag_Red_Mushrooms:
        return sidekick_meter_handle_full(FALSE, 2);
    case BIT_CloudRunner_Grubs:
        return sidekick_meter_handle_full(TRUE, 2);
    case BIT_Dino_Bag_Old_Grubs:
        return sidekick_meter_handle_full(TRUE, 1);
    case BIT_Dino_Bag_Red_Grubs:
        return sidekick_meter_handle_full(FALSE, 2);
    }

    return 0;
}

/** Handle special cases: e.g. Tricky refusing food */
RECOMP_PATCH int cmdmenu_was_this_item_used(s32 itemGamebitID) {
    //@recomp: handle special cases (e.g. Tricky refusing food)
    if (cmdmenu_handle_item_use_special_cases(itemGamebitID)) {
        return FALSE;
    }

    if (itemGamebitID == sUsedItemGamebitID) {
        //Don't play the "item refused" sound when an item was used successfully
        sUsedItemSoundType = CMDMENU_SOUND_NONE;
        return TRUE;
    }
    return FALSE;
}

#define ALL_MENUOPEN_C_BUTTONS (L_CBUTTONS | R_CBUTTONS | D_CBUTTONS)
#define ALL_MENUOPEN_D_PAD (L_JPAD | R_JPAD | D_JPAD)

/**
  * For inventory's New Controls mode: gets next available page category to the left of the current page.
  * (e.g. Items -> Sidekick, Sidekick -> Spells, Spells -> Items)
  *
  * A category is skipped if it's unavailable (i.e. no items to show, or sidekick missing)
  */
s8 cmdmenu_new_get_next_category_left(Object* player, Object* sidekick, s8* rMoveToCategory, s8* rMoveToPage) {
    s8 pageSidekick = (sidekick && (sidekick->id == OBJ_Kyte)) ? CMDMENU_PAGE_8_Sidekick_Kyte : CMDMENU_PAGE_7_Sidekick_Tricky;
    s8 pageItems = (player && (player->id == OBJ_Krystal)) ? CMDMENU_PAGE_0_Items_Krystal : CMDMENU_PAGE_1_Items_Sabre;

    if (!player) {
        return 0;
    }

    switch (dPageCategory) {
    case CMDMENU_CATEGORY_4_Spells:
        if (sidekick && cmdmenu_page_count_shown_items(dCmdmenuPages[pageSidekick].items, TRUE)) {
            *rMoveToCategory = CMDMENU_CATEGORY_2_Sidekick;
            *rMoveToPage = pageSidekick;
            return 1;
        }
        if (cmdmenu_page_count_shown_items(dCmdmenuPages[pageItems].items, FALSE)) {
            *rMoveToCategory = CMDMENU_CATEGORY_3_Items;
            *rMoveToPage = pageItems;
            return 1;
        }
        break;
    case CMDMENU_CATEGORY_2_Sidekick:
        if (cmdmenu_page_count_shown_items(dCmdmenuPages[pageItems].items, FALSE)) {
            *rMoveToCategory = CMDMENU_CATEGORY_3_Items;
            *rMoveToPage = pageItems;
            return 1;
        }
        if (cmdmenu_page_count_shown_items(dCmdmenuPages[CMDMENU_PAGE_6_Spells].items, FALSE)) {
            *rMoveToCategory = CMDMENU_CATEGORY_4_Spells;
            *rMoveToPage = CMDMENU_PAGE_6_Spells;
            return 1;
        }
        break;
    case CMDMENU_CATEGORY_3_Items:
        if (cmdmenu_page_count_shown_items(dCmdmenuPages[CMDMENU_PAGE_6_Spells].items, FALSE)) {
            *rMoveToCategory = CMDMENU_CATEGORY_4_Spells;
            *rMoveToPage = CMDMENU_PAGE_6_Spells;
            return 1;
        }
        if (sidekick && cmdmenu_page_count_shown_items(dCmdmenuPages[pageSidekick].items, TRUE)) {
            *rMoveToCategory = CMDMENU_CATEGORY_2_Sidekick;
            *rMoveToPage = pageSidekick;
            return 1;
        }
        break;
    }
    
    return 0;
}

/**
  * For inventory's New Controls mode: gets next available page category to the right of the current page.
  * (e.g. Spells -> Sidekick, Sidekick -> Items, Items -> Spells)
  *
  * A category is skipped if it's unavailable (i.e. no items to show, or sidekick missing)
  */
s8 cmdmenu_new_get_next_category_right(Object* player, Object* sidekick, s8* rMoveToCategory, s8* rMoveToPage) {
    s8 pageSidekick = (sidekick && (sidekick->id == OBJ_Kyte)) ? CMDMENU_PAGE_8_Sidekick_Kyte : CMDMENU_PAGE_7_Sidekick_Tricky;
    s8 pageItems = (player && (player->id == OBJ_Krystal)) ? CMDMENU_PAGE_0_Items_Krystal : CMDMENU_PAGE_1_Items_Sabre;

    if (!player) {
        return 0;
    }

    switch (dPageCategory) {
    case CMDMENU_CATEGORY_3_Items:
        if (sidekick && cmdmenu_page_count_shown_items(dCmdmenuPages[pageSidekick].items, TRUE)) {
            *rMoveToCategory = CMDMENU_CATEGORY_2_Sidekick;
            *rMoveToPage = pageSidekick;
            return 1;
        }
        if (cmdmenu_page_count_shown_items(dCmdmenuPages[CMDMENU_PAGE_6_Spells].items, FALSE)) {
            *rMoveToCategory = CMDMENU_CATEGORY_4_Spells;
            *rMoveToPage = CMDMENU_PAGE_6_Spells;
            return 1;
        }
        break;
    case CMDMENU_CATEGORY_2_Sidekick:
        if (cmdmenu_page_count_shown_items(dCmdmenuPages[CMDMENU_PAGE_6_Spells].items, FALSE)) {
            *rMoveToCategory = CMDMENU_CATEGORY_4_Spells;
            *rMoveToPage = CMDMENU_PAGE_6_Spells;
            return 1;
        }
        if (cmdmenu_page_count_shown_items(dCmdmenuPages[pageItems].items, FALSE)) {
            *rMoveToCategory = CMDMENU_CATEGORY_3_Items;
            *rMoveToPage = pageItems;
            return 1;
        }
        break;
    case CMDMENU_CATEGORY_4_Spells:
        if (cmdmenu_page_count_shown_items(dCmdmenuPages[pageItems].items, FALSE)) {
            *rMoveToCategory = CMDMENU_CATEGORY_3_Items;
            *rMoveToPage = pageItems;
            return 1;
        }
        if (sidekick && cmdmenu_page_count_shown_items(dCmdmenuPages[pageSidekick].items, TRUE)) {
            *rMoveToCategory = CMDMENU_CATEGORY_2_Sidekick;
            *rMoveToPage = pageSidekick;
            return 1;
        }
        break;
    }
    
    return 0;
}

/** Map the blocked C-buttons onto the D-pad when using optional D-pad controls */
static u16 cmdmenu_get_extended_disabled_buttons() {
    u8 rNewControls = recomp_get_config_u32("cmdmenu_new_controls");  //whether to use new controls
    u8 rDControls = recomp_get_config_u32("cmdmenu_d_controls") > DPAD_OFF; //whether D-pad can navigate
    u16 joyButtonMaskExtended = sJoyDisabledButtons;

    //@recomp: handle optional new controls
    if (rNewControls && (joyButtonMaskExtended & D_CBUTTONS)) {
        joyButtonMaskExtended |= U_CBUTTONS;
    }

    //@recomp: handle optional D-pad controls
    if (rDControls) {
        if (joyButtonMaskExtended & U_CBUTTONS) { 
            joyButtonMaskExtended |= U_JPAD;
        }

        if (joyButtonMaskExtended & D_CBUTTONS) { 
            joyButtonMaskExtended |= D_JPAD;
        }

        if (joyButtonMaskExtended & L_CBUTTONS) { 
            joyButtonMaskExtended |= L_JPAD;
        }

        if (joyButtonMaskExtended & R_CBUTTONS) { 
            joyButtonMaskExtended |= R_JPAD;
        }
    }

    return joyButtonMaskExtended;
}

/** 
  * - Add optional D-pad controls (either alongside C-button controls, or replacing C-button controls)
  *
  * - Add optional New Controls mode: 
  *   C-left/down/right opens inventory to Spells/Sidekick/Items as before.
  *   But when the inventory is open, C-left/right go to previous/next page category.
  *   This frees up C-down/up so they can be used to scroll down OR up through the page's items.
  */
RECOMP_PATCH void cmdmenu_update2(void) {
    Object* player;
    Object* sidekick;
    s16 pad1;
    s8 pad2;
    s16 newStringID;
    s8 autoShowInfoScroll;
    s8 newPageIndex;
    /* RECOMP */
    u8 rNewControls = recomp_get_config_u32("cmdmenu_new_controls"); //whether to use new controls
    u8 rDControls = recomp_get_config_u32("cmdmenu_d_controls") > DPAD_OFF; //whether D-pad can navigate
    u8 rCControls = recomp_get_config_u32("cmdmenu_d_controls") < DPAD_ON_CBUTTONS_OFF; //whether C-buttons can navigate
    u16 rAllMenuOpenButtons = (rCControls * ALL_MENUOPEN_C_BUTTONS) | (rDControls * ALL_MENUOPEN_D_PAD);
    u16 rMenuLeft = (rCControls * L_CBUTTONS) | (rDControls * L_JPAD);
    u16 rMenuDown = (rCControls * D_CBUTTONS) | (rDControls * D_JPAD);
    u16 rMenuRight = (rCControls * R_CBUTTONS) | (rDControls * R_JPAD);
    u16 rRawDPad = joy_get_pressed_raw(0) & ALL_MENUOPEN_D_PAD; //D-right seems to be masked, so getting it separately
    u8 rIsInventoryOpen;
    s8 rMoveToCategory;
    s8 rMoveToPage;
    u16 rJoyButtonMaskExtended = cmdmenu_get_extended_disabled_buttons();

    //@recomp: set button mask
    if (rDControls) {
        joy_disable_buttons(0, rJoyButtonMaskExtended | ALL_MENUOPEN_D_PAD);
    }

    player = get_player();
    sidekick = get_sidekick();

    if (player == NULL) {
        return;
    }

    //Get controller button presses/holds
    sJoyPressedButtons = joy_get_pressed(0) | rRawDPad;
    sJoyHeldButtons = joy_get_buttons(0); //D-right holds not needed anyway

    if (player->stateFlags & OBJSTATE_IN_SEQ) {
        joy_disable_buttons(0, rAllMenuOpenButtons);
        sJoyPressedButtons &= ~(R_TRIG | rAllMenuOpenButtons);
        sJoyHeldButtons &= ~(R_TRIG | rAllMenuOpenButtons);
    } else if (rJoyButtonMaskExtended != 0) {
        joy_disable_buttons(0, rJoyButtonMaskExtended);
        sJoyPressedButtons &= ~rJoyButtonMaskExtended;
        sJoyHeldButtons &= ~rJoyButtonMaskExtended;
    }

    if (sShouldOverrideJoypadButtons) {
        sJoyPressedButtons = sJoyPressedButtonsOverride;
    } else {
        sJoyPressedButtons |= sJoyPressedButtonsOverride;
        if ((player->stateFlags & OBJSTATE_IN_SEQ) || (rJoyButtonMaskExtended != 0)) {
            sJoyPressedButtons |= B_BUTTON;
        }
    }

    //[CLASSIC CONTROLS] Use C buttons (left/down/right) to open/change inventory pages
    if (rNewControls == FALSE) {
        if ((sJoyPressedButtons & rMenuDown) && (sidekick != NULL) && (dPageCategory != CMDMENU_CATEGORY_2_Sidekick)) {
            //C-down: Sidekick Commands
            newPageIndex = sidekick->id == OBJ_Kyte ? CMDMENU_PAGE_8_Sidekick_Kyte : CMDMENU_PAGE_7_Sidekick_Tricky;
            if (cmdmenu_page_count_shown_items(dCmdmenuPages[newPageIndex].items, TRUE)) {
                joy_disable_buttons(0, rMenuDown);
                dNextPageCategory = CMDMENU_CATEGORY_2_Sidekick;
                sInventoryPageID = newPageIndex;
            }
        } else if ((sJoyPressedButtons & rMenuRight) && (dPageCategory != CMDMENU_CATEGORY_3_Items) && (dPageCategory != CMDMENU_CATEGORY_6_Food)) {
            //C-right: Items
            newPageIndex = player->id == OBJ_Krystal ? CMDMENU_PAGE_0_Items_Krystal : CMDMENU_PAGE_1_Items_Sabre;
            if (cmdmenu_page_count_shown_items(dCmdmenuPages[newPageIndex].items, FALSE)) {
                joy_disable_buttons(0, rMenuRight);
                dNextPageCategory = CMDMENU_CATEGORY_3_Items;
                sInventoryPageID = newPageIndex;
            }
        } else if ((sJoyPressedButtons & rMenuLeft) && (dPageCategory != CMDMENU_CATEGORY_4_Spells)) {
            //C-left: Magic Spells
            if (cmdmenu_page_count_shown_items(dCmdmenuPages[CMDMENU_PAGE_6_Spells].items, FALSE)) {
                joy_disable_buttons(0, rMenuLeft);
                dNextPageCategory = CMDMENU_CATEGORY_4_Spells;
                sInventoryPageID = CMDMENU_PAGE_6_Spells;
            }
        } else if (sUsedItemPageID != EXIT) {
            //No C-buttons were pressed, but the selected item opened an inventory page (e.g. foodbags)
            sSubmenuGamebitID = sUsedItemGamebitID;
            sInventoryPageID = sUsedItemPageID;

            //Sidekick Commands -> Sidekick Foodbag
            if ((dPageCategory == CMDMENU_CATEGORY_2_Sidekick) || (dPageCategory == CMDMENU_CATEGORY_5)) {
                dNextPageCategory = CMDMENU_CATEGORY_7_Sidekick_Food; //@bug? Maybe Category 5 is intended for Sidekick Food (i.e. 2->5, 3->6, 4->7; instead of 2->7, 3->6, 4->7)
            //Items -> Player Foodbag
            } else if ((dPageCategory == CMDMENU_CATEGORY_3_Items) || (dPageCategory == CMDMENU_CATEGORY_6_Food)) {
                dNextPageCategory = CMDMENU_CATEGORY_6_Food;
            //Spells -> Spell subpage (unused)
            } else if ((dPageCategory == CMDMENU_CATEGORY_4_Spells) || (dPageCategory == CMDMENU_CATEGORY_7_Sidekick_Food)) {
                dNextPageCategory = CMDMENU_CATEGORY_7_Sidekick_Food;
            }
        }
    }

    //[NEW CONTROLS] Use C buttons (left/down/right) to open inventory on desired page, and C-left/C-right to move between pages 
    if (rNewControls == TRUE) {
        rIsInventoryOpen = !cmdmenu_is_inventory_closed();

        if ((!rIsInventoryOpen) && (sJoyPressedButtons & rMenuDown) && (sidekick != NULL) && (dPageCategory != CMDMENU_CATEGORY_2_Sidekick)
        ) {
            //C-down while Closed: Open on Sidekick Commands
            newPageIndex = sidekick->id == OBJ_Kyte ? CMDMENU_PAGE_8_Sidekick_Kyte : CMDMENU_PAGE_7_Sidekick_Tricky;
            if (cmdmenu_page_count_shown_items(dCmdmenuPages[newPageIndex].items, TRUE)) {
                joy_disable_buttons(0, rMenuDown);
                dNextPageCategory = CMDMENU_CATEGORY_2_Sidekick;
                sInventoryPageID = newPageIndex;
            }
        } else if ((!rIsInventoryOpen) && (sJoyPressedButtons & rMenuRight) && (dPageCategory != CMDMENU_CATEGORY_3_Items) && (dPageCategory != CMDMENU_CATEGORY_6_Food)) {
            //C-right while Closed: Open on Items
            newPageIndex = player->id == OBJ_Krystal ? CMDMENU_PAGE_0_Items_Krystal : CMDMENU_PAGE_1_Items_Sabre;
            if (cmdmenu_page_count_shown_items(dCmdmenuPages[newPageIndex].items, FALSE)) {
                joy_disable_buttons(0, rMenuRight);
                dNextPageCategory = CMDMENU_CATEGORY_3_Items;
                sInventoryPageID = newPageIndex;
            }
        } else if ((!rIsInventoryOpen) && (sJoyPressedButtons & rMenuLeft) && (dPageCategory != CMDMENU_CATEGORY_4_Spells)) {
            //C-left while Closed: Open on Magic Spells
            if (cmdmenu_page_count_shown_items(dCmdmenuPages[CMDMENU_PAGE_6_Spells].items, FALSE)) {
                joy_disable_buttons(0, rMenuLeft);
                dNextPageCategory = CMDMENU_CATEGORY_4_Spells;
                sInventoryPageID = CMDMENU_PAGE_6_Spells;
            }
        } else if (rIsInventoryOpen && (sJoyPressedButtons & rMenuLeft) && cmdmenu_new_get_next_category_right(player, sidekick, &rMoveToCategory, &rMoveToPage)) {
            //C-left while Open: go to previous category
            joy_disable_buttons(0, rMenuLeft);
            dNextPageCategory = rMoveToCategory;
            sInventoryPageID = rMoveToPage;
        } else if (rIsInventoryOpen && (sJoyPressedButtons & rMenuRight) && cmdmenu_new_get_next_category_left(player, sidekick, &rMoveToCategory, &rMoveToPage)) {
            //C-right while Open: go to next category
            joy_disable_buttons(0, rMenuRight);
            dNextPageCategory = rMoveToCategory;
            sInventoryPageID = rMoveToPage;
        } else if (sUsedItemPageID != EXIT) {
            //No C-buttons were pressed, but the selected item opened an inventory page (e.g. foodbags)
            sSubmenuGamebitID = sUsedItemGamebitID;
            sInventoryPageID = sUsedItemPageID;

            //Sidekick Commands -> Sidekick Foodbag
            if ((dPageCategory == CMDMENU_CATEGORY_2_Sidekick) || (dPageCategory == CMDMENU_CATEGORY_5)) {
                dNextPageCategory = CMDMENU_CATEGORY_7_Sidekick_Food; //@bug? Maybe Category 5 is intended for Sidekick Food (i.e. 2->5, 3->6, 4->7; instead of 2->7, 3->6, 4->7)
            //Items -> Player Foodbag
            } else if ((dPageCategory == CMDMENU_CATEGORY_3_Items) || (dPageCategory == CMDMENU_CATEGORY_6_Food)) {
                dNextPageCategory = CMDMENU_CATEGORY_6_Food;
            //Spells -> Spell subpage (unused)
            } else if ((dPageCategory == CMDMENU_CATEGORY_4_Spells) || (dPageCategory == CMDMENU_CATEGORY_7_Sidekick_Food)) {
                dNextPageCategory = CMDMENU_CATEGORY_7_Sidekick_Food;
            }
        }
    }

    //Keep the inventory closed during the Galleon battle
    if (gDLL_2_Camera->vtbl->get_dll_ID() == DLL_ID_CAMSHIPBATTLE2) {
        cmdmenu_close_inventory();
    } else if (dNextPageCategory != 0) {
        //Handle opening the inventory (or opening a different page category)
        if (cmdmenu_is_inventory_closed()) {
            switch (dNextPageCategory) {
            case CMDMENU_CATEGORY_3_Items:
                gDLL_6_AMSFX->vtbl->play(NULL, SOUND_5EC_Cmdmenu_OpenBag, MAX_VOLUME, NULL, NULL, 0, NULL);
                break;
            case CMDMENU_CATEGORY_2_Sidekick:
                gDLL_6_AMSFX->vtbl->play(NULL, SOUND_5F0_Cmdmenu_OpenSidekickMenu, MAX_VOLUME, NULL, NULL, 0, NULL);
                break;
            case CMDMENU_CATEGORY_4_Spells:
                gDLL_6_AMSFX->vtbl->play(NULL, SOUND_5ED_Cmdmenu_OpenSpellBook, MAX_VOLUME, NULL, NULL, 0, NULL);
                break;
            default:
                gDLL_6_AMSFX->vtbl->play(NULL, SOUND_28D_Cmdmenu_OpenBag_HighPitch, MAX_VOLUME, NULL, NULL, 0, NULL);
                break;
            }

            cmdmenu_open_inventory();

            //Sidekick command pages always open on index 0 (unlike other pages which remember last selection)
            dCmdmenuPages[CMDMENU_PAGE_7_Sidekick_Tricky].selectedIndex = 0;
            dCmdmenuPages[CMDMENU_PAGE_8_Sidekick_Kyte].selectedIndex = 0;

            dPageCategory = dNextPageCategory;
            sJoyPressedButtons = 0;
            dInventoryMovesQueued = 0;
            dNextPageCategory = 0;
        } else {
            /* When changing page category (e.g. from Items to Spells),
               first close up the scroll's current page, before reopening with the new page */
            cmdmenu_close_inventory();
        }
    }

    cmdmenu_update_stats();
    cmdmenu_inventory_animate();
    cmdmenu_info_scroll_animate();

    //Count how many times this function has run (clamped from 0-2)
    sInventoryFrameCounter++;
    if (sInventoryFrameCounter > 2) {
        sInventoryFrameCounter = 2;
    }

    /* If the inventory is visible, use the selected item's gametext,
       otherwise use the target Object's description gametext */
    if ((dInventoryShow != FALSE) || (dNextPageCategory != 0)) {
        newStringID = dSelectedItemTextID;
    } else {
        newStringID = gDLL_2_Camera->vtbl->get_target_gametextID();
    }
    dSelectedItemTextID = NO_GAMETEXT;

    //Check if the info scroll should be auto-shown
    autoShowInfoScroll = FALSE;
    if (sInfoScrollOverrideTextID > NO_GAMETEXT) {
        newStringID = sInfoScrollOverrideTextID;

        //When auto-shown, scroll's screen position can temporarily be changed
        dInfoScrollY = sInfoScrollOverrideY; 
        dInfoScrollX = sInfoScrollOverrideX;

        autoShowInfoScroll = TRUE;
    } else {
        //Restore the scroll's default position when closed
        if (cmdmenu_is_info_scroll_closed() &&
            (dInfoScrollOpacity == 0) //@recomp: don't reset position until fully hidden
        ) {
            dInfoScrollX = INFO_SCROLL_X;
            dInfoScrollY = INFO_SCROLL_Y;
        }
    }
    sInfoScrollOverrideTextID = NO_GAMETEXT;

    //Open the scroll when R is held (or when auto-shown) and a textID is set
    if (((sJoyHeldButtons & R_TRIG) || autoShowInfoScroll) && (newStringID > NO_GAMETEXT)) {
        //Free the scroll's current strings when the textID changes
        if (newStringID != dInfoScrollTextID) {
            dInfoScrollTextID = newStringID;
            if (*dInfoScrollStrings != NULL) {
                mmFree(*dInfoScrollStrings);
                *dInfoScrollStrings = NULL;
            }
        }
        cmdmenu_open_info_scroll();
    } else {
        cmdmenu_close_info_scroll();
        //Free the scroll's current strings when fully closed
        if (cmdmenu_is_info_scroll_closed() && (*dInfoScrollStrings != NULL)) {
            mmFree(*dInfoScrollStrings);
            *dInfoScrollStrings = NULL;
            dInfoScrollTextID = NO_GAMETEXT;
        }
    }

    joy_disable_buttons(0, rAllMenuOpenButtons);
    sJoyDisabledButtons = 0;
}

#define NEW_CONTROLS_CONTINUOUS_SCROLL_WAIT 30
#define NEW_CONTROLS_CONTINUOUS_SCROLL_REPEAT 20

/**
  * - Fix a visual "pop" when scrolling through inventory items:
  *
  *   The item icon strip would jump too far (especially when wrapping from top-to-bottom),
  *   causing a visual judder and occasionally exposing an empty gap at the top of the icon strip.
  *
  *   Rare seem to have accidentally used the tiles' width instead of height to calculate the jump size 
  *   (possibly leftover from the horizontally-scrolling design) - changing it to the height fixes it!
  *
  * - Add optional New Controls mode: 
  *   C-down/up let you scroll down OR up through the page's items, 
  *   instead of only being able to scroll down with the page's particular C button.
  *
  *   New Controls also lets you hold C-down/up to scroll continuously through your items, 
  *   without having to tap repeatedly.
  */
RECOMP_PATCH void cmdmenu_tick_inventory_page(void) {
    s16* pageSelectionIndex;
    Object* player;
    InventoryItem *pageItems;
    u32 pageMsg;
    s32 pageBtnMask;
    s32 usedGamebit;
    s8 isSidekickMenu;
    /* RECOMP */
    u8 rNewControls = recomp_get_config_u32("cmdmenu_new_controls"); //whether to use new controls
    u8 rDControls = recomp_get_config_u32("cmdmenu_d_controls") > DPAD_OFF; //whether D-pad can navigate
    u8 rCControls = recomp_get_config_u32("cmdmenu_d_controls") < DPAD_ON_CBUTTONS_OFF; //whether C-buttons can navigate
    u16 rAllMenuOpenButtons = (rCControls * ALL_MENUOPEN_C_BUTTONS) | (rDControls * ALL_MENUOPEN_D_PAD);
    u16 rMenuLeft = (rCControls * L_CBUTTONS) | (rDControls * L_JPAD);
    u16 rMenuDown = (rCControls * D_CBUTTONS) | (rDControls * D_JPAD);
    u16 rMenuRight = (rCControls * R_CBUTTONS) | (rDControls * R_JPAD);
    u16 rMenuUp = (rCControls * U_CBUTTONS) | (rDControls * U_JPAD);
    u16 rRawDPad = joy_get_pressed_raw(0) & (ALL_MENUOPEN_D_PAD | U_JPAD); //D-right seems to be masked, so getting it separately
    u32 rJoyHeldButtons = 0;
    static s8 rsHoldDirectionTimer = NEW_CONTROLS_CONTINUOUS_SCROLL_WAIT;
    static s16 rsHeldButton = 0;
    u16 rJoyButtonMaskExtended = cmdmenu_get_extended_disabled_buttons();

    player = get_player();

    isSidekickMenu = FALSE;

    //Do nothing if the player's unloaded
    if (player == NULL) {
        return;
    }

    //Lock/unlock accessing the C-button scroll menu
    if (player->stateFlags & OBJSTATE_IN_SEQ) {
        joy_disable_buttons(0, (rCControls * (ALL_MENUOPEN_C_BUTTONS | U_CBUTTONS)) | (rDControls * (ALL_MENUOPEN_D_PAD | U_JPAD)));
    } else if (rJoyButtonMaskExtended != 0) {
        joy_disable_buttons(0, rJoyButtonMaskExtended);
    }

    //Get button presses (or simulated ones, for tutorial sequences)
    if (sShouldOverrideJoypadButtons) {
        sJoyPressedButtons = sJoyPressedButtonsOverride;
    } else {
        sJoyPressedButtons = joy_get_pressed(0) | rRawDPad; //@recomp: optionally factor in D-pad
        rJoyHeldButtons = rNewControls ? gContPads[gVirtualContPortMap[0]].button : 0; //@recomp: get held buttons (for continuous scrolling)

        if ((player->stateFlags & OBJSTATE_IN_SEQ) || (rJoyButtonMaskExtended != 0)) {
            sJoyPressedButtons |= B_BUTTON;
        }
    }

    //NEW CONTROLS: Allow held buttons as well, for delayed continuous scrolling (without having to tap repeatedly)
    {
        #if DEBUG_INVENTORY_SCROLLING
        {
            diPrintf("sJoyPressedButtons: %d\n", sJoyPressedButtons);
            diPrintf("rJoyHeldButtons: %d\n", rJoyHeldButtons);
            diPrintf("rsHeldButton: %d\n", rsHeldButton);
            diPrintf("rsHoldDirectionTimer: %d\n\n", rsHoldDirectionTimer);
        }
        #endif

        //Forget held button once released
        if (sShouldOverrideJoypadButtons) {
            rsHeldButton = 0;
            rsHoldDirectionTimer = NEW_CONTROLS_CONTINUOUS_SCROLL_WAIT;
        } else if ((rJoyHeldButtons & rsHeldButton) == FALSE) { 
            rsHeldButton = 0;
            rsHoldDirectionTimer = NEW_CONTROLS_CONTINUOUS_SCROLL_WAIT;

            if (rJoyHeldButtons & rMenuDown) {
                rsHeldButton = rMenuDown;
            } else if (rJoyHeldButtons & rMenuUp){
                rsHeldButton = rMenuUp;
            }
        } else {
            rsHoldDirectionTimer -= gUpdateRate;
            if (rsHoldDirectionTimer < 0) {
                //Simulate press
                if (rsHeldButton == (D_CBUTTONS | D_JPAD)) {
                    sJoyPressedButtons |= D_CBUTTONS;
                } else if (rsHeldButton == (U_CBUTTONS | U_JPAD)) {
                    sJoyPressedButtons |= U_CBUTTONS;
                } else {
                    sJoyPressedButtons |= rsHeldButton;
                }
                rsHoldDirectionTimer = NEW_CONTROLS_CONTINUOUS_SCROLL_REPEAT;
            }
        }
    }

    //@recomp: disable C-up while inventory open (to prevent entering 1st-person while navigating up)
    if (rNewControls && (dInventoryShow > 0)) {
        joy_disable_buttons(0, rJoyButtonMaskExtended | rMenuUp);
    }

    //Play item use sound if needed
    switch (sUsedItemSoundType) {
    case CMDMENU_SOUND_NONE:
        break;
    case CMDMENU_SOUND_ITEM:
        gDLL_6_AMSFX->vtbl->play(NULL, SOUND_79C_Cmdmenu_CantUse, MAX_VOLUME, NULL, NULL, 0, NULL);
        break;
    case CMDMENU_SOUND_PAGE:
        gDLL_6_AMSFX->vtbl->play(NULL, SOUND_814_Cmdmenu_OpenSubMenu, MAX_VOLUME, NULL, NULL, 0, NULL);
        break;
    }

    sUsedItemGamebitID = NO_GAMEBIT;
    sUsedItemSoundType = CMDMENU_SOUND_NONE;
    sUsedItemPageID = NO_PAGE;

    pageItems = dCmdmenuPages[sInventoryPageID].items;
    pageSelectionIndex = &dCmdmenuPages[sInventoryPageID].selectedIndex;
    pageMsg = dCmdmenuPages[sInventoryPageID].mesgID;
    pageBtnMask = dCmdmenuPages[sInventoryPageID].btnMask;

    //@recomp: handle optional D-pad and/or C-button controls
    if (rDControls && rCControls) {
        if (pageBtnMask & L_CBUTTONS) { //D-pad controls AND C-button controls
            pageBtnMask |= L_JPAD;
        }
        if (pageBtnMask & D_CBUTTONS) {
            pageBtnMask |= D_JPAD;
        }
        if (pageBtnMask & R_CBUTTONS) {
            pageBtnMask |= R_JPAD;
        }
    } else if (rDControls && !rCControls) { //D-pad controls only
        if (pageBtnMask & L_CBUTTONS) {
            pageBtnMask &= ~L_CBUTTONS;
            pageBtnMask |= L_JPAD;
        }
        if (pageBtnMask & D_CBUTTONS) {
            pageBtnMask &= ~D_CBUTTONS;
            pageBtnMask |= D_JPAD;
        }
        if (pageBtnMask & R_CBUTTONS) {
            pageBtnMask &= ~R_CBUTTONS;
            pageBtnMask |= R_JPAD;
        }
    }

    //Check if the current page is a sidekick command page
    if ((sInventoryPageID == CMDMENU_PAGE_7_Sidekick_Tricky) || 
        (sInventoryPageID == CMDMENU_PAGE_8_Sidekick_Kyte)
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

        //CLASSIC CONTROLS (press category's C-button to scroll it down once)
        if (rNewControls == FALSE) {
            if ((sJoyPressedButtons & pageBtnMask) && 
                (sInventoryScrollOffset < 8) && 
                (dInventoryIsScrolling == FALSE)
            ) {
                gDLL_6_AMSFX->vtbl->play(NULL, SOUND_28A_Cmdmenu_MoveSelection, MAX_VOLUME, NULL, NULL, 0, NULL);
                dInventoryMovesQueued++;

                if (sInventoryScrollOffset != 0) {
                    dInventoryIsScrolling = TRUE;
                }
            }

        } else {
        //NEW CONTROLS (down to scroll down, up to scroll up)
            if ((sJoyPressedButtons & rMenuDown) && 
                (SQ(sInventoryScrollOffset) < SQ(8)) &&
                (dInventoryIsScrolling == FALSE)
            ) {
                gDLL_6_AMSFX->vtbl->play(NULL, SOUND_28A_Cmdmenu_MoveSelection, MAX_VOLUME, NULL, NULL, 0, NULL);
                dInventoryMovesQueued++;

                if (sInventoryScrollOffset != 0) {
                    dInventoryIsScrolling = TRUE;
                }
            } else if ((sJoyPressedButtons & rMenuUp) && 
                (SQ(sInventoryScrollOffset) < SQ(8)) && 
                (dInventoryIsScrolling == FALSE)
            ) {
                gDLL_6_AMSFX->vtbl->play(NULL, SOUND_28A_Cmdmenu_MoveSelection, MAX_VOLUME, NULL, NULL, 0, NULL);
                dInventoryMovesQueued--;

                if (sInventoryScrollOffset != 0) {
                    dInventoryIsScrolling = TRUE;
                }
            }
        }

        //Limit the number of moves
        if (dInventoryMovesQueued > 255) {
            dInventoryMovesQueued = 255;
        } else if (dInventoryMovesQueued < -255) { //@recomp: up limit
            dInventoryMovesQueued = -255;
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
                    sInventoryScrollOffset = 2 * MENU_ITEM_HEIGHT; //@recomp: fix
                    dInventoryMoveSpeed = 2;
                    dInventoryIsScrolling = FALSE;
                } else {
                    //@bug: causes visual pop: should be MENU_ITEM_HEIGHT
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

        } else if (rNewControls && 
            ((dInventoryMovesQueued < 0) && (sInventoryScrollOffset == 0))
        ) {
            dInventoryMovesQueued++;

            if (sDisplayedItemCount > 1) {
                //When wrapping to top, skip over a gap in the inventory in specific cases (2 or 4 items)
                if ((sMenuSelectedItemIdx == 0) &&
                    ((sDisplayedItemCount == 2) || (sDisplayedItemCount == 4))
                ) {
                    sInventoryScrollOffset = -2 * MENU_ITEM_HEIGHT;
                    dInventoryMoveSpeed = 2;
                    dInventoryIsScrolling = FALSE;
                } else {
                    sInventoryScrollOffset = -MENU_ITEM_HEIGHT;
                    dInventoryMoveSpeed = 2;
                    dInventoryIsScrolling = FALSE;
                }

                //Select previous item
                sMenuSelectedItemIdx--;

                //Wrap selection down to bottom of item list
                if (sMenuSelectedItemIdx < 0) {
                    sMenuSelectedItemIdx = sDisplayedItemCount - 1;
                }
            }
        //Close the inventory with the B button
        } else if (sJoyPressedButtons & B_BUTTON) {
            gDLL_6_AMSFX->vtbl->play(NULL, SOUND_28C_Cmdmenu_Close, MAX_VOLUME, NULL, NULL, 0, NULL);
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
                gDLL_6_AMSFX->vtbl->play(NULL, SOUND_28B_Cmdmenu_Use, MAX_VOLUME, NULL, NULL, 0, NULL);
                cmdmenu_close_inventory();
                
            //Sidekick commands
            } else {
                if (sMenuItemVisibilities[sMenuSelectedItemIdx]) {
                    gDLL_6_AMSFX->vtbl->play(NULL, SOUND_28B_Cmdmenu_Use, MAX_VOLUME, NULL, NULL, 0, NULL);
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
                    gDLL_6_AMSFX->vtbl->play(NULL, SOUND_A0_Cmdmenu_Item_Locked, MAX_VOLUME, NULL, NULL, 0, NULL);
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
        joy_disable_buttons(0, A_BUTTON | B_BUTTON);
    }

    *pageSelectionIndex = sMenuSelectedItemIdx;
}

/** Returns whether the cmdmenu has an active button override */
int cmdmenu_is_button_override_active() {
    return sShouldOverrideJoypadButtons;
}

/** Make sure inventory tutorials still function the same while using optional D-pad controls */
RECOMP_PATCH void cmdmenu_set_buttons_override(s32 buttonsOverride) {
    // u8 rNewControls; //whether to use new controls
    u8 rDControls; //whether D-pad can navigate
    u8 rCControls; //whether C-buttons can navigate

    if (buttonsOverride == CMDMENU_CLEAR_BUTTONS_OVERRIDE) {
        sJoyPressedButtonsOverride = 0;
        sShouldOverrideJoypadButtons = FALSE;
    } else {
        // rNewControls = recomp_get_config_u32("cmdmenu_new_controls");
        rDControls = recomp_get_config_u32("cmdmenu_d_controls") > DPAD_OFF;
        rCControls = recomp_get_config_u32("cmdmenu_d_controls") < DPAD_ON_CBUTTONS_OFF;

        //@recomp: handle optional D-pad and/or C-button controls
        if (rDControls && rCControls) {
            //D-pad controls AND C-button controls

            if ((buttonsOverride & U_JPAD) && !(buttonsOverride & U_CBUTTONS) ) { 
                buttonsOverride &= ~U_JPAD;
                buttonsOverride |= U_CBUTTONS;
            }

            if ((buttonsOverride & D_JPAD) && !(buttonsOverride & D_CBUTTONS) ) { 
                buttonsOverride &= ~D_JPAD;
                buttonsOverride |= D_CBUTTONS;
            }


            if ((buttonsOverride & L_JPAD) && !(buttonsOverride & L_CBUTTONS) ) { 
                buttonsOverride &= ~L_JPAD;
                buttonsOverride |= L_CBUTTONS;
            }

            if ((buttonsOverride & R_JPAD) && !(buttonsOverride & R_CBUTTONS) ) { 
                buttonsOverride &= ~R_JPAD;
                buttonsOverride |= R_CBUTTONS;
            }
        } else if (rDControls && !rCControls) { 
            //D-pad controls only
            if (buttonsOverride & U_CBUTTONS) { 
                buttonsOverride |= U_JPAD;
            }

            if (buttonsOverride & D_CBUTTONS) { 
                buttonsOverride |= D_JPAD;
            }


            if (buttonsOverride & L_CBUTTONS) { 
                buttonsOverride |= L_JPAD;
            }

            if (buttonsOverride & R_CBUTTONS) { 
                buttonsOverride |= R_JPAD;
            }
        }

        sJoyPressedButtonsOverride = buttonsOverride;
        sShouldOverrideJoypadButtons = TRUE;
    }
}

#define ACTIVE_ICON_FADE_IN_SPEED 32
#define ACTIVE_ICON_FADE_OUT_SPEED 8
#define ACTIVE_ICON_FADE_OUT_SPEED_QUICK 32

/** 
  * - Add animation for scrolling up as well as down.
  * - Optionally move/fade the Active Spell/Sidekick Command icons to avoid clashing with the inventory.
  */
RECOMP_PATCH void cmdmenu_inventory_animate(void) {
    /* RECOMP */
    u8 rMoveActiveIcons; 
    u8 rConfigActiveIconOverlapFix = recomp_get_config_u32("cmdmenu_active_icon_overlap");
    u8 rConfigActiveIconFade = recomp_get_config_u32("cmdmenu_active_icon_fade");

    //Animate moving to the next inventory item (@recomp: down or up, instead of just down)
    if (sInventoryScrollOffset > 0) {
        sInventoryScrollOffset -= gUpdateRate << dInventoryMoveSpeed;
        if (sInventoryScrollOffset < 0) {
            sInventoryScrollOffset = 0;
        } 
    } else if (sInventoryScrollOffset < 0) {
        sInventoryScrollOffset += gUpdateRate << dInventoryMoveSpeed;
        if (sInventoryScrollOffset > 0) {
            sInventoryScrollOffset = 0;
        } 
    }

    //Manage the inventory scroll's opacity
    if (dInventoryShow) {
        //Fade in when opened
        dInventoryOpacity += gUpdateRate * 8;
        if (dInventoryOpacity > MAX_OPACITY) {
            dInventoryOpacity = MAX_OPACITY;
        }
    } else if (sInventoryUnrollY == 0) {
        //Fade out when fully closed
        dInventoryOpacity -= gUpdateRate * 8;
        if (dInventoryOpacity < 0) {
            dInventoryOpacity = 0;
        }
    }

    //Animate the inventory scroll expanding/collapsing
    if (dInventoryShow && (dInventoryOpacity > 64)) {
        //Expand/unfurl scroll when inventory opacity partially faded in
        sInventoryUnrollY += gUpdateRate * 4;
        if (sInventoryUnrollY > dInventoryUnrollMax) {
            sInventoryUnrollY = dInventoryUnrollMax;
        } else {
            //Increment/decrement roll frames while inventory is expanding
            dInventoryFrameTop++;
            dInventoryFrameBottom--;
        }
    } else {
        sInventoryUnrollY -= gUpdateRate * 4;
        if (sInventoryUnrollY < 0) {
            sInventoryUnrollY = 0;
        } else {
            //Decrement/increment roll frames while inventory is collapsing
            dInventoryFrameTop--;
            dInventoryFrameBottom++;
        }
    }

    if (dInventoryOpacity) {
        //Increment roll frames while the inventory is scrolling
        if (sInventoryScrollOffset > 0) {
            dInventoryFrameTop++;
            dInventoryFrameBottom++;
        }
        
        //Update frame counters, keeping them in 0-2 range (effectively `dInventoryFrame %= 3`)
        while (dInventoryFrameTop >= 3) {
            dInventoryFrameTop -= 3;
        }
        while (dInventoryFrameTop < 0) {
            dInventoryFrameTop += 3;
        }
        while (dInventoryFrameBottom >= 3) {
            dInventoryFrameBottom -= 3;
        }
        while (dInventoryFrameBottom < 0) {
            dInventoryFrameBottom += 3;
        }
    }

    //@recomp: optionally animate moving the Active Spell/Sidekick Command icons
    if (rConfigActiveIconOverlapFix == ACTIVEICON_MOVE) {
        // Check if the Active Sidekick Command icon should move
        if ((dInventoryShow || dNextPageCategory) && (dInventoryOpacity > 64)) {
            rMoveActiveIcons = TRUE;
        } else {
            rMoveActiveIcons = FALSE;
        }

        //Move the Active Sidekick Command icon when the inventory's open
        if (rMoveActiveIcons) {
            rsLerpActiveSideCommand += gUpdateRateF * INVENTORY_LERP_SPEED;
        } else {
            rsLerpActiveSideCommand -= gUpdateRateF * INVENTORY_LERP_SPEED;
        }

        /* If the inventory is open and both the Sidekick/Spell icons are visible, 
          scoot the Spell icon as well */
        if (sActiveSidekickCommandIcon && rMoveActiveIcons) {
            rsLerpActiveSpell += gUpdateRateF * INVENTORY_LERP_SPEED;
        } else {
            rsLerpActiveSpell -= gUpdateRateF * INVENTORY_LERP_SPEED;
        }

        LIMIT(rsLerpActiveSideCommand, 0, 1);
        LIMIT(rsLerpActiveSpell, 0, 1);
    }

    //@recomp: optionally animate fading the Active Spell/Sidekick Command icons
    if (rConfigActiveIconFade) {
        //Spell icon
        if (rsShowActiveSpell) {
            if (rsOpacityActiveSpell < MAX_OPACITY) {
                rsOpacityActiveSpell += gUpdateRate * ACTIVE_ICON_FADE_IN_SPEED;
                if (rsOpacityActiveSpell > MAX_OPACITY) {
                    rsOpacityActiveSpell = MAX_OPACITY;
                }
            }
        } else {
            if (rsOpacityActiveSpell > 0) {
                rsOpacityActiveSpell -= gUpdateRate * ACTIVE_ICON_FADE_OUT_SPEED;
                if (rsOpacityActiveSpell <= 0) {
                    rsOpacityActiveSpell = 0;
                }
            }
        }

        //Sidekick icon       
        if (rsShowActiveSideCommand && 
            !((rConfigActiveIconOverlapFix == ACTIVEICON_HIDE) && 
                (cmdmenu_is_inventory_closed() == FALSE)
            )
        ) {
            if (rsOpacityActiveSideCommand < MAX_OPACITY) {
                rsOpacityActiveSideCommand += gUpdateRate * ACTIVE_ICON_FADE_IN_SPEED;
                if (rsOpacityActiveSideCommand > MAX_OPACITY) {
                    rsOpacityActiveSideCommand = MAX_OPACITY;
                }
            }
        } else {
            if (rsOpacityActiveSideCommand > 0) {
                //Fade quickly if the inventory's open and the sidekick icon should fade out to handle the inventory clash
                if ((rConfigActiveIconOverlapFix == ACTIVEICON_HIDE) && (cmdmenu_is_inventory_closed() == FALSE)) {
                    rsOpacityActiveSideCommand -= gUpdateRate * ACTIVE_ICON_FADE_OUT_SPEED_QUICK;
                } else {
                    rsOpacityActiveSideCommand -= gUpdateRate * ACTIVE_ICON_FADE_OUT_SPEED;
                }

                if (rsOpacityActiveSideCommand <= 0) {
                    rsOpacityActiveSideCommand = 0;
                }
            }
        }

    } else {
        rsOpacityActiveSpell = (sActiveSpellIcon != NULL) ? MAX_OPACITY : 0;
        rsOpacityActiveSideCommand = (sActiveSidekickCommandIcon != NULL) ? MAX_OPACITY : 0;
    }
}

/** Fix display list desync bug */
RECOMP_PATCH void cmdmenu_draw_info_scroll(Gfx** gdl, Mtx** mtxs, Vertex** vtxs) {
    s32 x;
    s32 y;
    s32 halfWidth;
    s32 height;
    s32 tempY;
    s32 lineIdx;
    Gfx* dl;

    if (dInfoScrollOpacity == 0) {
        return;
    }

    dl = *gdl;

    gDPSetCombineMode(dl, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    dl_apply_combine(&dl);
    gDPSetOtherMode(dl,
        G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_POINT | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | G_PM_NPRIMITIVE, 
        G_AC_NONE | G_ZS_PIXEL | G_RM_XLU_SURF | G_RM_XLU_SURF2);
    dl_apply_other_mode(&dl);
    gSPLoadGeometryMode(dl, G_SHADE | G_SHADING_SMOOTH);
    dl_apply_geometry_mode(&dl);
    dl_set_prim_color(&dl, 255, 255, 255, dInfoScrollOpacity);

    //Get dimensions (note: coords multiplied by 4 for gSPTextureRectangle)
    x = dInfoScrollX << 2;
    halfWidth = dInfoScrollWidthHalf << 2;
    halfWidth -= INFO_SCROLL_PAGE_EDGE_WIDTH << 2; //Subtract the page's edge margin
    y = dInfoScrollY << 2;
    height = sInfoScrollUnrollY << 2;

    //Draw main paper background
    {
        //Middle section (excluding the paper's left/right edges)
        cmdmenu_gfx_set_texture(&dl, sTextures[CMDMENU_TEX_06_InfoScroll_BG], 0);
        gSPTextureRectangle(dl++, 
            /*ulx*/ x - halfWidth,
            /*uly*/ y,
            /*lrx*/ x + halfWidth,
            /*lry*/ y + height,
            /*tile*/ G_TX_RENDERTILE,
            /*s*/ qs105(0), /*t*/ qs105(0), 
            /*dsdx*/ qs510(1), /*dtdy*/ qs510(1)
        );
        gDLBuilder->needsPipeSync = TRUE;

        //Left edge of paper
        cmdmenu_gfx_set_texture(&dl, sTextures[CMDMENU_TEX_05_InfoScroll_Side], 0);
        gSPTextureRectangle(dl++, 
            /*lrx*/ x - halfWidth - (INFO_SCROLL_PAGE_EDGE_WIDTH << 2),
            /*lry*/ y,
            /*ulx*/ x - halfWidth,
            /*uly*/ y + height,
            /*tile*/ G_TX_RENDERTILE,
            /*s*/ qs105(0), /*t*/ qs105(0), 
            /*dsdx*/ qs510(1), /*dtdy*/ qs510(1)
        );
        gDLBuilder->needsPipeSync = TRUE;

        //Right edge of paper
        gSPTextureRectangle(dl++, 
            /*ulx*/ x + halfWidth,
            /*uly*/ y,
            /*lrx*/ x + halfWidth + (INFO_SCROLL_PAGE_EDGE_WIDTH << 2),
            /*lry*/ y + height,
            /*tile*/ G_TX_RENDERTILE,
            /*s*/ qs105(15), /*t*/ qs105(0),
            /*dsdx*/ qs510(-1), /*dtdy*/ qs510(1)
        );
        gDLBuilder->needsPipeSync = TRUE;

        halfWidth += (INFO_SCROLL_PAGE_EDGE_WIDTH << 2); //restore half-width
    }

    //Draw the top/bottom rolls' page shadows
    if (sInfoScrollUnrollY > INFO_SCROLL_PAGE_SHADOW_HEIGHT) {
        cmdmenu_gfx_set_texture(&dl, sTextures[CMDMENU_TEX_07_InfoScroll_SelfShadow], 0);
        dl_set_prim_color(&dl, 255, 128, 128, 128);
        
        //Top shadow
        tempY = y;
        gSPTextureRectangle(dl++, 
            /*ulx*/ x - halfWidth,
            /*uly*/ tempY,
            /*lrx*/ x + halfWidth,
            /*lry*/ tempY + (INFO_SCROLL_PAGE_SHADOW_HEIGHT << 2),
            /*tile*/ G_TX_RENDERTILE,
            /*s*/ qs105(0), /*t*/ qs105(7), 
            /*dsdx*/ qs510(1), /*dtdy*/ qs510(-1)
        );
        gDLBuilder->needsPipeSync = TRUE;

        //Bottom shadow
        tempY = y + height - ((INFO_SCROLL_PAGE_SHADOW_HEIGHT - 2) << 2);
        gSPTextureRectangle(dl++, 
            /*ulx*/ x - halfWidth,
            /*uly*/ tempY,
            /*lrx*/ x + halfWidth,
            /*lry*/ tempY + (INFO_SCROLL_PAGE_SHADOW_HEIGHT << 2),
            /*tile*/ G_TX_RENDERTILE,
            /*s*/ qs105(0), /*t*/ qs105(0), 
            /*dsdx*/ qs510(1), /*dtdy*/ qs510(1)
        );
        gDLBuilder->needsPipeSync = TRUE;

        //Restore prim colour
        dl_set_prim_color(&dl, 255, 255, 255, dInfoScrollOpacity);
    }

    //Draw the top/bottom rolls
    {
        //Top roll (middle span)
        {
            tempY = y - (INFO_SCROLL_ROLL_TOP_Y_OFFSET << 2);
            cmdmenu_gfx_set_texture(
                &dl,
                sTextures[CMDMENU_TEX_04_InfoScroll_Roll], 
                dInfoScrollFrameTop //Unused/scrapped rolling animation (always set to frame 0)
            );
            gSPTextureRectangle(dl++, 
                /*ulx*/ x - halfWidth,
                /*uly*/ tempY,
                /*lrx*/ x + halfWidth,
                /*lry*/ tempY + (INFO_SCROLL_ROLL_HEIGHT << 2),
                /*tile*/ G_TX_RENDERTILE,
                /*s*/ qs105(0), /*t*/ qs105(15.9688), 
                /*dsdx*/ qs510(1), /*dtdy*/ qs510(-1)
            );
            gDLBuilder->needsPipeSync = TRUE;
        }

        //Bottom roll (middle span)
        {
            tempY = y + height - (INFO_SCROLL_ROLL_BOTTOM_Y_OFFSET << 2);
            cmdmenu_gfx_set_texture(&dl, sTextures[CMDMENU_TEX_04_InfoScroll_Roll], dInfoScrollFrameBottom);
            gSPTextureRectangle(dl++, 
                /*ulx*/ x - halfWidth,
                /*uly*/ tempY,
                /*lrx*/ x + halfWidth,
                /*lry*/ tempY + (INFO_SCROLL_ROLL_HEIGHT << 2),
                /*tile*/ G_TX_RENDERTILE,
                /*s*/ qs105(0), /*t*/ qs105(15.9688), 
                /*dsdx*/ qs510(1), /*dtdy*/ qs510(-1)
            );
            gDLBuilder->needsPipeSync = TRUE;
        }

        //Roll handles
        {
            cmdmenu_gfx_set_texture(
                &dl, 
                sTextures[CMDMENU_TEX_03_InfoScroll_Roll_End], 
                0
            );
            
            //Top roll handle (left)
            tempY = y - (INFO_SCROLL_ROLL_TOP_Y_OFFSET << 2);
            gSPTextureRectangle(dl++, 
                /*ulx*/ x - halfWidth - (INFO_SCROLL_HANDLE_WIDTH << 2),
                /*uly*/ tempY,
                /*lrx*/ x - halfWidth,
                /*lry*/ tempY + (INFO_SCROLL_HANDLE_HEIGHT << 2),
                /*tile*/ G_TX_RENDERTILE,
                /*s*/ qs105(0), /*t*/ qs105(15.9688), 
                /*dsdx*/ qs510(1), /*dtdy*/ qs510(-1)
            );
            gDLBuilder->needsPipeSync = TRUE;

            //Top roll handle (right)
            gSPTextureRectangle(dl++, 
                /*ulx*/ x + halfWidth,
                /*uly*/ tempY,
                /*lrx*/ x + halfWidth + (INFO_SCROLL_HANDLE_WIDTH << 2),
                /*lry*/ tempY + (INFO_SCROLL_HANDLE_HEIGHT << 2),
                /*tile*/ G_TX_RENDERTILE,
                /*s*/ qs105(15), /*t*/ qs105(15.9688), 
                /*dsdx*/ qs510(-1), /*dtdy*/ qs510(-1)
            );
            gDLBuilder->needsPipeSync = TRUE;

            //Bottom roll handle (left)
            tempY = y + height - (INFO_SCROLL_ROLL_BOTTOM_Y_OFFSET << 2);
            gSPTextureRectangle(dl++, 
                /*ulx*/ x - halfWidth - (INFO_SCROLL_HANDLE_WIDTH << 2),
                /*uly*/ tempY,
                /*lrx*/ x - halfWidth,
                /*lry*/ tempY + (INFO_SCROLL_HANDLE_HEIGHT << 2),
                /*tile*/ G_TX_RENDERTILE,
                /*s*/ qs105(0), /*t*/ qs105(15.9688), 
                /*dsdx*/ qs510(1), /*dtdy*/ qs510(-1)
            );
            gDLBuilder->needsPipeSync = TRUE;

            //Bottom roll handle (right)
            gSPTextureRectangle(dl++, 
                /*ulx*/ x + halfWidth,
                /*uly*/ tempY,
                /*lrx*/ x + halfWidth + (INFO_SCROLL_HANDLE_WIDTH << 2),
                /*lry*/ tempY + (INFO_SCROLL_HANDLE_HEIGHT << 2),
                /*tile*/ G_TX_RENDERTILE,
                /*s*/ qs105(15), /*t*/ qs105(15.9688), 
                /*dsdx*/ qs510(-1), /*dtdy*/ qs510(-1)
            );
            gDLBuilder->needsPipeSync = TRUE;
        }
    }

    //Restore prim colour
    dl_set_prim_color(&dl, 255, 255, 255, 255);

    //Return early if there's no gametext
    if (dInfoScrollTextID <= NO_GAMETEXT) {
        *gdl = dl; //@bug: fix display list desync bug
        return;
    }

    //Get the gametext lines
    if (dInfoScrollStrings[0] == NULL) {
        //Get the item's string (use a different gametext file for textIDs beyond 255) 
        dInfoScrollStrings[0] = dInfoScrollTextID >= 256 
            ? gDLL_21_Gametext->vtbl->get_text(GAMETEXT_238_UI_Text_2, dInfoScrollTextID - 256) 
            : gDLL_21_Gametext->vtbl->get_text(GAMETEXT_003_UI_Text_1, dInfoScrollTextID);
        dInfoScrollStrings[1] = NULL;
        dInfoScrollStrings[2] = NULL;
        dInfoScrollStrings[3] = NULL;

        //Check for bar delimiter in text (separates lines)
        for (x = 0, lineIdx = 1; dInfoScrollStrings[0][x] != '\0'; x++) {
            if (dInfoScrollStrings[0][x] == '|') {
                dInfoScrollStrings[0][x] = '\0';
                x += 2; //ROM's text should have space after bar ("| "), so skipping over it
                
                dInfoScrollStrings[lineIdx] = &dInfoScrollStrings[0][x]; //store address of new line
                lineIdx++;
            }            
        }
    }

    //Draw text if the scroll's open
    if (sInfoScrollUnrollY > 0) {
        font_window_set_coords(3, 
            /*x1*/ dInfoScrollX - dInfoScrollWidthHalf, 
            /*y1*/ dInfoScrollY, 
            /*x2*/ dInfoScrollX + dInfoScrollWidthHalf, 
            /*y2*/ dInfoScrollY + sInfoScrollUnrollY);
        font_window_use_font(3, FONT_DINO_SUBTITLE_FONT_1);
        font_window_set_bg_colour(3, 0, 0, 0, 0);
        font_window_flush_strings(3);
        font_window_set_text_colour(3, 0, 0, 255, 255, 255);

        //Print the text lines
        for (y = INFO_SCROLL_TEXT_Y, lineIdx = 0; lineIdx <= 3; lineIdx++) {
            if (dInfoScrollStrings[lineIdx] == NULL) {
                break;
            }
            font_window_add_string_xy(3, -0x8000, y, dInfoScrollStrings[lineIdx], 1, ALIGN_TOP_CENTER);
            font_window_set_text_colour(3, 20, 20, 20, 255, 255);
            font_window_use_font(3, FONT_DINO_SUBTITLE_FONT_1);
            y += INFO_SCROLL_LINE_HEIGHT;
        }

        font_window_draw(&dl, mtxs, vtxs, 3);
    }

    *gdl = dl;
}

/** Hijack the print function to allow patching of `cmdmenu_draw_main`
  * (Base Recomp already patches `cmdmenu_draw_main`, 
  * and the nearest relevant export we can hijack is `cmdmenu_print`) */
typedef s32 (*CmdmenuPrint)(Gfx **gdl, s32 arg1);
static CmdmenuPrint print_func; 
static void cmdmenu_print_custom(Gfx** gdl, Mtx** mtxs, Vertex** vtxs);
static void cmdmenu_draw_main_custom(Gfx** gdl, Mtx** mtxs, Vertex** vtxs);

RECOMP_HOOK_DLL(cmdmenu_ctor) void cmdmenu_ctor_hook(DLLFile *dll) {
    print_func = dinomod_hijack_dll_export(dll, 2, cmdmenu_print_custom);
}

RECOMP_HOOK_RETURN_DLL(cmdmenu_dtor) void cmdmenu_dtor_hook() {
    print_func = NULL;
}

/**
  * - Duplicates Shinx's base recomp widescreen HUD patches, allowing more patches to be added on top.
  * - Ports the ROM patches' widescreen reticle aiming fix (not used in recomp itself).
  */
static void cmdmenu_print_custom(Gfx** gdl, Mtx** mtxs, Vertex** vtxs) {
    Object* player;
    s32 viSize;
    s32 screenX;
    s32 screenY;

    player = get_player();
    if (player == NULL) {
        return;
    }

    // @recomp: Fullscreen scissors
    #ifndef DINOMOD_ROM_PATCH
    gEXSetScissorAlign((*gdl)++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, 0, -SCREEN_WIDTH, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    #endif

    //Draw Spell reticle when aiming (@bug: x coord not adjusted in widescreen)
    if (((DLL_210_Player*)player->dll)->vtbl->func77(player, &screenX, &screenY)) {
        tex_animate(sCrosshairTex, &sCrosshairAnimRenderFlags, &sCrosshairAnimProgress);
        
        #ifdef DINOMOD_ROM_PATCH
        {
            if (D_8008C890) {
                //Widescreen aspect
                rcp_screen_full_write(
                    gdl, 
                    sCrosshairTex, 
                    (u32)(screenX*0.75f) - (AIMING_RETICLE_WIDTH/2) + 43, 
                    screenY - (AIMING_RETICLE_HEIGHT/2), 
                    0, 
                    sCrosshairAnimProgress >> 8, 
                    AIMING_RETICLE_OPACITY, 
                    SCREEN_WRITE_TRANSLUCENT
                );
            } else {
                //Standard aspect
                rcp_screen_full_write(
                    gdl, 
                    sCrosshairTex, 
                    screenX - (AIMING_RETICLE_WIDTH/2), 
                    screenY - (AIMING_RETICLE_HEIGHT/2), 
                    0, 
                    sCrosshairAnimProgress >> 8, 
                    AIMING_RETICLE_OPACITY, 
                    SCREEN_WRITE_TRANSLUCENT
                );
            }
        }
        #else
        rcp_screen_full_write(
            gdl, 
            sCrosshairTex, 
            screenX - (AIMING_RETICLE_WIDTH/2), 
            screenY - (AIMING_RETICLE_HEIGHT/2), 
            0, 
            sCrosshairAnimProgress >> 8, 
            AIMING_RETICLE_OPACITY, 
            SCREEN_WRITE_TRANSLUCENT
        );
        #endif
    }

    cmdmenu_draw_player_stats_custom(gdl, mtxs, vtxs);

    viSize = vi_get_current_size();
    gDPSetScissor((*gdl)++, G_SC_NON_INTERLACE, 
        0, 
        0, 
        GET_VIDEO_WIDTH(viSize), 
        GET_VIDEO_HEIGHT(viSize));

    // @recomp: Align C buttons/sidekick meter to right
    #ifndef DINOMOD_ROM_PATCH
    gEXSetViewportAlign((*gdl)++, G_EX_ORIGIN_RIGHT, -SCREEN_WIDTH * 4, 0);
    gEXSetRectAlign((*gdl)++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, -SCREEN_WIDTH * 4, 0, -SCREEN_WIDTH * 4, 0);
    #endif

    cmdmenu_draw_c_buttons_and_sidekick_meter(gdl, mtxs, vtxs);

    // @recomp: Reset alignment
    #ifndef DINOMOD_ROM_PATCH
    gEXSetRectAlign((*gdl)++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
    gEXSetViewportAlign((*gdl)++, G_EX_ORIGIN_NONE, 0, 0);
    #endif

    cmdmenu_draw_info_scroll(gdl, mtxs, vtxs);
    cmdmenu_draw_tutorial_textbox(gdl, mtxs, vtxs);
    cmdmenu_draw_main_custom(gdl, mtxs, vtxs); //@recomp
    camera_apply_scissor(gdl);

    // @recomp: Reset scissor align
    #ifndef DINOMOD_ROM_PATCH
    gEXSetScissorAlign((*gdl)++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    #endif
}

/**
  * Hides the active spell/sidekick-command icons if an important sequence is playing.
  */
static void cmdmenu_hide_active_spell_sidekick_command_during_sequences() {
    Player_Data* playerData;
    Object* player = get_player();
    if (!player || !player->data) {
        return;
    }

    playerData = player->data;

    //Check if the letterbox is active, and the player is either in a sequence or locked
    if (camera_get_letterbox() && (
            (player->stateFlags & OBJSTATE_IN_SEQ) ||                                       //Player involved in sequence
            (!(player->stateFlags & OBJSTATE_IN_SEQ) && (playerData->flags & 0x200000))     //Player not in sequence, but locked
        )
    ) {
        rsOpacityActiveSpell = 0;
        rsOpacityActiveSideCommand = 0;
    }
}

/** 
  * - Fix sidekick icon appearing half-way through fading out from exiting Items/Spells page.
  * - Optionally move/fade the Active Spell/Sidekick Command icons to avoid clashing with the inventory.
  */
static void cmdmenu_draw_main_custom(Gfx** gdl, Mtx** mtxs, Vertex** vtxs) {
    s16 commandTexTableID;
    Object* player;
    s32 activeSpellGamebit;
    s32 stripY;
    s32 spellTexTableID;
    s32 numSlotsAboveSelected;
    s8 slot[MAX_LOADED_ITEMS];
    s32 itemIdx;
    s32 i;
    s32 sideCommandIndex;
    s32 tileCount;
    u8 iconOpacity;
    u8 pageIcon = 0;
    s8 offsetX;
    s8 offsetY;
    Object* sidekick;
    /* RECOMP */
    s32 rNumSlotsPaddedAtTop = 0;
    s32 rTotalOccupiedSlots = 0;
    s32 rActiveIconCoord = 0;
    u8 rConfigActiveIconOverlapFix = recomp_get_config_u32("cmdmenu_active_icon_overlap");
    u8 rConfigActiveIconFade = recomp_get_config_u32("cmdmenu_active_icon_fade");

    player = get_player();
    offsetX = 0;
    offsetY = 0;
    sidekick = get_sidekick();

    // @recomp: Align item popup to left
    #ifndef DINOMOD_ROM_PATCH
    gEXSetViewportAlign((*gdl)++, G_EX_ORIGIN_LEFT, 0, 0);
    gEXSetRectAlign((*gdl)++, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_LEFT, 0, 0, 0, 0);
    #endif

    //Call the item info pop-up's draw
    cmdmenu_info_draw(gdl, &sInfoPopup);

    // @recomp: Align active spell/active sidekick command to right
    #ifndef DINOMOD_ROM_PATCH
    gEXSetViewportAlign((*gdl)++, G_EX_ORIGIN_RIGHT, -SCREEN_WIDTH * 4, 0);
    gEXSetRectAlign((*gdl)++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, -SCREEN_WIDTH * 4, 0, -SCREEN_WIDTH * 4, 0);
    #endif

    // @recomp: Hide the active Spell/Sidekick-command icons if an important sequence is playing
    cmdmenu_hide_active_spell_sidekick_command_during_sequences();

    //Draw active spell icon
    {
        activeSpellGamebit = ((DLL_210_Player*)player->dll)->vtbl->func50(player);

        //@recomp: handle initial value
        if (sPrevActiveSpellGamebit == 0) {
            sPrevActiveSpellGamebit = NO_GAMEBIT;
        }

        //Clear the icon's data before any change
        if ((sActiveSpellIcon != NULL) && (activeSpellGamebit != sPrevActiveSpellGamebit)) {
            //@recomp: handle optional icon fading
            if (activeSpellGamebit == NO_GAMEBIT) {
                rsShowActiveSpell = FALSE;
            }

            if (!rConfigActiveIconFade || (activeSpellGamebit != NO_GAMEBIT) || (rsOpacityActiveSpell == 0)) {
                tex_free(sActiveSpellRing);
                tex_free(sActiveSpellIcon);
                sPrevActiveSpellGamebit = NO_GAMEBIT;
                sActiveSpellIcon = NULL;
            }
        }

        //Load the icon's textures when needed
        if ((sActiveSpellIcon == NULL) && (activeSpellGamebit != NO_GAMEBIT)) {
            rsShowActiveSpell = TRUE; //@recomp: handle optional icon fading

            //@recomp: optionally start at shifted position if inventory already open and active Sidekick Command visible
            if ((dInventoryOpacity > 64) && sActiveSidekickCommandIcon && (rsOpacityActiveSpell == 0)) {
                rsLerpActiveSpell = 1;
            }

            spellTexTableID = cmdmenu_get_spell_textable(activeSpellGamebit);
            if (spellTexTableID != NO_TEXTURE) {
                sActiveSpellIcon = tex_load_deferred(spellTexTableID);
                sActiveSpellRing = tex_load_deferred(TEXTABLE_574_CMDMENU_Active_Spell_Ring);
            }

            sPrevActiveSpellGamebit = activeSpellGamebit; //@recomp: only set this when icon is shown
        }

        //@recomp: handle options for moving/fading icon
        if (sActiveSpellIcon != NULL) {
            switch (rConfigActiveIconOverlapFix) {
            case ACTIVEICON_MOVE:
                rActiveIconCoord = lerp_float(ease_in_out_quad(rsLerpActiveSpell), ACTIVE_SPELL_X, ACTIVE_SPELL_X - 48);

                rcp_screen_full_write(gdl, sActiveSpellRing, rActiveIconCoord,                                  ACTIVE_SPELL_Y,      0, 0, rsOpacityActiveSpell, SCREEN_WRITE_TRANSLUCENT);
                rcp_screen_full_write(gdl, sActiveSpellIcon, (rActiveIconCoord + ACTIVE_SPELL_ICON_OFFSET_X),   ACTIVE_SPELL_ICON_Y, 0, 0, rsOpacityActiveSpell, SCREEN_WRITE_TRANSLUCENT);
                break;
            default:
                rcp_screen_full_write(gdl, sActiveSpellRing, ACTIVE_SPELL_X,      ACTIVE_SPELL_Y,      0, 0, rsOpacityActiveSpell, SCREEN_WRITE_TRANSLUCENT);
                rcp_screen_full_write(gdl, sActiveSpellIcon, ACTIVE_SPELL_ICON_X, ACTIVE_SPELL_ICON_Y, 0, 0, rsOpacityActiveSpell, SCREEN_WRITE_TRANSLUCENT);
                break;
            }
        }
    }

    //Draw active sidekick command icon
    {
        if (sidekick != NULL) {
            // @bug: sideCommandIndex is undefined if this sidekick func returns 0
#ifndef AVOID_UB
            ((DLL_ISidekick*)sidekick->dll)->vtbl->func26(sidekick, &sideCommandIndex);
#else
            if (!((DLL_ISidekick*)sidekick->dll)->vtbl->func26(sidekick, &sideCommandIndex)) {
                sideCommandIndex = sPrevSidekickCommandIndex;
            }
#endif
        } else {
            sideCommandIndex = NO_SIDEKICK_COMMAND;
        }

        //Clear the icon's data before any change
        if ((sActiveSidekickCommandIcon != NULL) && (sideCommandIndex != sPrevSidekickCommandIndex)) {
            //@recomp: handle optional icon fading
            if (sideCommandIndex <= 0) {
                rsShowActiveSideCommand = FALSE;
            }

            if (!rConfigActiveIconFade || (sideCommandIndex > 0) || (rsOpacityActiveSideCommand == 0)) {
                tex_free(sActiveSidekickCommandRing);
                tex_free(sActiveSidekickCommandIcon);
                sPrevSidekickCommandIndex = NO_SIDEKICK_COMMAND;
                sActiveSidekickCommandIcon = NULL;
            }
        }

        //Load the icon's textures when needed
        if ((sActiveSidekickCommandIcon == NULL) && (sideCommandIndex > 0)) {
            rsShowActiveSideCommand = TRUE; //@recomp: handle optional icon fading

            //@recomp: optionally start at shifted position if inventory already open
            if ((dInventoryOpacity > 64) && (rsOpacityActiveSideCommand == 0)) {
                rsLerpActiveSideCommand = 1;
            }

            commandTexTableID = dCommandTextableIDs[sideCommandIndex];
            if (commandTexTableID != NO_TEXTURE) {
                sActiveSidekickCommandIcon = tex_load_deferred(commandTexTableID);
                sActiveSidekickCommandRing = tex_load_deferred(TEXTABLE_584_CMDMENU_Active_Sidekick_Command_Ring);
            }

            sPrevSidekickCommandIndex = sideCommandIndex; //@recomp: only set this when icon is shown
        }

        //@recomp: handle options for moving/fading icon
        if (sActiveSidekickCommandIcon != NULL) {
            switch (rConfigActiveIconOverlapFix) {
            case ACTIVEICON_MOVE:
                rActiveIconCoord = lerp_float(ease_in_out_quad(rsLerpActiveSpell), ACTIVE_SIDECOMMAND_Y, ACTIVE_SPELL_Y);

                rcp_screen_full_write(gdl, sActiveSidekickCommandRing, ACTIVE_SIDECOMMAND_X,      rActiveIconCoord,                                0, 0, rsOpacityActiveSideCommand, SCREEN_WRITE_TRANSLUCENT);
                rcp_screen_full_write(gdl, sActiveSidekickCommandIcon, ACTIVE_SIDECOMMAND_ICON_X, (rActiveIconCoord + ACTIVE_SPELL_ICON_OFFSET_Y), 0, 0, rsOpacityActiveSideCommand, SCREEN_WRITE_TRANSLUCENT);
                break;
            case ACTIVEICON_HIDE:
                //Hide Sidekick Command icon when inventory open
                if (!rConfigActiveIconFade && (cmdmenu_is_inventory_closed() == FALSE)) {
                    break;
                }
            default:
                rcp_screen_full_write(gdl, sActiveSidekickCommandRing, ACTIVE_SIDECOMMAND_X,      ACTIVE_SIDECOMMAND_Y,      0, 0, rsOpacityActiveSideCommand, SCREEN_WRITE_TRANSLUCENT);
                rcp_screen_full_write(gdl, sActiveSidekickCommandIcon, ACTIVE_SIDECOMMAND_ICON_X, ACTIVE_SIDECOMMAND_ICON_Y, 0, 0, rsOpacityActiveSideCommand, SCREEN_WRITE_TRANSLUCENT);
                break;
            }
        }
    }

    // @recomp: Reset alignment for energy bar
    #ifndef DINOMOD_ROM_PATCH
    gEXSetRectAlign((*gdl)++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
    gEXSetViewportAlign((*gdl)++, G_EX_ORIGIN_NONE, 0, 0);
    #endif

    //Call the energy bar's draw
    cmdmenu_draw_energy_bar(gdl);

    // @recomp: Align command menu to right
    #ifndef DINOMOD_ROM_PATCH
    gEXSetViewportAlign((*gdl)++, G_EX_ORIGIN_RIGHT, -SCREEN_WIDTH * 4, 0);
    gEXSetRectAlign((*gdl)++, G_EX_ORIGIN_RIGHT, G_EX_ORIGIN_RIGHT, -SCREEN_WIDTH * 4, 0, -SCREEN_WIDTH * 4, 0);
    #endif

    //@recomp: draw top of the scroll here instead (so icons draw on top of it)
    #ifdef FIX_ICON_STRIP_PIXEL_ROW_ONE
    if (dInventoryOpacity != 0) {
        rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_02_Scroll_Top],    MENU_SCROLL_X, MENU_SCROLL_TOP_Y,                        255, 255, 255, dInventoryOpacity);
    }
    #endif

    //Draw the inventory's vertical icon strip
    //(i.e. every part of the scroll except its top/bottom rolls)
    if (sDisplayedItemCount != 0) {
        if (dInventoryOpacity != 0) {
            /* 
                Figure out the number of tiles to draw (minimum of 3).

                Empty tiles are inserted to pad out the strip when 
                there're only 1/2/4 items shown on the page.
            */
            tileCount = 3;
            if (sDisplayedItemCount <= 3) {
                numSlotsAboveSelected = 1;
            } else {
                tileCount = 5;
                numSlotsAboveSelected = 2;
            }

            //Set a scissor mask for the inner strip of the inventory scroll
            cmdmenu_gfx_set_scroll_scissor(gdl);

            sTempIcon->y = 0;

            //Calculate the screen Y-coord of the top of the tile strip
            #ifdef FIX_ICON_STRIP_PIXEL_ROW_ONE
            stripY = MENU_SCROLL_CENTRE_Y - (tileCount * (MENU_ITEM_HEIGHT/2)) - 1; //@recomp: move up by 1 pixel
            #else
            stripY = MENU_SCROLL_CENTRE_Y - (tileCount * (MENU_ITEM_HEIGHT/2));
            #endif

            sTempIcon[1].tex = NULL;

            //Set the slots that show icons
            for (i = 0; i < sDisplayedItemCount; i++) {
                slot[i] = SLOT_OCCUPIED;
            }
            //Set the padded slots (empty BG tiles)
            for (i = sDisplayedItemCount; i < tileCount; i++) {
                slot[i] = SLOT_PADDED;
            }

            #if DEBUG_INVENTORY_SCROLLING
            diPrintf("Total items on page: %d\n", sDisplayedItemCount);
            #endif

            //Change sDisplayedItemCount so it's at least the size of the tile strip (including empty slots)
            if (sDisplayedItemCount < tileCount) {
                sDisplayedItemCount = tileCount;
            }

            //While animating moving between items
            {
                //Shift the top of the strip up by 1 item
                if (sInventoryScrollOffset > 0) {
                    numSlotsAboveSelected++;
                    stripY -= MENU_ITEM_HEIGHT;
                    tileCount++;

                    //Or shift it up by 2 during large offsets (when wrapping from bottom-to-top)
                    if (sInventoryScrollOffset > MENU_ITEM_HEIGHT) {
                        numSlotsAboveSelected++;
                        stripY -= MENU_ITEM_HEIGHT;
                        tileCount++;
                    }
                } else if (sInventoryScrollOffset < 0) { //@recomp: handle scrolling up as well
                    numSlotsAboveSelected--;
                    rNumSlotsPaddedAtTop++;
                    stripY += MENU_ITEM_HEIGHT;
                    tileCount++;

                    //Or shift it down by 2 during large offsets (when wrapping from top-to-bottom)
                    if (sInventoryScrollOffset < -MENU_ITEM_HEIGHT) {
                        numSlotsAboveSelected--;
                        rNumSlotsPaddedAtTop++;
                        stripY += MENU_ITEM_HEIGHT;
                        tileCount++;
                    }
                }
            }

            //Have the strip move with the bottom of the scroll (during its expanding/collapsing animation)
            stripY += sInventoryUnrollY - dInventoryUnrollMax;

            //Calculate the item index of the uppermost slot in the strip
            itemIdx = sMenuSelectedItemIdx - numSlotsAboveSelected - rNumSlotsPaddedAtTop;
            if (itemIdx < 0) {
                itemIdx += sDisplayedItemCount;
            } else if (itemIdx >= sDisplayedItemCount) { //@recomp
                itemIdx -= sDisplayedItemCount;
            }

            //Iterate down the strip, drawing the slots' tiles
            for (i = 0; i < tileCount; i++) {
                if (slot[itemIdx] == SLOT_OCCUPIED) {
                    sTempIcon->animProgress = 0;
                    iconOpacity = dInventoryOpacity;

                    //Shift the selected item's tile when scrolling's finished (does nothing: cases identical)
                    //May suggest Rare considered lifting/popping out the selected item slightly
                    if ((itemIdx == sMenuSelectedItemIdx) && (sInventoryScrollOffset == 0)) {
                        sTempIcon->x = 0;
                        sTempIcon->y = 0;
                    } else {
                        sTempIcon->x = 0;
                        sTempIcon->y = 0;
                    }

                    if (sMenuItemVisibilities[itemIdx] != FALSE) {
                        sTempIcon->tex = sMenuItemTextures[itemIdx];

                        //Draw icon
                        if (track_func_80041E08()) {
                            //Widescreen aspect
                            rcp_tile_write(
                                gdl, 
                                sTempIcon, 
                                MENU_ITEM_X - 1, 
                                stripY + ((i - rNumSlotsPaddedAtTop) * MENU_ITEM_HEIGHT) + sInventoryScrollOffset, 
                                0xFF, 0xFF, 0xFF, iconOpacity
                            );
                        } else {
                            //Standard aspect
                            rcp_tile_write(
                                gdl, 
                                sTempIcon, 
                                MENU_ITEM_X, 
                                stripY + ((i - rNumSlotsPaddedAtTop) * MENU_ITEM_HEIGHT) + sInventoryScrollOffset, 
                                0xFF, 0xFF, 0xFF, iconOpacity
                            );
                        }

                        //Draw quantity text (for stackable items)
                        if (sMenuItemQuantities[itemIdx] > 1) {
                            sTempIcon->tex = sInventoryStackNumbersTex;
                            sTempIcon->animProgress = (sMenuItemQuantities[itemIdx] - 2) << 8; //Numbers only shown from 2 onwards (up to 10)
                            rcp_tile_write(
                                gdl, 
                                sTempIcon, 
                                MENU_ITEM_QUANTITY_X, 
                                stripY + ((i - rNumSlotsPaddedAtTop) * MENU_ITEM_HEIGHT) + sInventoryScrollOffset + MENU_ITEM_QUANTITY_OFFSET_Y, 
                                0xFF, 0xFF, 0xFF, iconOpacity //@recomp: use opacity
                            );
                        }
                    }
                } else {
                    //Draw empty tile
                    if (track_func_80041E08()) {
                        //Widescreen aspect
                        rcp_tile_write(
                            gdl, 
                            sTextureTiles[CMDMENU_TEX_00_Scroll_BG], 
                            MENU_ITEM_X - 1, 
                            stripY + ((i - rNumSlotsPaddedAtTop) * MENU_ITEM_HEIGHT) + sInventoryScrollOffset, 
                            0xFF, 0xFF, 0xFF, dInventoryOpacity //@recomp: use opacity
                        );
                    } else {
                        //Standard aspect
                        rcp_tile_write(
                            gdl, 
                            sTextureTiles[CMDMENU_TEX_00_Scroll_BG], 
                            MENU_ITEM_X, 
                            stripY + ((i - rNumSlotsPaddedAtTop) * MENU_ITEM_HEIGHT) + sInventoryScrollOffset, 
                            0xFF, 0xFF, 0xFF, dInventoryOpacity //@recomp: use opacity
                        );
                    }
                }

                //Increment/wrap the item index
                itemIdx++;
                if (itemIdx >= sDisplayedItemCount) {
                    itemIdx -= sDisplayedItemCount;
                }
            }

            //Draw a selection square around the currently highlighted item
            rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_31_Highlight_Corner_Top_Left],     ITEM_HL_X1, (sInventoryUnrollY - dInventoryUnrollMax) + ITEM_HL_Y1, 255, 255, 255, dInventoryOpacity);
            rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_32_Highlight_Corner_Top_Right],    ITEM_HL_X2, (sInventoryUnrollY - dInventoryUnrollMax) + ITEM_HL_Y1, 255, 255, 255, dInventoryOpacity);
            rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_33_Highlight_Corner_Bottom_Left],  ITEM_HL_X1, (sInventoryUnrollY - dInventoryUnrollMax) + ITEM_HL_Y2, 255, 255, 255, dInventoryOpacity);
            rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_34_Highlight_Corner_Bottom_Right], ITEM_HL_X2, (sInventoryUnrollY - dInventoryUnrollMax) + ITEM_HL_Y2, 255, 255, 255, dInventoryOpacity);
            
            //Restore full-screen scissor
            cmdmenu_gfx_set_screen_scissor(gdl);
        }

        #ifdef FIX_ICON_STRIP_PIXEL_ROW_ONE
        //@recomp: draw bottom of the scroll here instead
        if (dInventoryOpacity != 0) {
            rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_01_Scroll_Bottom], 
                MENU_SCROLL_X, 
                MENU_SCROLL_BOTTOM_Y + sInventoryUnrollY - 1, //@recomp: 1px higher
                255, 255, 255, dInventoryOpacity);
        }
        #else
        //@recomp: Draw top & bottom of the scroll here instead
        if (dInventoryOpacity != 0) {
            rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_02_Scroll_Top],    MENU_SCROLL_X, MENU_SCROLL_TOP_Y,                        255, 255, 255, dInventoryOpacity);
            rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_01_Scroll_Bottom], MENU_SCROLL_X, MENU_SCROLL_BOTTOM_Y + sInventoryUnrollY, 255, 255, 255, dInventoryOpacity);
        }
        #endif

        //Get page icon (Bag/SpellBook/Kyte/Tricky)
        if (dInventoryShow || 
            dInventoryOpacity == MAX_OPACITY || 
            (dInventoryOpacity != 0 && dOpacitySidekickMeter == 0)
        ) {
            switch (sInventoryPageID) {
            case CMDMENU_PAGE_7_Sidekick_Tricky:
                pageIcon = CMDMENU_TEX_42_Tricky;
                offsetY = 3;
                break;
            case CMDMENU_PAGE_8_Sidekick_Kyte:
                pageIcon = CMDMENU_TEX_54_Kyte;
                break;
            case CMDMENU_PAGE_6_Spells:
                offsetX = -2;
                offsetY = 9;
                pageIcon = CMDMENU_TEX_49_MagicBook;
                break;
            default:
            case CMDMENU_PAGE_0_Items_Krystal:
            case CMDMENU_PAGE_1_Items_Sabre:
            case CMDMENU_PAGE_2_Food_Actions_Krystal:
            case CMDMENU_PAGE_3_Food_Actions_Sabre:
            case CMDMENU_PAGE_4_Food_Krystal:
            case CMDMENU_PAGE_5_Food_Sabre:
                offsetX = 1;
                offsetY = 9;
                pageIcon = CMDMENU_TEX_50_Bag;
                break;
            }

            if (dOpacitySidekickMeter < dInventoryOpacity) {
                iconOpacity = dInventoryOpacity;
            } else {
                iconOpacity = dOpacitySidekickMeter;
            }
        } else {
            //Show sidekick's icon when the sidekick meter should be visible
            //@bug: can suddenly switch to sidekick icon halfway through fading out from bag
            if ((dOpacitySidekickMeter != 0) && (sidekick != NULL)) {
                if (sidekick != NULL && sidekick->id == OBJ_Kyte) {
                    pageIcon = CMDMENU_TEX_54_Kyte;
                    iconOpacity = dOpacitySidekickMeter;
                } else {
                    offsetY = 3;
                    pageIcon = CMDMENU_TEX_42_Tricky;
                    iconOpacity = dOpacitySidekickMeter;
                }
            } else {
                iconOpacity = 0;
            }
        }

        //Draw page icon
        if (iconOpacity && pageIcon) {
            dInventoryPageIcon = tex_load_deferred(dTextableIDs[pageIcon]);
            rcp_screen_full_write(
                gdl, 
                dInventoryPageIcon, 
                PAGE_ICON_X + offsetX,
                PAGE_ICON_Y + offsetY,
                0, 
                0, 
                iconOpacity, 
                SCREEN_WRITE_TRANSLUCENT
            );
            tex_free(dInventoryPageIcon);
        }
    
        #if DEBUG_INVENTORY_SCROLLING
        if (dInventoryOpacity) {
            diPrintf("numSlotsAboveSelected: %d\n", numSlotsAboveSelected);
            diPrintf("rNumSlotsPaddedAtTop: %d\n", rNumSlotsPaddedAtTop);
            diPrintf("tileCount: %d\n", tileCount);
            diPrintf("sInventoryScrollOffset: %d\n", sInventoryScrollOffset);
        }
        #endif
    }

    // @recomp: Reset alignment
    #ifndef DINOMOD_ROM_PATCH
    gEXSetRectAlign((*gdl)++, G_EX_ORIGIN_NONE, G_EX_ORIGIN_NONE, 0, 0, 0, 0);
    gEXSetViewportAlign((*gdl)++, G_EX_ORIGIN_NONE, 0, 0);
    #endif
}

#ifdef FIX_ICON_STRIP_PIXEL_ROW_ONE
RECOMP_PATCH void cmdmenu_gfx_set_scroll_scissor(Gfx **gdl) {
    if (track_func_80041E08()) {
        //Widescreen aspect
        gDPSetScissorFrac((*gdl)++, G_SC_NON_INTERLACE, 
            qu102(314.75), 
            qu102(48.25), //TODO: N64 widescreen fix
            qu102(366.75), 
            ((sInventoryUnrollY * 0.82f) + 48.38f + 2.0f) * 4.0f  //TODO: N64 widescreen fix
        );
    } else {
        //Standard aspect
        gDPSetScissor((*gdl)++, G_SC_NON_INTERLACE, 
            MENU_ITEM_X, 
            MENU_ITEM_Y - 1, //@recomp: move up by 1 pixel, to fix 1px row at top of strip where icons disappear
            MENU_ITEM_X + MENU_ITEM_WIDTH, 
            sInventoryUnrollY + MENU_ITEM_Y + 1 //@recomp: move up by 1 pixel
        );
    }
}
#endif

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

/**
  * Checks if the selected inventory item is a sidekick food item
  * (for showing the sidekick meter while food is selected).
  */
static int cmdmenu_is_selected_item_sidekick_food(int isForTricky) {
    switch (sMenuItemGamebits[sMenuSelectedItemIdx]) {
        //Tricky's food
        case BIT_Inventory_Blue_Mushrooms:
        case BIT_Dino_Bag_Blue_Mushrooms:
        case BIT_Dino_Bag_Old_Mushrooms:
        case BIT_Dino_Bag_Red_Mushrooms:
            return isForTricky;
        //Kyte's food
        case BIT_CloudRunner_Grubs:
        case BIT_Dino_Bag_Blue_Grubs:
        case BIT_Dino_Bag_Old_Grubs:
        case BIT_Dino_Bag_Red_Grubs:
            return !isForTricky;
    }

    return FALSE;
}

/** Checks if the sidekick meter should appear while a sidekick food item is selected in the inventory */
static int cmdmenu_should_sidekick_meter_appear_over_food_items(Object* sidekick) {
    if (sidekick == NULL) {
        return FALSE;
    }
    
    if (recomp_get_config_u32("cmdmenu_sidekick_meter_show_over_food") == FALSE) {
        return FALSE;
    }
        
    if (dPageCategory != CMDMENU_CATEGORY_3_Items && dPageCategory != CMDMENU_CATEGORY_7_Sidekick_Food) {
        return FALSE;
    }
    
    return (cmdmenu_is_selected_item_sidekick_food(sidekick->id == OBJ_Tricky));
}

RECOMP_PATCH void cmdmenu_draw_c_buttons_and_sidekick_meter(Gfx** gdl, Mtx** mtxs, Vertex** vtxs) {
    Gfx* dl;
    Object* sidekick;
    Object* player;
    u8 texIdx;
    u8 pad;
    u8 isKyte;
    u8 pageIdx;
    u8 hasHalfRed;
    u8 fullBlueEnd;
    u8 hasHalfBlue;
    u8 cIconFlags;
    u8 fullRedEnd;
    u8 i;
    u8 iconIndex;
    /* RECOMP */
    SidekickStats* dinoStats;
    u8 isSidekickFoodSelected;

    sidekick = get_sidekick();
    player = get_player();
    isKyte = FALSE;
    cIconFlags = CIcon_FLAG_None;

    if (sidekick != NULL) {
        isKyte = (sidekick->id == OBJ_Kyte);
    }

    //Draw C buttons (only while the inventory scroll is hidden)
    if (dInventoryOpacity == 0) {
        if (sOpacityR != 0.0f) {
            //Check whether each C button has items to show
            {
                //Check if player has inventory items
                if (player != NULL) {
                    pageIdx = player->id == OBJ_Krystal ? CMDMENU_PAGE_0_Items_Krystal : CMDMENU_PAGE_1_Items_Sabre;
                    cIconFlags = CIcon_FLAG_None;
                    if (cmdmenu_page_count_shown_items(dCmdmenuPages[pageIdx].items, FALSE) != 0) {
                        cIconFlags = CIcon_FLAG_Have_Items;
                    }
                }

                //Check if Sidekick Commands are available
                if (sidekick != NULL) {
                    pageIdx = sidekick->id == OBJ_Kyte ? CMDMENU_PAGE_8_Sidekick_Kyte : CMDMENU_PAGE_7_Sidekick_Tricky;
                    if (cmdmenu_page_count_shown_items(dCmdmenuPages[pageIdx].items, TRUE) != 0) {
                        cIconFlags |= CIcon_FLAG_Have_Sidekick_Commands;
                    }
                }

                //Check if Spells are available
                if (cmdmenu_page_count_shown_items(dCmdmenuPages[CMDMENU_PAGE_6_Spells].items, FALSE) != 0) {
                    cIconFlags |= CIcon_FLAG_Have_Spells;
                }
            }

            //Draw C-right button
            {
                if (cIconFlags & CIcon_FLAG_Have_Items) {
                    //With inventory bag
                    texIdx = CMDMENU_TEX_47_RightButton_With_Bag;
                    dInventoryPageIcon = tex_load_deferred(dTextableIDs[texIdx]);
                    rcp_screen_full_write(gdl, 
                        dInventoryPageIcon, 
                        C_BUTTONS_RIGHT_BAG_X, 
                        C_BUTTONS_RIGHT_BAG_Y, 
                        0, 
                        0, 
                        sOpacityR, 
                        SCREEN_WRITE_TRANSLUCENT
                    );
                    tex_free(dInventoryPageIcon);
                } else {
                    //Empty C-right button
                    texIdx = CMDMENU_TEX_41_C_Right;
                    rcp_tile_write(
                        gdl,
                        sTextureTiles[texIdx], 
                        C_BUTTONS_RIGHT_EMPTY_X, 
                        C_BUTTONS_RIGHT_EMPTY_Y, 
                        255, 255, 255, sOpacityR
                    );
                }
            }

            //Draw C-left and C-down buttons
            {
                if (((cIconFlags & CIcon_FLAG_Have_Sidekick_Commands) && sidekick) || 
                    (cIconFlags & CIcon_FLAG_Have_Spells)
                ) {
                    //If Sidekick Commands AND Spells are available
                    if ((cIconFlags & CIcon_FLAG_Have_Sidekick_Commands) && sidekick && (cIconFlags & CIcon_FLAG_Have_Spells)) {
                        //Show the SpellBook on C-left and Kyte/Tricky on C-down
                        if (isKyte) {
                            texIdx = CMDMENU_TEX_38_LeftDownButtons_SpellBook_With_Kyte;
                        } else {
                            texIdx = CMDMENU_TEX_43_LeftDownButtons_SpellBook_With_Tricky;
                        }
                    } else if (cIconFlags & CIcon_FLAG_Have_Spells) { //@recomp: check for spells instead of items
                        //Show the SpellBook on C-left and nothing on C-down
                        texIdx = CMDMENU_TEX_48_LeftDownButtons_SpellBook_NoSidekick;
                    }
                    /* @bug: texIdx could end up reusing the C-right section's value here if neither condition is met
                       (i.e. have commands but not spells and items, or have spells but not commands and items) 
                    */
                    //TODO: fix condition not being met

                    dInventoryPageIcon = tex_load_deferred(dTextableIDs[texIdx]);
                    rcp_screen_full_write(
                        gdl, 
                        dInventoryPageIcon, 
                        C_BUTTONS_LEFT_DOWN_BOOK_SIDEKICK_X, 
                        C_BUTTONS_LEFT_DOWN_BOOK_SIDEKICK_Y, 
                        0, 
                        0, 
                        sOpacityR, 
                        SCREEN_WRITE_TRANSLUCENT
                    );
                    tex_free(dInventoryPageIcon);
                } else {
                    //Draw empty C-down and C-left buttons
                    rcp_tile_write(
                        gdl,
                        sTextureTiles[CMDMENU_TEX_37_C_Down], 
                        C_BUTTONS_DOWN_EMPTY_X, 
                        C_BUTTONS_DOWN_EMPTY_Y, 
                        255, 255, 255, sOpacityR
                    );
                    rcp_tile_write(
                        gdl, 
                        sTextureTiles[CMDMENU_TEX_39_C_Left], 
                        C_BUTTONS_LEFT_EMPTY_X, 
                        C_BUTTONS_LEFT_EMPTY_Y, 
                        255, 255, 255, sOpacityR
                    );
                }
            }
        }
    } else {
        sOpacityR = 0.0f;
    }

    dl = *gdl;

    //Draw the top/bottom of the inventory scroll (@recomp: drawn with the rest of the inventory instead)
    // if (dInventoryOpacity != 0) {
    //     rcp_tile_write(&dl, sTextureTiles[CMDMENU_TEX_02_Scroll_Top],    MENU_SCROLL_X, MENU_SCROLL_TOP_Y,                        255, 255, 255, dInventoryOpacity);
    //     rcp_tile_write(&dl, sTextureTiles[CMDMENU_TEX_01_Scroll_Bottom], MENU_SCROLL_X, MENU_SCROLL_BOTTOM_Y + sInventoryUnrollY, 255, 255, 255, dInventoryOpacity);
    // }

    //Draw the sidekick food meter 
    //(@recomp: animate opacity even when sidekick's missing, so it doesn't get stuck)
    {
        //Handle the meter's opacity
        if (sidekick && sStats.sidekickMaxFood && //@recomp: don't fade in when sidekick is missing, or sidekick data not loaded
            (((sInventoryPageID == CMDMENU_PAGE_7_Sidekick_Tricky || sInventoryPageID == CMDMENU_PAGE_8_Sidekick_Kyte) && dInventoryOpacity) || 
            ((sStatsChangeTimers.sidekickBlueFood >= 0.0f)) ||
            cmdmenu_should_sidekick_meter_appear_over_food_items(sidekick)) //@recomp: optionally show meter while relevant food selected on Items page
        ) {
            /* Fade in while the sidekick's inventory page is open, 
               or when the sidekick's blue food count has recently changed */
            dOpacitySidekickMeter += gUpdateRate * 8;
            if (dOpacitySidekickMeter > MAX_OPACITY) {
                dOpacitySidekickMeter = MAX_OPACITY;
            }
        } else {
            //Otherwise fade out
            dOpacitySidekickMeter -= gUpdateRate * 8;
            if (dOpacitySidekickMeter < 0) {
                dOpacitySidekickMeter = 0;
            }
        }

        if (dOpacitySidekickMeter != 0) {
            //Get sidekick's red/blue food counts (@unfinished: hard-coded quantities)
            // sStats.sidekickRedFood = 3;
            // sStats.sidekickBlueFood = 3;

            //@recomp: use sidekick's actual stats, instead of hard-coded quantities
            dinoStats = gDLL_29_Gplay->vtbl->get_sidekick_stats();
            if (dinoStats) {
                //Option to hide red food for now (since there's no way to increase it through the inventory)
                if (recomp_get_config_u32("cmdmenu_sidekick_meter_hide_red")) {
                    sStats.sidekickRedFood = 0;
                } else {
                    sStats.sidekickRedFood = dinoStats->redFood;
                }

                sStats.sidekickBlueFood = dinoStats->blueFood;
            }

            //Determine the values to show on the meter (2 food units per icon)
            fullRedEnd = sStats.sidekickRedFood >> 1;
            hasHalfRed = sStats.sidekickRedFood & 1;
            fullBlueEnd = (sStats.sidekickBlueFood >> 1) + fullRedEnd + hasHalfRed;
            hasHalfBlue = sStats.sidekickBlueFood & 1;

            //Draw the sidekick's food icons
            for (i = 0; i < sStats.sidekickMaxFood; i++) {
                //Draw red food first (full icons)
                if (i < fullRedEnd) {
                    if (isKyte) {
                        iconIndex = CMDMENU_TEX_56_Grub_Red_Full;
                    } else {
                        iconIndex = CMDMENU_TEX_45_Mushroom_Red_Full;
                    }
                //Draw red food's remainder second (half icon)
                } else if (i == fullRedEnd && hasHalfRed) {
                    if (isKyte) {
                        iconIndex = CMDMENU_TEX_57_Grub_Red_Half;
                    } else {
                        iconIndex = CMDMENU_TEX_46_Mushroom_Red_Half;
                    }
                //Draw blue food third (full icons)
                } else if (i < fullBlueEnd) {
                    if (isKyte) {
                        iconIndex = CMDMENU_TEX_13_Grub_Blue_Full;
                    } else {
                        iconIndex = CMDMENU_TEX_12_Mushroom_Blue_Full;
                    }
                //Draw blue food's remainder last (half icon)
                } else if (i == fullBlueEnd && hasHalfBlue) {
                    if (isKyte) {
                        iconIndex = CMDMENU_TEX_16_Grub_Blue_Half;
                    } else {
                        iconIndex = CMDMENU_TEX_51_Mushroom_Blue_Half;
                    }
                //Pad out with empty icons
                } else {
                    if (isKyte) {
                        iconIndex = CMDMENU_TEX_55_Grub_Empty;
                    } else {
                        iconIndex = CMDMENU_TEX_44_Mushroom_Empty;
                    }
                }
                
                //Draw icons (in 2x4 grid: starting top-right to bottom-right, then top-left to bottom-left)
                rcp_tile_write(&dl, sTextureTiles[iconIndex], 
                    SIDEKICK_METER_X - ((i / SIDEKICK_METER_ICONS_PER_COLUMN) * SIDEKICK_METER_SPACING_X), 
                    SIDEKICK_METER_Y + ((i % SIDEKICK_METER_ICONS_PER_COLUMN) * SIDEKICK_METER_SPACING_Y), 
                    255, 255, 255, dOpacitySidekickMeter
                );
            }
        }
    }

    dl_set_prim_color(&dl, 255, 255, 255, 255);

    *gdl = dl;
}

static s8 rsInfoPopupUnroll = 0;

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
        rsInfoPopupUnroll = 0; //@recomp: close up scroll
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
        POPUP_FIX_ICON_CENTRE_Y - rsInfoPopupUnroll, 
        POPUP_FIX_ICON_X + MENU_ITEM_WIDTH, 
        POPUP_FIX_ICON_CENTRE_Y + rsInfoPopupUnroll
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

    if (!box || !box->texture) {
        return;
    }

    //@recomp: do nothing if the minimap is still visible
    if (minimap_get_opacity() > 0) {
        return;
    }
    
    //@recomp: hide if the tutorial textbox is visible
    if ((dTutorialBoxOpacity > 0) && (box->timer > 30.0f)) {
        box->timer = 30.0f;
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
        rsInfoPopupUnroll -= gUpdateRate;
        if (rsInfoPopupUnroll < 0) {
            rsInfoPopupUnroll = 0;
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
            rsInfoPopupUnroll += gUpdateRate;
            if (rsInfoPopupUnroll > MENU_ITEM_HEIGHT/2) {
                rsInfoPopupUnroll = MENU_ITEM_HEIGHT/2;
            }
        }
    }

    //@recomp: draw the top of the scroll
    rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_02_Scroll_Top], 
        POPUP_FIX_TOP_X,
        POPUP_FIX_TOP_Y + 12 - rsInfoPopupUnroll,
        0xFF, 
        0xFF, 
        0xFF, 
        box->opacity
    );        

    //@recomp: draw the bottom of the scroll
    rcp_tile_write(gdl, sTextureTiles[CMDMENU_TEX_01_Scroll_Bottom], 
        POPUP_FIX_BOTTOM_X,
        POPUP_FIX_BOTTOM_Y - 12 + rsInfoPopupUnroll,
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
    if ((box->count > 1)) {
        sTempIcon->tex = sInventoryStackNumbersTex;

        //@recomp: don't try to draw beyond 10, since it'll crash
        sTempIcon->animProgress = (MIN(8, box->count - 2)) << 8; 
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

    //@recomp: null checks
    if (!box || !box->texture) {
        return;
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

        //@recomp: don't try to draw beyond 10, since it'll crash
        sTempIcon->animProgress = (MIN(8, box->count - 2)) << 8; 
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

/** Prevents a crash when trying to leave CloudRunner Fortress' racetrack (originally by MusicalProgrammer, 25th February 2024) */
RECOMP_PATCH void cmdmenu_energy_bar_free(void) {
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
