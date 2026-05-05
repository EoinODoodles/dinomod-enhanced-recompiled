#include "modding.h"

#include "common.h"
#include "sys/map_enums.h"
#include "sys/menu.h"
#include "dlls/objects/210_player.h"

#include "recomp/dlls/objects/515_SHswapstone_recomp.h"

typedef enum {
    SWAPSTONE_PLAYER_HAS_SPIRIT = 0x1,
    SWAPSTONE_PLAYER_HAS_SPELLSTONE = 0x2,
    SWAPSTONE_IS_RUBBLE = 0x4
} SwapstoneFlags;

typedef struct {
    ObjSetup base;
    u8 _unk18[2];
    u8 rotation;
} SHswapstone_Setup;

typedef struct {
    u8 attachIdx;
    u8 bShowScreen;
    u8 flags;
    u8 unk3;
    u8 awake;
    s16 bitSwapStoneSpokenTo;   // game bits ID
    s16 bitIntroSeq;            // game bits ID
    s16 bitSwappedToSeq;        // game bits ID
} SHswapstone_Data;

typedef enum {
    Rocky_uID = 0x189e,
    Rubble_uID = 0x2732
} SwapStone_uIDs;

extern u16 sWarlockMountainWarps[2];
extern u16 sSwapStoneWarps[2];

extern int SHswapstone_anim_callback(Object* self, Object* overrideObj, AnimObj_Data* animData, s8 a3);
extern s32 SHswapstone_get_held_spirit(void);
extern s32 SHswapstone_has_spellstone(void);
extern void SHswapstone_restore_gameplay_menu(Object* self, s32 arg1, s32 arg2);
extern s32 SHswapstone_is_stick_direction_available(Object* self, s32 arg1, s32 arg2);

/** Make sure Rubble loads in SwapStone Circle (rather than Rocky) */
RECOMP_PATCH void SHswapstone_setup(Object* self, SHswapstone_Setup* setup, s32 arg2) {
    SHswapstone_Data* objdata;

    objdata = self->data;
    self->srt.yaw = setup->rotation << 8;
    self->animCallback = (void*)SHswapstone_anim_callback;

    //@recomp: change how we determine if this is Rubble or Rocky, avoiding incorrect outcome if local Block is unloaded
    if ((map_world_xz_to_map_id(self->srt.transl.x, self->srt.transl.z) == MAP_SWAPSTONE_CIRCLE) 
        || (setup->base.uID == Rubble_uID) //@recomp: fallback check
    ) {
        // We are Rubble
        objdata->bitSwapStoneSpokenTo = BIT_883;
        objdata->bitIntroSeq = BIT_Play_Seq_0107_Rocky_Intro_Unused;
        objdata->bitSwappedToSeq = BIT_Play_Seq_01FB_SwapStone_Back_In_SH;
        objdata->flags |= SWAPSTONE_IS_RUBBLE;
        self->modelInstIdx = 1;
    } else {
        // We are Rocky
        objdata->bitSwapStoneSpokenTo = BIT_Talked_to_Rocky;
        objdata->bitIntroSeq = BIT_Play_Seq_035F_Rocky_Intro;
        objdata->bitSwappedToSeq = BIT_Play_Seq_00D7_Swapped_to_Krystal;
        self->modelInstIdx = 0;
    }

    if (main_get_bits(BIT_Talking_to_Rocky) && main_get_bits(BIT_Talked_to_Rocky)) {
        objdata->awake = TRUE;
    } else {
        objdata->awake = FALSE;
    }

    main_set_bits(objdata->bitIntroSeq, 0);
}

/** Make sure Rubble loads in SwapStone Circle (rather than Rocky) */
RECOMP_PATCH u32 SHswapstone_get_model_flags(Object* self) {
    s32 modelno;

    //@recomp: change how we determine if this is Rubble or Rocky, avoiding incorrect outcome if local Block is unloaded
    if (map_world_xz_to_map_id(self->srt.transl.x, self->srt.transl.z) == MAP_SWAPSTONE_CIRCLE
        || (self->setup->uID == Rubble_uID) //@recomp: fallback check
    ) {
        // We are Rubble
        modelno = 1;
    } else {
        // We are Rocky
        modelno = 0;
    }
    return MODFLAGS_MODEL_INDEX(modelno) | MODFLAGS_LOAD_SINGLE_MODEL;
}

#define CMD_BASE_SELECTION 13
#define SELECT_SCREEN(var) (CMD_BASE_SELECTION + var)

