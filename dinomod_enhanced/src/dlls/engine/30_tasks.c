#include "modding.h"
#include "recomputils.h"
#include "recomp/dlls/engine/30_task_recomp.h"

#include "PR/ultratypes.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/print.h"
#include "dll.h"
#include "types.h"
#include "macros.h"

extern u8 sRecentlyCompleted[5];
extern u8 sCompletionIdx;
extern s8 sRecentlyCompletedNextIdx;

//Recomp mod-specific
static s8 firstZero = -1;
static s8 nextNonzero = -1;
static s8 taskHistory[5] = {0,0,0,0,0};
static s8 DEBUG = FALSE;

/** Finds the index of the first zero in the task history (or -1 if none found), and the index of the first nonzero item after that zero (or -1 if none found) */
static void get_first_zero_and_next_nonzero(s8 taskHistory[], s8* firstZero, s8 *nextNonzero){
    s8 i;

    *firstZero = -1;
    *nextNonzero = -1;

    for (i = 0; i < 5; i++) {
        //Find first 0 in list
        if (*firstZero == -1 && taskHistory[i] == 0){
            *firstZero = i;
            continue;
        }

        //Find first nonzero taskID after first 0 in list
        if (*firstZero >= 0 && taskHistory[i] > 0){
            *nextNonzero = i;
            break;
        }
    }
}

static void shift_task_history_indices(s8 taskHistory[], s8 firstZero, s8 nextNonzero){
    s8 i; 
    s8 j; 

    for (i = nextNonzero, j = 0; i < 5; i++, j++){
        taskHistory[firstZero + j] = taskHistory[i];
        taskHistory[i] = 0;
    }
};

/** Ensures that there isn't a gap in the task history (i.e. task0 isn't in the middle of the array like [5, 6, 0, 7, 8]) */
static void cleanup_task_history(){
    s8 i;
    Object* player = get_player();

    //Get the task history flags
    for (i = 0; i < 5; i++) {
        taskHistory[i] = main_get_bits(BIT_Recent_Task_1 + i);
    }

    //Shift indices to remove any zeroes that sit in the middle of the list
    get_first_zero_and_next_nonzero(taskHistory, &firstZero, &nextNonzero);
    while (firstZero >= 0 && nextNonzero > firstZero){
        shift_task_history_indices(taskHistory, firstZero, nextNonzero);
        get_first_zero_and_next_nonzero(taskHistory, &firstZero, &nextNonzero);
    }

    //Update the flags, the DLL's task history array, and the task history item count
    sRecentlyCompletedNextIdx = -1;
    for (i = 0; i < 5; i++) {
        if (taskHistory[i] > 0)
            sRecentlyCompletedNextIdx = i;
        sRecentlyCompleted[i] = taskHistory[i];
        if (player)
            main_set_bits(BIT_Recent_Task_1 + i, taskHistory[i]);
    }
}

/** Prints the task history throughout gameplay */
// TODO: ifdef DEBUG
RECOMP_CALLBACK("*", recomp_on_game_tick_start) void printTaskHistory() {
    if (!DEBUG)
        return;

    diPrintf("Task history: [%-3d, %-3d, %-3d, %-3d, %-3d] (%d)\n", 
        main_get_bits(BIT_Recent_Task_1 + 0), 
        main_get_bits(BIT_Recent_Task_1 + 1), 
        main_get_bits(BIT_Recent_Task_1 + 2), 
        main_get_bits(BIT_Recent_Task_1 + 3), 
        main_get_bits(BIT_Recent_Task_1 + 4), 
        main_get_bits(BIT_Recent_Task_1 + 5));

    diPrintf("First zero: %d\nNext nonzero: %d\n", firstZero, nextNonzero);
    diPrintf("Last occupied index: %d\n", sRecentlyCompletedNextIdx);
}

/** Patches the "Previously on Dinosaur Planet" screen to show your 3 most recent tasks (instead of the oldest 3 of the 5 in the task history) */
RECOMP_PATCH void task_load_recently_completed(void) {
    u8 val;

    cleanup_task_history();

    //Store upcoming task
    val = main_get_bits(BIT_Furthest_Completed_Task);
    sCompletionIdx = val;
    if (val == 0) {
        sCompletionIdx = 1;
        sRecentlyCompletedNextIdx = -1;
    }
}

/** Stop taskID 0 from ending up in the middle of the task history (which used happen when visiting the SwapStones, and other circumstances) */
RECOMP_PATCH void task_mark_task_completed(u8 task) {
    s16 i;
    s16 bs_entry;
    s16 bit_idx;
    u32 bs_value;
    s16 bs_entry2;

    cleanup_task_history();

    // Bail if already recently completed
    for (i = 0; i < 5; i++) {
        if (task == sRecentlyCompleted[i]) {
            return;
        }
    }

    // Set checkpoint and bail if task0
    if (task == 0) {
        gDLL_29_Gplay->vtbl->checkpoint(NULL, 0, 1, map_get_layer());
        return;
    }

    // Add task to recently completed list
    if (sRecentlyCompletedNextIdx < 4) {
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
}

/** Allows the "Previously on Dinosaur Planet" screen to only retrieve the 3 most recent tasks in the history (instead of the oldest 3) */
RECOMP_PATCH char *task_get_recently_completed_task_text(u8 idx) {
    s8 startIndex; 

    cleanup_task_history();
    
    startIndex = sRecentlyCompletedNextIdx - 2;
    if (startIndex < 0)
        startIndex = 0;
    if (idx > 2)
        idx = 2;

    return gDLL_21_Gametext->vtbl->get_text(sRecentlyCompleted[startIndex + idx] + 244, 0);
}
