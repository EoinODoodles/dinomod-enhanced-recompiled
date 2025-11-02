#include "modding.h"

#include "PR/ultratypes.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/214_animobj.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/objects.h"
#include "sys/menu.h"
#include "dll.h"
#include "functions.h"

#include "recomp/dlls/objects/515_SHswapstone_recomp.h"

typedef enum {
    SWAPSTONE_PLAYER_HAS_SPIRIT = 0x1,
    SWAPSTONE_PLAYER_HAS_SPELLSTONE = 0x2,
    SWAPSTONE_IS_RUBBLE = 0x4
} SwapstoneFlags;

typedef struct {
    u8 attachIdx;
    u8 bShowScreen;
    u8 flags;
    u8 unk3;
    u8 unk4;
    s16 bitSwapStoneSpokenTo; // game bits ID
    s16 bitIntroSeq; // game bits ID
    s16 bitSwappedToSeq; // game bits ID
} SHswapstone_Data;

extern u16 sWarlockMountainWarps[2];
extern u16 sSwapStoneWarps[2];

extern s32 SHswapstone_func_448(Object* self, Object* a1, AnimObj_Data* a2, void* a3);
extern s32 SHswapstone_get_held_spirit(void);
extern s32 SHswapstone_has_spellstone(void);
extern void SHswapstone_func_A8C(Object* self, s32 arg1, s32 arg2);
extern s32 SHswapstone_func_AD4(Object* self, s32 arg1, s32 arg2);

RECOMP_PATCH s32 SHswapstone_func_448(Object* self, Object* a1, AnimObj_Data* a2, void* a3) {
    SHswapstone_Data* objdata;
    s32 playerno;
    s32 i;

    objdata = self->data;
    if (menu_get_current() != MENU_16) {
        menu_set(MENU_16);
    }
    a2->unkF8 = SHswapstone_func_AD4;
    a2->unkF4 = SHswapstone_func_A8C;
    if (a2->unk62 != 0) {
        objdata->flags &= ~(SWAPSTONE_PLAYER_HAS_SPIRIT | SWAPSTONE_PLAYER_HAS_SPELLSTONE);
        if (SHswapstone_get_held_spirit() != PLAYER_NO_SPIRIT) {
            objdata->flags |= SWAPSTONE_PLAYER_HAS_SPIRIT;
        }
        if (SHswapstone_has_spellstone() != 0) {
            objdata->flags |= SWAPSTONE_PLAYER_HAS_SPELLSTONE;
        }
        a2->unk62 = 0;
        if (main_get_bits(objdata->bitSwapStoneSpokenTo) != 0) {
            a2->unk9D |= 4;
        }
    }
    for (i = 0; i < a2->unk98; i++) {
        switch (a2->unk8E[i]) {
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
        case 13:
        case 14:
        case 15:
        case 16:
            if (menu_get_current() == MENU_16) {
                ((DLL_Menu16*)menu_get_active_dll())->vtbl->func3(a2->unk8E[i] - 0xD);
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
