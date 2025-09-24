#pragma once

#include "PR/ultratypes.h"

typedef struct {
    void *data;
    // The size of this individual entry.
    u32 sizeBytes;
    // The maximum number of bytes that can be read starting from the beginning of this entry.
    // For tabs with a stride, this will include all of the entries after this one up to the end of the stride.
    // For tabs without a stride, this will equal [sizeBytes].
    u32 maxReadSizeBytes;
} TabExtEntry;

typedef struct {
    const char *name;
    s32 id;
    s32 binId;
    // The number of files that the game reads at a time for this tab. Usually 1.
    // When >1, files within a stride will be merged together when the tab is rebuilt
    // so that the game can read across file boundaries within the stride.
    s32 stride;

    // Length: [entryCount]
    u32 *originalEntries;
    u32 entryCount;
    u32 sizeBytes;
    _Bool hasEndMarker;
    
    // Length: [entryCount]
    TabExtEntry *replacements;
    // Length: [entryCount]
    u32 *rebuiltEntries;
    // For each entry, an offset from the rebuilt entry offset to the original entry offset.
    // Necessary for entries that weren't replaced so the original ROM read can be performed.
    // Length: [entryCount]
    s32 *relocations;
} TabExt;

void tab_ext_init(TabExt *tab);
void tab_ext_set_entry_replacement(TabExt *tab, s32 tabIdx, const void *data, u32 sizeBytes);
void *tab_ext_get_entry(TabExt *tab, s32 tabIdx, u32 *outSize);
void *tab_ext_resize_entry(TabExt *tab, s32 tabIdx, u32 newSize);
void tab_ext_rebuild(TabExt *tab);
_Bool tab_ext_get_rebuilt_entries(TabExt *tab, void **outRebuiltEntries);
_Bool tab_ext_try_read_bin(TabExt *tab, void *dst, u32 offset, u32 size);
_Bool tab_ext_get_rebuilt_size(TabExt *tab, u32 *outSize);
