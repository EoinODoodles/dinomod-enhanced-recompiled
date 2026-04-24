#include "dll.h"
#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/joypad.h"
#include "sys/menu.h"
#include "sys/objects.h"
#include "sys/print.h"
#include "types.h"
#include "dlls/engine/29_gplay.h"

#include "core/main.h"
#include "engine/78_credits.h"

extern GameState *gGplayState;
extern BitTableEntry *gFile_BITTABLE;
extern s16 gSizeBittable;

/** Prevents cases where the game would try to set out of bounds flags, which would cause data corruption */
RECOMP_PATCH void main_set_bits(s32 entry, u32 value) {
    u8 *bitString;
    u8 _pad[12]; // fake match
    s32 idx;
    s32 mask;
    s32 endBit;
    s32 startBit;

    //@recomp: Prevent data corruption
    if (entry < -1 || entry >= gSizeBittable) {
        recomp_eprintf("Attempted to set out of bounds flagID (%04d)!\n", entry);
        return;
    }

    if (entry != BIT_ALWAYS_1 && entry != BIT_ALWAYS_0 && entry != -1) {
        switch (gFile_BITTABLE[entry].field_0x2 >> 6) {
            case 0: // Never saved to savegame
                bitString = &gGplayState->bitString[0];
                break;
            case 1: // Saved with checkpoints
                bitString = &gGplayState->save.chkpnt.bitString[0];
                break;
            case 2: // Always saved
                bitString = &gGplayState->save.file.bitString[0];
                break;
            case 3: // Saved with map saves
                bitString = &gGplayState->save.map.bitString[0];
                break;
        }

        if (gFile_BITTABLE[entry].field_0x2 & (1 << 5)) {
            gDLL_30_Task->vtbl->mark_task_completed(gFile_BITTABLE[entry].task);
        }

        startBit = gFile_BITTABLE[entry].start;
        endBit = (gFile_BITTABLE[entry].field_0x2 & 0x1f) + 1;
        mask = 1;

        for (idx = startBit; idx < (startBit + endBit); idx++) {
            if (mask & value) {
                *(u8 *)((u32)bitString + (idx >> 3)) |= (1 << (idx & 7));
            } else {
                *(u8 *)((u32)bitString + (idx >> 3)) &= ~(1 << (idx & 7));
            }

            mask = mask << 1;
        }
    }
}

/** Prevents cases where the game would try to increment out of bounds flags, which would cause data corruption */
RECOMP_PATCH s32 main_increment_bits(s32 entry) {
    s32 val;
    s32 maxVal;

    //@recomp: Prevent data corruption
    if (entry == -1) {
        return 0;
    }
    if (entry < 0 || entry >= gSizeBittable) {
        recomp_eprintf("Attempted to increment out of bounds flagID (%04d)!\n", entry);
        return 0;
    }

    val = main_get_bits(entry) + 1;

    maxVal = 1 << ((gFile_BITTABLE[entry].field_0x2 & 0x1f) + 1);

    if (val < maxVal) {
        main_set_bits(entry, val);
    } else {
        val -= 1;
    }

    return val;
}

/** Prevents cases where the game would try to decrement out of bounds flags, which would cause data corruption */
RECOMP_PATCH s32 main_decrement_bits(s32 entry) {
    s32 val = main_get_bits(entry);

    //@recomp: Prevent data corruption
    if (entry == -1) {
        return 0;
    }
    if (entry < 0 || entry >= gSizeBittable) {
        recomp_eprintf("Attempted to decrement out of bounds flagID (%04d)!\n", entry);
        return 0;
    }

    if (val != 0) {
        main_set_bits(entry, --val);
        return val;
    }

    return 0;
}

/** Allows pausing to be blocked temporarily */
static s8 rsBlockPausing = FALSE;

/** Allows pausing to be blocked temporarily */
void main_block_pausing(PauseBlockingStates value) {
    rsBlockPausing = value;
}

extern s8 func_800143FC(void);
extern void update_PlayerPosBuffer();

extern s8 D_8008C94C;
extern Gfx *gCurGfx;
extern Mtx *gCurMtx;
extern Vertex *gCurVtx;
extern Triangle *gCurPol;
extern s8 gPauseState;

/** Allow pausing to be blocked temporarily */
RECOMP_PATCH void func_80013D80(void) {
    s32 button;

    joy_set_button_mask(0, U_JPAD | R_JPAD);
    gDLL_2_Camera->vtbl->lock_icon_tick();
    gDLL_22_Subtitles->vtbl->func_4C0();

    //@recomp: block pause
    if (rsBlockPausing) {
        gPauseState = 0;

        if (rsBlockPausing != PauseBlock_On_Until_Removed) {
            rsBlockPausing = FALSE;
        }

        if (menu_get_current() == MENU_PAUSE) {
            if (credits_get_frame() > 0) {
                menu_set(MENU_15);
            } else {
                menu_set(MENU_GAMEPLAY);
            }
        }
    }

    if (menu_update1() == 0) {
        button = joy_get_pressed(0);

        if (gPauseState != 0) {
            draw_pause_screen_freeze_frame(&gCurGfx);
        }

        if (gPauseState == 0) {
            update_objects();
            func_80042174(0);

            if ((camera_is_alternate_active() == 0) && (D_8008C94C == 0) && (func_800143FC() == 0) && ((button & START_BUTTON) != 0) && (main_get_bits(BIT_44F) == 0)) {
                gPauseState = 1;
                joy_set_button_mask(0, START_BUTTON);
                menu_set(MENU_PAUSE);
            }

            gDLL_29_Gplay->vtbl->tick();
        } else {
            update_obj_models();
        }

        if (gPauseState == 0) {
            update_PlayerPosBuffer();
        }

        menu_update2();
        func_800591EC();
        func_8004A67C();
        map_update_streaming();
        func_800210DC();

        gDLL_4_Race->vtbl->func14();

        if (gPauseState == 0) {
            func_8004225C(&gCurGfx, &gCurMtx, &gCurVtx, &gCurPol, &gCurVtx, &gCurPol);
        }

        gDLL_20_Screens->vtbl->draw(&gCurGfx);
        menu_draw(&gCurGfx, &gCurMtx, &gCurVtx, &gCurPol);

        D_8008C94C -= gUpdateRate;

        if ((s32)D_8008C94C < 0) {
            D_8008C94C = 0;
        }
    }
}

