#include "game/gamebits.h"
#include "game/gametexts.h"
#include "modding.h"
#include "recompconfig.h"
#include "configs.h"

#include "dll.h"
#include "dlls/engine/73.h"
#include "recomputils.h"
#include "sys/dll.h"
#include "sys/fonts.h"
#include "sys/gfx/textable.h"
#include "sys/main.h"
#include "sys/memory.h"
#include "sys/menu.h"
#include "sys/map_enums.h"
#include "sys/print.h"

#include "engine/73_old_picmenu.h"

#include "recomp/dlls/engine/70_old_levelselect_recomp.h"

static DLL_73* gDLL_73_PicmenuOld;

// extern DLL_73* dll_throw_fault; //NOTE: BROKEN! This is a function now, not DLL 73.

// #define LINE_HEIGHT 32
#define LINE_HEIGHT 14 //@recomp: seems to have been the line height of the older font, based on the different (unused) line height the "LEVEL NOT AVAILABLE" print uses
#define TOTAL_STRINGS 53
#define STRING_LENGTH 0x40
#define STRINGS_PER_PAGE 9
#define LAST_PAGE_FIRST_IDX (TOTAL_STRINGS - STRINGS_PER_PAGE)

typedef enum {
    LEVELSELECT_IDX_00_NEW_GAME,
    LEVELSELECT_IDX_01_LOAD_GAME,
    LEVELSELECT_IDX_02,
    LEVELSELECT_IDX_03_SWAPSTONE_HOLLOW,
    LEVELSELECT_IDX_04_SWAPSTONE_CIRCLE,
    LEVELSELECT_IDX_05,
    LEVELSELECT_IDX_06_DARKICE_MINES,
    LEVELSELECT_IDX_07_DARKICE_MINES_TWO,
    LEVELSELECT_IDX_08_WALLED_CITY,
    LEVELSELECT_IDX_09_DRAGON_ROCK,
    LEVELSELECT_IDX_0A_CLOUDRUNNER_FORTRESS,
    LEVELSELECT_IDX_0B_KRAZOA_PALACE,
    LEVELSELECT_IDX_0C_BLACKWATER_CANYON,
    LEVELSELECT_IDX_0D,
    LEVELSELECT_IDX_0E_NORTHERN_WASTES,
    LEVELSELECT_IDX_0F_EARTHWALKER_TEMPLE,
    LEVELSELECT_IDX_10_WILLOW_GROVE,
    LEVELSELECT_IDX_11_DIAMOND_BAY,
    LEVELSELECT_IDX_12_DISCOVERY_FALLS,
    LEVELSELECT_IDX_13_MOON_MOUNTAIN_PASS,
    LEVELSELECT_IDX_14_CAPE_CLAW,
    LEVELSELECT_IDX_15_GOLDEN_PLAINS,
    LEVELSELECT_IDX_16,
    LEVELSELECT_IDX_17_TEST_OF_COMBAT,
    LEVELSELECT_IDX_18_TEST_OF_STRENGTH,
    LEVELSELECT_IDX_19_TEST_OF_FEAR,
    LEVELSELECT_IDX_1A_TEST_OF_CHARACTER,
    LEVELSELECT_IDX_1B_TEST_OF_KNOWLEDGE,
    LEVELSELECT_IDX_1C_TEST_OF_SACRIFICE,
    LEVELSELECT_IDX_1D_TEST_OF_SKILL,
    LEVELSELECT_IDX_1E_TEST_OF_MAGIC,
    LEVELSELECT_IDX_1F,
    LEVELSELECT_IDX_20,
    LEVELSELECT_IDX_21_DARKICE_BOSS,
    LEVELSELECT_IDX_22_GENERAL_SCALES_BOSS,
    LEVELSELECT_IDX_23_BLACKWATER_BOSS,
    LEVELSELECT_IDX_24_CLOUDRUNNER_RACE,
    LEVELSELECT_IDX_25_KAMERIA_DRAGON_BOSS,
    LEVELSELECT_IDX_26_DRAKOR_FINAL_BOSS,
    LEVELSELECT_IDX_27_ICE_MOUNTAIN,
    LEVELSELECT_IDX_28,
    LEVELSELECT_IDX_29_DESERT_FORCE_POINT,
    LEVELSELECT_IDX_2A_VOLCANO_FORCE_POINT,
    LEVELSELECT_IDX_2B,
    LEVELSELECT_IDX_2C_EARTHWALKER_ACT_TWO, 
    LEVELSELECT_IDX_2D_ENERGY_DEMO,
    LEVELSELECT_IDX_2E, 
    LEVELSELECT_IDX_2F_WARLOCK_ACT_ONE,
    LEVELSELECT_IDX_30_WARLOCK_ACT_TWO,
    LEVELSELECT_IDX_31_WARLOCK_ACT_THREE,
    LEVELSELECT_IDX_32_WARLOCK_ACT_FOUR,
    LEVELSELECT_IDX_33_WARLOCK_ACT_FIVE,
    LEVELSELECT_IDX_34_WARLOCK_ACT_SIX
} LevelSelectOld_Indices;

