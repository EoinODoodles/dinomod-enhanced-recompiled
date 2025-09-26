#include "dll.h"
#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/print.h"
#include "types.h"
#include "dlls/engine/29_gplay.h"

extern GplayStruct7 *gGplayState;
extern BitTableEntry *gFile_BITTABLE;

/** Prevents cases where the game would try to set flag -1, which would cause data corruption */
RECOMP_PATCH void set_gplay_bitstring(s32 entry, u32 value) {
    u8 *bitString;
    u8 _pad[12]; // fake match
    s32 idx;
    s32 mask;
    s32 endBit;
    s32 startBit;

    //@recomp: Prevent data corruption
    if (entry < 0){
        recomp_eprintf("Attempted to increment negative flagID (%04d)!\n", entry);
        return;
    }

    if (entry != 149 && entry != 150 && entry != -1) {
        switch (gFile_BITTABLE[entry].field_0x2 >> 6) {
            case 0:
                bitString = &gGplayState->bitString[0];
                break;
            case 1:
                bitString = &gGplayState->unk0.unk0.bitString[0];
                break;
            case 2:
                bitString = &gGplayState->unk0.unk0.unk0.bitString[0];
                break;
            case 3:
                bitString = &gGplayState->unk0.bitString[0];
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

/** Prevents cases where the game would try to increment flag -1, which would cause data corruption */
RECOMP_PATCH s32 increment_gplay_bitstring(s32 entry) {
    s32 val;
    s32 maxVal;

    val = get_gplay_bitstring(entry) + 1;

    maxVal = 1 << ((gFile_BITTABLE[entry].field_0x2 & 0x1f) + 1);

    //@recomp: Prevent data corruption
    if (entry < 0){
        recomp_eprintf("Attempted to increment negative flagID (%04d)!\n", entry);
        return val < maxVal ? val : val - 1;
    }


    if (val < maxVal) {
        set_gplay_bitstring(entry, val);
    } else {
        val -= 1;
    }

    return val;
}

/** Prevents cases where the game would try to decrement flag -1, which would cause data corruption */
RECOMP_PATCH s32 decrement_gplay_bitstring(s32 entry) {
    s32 val = get_gplay_bitstring(entry);

    //@recomp: Prevent data corruption
    if (entry < 0){
        recomp_eprintf("Attempted to decrement negative flagID (%04d)!\n", entry);
        return val > 0 ? val - 1 : 0;
    }

    if (val != 0) {
        set_gplay_bitstring(entry, --val);
        return val;
    }
    return 0;
}
