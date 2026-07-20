#include "dll.h"
#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/menu.h"
#include "types.h"
#include "dlls/engine/29_gplay.h"

#include "core/mp3/mp3.h"
#include "core/main.h"
#include "engine/78_credits.h"

extern GameState *gGplayState;
extern BitTableEntry *gFile_BITTABLE;
extern s16 gSizeBittable;

extern void main_func_8001440C(s32 arg0);

/** Prevents cases where the game would try to set out of bounds flags, which would cause data corruption */
RECOMP_PATCH void mainSetBits(s32 entry, u32 value) {
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
            case 1: // Saved with savepoints
                bitString = &gGplayState->save.main.bitString[0];
                break;
            case 2: // Always saved
                bitString = &gGplayState->save.file.bitString[0];
                break;
            case 3: // Saved with savepoints that include a map save
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
RECOMP_PATCH s32 mainIncrementBits(s32 entry) {
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

    val = mainGetBits(entry) + 1;

    maxVal = 1 << ((gFile_BITTABLE[entry].field_0x2 & 0x1f) + 1);

    if (val < maxVal) {
        mainSetBits(entry, val);
    } else {
        val -= 1;
    }

    return val;
}

/** Prevents cases where the game would try to decrement out of bounds flags, which would cause data corruption */
RECOMP_PATCH s32 mainDecrementBits(s32 entry) {
    s32 val = mainGetBits(entry);

    //@recomp: Prevent data corruption
    if (entry == -1) {
        return 0;
    }
    if (entry < 0 || entry >= gSizeBittable) {
        recomp_eprintf("Attempted to decrement out of bounds flagID (%04d)!\n", entry);
        return 0;
    }

    if (val != 0) {
        mainSetBits(entry, --val);
        return val;
    }

    return 0;
}

/** Allows pausing to be blocked temporarily */
static s8 rsBlockPausing = PauseBlock_Off;

/** Allows pausing to be blocked temporarily */
void main_block_pausing(PauseBlockingStates value) {
    rsBlockPausing = value;
    main_func_8001440C(value ? 1 : 0); // tell main code to disallow/allow pausing
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
RECOMP_HOOK("main_func_80013D80") void main_func_80013D80_hook(void) {
    if (rsBlockPausing) {
        gPauseState = 0;
        
        if (rsBlockPausing != PauseBlock_On_Until_Removed) {
            main_block_pausing(PauseBlock_Off);
        }

        if (menuGetCurrent() == MENU_PAUSE) {
            if (credits_get_frame() > 0) {
                menuSet(MENU_CREDITS);
            } else {
                menuSet(MENU_GAMEPLAY);
            }
        }
    }
}

extern s8 gPauseState;

RECOMP_PATCH void mainSetPauseState(s32 state) {
    gPauseState = state;

    //@recomp: pause/unpause MP3s as well
    if (state) {
        mp3PauseIfPlaying();
    } else {
        mp3UnpauseIfPaused();
    }
}

RECOMP_PATCH void mainUnpause(void) {
    gPauseState = 0;

    //@recomp: unpause MP3s as well
    mp3UnpauseIfPaused();
}
