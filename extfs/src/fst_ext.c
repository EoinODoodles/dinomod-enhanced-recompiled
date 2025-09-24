#include "fst_ext.h"

#include "modding.h"
#include "extfs_common.h"

#include "PR/ultratypes.h"
#include "sys/fs.h"

typedef struct {
    const void *data;
    u32 size;
} FstExtEntry;

static FstExtEntry fstReplacements[NUM_FILES];

RECOMP_EXPORT void extfs_set_fst_file_replacement(s32 fileID, const void *data, u32 size) {
    extfs_assert(fileID >= 0 && fileID < NUM_FILES, "[extfs] File ID out of bounds: %d", fileID);

    FstExtEntry *replacement = &fstReplacements[fileID];
    replacement->data = data;
    replacement->size = size;

    extfs_log("[extfs] Set FST file replacement for %d.\n", fileID);
}

u32 fst_ext_get_file_size(s32 fileID) {
    FstExtEntry *replacement = &fstReplacements[fileID];
    if (replacement->data != NULL) {
        return replacement->size;
    } else {
        return gFST->offsets[fileID + 1] - gFST->offsets[fileID];
    }
}

void fst_ext_read_from_file(s32 fileID, void *dst, u32 offset, u32 size) {
    FstExtEntry *replacement = &fstReplacements[fileID];
    if (replacement->data != NULL) {
        // Read replacement file
        bcopy((u8*)replacement->data + offset, dst, size);
        //extfs_log("[extfs] Reading from FST file replacement for %d.\n", fileID);
    } else {
        // Read original ROM
        u32 fileOffset = gFST->offsets[fileID];

        read_from_rom((u32)&__file1Address + fileOffset + offset, dst, size);
    }
}
