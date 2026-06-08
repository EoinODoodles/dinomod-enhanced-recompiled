#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "dll.h"

#include "recomp/dlls/engine/30_task_recomp.h"

extern u8 sRecentlyCompleted[5];
extern u8 sCompletionIdx;
extern s8 sRecentlyCompletedNextIdx;

RECOMP_PATCH void task_mark_task_completed(u8 task) {
    s16 i;
    s16 bs_entry;
    s16 bit_idx;
    u32 bs_value;
    s16 bs_entry2;

    // @recomp: Bail if already completed ever, rather than only checking the recent list
    // for (i = 0; i < 5; i++) {
    //     if (task == sRecentlyCompleted[i]) {
    //         return;
    //     }
    // }
    bs_entry = task / 32;
    bit_idx = task % 32;
    bs_value = main_get_bits(BIT_Task_Bits_1 + bs_entry);

    if ((bs_value >> bit_idx) & 1) {
        return;
    }

    // @recomp: Don't let task 0 be added to the recent list
    if (task != 0) {
        // Add task to recently completed list
        if (sRecentlyCompletedNextIdx != 4) {
            // Append if there's room
            sRecentlyCompletedNextIdx++;
            sRecentlyCompleted[sRecentlyCompletedNextIdx] = task;

            main_set_bits(sRecentlyCompletedNextIdx + BIT_Recent_Task_1, task);
        } else {
            // Otherwise, shift everything down and add to the end
            for (i = 0; i < 4; i++) {
                sRecentlyCompleted[i] = sRecentlyCompleted[i + 1];
            }

            sRecentlyCompleted[4] = task;

            for (i = 0; i < 5; i++) {
                main_set_bits(BIT_Recent_Task_1 + i, sRecentlyCompleted[i]);
            }
        }
    }

    // Set bit for task in bitstring
    //
    // This is a 256-bit bitstring from bit entry 303 to 315 (8 entries)
    bs_entry = (task / 32) + BIT_Task_Bits_1;

    bs_value = main_get_bits(bs_entry);
    bit_idx = task % 32;
    bs_value = (1 << (bit_idx)) | bs_value;

    main_set_bits(bs_entry, bs_value);

    // Determine new completion index
    if (sCompletionIdx == task) {
        do {
            sCompletionIdx++;

            bs_entry2 = (sCompletionIdx / 32) + BIT_Task_Bits_1;
            if (bs_entry2 != bs_entry) {
                bs_entry = bs_entry2;

                bs_value = main_get_bits(bs_entry2);
            }
            bit_idx = sCompletionIdx % 32;

        } while ((bs_value >> bit_idx) & 1);

        main_set_bits(BIT_Furthest_Completed_Task, sCompletionIdx);
    }

    // hmm
    if (!task) {
        gDLL_29_Gplay->vtbl->savepoint(NULL, 0, GPLAY_SAVEPOINT_SkipMapSave, map_get_layer());
    }
}

/** Allows the "Previously on Dinosaur Planet" screen to only retrieve the 3 most recent tasks in the history (instead of the oldest 3) */
RECOMP_PATCH char *task_get_recently_completed_task_text(u8 idx) {
    s32 startIndex = sRecentlyCompletedNextIdx - 2;
    if (startIndex < 0)
        startIndex = 0;
    if (idx > 2)
        idx = 2;

    return gDLL_21_Gametext->vtbl->get_text(sRecentlyCompleted[startIndex + idx] + 244, 0);
}