/*0x0*/ extern char dStrings[TOTAL_STRINGS][STRING_LENGTH];

#define END {NULL, -1, 0, 0}

/*0x0*/ static TextureTile dTexTiles[] = {
    /* Row 0 */
    {NULL,   0,   -96, -32}, //missing
    {NULL,   0,   -64, -32}, //TEX0 688
    {NULL,   0,   -32, -32}, //TEX0 689
    {NULL,   0,   0,   -32}, //TEX0 690 
    {NULL,   0,   32,  -32}, //TEX0 691
    {NULL,   0,   64,  -32}, //TEX0 692

    /* Row 1 */
    {NULL,   0,   -96, 0}, //TEX0 693
    {NULL,   0,   -64, 0}, //TEX0 694 
    {NULL,   0,   -32, 0}, //TEX0 695
    {NULL,   0,   0,   0}, //TEX0 696
    {NULL,  0,   32,  0}, //TEX0 697
    {NULL,  0,   64,  0}, //TEX0 698

    /* Row 2 */
    {NULL,  0,   -96, 32}, //TEX0 699 
    {NULL,  0,   -64, 32}, //TEX0 700
    {NULL,  0,   -32, 32}, //TEX0 701
    {NULL,  0,   0,   32}, //TEX0 702
    {NULL,  0,   32,  32}, //TEX0 703
    {NULL,  0,   64,  32}, //TEX0 704

    END
};

static void dinomod_load_logo_textures(void) {
    for (s32 i = 0; dTexTiles[i].animProgress != -1; i++) {
        //Special case: handle first texture tile of old DP logo (since it was replaced by a full image of the newer DP logo)
        if (i == 0) {
            dTexTiles[i].tex = tex_load_deferred(TEXTABLE_254);
            continue;
        }
        
        dTexTiles[i].tex = tex_load_deferred(TEXTABLE_C5_DinosaurPlanetLogo + i);
    }    
}

static void dinomod_unload_logo_textures(void) {
    for (s32 i = 0; dTexTiles[i].animProgress != -1; i++) {
        tex_free(dTexTiles[i].tex);
    }    
}

static void dinomod_setup_standard_resolution(void) {
    extern void func_8001440C(s32 arg0);

    main_set_bits(BIT_Menus_Selection_Blocked, FALSE);

    vi_init(1, get_ossched(), FALSE);
    track_set_z_buffer_on(1);
    track_set_sky_on(1);

    func_8001440C(0);
}

static s32 dinomod_is_save_slot_empty(s32 saveSlotIdx) {
    Savefile* saveFile;
    
    
    if ((u8)gDLL_29_Gplay->vtbl->load_save(saveSlotIdx, /*startGame*/FALSE) == 0) {
        return TRUE;
    }
    
    saveFile = &gDLL_29_Gplay->vtbl->get_state()->save.file;
    if (saveFile->isEmpty) {
        return FALSE;
    } else {
        return TRUE;
    }
}

extern void old_levelselect_start(MapIDs mapID, s32 act, PlayerNo playerNo);

#define TEXT_FONT FONT_FUN_FONT
#define TEXT_COLOUR_LIGHT 0xFFD73DFF
#define TEXT_COLOUR_DARK 0x5B4530FF