RECOMP_PATCH int SHswapstone_anim_callback(Object* self, Object* overrideObj, AnimObj_Data* animData, s8 a3) {
    SHswapstone_Data* objdata;
    s32 playerno;
    s32 i;

    objdata = self->data;
    if (menu_get_current() != MENU_SELECTION) {
        menu_set(MENU_SELECTION);
    }

    animData->unkF8 = (AnimObj_DataF8Callback)SHswapstone_is_stick_direction_available;
    animData->unkF4 = (AnimObj_DataF4Callback)SHswapstone_restore_gameplay_menu;

    if (animData->unk62 != 0) {
        objdata->flags &= ~(SWAPSTONE_PLAYER_HAS_SPIRIT | SWAPSTONE_PLAYER_HAS_SPELLSTONE);
        if (SHswapstone_get_held_spirit() != PLAYER_NO_SPIRIT) {
            objdata->flags |= SWAPSTONE_PLAYER_HAS_SPIRIT;
        }
        if (SHswapstone_has_spellstone() != 0) {
            objdata->flags |= SWAPSTONE_PLAYER_HAS_SPELLSTONE;
        }
        animData->unk62 = 0;
        if (main_get_bits(objdata->bitSwapStoneSpokenTo) != 0) {
            animData->unk9D |= 4;
        }
    }

    for (i = 0; i < animData->unk98; i++) {
        switch (animData->unk8E[i]) {
        case 3:
            objdata->attachIdx = 0;
            break;
        case 4:
            objdata->attachIdx = 1;
            break;
        case 6:
            // Swap players
            playerno = 1 - gDLL_29_Gplay->vtbl->get_playerno();
            gDLL_29_Gplay->vtbl->set_playerno(playerno);
            if (objdata->flags & SWAPSTONE_IS_RUBBLE) {
                // Going to SwapStone Hollow
                gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_SWAPSTONE_HOLLOW, 0, 1);
                gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_SWAPSTONE_HOLLOW, 7, 1);
                gDLL_29_Gplay->vtbl->set_map_setup(MAP_SWAPSTONE_HOLLOW, 1);
                if ((main_get_bits(BIT_Play_Seq_0180_Release_Spirit_1) != 0) && (main_get_bits(BIT_Played_Seq_01FD_Rocky_Teaches_Distract) == 0)) {
                    // Set Rocky to give Tricky the distract command
                    main_set_bits(BIT_Play_Seq_01FD_Rocky_Teaches_Distract, 1);
                } else {
                    main_set_bits(objdata->bitSwappedToSeq, 1);
                }
            } else {
                // Going to SwapStone Circle
                gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_SWAPSTONE_CIRCLE, 0, 1);
                //gDLL_29_Gplay->vtbl->set_map_setup(MAP_SWAPSTONE_CIRCLE, 2); // @recomp: Don't reset SwapStone circle act
                main_set_bits(objdata->bitSwappedToSeq, 1);
            }
            menu_set(MENU_GAMEPLAY); // @recomp: Reactivate gameplay menu (so spells/bag can be used again)
            warpPlayer(sSwapStoneWarps[playerno], /*fadeToBlack=*/FALSE);
            break;
        case 7:
            // Warp to Warlock Mountain
            playerno = gDLL_29_Gplay->vtbl->get_playerno();
            switch (SHswapstone_get_held_spirit()) {
            case PLAYER_SPIRIT_2:
            case PLAYER_SPIRIT_4:
            case PLAYER_SPIRIT_5:
            case PLAYER_SPIRIT_7:
                // Sabre has a spirit
                gDLL_29_Gplay->vtbl->set_map_setup(MAP_ICE_MOUNTAIN_1, 1);
                gDLL_29_Gplay->vtbl->set_map_setup(MAP_SNOWHORN_WASTES, 1);
                //gDLL_29_Gplay->vtbl->set_map_setup(MAP_WARLOCK_MOUNTAIN, 3); // @recomp: Don't reset Warlock Mountain act
                break;
            case PLAYER_SPIRIT_1:
            case PLAYER_SPIRIT_3:
            case PLAYER_SPIRIT_6:
            case PLAYER_SPIRIT_8:
                // Krystal has a spirit
                gDLL_29_Gplay->vtbl->set_map_setup(MAP_ICE_MOUNTAIN_1, 1);
                gDLL_29_Gplay->vtbl->set_map_setup(MAP_SNOWHORN_WASTES, 1);
                //gDLL_29_Gplay->vtbl->set_map_setup(MAP_WARLOCK_MOUNTAIN, 2); // @recomp: Don't reset Warlock Mountain act
                break;
            }
            menu_set(MENU_GAMEPLAY); // @recomp: Reactivate gameplay menu (so spells/bag can be used again)
            warpPlayer(sWarlockMountainWarps[playerno], /*fadeToBlack=*/FALSE);
            break;
        case 10:
            objdata->bShowScreen ^= 1;
            break;
        case 9:
            playerno = gDLL_29_Gplay->vtbl->get_playerno();
            gDLL_29_Gplay->vtbl->set_playerno(1 - playerno);
            gDLL_29_Gplay->vtbl->set_map_setup(MAP_ICE_MOUNTAIN_1, 1);
            gDLL_29_Gplay->vtbl->set_map_setup(MAP_SWAPSTONE_CIRCLE, 2);
            menu_set(MENU_GAMEPLAY);
            warpPlayer(WARP_ICE_MOUNTAIN_CAMPSITE, /*fadeToBlack=*/FALSE);
            break;
        case 12:
            warpPlayer(WARP_SWAPSTONE_SHOP_ENTRANCE, /*fadeToBlack=*/FALSE);
            break;
        case (SELECT_SCREEN(SelectionMenu_STATE_0_Fade_Out)):
        case (SELECT_SCREEN(SelectionMenu_STATE_1_SwapStone_Choices)):
        case (SELECT_SCREEN(SelectionMenu_STATE_2_Confirm_Right)):
        case (SELECT_SCREEN(SelectionMenu_STATE_3_Confirm_Left)):
            if (menu_get_current() == MENU_SELECTION) {
                ((DLL_Menu16*)menu_get_active_dll())->vtbl->set_selection_state(animData->unk8E[i] - CMD_BASE_SELECTION);
            }
            break;
        default:
            break;
        }
    }

    if (objdata->bShowScreen != 0) {
        gDLL_20_Screens->vtbl->show_screen(0);
    }

    return 0;
}
