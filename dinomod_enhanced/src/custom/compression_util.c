#include "recomputils.h"

#include "PR/ultratypes.h"
#include "PR/os.h"
#include "sys/rarezip.h"
#include "sys/memory.h"

void* dinomod_model_decompress(void *data, u32 size, u32 *outSize) {
    s32 decompressedSize = rarezip_uncompress_size((u8*)data + 8);
    if (decompressedSize <= 0) {
        // Already decompressed or zero size
        void *newData = recomp_alloc(size);
        bcopy(data, newData, size);

        if (outSize != NULL) {
            *outSize = size;
        }

        return newData;
    }

    // header + gzip header - 1 + data
    u32 newDataOffset = 8 + 4; // Note: Must be 4-byte aligned
    u32 newSize = newDataOffset + decompressedSize;
    void *newData = recomp_alloc(newSize);
    bzero(newData, newSize);

    bcopy(data, newData, 8); // header
    *((s32*)((u8*)newData + 0x8)) = -1; // decompressedSize
    rarezip_uncompress((u8*)data + 8, (u8*)newData + newDataOffset, decompressedSize);

    if (outSize != NULL) {
        *outSize = newSize;
    }

    return newData;
}