// offset: 0x0 | ctor
RECOMP_PATCH void old_levelselect_ctor(void *dll) {
    //@recomp: load DLL 73 as a static, to fix this menu's broken function calls
    if (gDLL_73_PicmenuOld == NULL) {
        gDLL_73_PicmenuOld = dll_load_deferred(DLL_ID_OLD_PICMENU, 8);
    }

    //@recomp: load logo textures
    dinomod_load_logo_textures();

    //@recomp: set smaller fonts
    dll_73_set_fontIDs(TEXT_FONT, TEXT_FONT);
    dll_73_set_font_colours(TEXT_COLOUR_LIGHT, TEXT_COLOUR_DARK);

    //@recomp: set up drop-shadow
    dll_73_enable_drop_shadow(TRUE);
    dll_73_set_drop_shadow_position(1, 1);
}

// offset: 0xC | dtor
RECOMP_PATCH void old_levelselect_dtor(void *dll) {
    //@recomp: unload DLL 73 when finished
    if (gDLL_73_PicmenuOld) {
        dll_unload(gDLL_73_PicmenuOld);
        gDLL_73_PicmenuOld = NULL;
    }

    //@recomp: unload logo textures
    dinomod_unload_logo_textures();
}

// offset: 0x2C | func: 2 | export: 2
RECOMP_PATCH void old_levelselect_draw(Gfx** gdl, Mtx** mtx, Vertex** vtx) {
    /*0xD40*/ static s32 dSelectedPrintIdx = 0;         //Selected index within 9 strings currently shown
    /*0xD44*/ static s32 dSelectedListIdx = 0;          //Selected index within full list of strings
    /*0xD48*/ static s32 dShowLevelUnavailable = FALSE; //Shows a "LEVEL NOT AVAILABLE" message instead of the level list
    /*0x000*/ static s32 sButtonsEnabled;
    static s32 rsUnavailableMessageID = 0;

    s32 start;
    s32 menuAction;
    s32 i;
    char *string;
    s32 prevSelectedPrintIdx;

    func_80014508(20);

    // gDLL_20_Screens->vtbl->show_screen(2); //NOTE: this screen is missing - it likely contained the old Dinosaur Planet logo composited with the gradient background
    
    // @recomp: put old_mainmenu's unused TextureTiles to use, drawing the old Dinosaur Planet logo (slightly smaller than the missing SCREENS.bin version seen in the One Hour Footage)
    rcp_tile_write(gdl, dTexTiles, 160, 40, 0xFF, 0xFF, 0xFF, 0xFF);

    //Display either the Level Select list, or a "LEVEL NOT AVAILABLE" message
    if (dShowLevelUnavailable == FALSE) {
        //@recomp: add a bit of vertical space if FUN_FONT is being used
        s32 rInitialY = 120;
        if ((dll_73_get_fontID_light() == FONT_FUN_FONT) || (dll_73_get_fontID_dark() == FONT_FUN_FONT)) {
            rInitialY -= 15;
        }
        gDLL_73_PicmenuOld->vtbl->init_text_window(rInitialY);
    
        //Print 9 strings at a time
        if (dSelectedListIdx < 4) { 
            //Upper half of first page of strings
            for (i = 0, string = dStrings[0]; i < STRINGS_PER_PAGE; i++, string += STRING_LENGTH) {
                gDLL_73_PicmenuOld->vtbl->add_string(i, string, LINE_HEIGHT, dSelectedPrintIdx);
                
                if (gDLL_20_Screens){} //fake
            }
        } else if (dSelectedListIdx >= (TOTAL_STRINGS - 4)) { 
            //Lower half of last page of strings
            for (i = LAST_PAGE_FIRST_IDX, string = dStrings[LAST_PAGE_FIRST_IDX]; i < TOTAL_STRINGS; i++, string += STRING_LENGTH) {
                gDLL_73_PicmenuOld->vtbl->add_string(i, string, LINE_HEIGHT, dSelectedPrintIdx);
            }
        } else {
            //Scrolling between top/bottom of list
            start = dSelectedListIdx - 4;
            i = start;
            if (start < (dSelectedListIdx + 5)) {
                string = dStrings[start];
                do {
                    gDLL_73_PicmenuOld->vtbl->add_string(i, string, LINE_HEIGHT, dSelectedPrintIdx);
                    i++;
                    string += STRING_LENGTH;
                } while (i < (dSelectedListIdx + 5));
            }
        }
        
        //Handle moving up/down the list
        {
            prevSelectedPrintIdx = dSelectedPrintIdx;
            menuAction = gDLL_73_PicmenuOld->vtbl->handle_joystick_and_buttons(&dSelectedPrintIdx);
            
            if ((prevSelectedPrintIdx == 0) && (dSelectedPrintIdx == (STRINGS_PER_PAGE - 1))) {
                //Wrapping from top to bottom
                dSelectedListIdx = TOTAL_STRINGS - 1;
            } else if ((prevSelectedPrintIdx == (STRINGS_PER_PAGE - 1)) && (dSelectedPrintIdx == 0)) {
                //Wrapping from bottom to top
                dSelectedListIdx = 0;
            } else {
                //Moving up/down (starting on nearest string that isn't blank)
                if (prevSelectedPrintIdx > dSelectedPrintIdx) {             
                    do {
                        dSelectedListIdx--;
                    } while (dStrings[dSelectedListIdx][0] == '\0');
                } else if (prevSelectedPrintIdx < dSelectedPrintIdx) {
                    do {
                        dSelectedListIdx++;
                    } while (dStrings[dSelectedListIdx][0] == '\0');
                }
                
                //Determine which string is selected (out of the 9 shown)
                if (dSelectedListIdx < 4) {
                    dSelectedPrintIdx = dSelectedListIdx;
                } else if (dSelectedListIdx > (TOTAL_STRINGS - 5)) {
                    dSelectedPrintIdx = dSelectedListIdx - (TOTAL_STRINGS - STRINGS_PER_PAGE);
                } else {
                    dSelectedPrintIdx = 4;
                }
            }
        }

        //Handle selecting an item
        if (menuAction >= 0) {
            switch (dSelectedListIdx) {
            case LEVELSELECT_IDX_00_NEW_GAME:
                //@recomp: don't allow this option if a save file already exists in Slot 0
                if (dinomod_is_save_slot_empty(0)) {
                    dShowLevelUnavailable = TRUE; rsUnavailableMessageID = 2; //lock option to avoid overwriting save
                    break;
                }

                gDLL_5_AMSEQ2->vtbl->set(NULL, 0x23, 0, 0, 0);
                gDLL_29_Gplay->vtbl->init_save(0, NULL);
                gDLL_29_Gplay->vtbl->load_save(0, TRUE); //@recomp
                return;
            case LEVELSELECT_IDX_01_LOAD_GAME:
                gDLL_5_AMSEQ2->vtbl->set(NULL, 0x23, 0, 0, 0);
                gDLL_29_Gplay->vtbl->load_save(0, TRUE);
                return;
            case LEVELSELECT_IDX_02: //NOTE: blank string
                old_levelselect_start(MAP_FRONT_END, 0, PLAYER_KRYSTAL);
                return;
            case LEVELSELECT_IDX_03_SWAPSTONE_HOLLOW:
                old_levelselect_start(MAP_SWAPSTONE_HOLLOW, 0, PLAYER_SABRE);
                return;
            case LEVELSELECT_IDX_04_SWAPSTONE_CIRCLE:
                old_levelselect_start(MAP_SWAPSTONE_CIRCLE, 1, PLAYER_KRYSTAL);
                return;
            case LEVELSELECT_IDX_06_DARKICE_MINES:
                old_levelselect_start(MAP_DARK_ICE_MINES_1, 0, PLAYER_SABRE);
                return;
            case LEVELSELECT_IDX_07_DARKICE_MINES_TWO:
                old_levelselect_start(MAP_ANIMTEST, 0, PLAYER_SABRE); //DIM2 may have been moved down by one index, after Animtest?
                return;
            case LEVELSELECT_IDX_08_WALLED_CITY:
                dShowLevelUnavailable = TRUE;
                break;
            case LEVELSELECT_IDX_09_DRAGON_ROCK:
                dShowLevelUnavailable = TRUE; //Wasn't created at this stage?
                break;
            case LEVELSELECT_IDX_0A_CLOUDRUNNER_FORTRESS:
                old_levelselect_start(MAP_CLOUDRUNNER_FORTRESS, 0, PLAYER_KRYSTAL);
                return;
            case LEVELSELECT_IDX_0B_KRAZOA_PALACE:
                dShowLevelUnavailable = TRUE;  //Wasn't created at this stage?
                break;
            case LEVELSELECT_IDX_0C_BLACKWATER_CANYON:
                dShowLevelUnavailable = TRUE;  //Wasn't created at this stage?
                break;
            case LEVELSELECT_IDX_0E_NORTHERN_WASTES:
                old_levelselect_start(MAP_SNOWHORN_WASTES, 0, PLAYER_SABRE);
                return;
            case LEVELSELECT_IDX_0F_EARTHWALKER_TEMPLE:
                old_levelselect_start(MAP_EARTHWALKER_TEMPLE, 1, PLAYER_SABRE);
                return;
            case LEVELSELECT_IDX_10_WILLOW_GROVE:
                old_levelselect_start(MAP_WILLOW_GROVE, 0, PLAYER_KRYSTAL); //Starting as Krystal by mistake?
                return;
            case LEVELSELECT_IDX_11_DIAMOND_BAY:
                old_levelselect_start(MAP_DIAMOND_BAY, 0, PLAYER_SABRE);
                return;
            case LEVELSELECT_IDX_12_DISCOVERY_FALLS:
                old_levelselect_start(MAP_DISCOVERY_FALLS, 0, PLAYER_KRYSTAL);
                return;
            case LEVELSELECT_IDX_13_MOON_MOUNTAIN_PASS:
                old_levelselect_start(MAP_MOON_MOUNTAIN_PASS, 0, PLAYER_KRYSTAL);
                return;
            case LEVELSELECT_IDX_14_CAPE_CLAW:
                old_levelselect_start(MAP_CAPE_CLAW, 0, PLAYER_KRYSTAL);
                return;
            case LEVELSELECT_IDX_15_GOLDEN_PLAINS:
                old_levelselect_start(MAP_GOLDEN_PLAINS, 0, PLAYER_KRYSTAL);
                return;
            case LEVELSELECT_IDX_17_TEST_OF_COMBAT:
                old_levelselect_start(MAP_SHRINE_DISCOVERY_FALLS, 0, PLAYER_KRYSTAL);
                return;
            case LEVELSELECT_IDX_18_TEST_OF_STRENGTH:
                old_levelselect_start(MAP_SHRINE_DIAMOND_BAY, 0, PLAYER_SABRE);
                return;
            case LEVELSELECT_IDX_19_TEST_OF_FEAR:
                old_levelselect_start(MAP_SHRINE_MOON_MOUNTAIN_PASS, 0, PLAYER_KRYSTAL);
                return;
            case LEVELSELECT_IDX_1A_TEST_OF_CHARACTER:
                old_levelselect_start(MAP_SHRINE_CAPE_CLAW, 0, PLAYER_KRYSTAL);
                return;
            case LEVELSELECT_IDX_1B_TEST_OF_KNOWLEDGE:
                old_levelselect_start(MAP_SHRINE_GOLDEN_PLAINS, 0, PLAYER_KRYSTAL);
                return;
            case LEVELSELECT_IDX_1C_TEST_OF_SACRIFICE:
                old_levelselect_start(MAP_SHRINE_SNOWHORN_WASTES, 0, PLAYER_SABRE);
                return;
            case LEVELSELECT_IDX_1D_TEST_OF_SKILL:
                old_levelselect_start(MAP_SHRINE_WALLED_CITY, 0, PLAYER_SABRE);
                return;
            case LEVELSELECT_IDX_1E_TEST_OF_MAGIC:
                old_levelselect_start(MAP_SHRINE_WILLOW_GROVE, 0, PLAYER_SABRE);
                return;
            case LEVELSELECT_IDX_21_DARKICE_BOSS:
                old_levelselect_start(MAP_BOSS_GALADON, 0, PLAYER_KRYSTAL); //Starting as Krystal, just like in the old trailer!
                return;
            case LEVELSELECT_IDX_22_GENERAL_SCALES_BOSS:
                dShowLevelUnavailable = TRUE;
                break;
            case LEVELSELECT_IDX_23_BLACKWATER_BOSS:
                dShowLevelUnavailable = TRUE;
                break;
            case LEVELSELECT_IDX_24_CLOUDRUNNER_RACE:
                old_levelselect_start(MAP_CLOUDRUNNER_RACETRACK, 0, PLAYER_KRYSTAL); //CRF Trap Rooms scrapped by this stage?
                return;
            case LEVELSELECT_IDX_25_KAMERIA_DRAGON_BOSS:
                dShowLevelUnavailable = TRUE; //Wasn't created at this stage?
                break;
            case LEVELSELECT_IDX_26_DRAKOR_FINAL_BOSS:
                old_levelselect_start(MAP_BOSS_DRAKOR, 0, PLAYER_SABRE);
                break;
            case LEVELSELECT_IDX_27_ICE_MOUNTAIN:
                old_levelselect_start(MAP_ICE_MOUNTAIN_1, 0, PLAYER_SABRE);
                return;
            case LEVELSELECT_IDX_29_DESERT_FORCE_POINT:
                old_levelselect_start(MAP_DESERT_FORCE_POINT_TEMPLE_BOTTOM, 0, PLAYER_KRYSTAL);
                return;
            case LEVELSELECT_IDX_2A_VOLCANO_FORCE_POINT:
                old_levelselect_start(MAP_VOLCANO_FORCE_POINT_TEMPLE, 0, PLAYER_KRYSTAL); //Starting as Krystal by mistake?
                return;
            case LEVELSELECT_IDX_2C_EARTHWALKER_ACT_TWO:
                gDLL_5_AMSEQ2->vtbl->set(NULL, 0x23, 0, 0, 0);

                //NOTE: crashes due to out-of-bounds save index? They must've had some extra debug/demo saves at some stage!
                //All the other save-loading entries crash for the same reason, too.

                // gDLL_29_Gplay->vtbl->load_save(8, 1);

                dShowLevelUnavailable = TRUE; rsUnavailableMessageID = 1; //@recomp: lock option to avoid crash
                return;
            case LEVELSELECT_IDX_2D_ENERGY_DEMO:
                old_levelselect_start(MAP_EARTHWALKER_TEMPLE, 5, PLAYER_KRYSTAL);
                return;
            case LEVELSELECT_IDX_2F_WARLOCK_ACT_ONE:
                gDLL_5_AMSEQ2->vtbl->set(NULL, 0x23, 0, 0, 0);
                // gDLL_29_Gplay->vtbl->load_save(0xA, 1);
                dShowLevelUnavailable = TRUE; rsUnavailableMessageID = 1; //@recomp: lock option to avoid crash
                return;
            case LEVELSELECT_IDX_30_WARLOCK_ACT_TWO:
                gDLL_5_AMSEQ2->vtbl->set(NULL, 0x23, 0, 0, 0);
                // gDLL_29_Gplay->vtbl->load_save(0xB, 1);
                dShowLevelUnavailable = TRUE; rsUnavailableMessageID = 1; //@recomp: lock option to avoid crash
                return;
            case LEVELSELECT_IDX_31_WARLOCK_ACT_THREE:
                gDLL_5_AMSEQ2->vtbl->set(NULL, 0x23, 0, 0, 0);
                // gDLL_29_Gplay->vtbl->load_save(0xC, 1);
                dShowLevelUnavailable = TRUE; rsUnavailableMessageID = 1; //@recomp: lock option to avoid crash
                return;
            case LEVELSELECT_IDX_32_WARLOCK_ACT_FOUR:
                gDLL_5_AMSEQ2->vtbl->set(NULL, 0x23, 0, 0, 0);
                // gDLL_29_Gplay->vtbl->load_save(0xD, 1);
                dShowLevelUnavailable = TRUE; rsUnavailableMessageID = 1; //@recomp: lock option to avoid crash
                return;
            case LEVELSELECT_IDX_33_WARLOCK_ACT_FIVE:
                gDLL_5_AMSEQ2->vtbl->set(NULL, 0x23, 0, 0, 0);
                // gDLL_29_Gplay->vtbl->load_save(0xE, 1);
                dShowLevelUnavailable = TRUE; rsUnavailableMessageID = 1; //@recomp: lock option to avoid crash
                return;
            case LEVELSELECT_IDX_34_WARLOCK_ACT_SIX:
                gDLL_5_AMSEQ2->vtbl->set(NULL, 0x23, 0, 0, 0);
                // gDLL_29_Gplay->vtbl->load_save(0xF, 1);
                dShowLevelUnavailable = TRUE; rsUnavailableMessageID = 1; //@recomp: lock option to avoid crash
                return;
            default:
                dShowLevelUnavailable = TRUE;
                break;
            case LEVELSELECT_IDX_05:
            case LEVELSELECT_IDX_0D:
            case LEVELSELECT_IDX_16:
            case LEVELSELECT_IDX_1F:
            case LEVELSELECT_IDX_20:
            case LEVELSELECT_IDX_28:
            case LEVELSELECT_IDX_2B:
            case LEVELSELECT_IDX_2E:
                break;
            }
        }
    } else {
        //Display message if option/level can't be accessed
        sButtonsEnabled = FALSE;

        #define LINE_HEIGHT_SMALL 10
        
        //@recomp: show special messages
        switch (rsUnavailableMessageID) {
        default:
            gDLL_73_PicmenuOld->vtbl->init_text_window(180);
            gDLL_73_PicmenuOld->vtbl->add_string(0, "LEVEL NOT AVAILABLE", LINE_HEIGHT_SMALL, 0);
            break;
        case 1:    
            dll_73_set_font_colours(TEXT_COLOUR_LIGHT, TEXT_COLOUR_LIGHT);
            gDLL_73_PicmenuOld->vtbl->init_text_window(140);
            gDLL_73_PicmenuOld->vtbl->add_string(0, "DINOMOD: CRASH AVOIDED", 20, 0);
            dll_73_set_fontIDs(FONT_DINO_SUBTITLE_FONT_1, FONT_DINO_SUBTITLE_FONT_1);
            
            gDLL_73_PicmenuOld->vtbl->add_string(0, "(This item causes a crash when selected,", LINE_HEIGHT, 0);
            gDLL_73_PicmenuOld->vtbl->add_string(0, "because its save index is missing.)", LINE_HEIGHT, 0);
            break;
        case 2:
            dll_73_set_font_colours(TEXT_COLOUR_LIGHT, TEXT_COLOUR_LIGHT);
            gDLL_73_PicmenuOld->vtbl->init_text_window(140);
            gDLL_73_PicmenuOld->vtbl->add_string(0, "DINOMOD: NEW GAME", 18, 0);
            gDLL_73_PicmenuOld->vtbl->add_string(0, "NOT ALLOWED", 20, 0);
            dll_73_set_fontIDs(FONT_DINO_SUBTITLE_FONT_1, FONT_DINO_SUBTITLE_FONT_1);

            gDLL_73_PicmenuOld->vtbl->add_string(0, "(A save file already exists in save slot 1,", LINE_HEIGHT_SMALL, 0);
            gDLL_73_PicmenuOld->vtbl->add_string(0, "which would be overwritten.)", LINE_HEIGHT_SMALL, 0);
            break;
        }
        
        dll_73_set_fontIDs(TEXT_FONT, TEXT_FONT);
        dll_73_set_font_colours(TEXT_COLOUR_LIGHT, TEXT_COLOUR_DARK);
        
        if (gDLL_73_PicmenuOld->vtbl->handle_joystick_and_buttons(&sButtonsEnabled) >= 0) {
            dShowLevelUnavailable = FALSE;
            rsUnavailableMessageID = 0; //@recomp
        }
    }

    font_window_draw(gdl, 0, 0, 1);
}

// offset: 0xB58 | func: 3
/**
  * Starts the game with a specific mapID, Act number, and player character.
  */
RECOMP_PATCH void old_levelselect_start(MapIDs mapID, s32 act, PlayerNo playerNo) {
    gDLL_5_AMSEQ2->vtbl->set(NULL, 0x23, 0, 0, 0);

    //@recomp: only init the save if the slot's empty, to avoid losing save data
    if (dinomod_is_save_slot_empty(0) == FALSE) {
        gDLL_29_Gplay->vtbl->init_save(0, NULL);
    }
    
    main_change_map(mapID, act, playerNo, MENU_GAMEPLAY);

    //@recomp: ensure standard resolution is used
    dinomod_setup_standard_resolution();
}
