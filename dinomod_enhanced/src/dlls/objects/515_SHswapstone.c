#include "modding.h"

#include "PR/ultratypes.h"
#include "game/objects/object.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/214_animobj.h"
#include "sys/controller.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/objects.h"
#include "sys/menu.h"
#include "dll.h"
#include "functions.h"

#include "recomp/dlls/objects/515_SHswapstone_recomp.h"

typedef struct {
    u8 attachIdx;
    u8 unk1;
    u8 unk2;
    u8 unk3;
    u8 unk4;
    s16 unk6; // game bits ID
    s16 unk8; // game bits ID
    s16 unkA; // game bits ID
} SHswapstone_State;

extern u16 sWarlockMountainWarps[2];
extern u16 sSwapStoneWarps[2];

extern s32 SHswapstone_func_448(Object* self, Object* a1, AnimObjState* a2, void* a3);
extern s32 SHswapstone_get_held_spirit(void);
extern s32 SHswapstone_has_spellstone(void);
extern void SHswapstone_func_A8C(Object* self, s32 arg1, s32 arg2);
extern s32 SHswapstone_func_AD4(Object* self, s32 arg1, s32 arg2);

RECOMP_PATCH s32 SHswapstone_func_448(Object* self, Object* a1, AnimObjState* a2, void* a3) {
    SHswapstone_State* state;
    s32 playerno;
    s32 i;

    state = self->state;
    if (menu_get_current() != MENU_16) {
        menu_set(MENU_16);
    }
    a2->unkF8 = SHswapstone_func_AD4;
    a2->unkF4 = SHswapstone_func_A8C;
    if (a2->unk62 != 0) {
        state->unk2 &= ~3;
        if (SHswapstone_get_held_spirit() != 0) {
            state->unk2 |= 1;
        }
        if (SHswapstone_has_spellstone() != 0) {
            state->unk2 |= 2;
        }
        a2->unk62 = 0;
        if (get_gplay_bitstring(state->unk6) != 0) {
            a2->unk9D |= 4;
        }
    }
    for (i = 0; i < a2->unk98; i++) {
        switch (a2->unk8E[i]) {
        case 3:
            state->attachIdx = 0;
            break;
        case 4:
            state->attachIdx = 1;
            break;
        case 6:
            playerno = 1 - gDLL_29_Gplay->vtbl->get_playerno();
            gDLL_29_Gplay->vtbl->set_playerno(playerno);
            if (state->unk2 & 4) {
                gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_SWAPSTONE_HOLLOW, 0, 1);
                gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_SWAPSTONE_HOLLOW, 7, 1);
                gDLL_29_Gplay->vtbl->set_map_setup(MAP_SWAPSTONE_HOLLOW, 1);
                if ((get_gplay_bitstring(0x277) != 0) && (get_gplay_bitstring(0x178) == 0)) {
                    set_gplay_bitstring(0x885, 1);
                } else {
                    set_gplay_bitstring(state->unkA, 1);
                }
            } else {
                gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_SWAPSTONE_CIRCLE, 0, 1);
                gDLL_29_Gplay->vtbl->set_map_setup(MAP_SWAPSTONE_CIRCLE, 2);
                set_gplay_bitstring(state->unkA, 1);
            }
            menu_set(MENU_GAMEPLAY); // @recomp: Reactivate gameplay menu (so spells/bag can be used again)
            warpPlayer(sSwapStoneWarps[playerno], /*fadeToBlack=*/FALSE);
            break;
        case 7:
            playerno = gDLL_29_Gplay->vtbl->get_playerno();
            switch (SHswapstone_get_held_spirit()) {
            case 0x2:
            case 0x8:
            case 0x10:
            case 0x40:
                gDLL_29_Gplay->vtbl->set_map_setup(MAP_ICE_MOUNTAIN_1, 1);
                gDLL_29_Gplay->vtbl->set_map_setup(MAP_SNOWHORN_WASTES, 1);
                //gDLL_29_Gplay->vtbl->set_map_setup(MAP_WARLOCK_MOUNTAIN, 3); // @recomp: Don't reset Warlock Mountain act
                break;
            case 0x1:
            case 0x4:
            case 0x20:
            case 0x80:
                gDLL_29_Gplay->vtbl->set_map_setup(MAP_ICE_MOUNTAIN_1, 1);
                gDLL_29_Gplay->vtbl->set_map_setup(MAP_SNOWHORN_WASTES, 1);
                //gDLL_29_Gplay->vtbl->set_map_setup(MAP_WARLOCK_MOUNTAIN, 2); // @recomp: Don't reset Warlock Mountain act
                break;
            }
            menu_set(MENU_GAMEPLAY); // @recomp: Reactivate gameplay menu (so spells/bag can be used again)
            warpPlayer(sWarlockMountainWarps[playerno], /*fadeToBlack=*/FALSE);
            break;
        case 10:
            state->unk1 ^= 1;
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
    if (state->unk1 != 0) {
        gDLL_20_Screens->vtbl->show_screen(0);
    }
    return 0;
}
