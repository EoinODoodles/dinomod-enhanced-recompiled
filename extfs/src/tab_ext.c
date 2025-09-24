#include "tab_ext.h"

#include "recomputils.h"
#include "extfs_common.h"
#include "fst_ext.h"

#include "PR/ultratypes.h"
#include "sys/fs.h"
#include "sys/memory.h"

/**
 * @file tab_ext.c
 * @brief Extendable filesystem tab(le)-based files.
 *
 * Allows mods to modify and extend filesystem files that use .TAB/.BIN pairs.
 * Doesn't support all .TAB files as a fair amount are pretty unique.
 */

static TabExtEntry *load_tab_ext_entry(TabExt *tab, s32 tabIdx, const void *existingData, u32 existingDataSize);

static void resize_tab(TabExt *tab, u32 newCount) {
    extfs_assert(tab->rebuiltEntries == NULL, "[extfs] Cannot resize rebuilt tab: %s!", tab->name);

    u32 numToCopy = newCount < tab->entryCount ? newCount : tab->entryCount;

    u32 *newOriginalEntries = recomp_alloc(newCount * sizeof(u32));
    bcopy(tab->originalEntries, newOriginalEntries, numToCopy * sizeof(u32));
    for (u32 i = numToCopy; i < newCount; i++) {
        // New entries are all zero-length to start
        newOriginalEntries[i] = newOriginalEntries[numToCopy - 1];
    }
    recomp_free(tab->originalEntries);
    tab->originalEntries = newOriginalEntries;

    TabExtEntry *newReplacements = recomp_alloc(newCount * sizeof(TabExtEntry));
    bcopy(tab->replacements, newReplacements, numToCopy * sizeof(TabExtEntry));
    bzero(newReplacements + numToCopy, (newCount - numToCopy) * sizeof(TabExtEntry));
    for (u32 i = numToCopy; i < tab->entryCount; i++) {
        recomp_free(tab->replacements[i].data);
    }
    recomp_free(tab->replacements);
    tab->replacements = newReplacements;

    extfs_log("[extfs] Resized tab %s: %d -> %d\n", tab->name, tab->entryCount, newCount);

    tab->entryCount = newCount;
    tab->sizeBytes = newCount * sizeof(u32);
    if (tab->hasEndMarker) {
        tab->sizeBytes += sizeof(u32);
    }
}

void tab_ext_set_entry_replacement(TabExt *tab, s32 tabIdx, const void *data, u32 sizeBytes) {
    extfs_assert(extfsLoadStage == EXTFS_STAGE_SUBFILE_REPLACEMENTS, "[extfs] Cannot set replacements outside of replacements load stage/event.");
    extfs_assert(tabIdx >= 0, "[extfs] Negative tab index given for tab %s: %d", tab->name, tabIdx);

    if ((u32)tabIdx >= tab->entryCount) {
        resize_tab(tab, tabIdx + 1);
    }

    TabExtEntry *entry = &tab->replacements[tabIdx];
    if (entry->data != NULL) {
        extfs_log_error("[extfs] File %s/%d already has a replacement!\n", tab->name, tabIdx);
        return;
    }

    load_tab_ext_entry(tab, tabIdx, data, sizeBytes);

    extfs_log("[extfs] Set replacement file data for %s/%d.\n", tab->name, tabIdx);
}

void *tab_ext_get_entry(TabExt *tab, s32 tabIdx, u32 *outSize) {
    extfs_assert(extfsLoadStage == EXTFS_STAGE_MODIFICATIONS, "[extfs] Cannot get tab entries outside of modifications load stage/event.");
    extfs_assert(tabIdx >= 0, "[extfs] Negative tab index given for tab %s: %d", tab->name, tabIdx);

    if ((u32)tabIdx >= tab->entryCount) {
        resize_tab(tab, tabIdx + 1);
    }

    TabExtEntry *entry = load_tab_ext_entry(tab, tabIdx, NULL, 0);
    if (outSize != NULL) {
        *outSize = entry->sizeBytes;
    }

    return entry->data;
}

void *tab_ext_resize_entry(TabExt *tab, s32 tabIdx, u32 newSize) {
    extfs_assert(extfsLoadStage == EXTFS_STAGE_MODIFICATIONS, "[extfs] Cannot resize tab entries outside of modifications load stage/event.");
    extfs_assert(tabIdx >= 0, "[extfs] Negative tab index given for tab %s: %d", tab->name, tabIdx);

    if ((u32)tabIdx >= tab->entryCount) {
        resize_tab(tab, tabIdx + 1);
    }

    TabExtEntry *entry = load_tab_ext_entry(tab, tabIdx, NULL, 0);

    if (entry->sizeBytes == newSize) {
        // No need to resize
        return entry->data;
    }

    void *oldData = entry->data;
    u32 oldSize = entry->sizeBytes;

    entry->data = recomp_alloc(newSize);
    extfs_assert(entry->data, "[extfs] extfs_tab_resize_entry(%d) resize recomp_alloc failed: %d", tab->id, newSize);

    u32 toCopy = oldSize;
    if (toCopy > newSize) {
        toCopy = newSize; // truncate
    }

    bcopy(oldData, entry->data, toCopy);
    if (toCopy < newSize) {
        bzero((u8*)entry->data + toCopy, newSize - toCopy);
    }
    recomp_free(oldData);

    entry->sizeBytes = newSize;

    extfs_log("[extfs] Resized file data for %s/%d %x -> %x.\n", tab->name, tabIdx, oldSize, newSize);

    return entry->data;
}

void tab_ext_init(TabExt *tab) {
    if (tab->originalEntries == NULL) {
        u32 size = fst_ext_get_file_size(tab->id);

        void *originalEntries = recomp_alloc(size);
        extfs_assert(originalEntries != NULL, "[extfs] load_tab_ext(%d) originalEntries recomp_alloc failed: %d", tab->id, size);

        fst_ext_read_from_file(tab->id, originalEntries, 0, size);
        tab->originalEntries = originalEntries;
        tab->sizeBytes = size;

        u32 entryCount = size / sizeof(u32);
        if (tab->originalEntries[entryCount - 1] == 0xFFFFFFFF) {
            // Has special end marker
            tab->hasEndMarker = TRUE;
            entryCount--;
        }
        // Last number is not a real entry, just there to calculate the size of the prev
        entryCount--;
        tab->entryCount = entryCount;

        extfs_log("[extfs] Loaded original tab %s with %d entries.\n", tab->name, entryCount);
    }

    if (tab->replacements == NULL) {
        u32 replacementsSize = sizeof(TabExtEntry) * tab->entryCount;

        tab->replacements = recomp_alloc(replacementsSize);
        extfs_assert(tab->replacements != NULL, "[extfs] load_tab_ext(%d) replacements recomp_alloc failed: %d", tab->id, replacementsSize);

        bzero(tab->replacements, replacementsSize);
    }
}

static TabExtEntry *load_tab_ext_entry(TabExt *tab, s32 tabIdx, const void *existingData, u32 existingDataSize) {
    TabExtEntry *entry = &tab->replacements[tabIdx];
    if (entry->data == NULL) {
        if (tab->stride == 1) {
            if (existingData == NULL) {
                // No stride, no existing data hint
                u32 offset = tab->originalEntries[tabIdx];
                u32 size = existingData == NULL ? tab->originalEntries[tabIdx + 1] - offset : existingDataSize;

                entry->sizeBytes = size;
                entry->data = recomp_alloc(size);
                extfs_assert(entry->data != NULL, "[extfs] load_tab_ext_entry(%d, %d) entry data recomp_alloc failed: %d", 
                    tab->id, tabIdx, size);

                fst_ext_read_from_file(tab->binId, entry->data, offset, size);
                
                extfs_log("[extfs] Loaded original file data for %s/%d.\n", tab->name, tabIdx);
            } else {
                // No stride, has existing data hint
                entry->sizeBytes = existingDataSize;
                entry->data = recomp_alloc(existingDataSize);
                extfs_assert(entry->data != NULL, "[extfs] load_tab_ext_entry(%d, %d) entry data recomp_alloc failed: %d", 
                    tab->id, tabIdx, existingDataSize);
                
                bcopy(existingData, entry->data, existingDataSize);

                extfs_log("[extfs] Loaded original file data (with replacement) for %s/%d.\n", tab->name, tabIdx);
            }
        } else {
            // Read all entries in stride
            s32 baseTabIdx = (tabIdx / tab->stride) * tab->stride;
            u32 fullStrideSize = tab->originalEntries[baseTabIdx + tab->stride] - tab->originalEntries[baseTabIdx];
            u32 baseOffset = tab->originalEntries[baseTabIdx];

            void *fullStrideData = recomp_alloc(fullStrideSize);
            extfs_assert(fullStrideData != NULL, "[extfs] load_tab_ext_entry(%d, %d) fullStrideData recomp_alloc failed: %d", 
                tab->id, tabIdx, fullStrideSize);

            fst_ext_read_from_file(tab->binId, fullStrideData, baseOffset, fullStrideSize);

            // Break up into individual entry data
            u32 fullStrideOffset = 0;
            for (s32 i = 0; i < tab->stride; i++) {
                TabExtEntry *_entry = &tab->replacements[baseTabIdx + i];

                u32 offset = tab->originalEntries[baseTabIdx + i];
                u32 originalEntrySize = tab->originalEntries[baseTabIdx + i + 1] - offset;

                if (existingData == NULL || _entry != entry) {
                    // No existing data or not requested entry, use original data
                    _entry->sizeBytes = originalEntrySize;
                    _entry->data = recomp_alloc(originalEntrySize);
                    extfs_assert(_entry->data != NULL, "[extfs] load_tab_ext_entry(%d, %d) entry data recomp_alloc failed: %d", 
                        tab->id, tabIdx, originalEntrySize);

                    bcopy((void*)((u32)fullStrideData + fullStrideOffset), _entry->data, originalEntrySize);
                } else {
                    // Existing data and request entry, use existing data hint
                    _entry->sizeBytes = existingDataSize;
                    _entry->data = recomp_alloc(existingDataSize);
                    extfs_assert(_entry->data != NULL, "[extfs] load_tab_ext_entry(%d, %d) entry data recomp_alloc failed: %d", 
                        tab->id, tabIdx, existingDataSize);
                    
                    bcopy(existingData, _entry->data, existingDataSize);
                }

                fullStrideOffset += originalEntrySize;
            }

            if (fullStrideOffset != fullStrideSize) {
                extfs_log_error("[extfs] load_tab_ext_entry(%d, %d) Leftovers from breaking up stride! fullStrideOffset: %x, fullStrideSize: %x\n", 
                    tab->id, tabIdx, fullStrideOffset, fullStrideSize);
            }

            // Clean up
            recomp_free(fullStrideData);

            if (existingData == NULL) {
                extfs_log("[extfs] Loaded original file data for %s/%d-%d.\n", tab->name, baseTabIdx, baseTabIdx + tab->stride);
            } else {
                extfs_log("[extfs] Loaded original file data (with replacement for %d) for %s/%d-%d.\n", tabIdx, tab->name, baseTabIdx, baseTabIdx + tab->stride);
            }
        }
    }

    return entry;
}

void tab_ext_rebuild(TabExt *tab) {
    // Rebuild tab with replacements in mind
    s32 *relocs = recomp_alloc(tab->entryCount * sizeof(s32));
    extfs_assert(relocs != NULL, "[extfs] rebuild_tab %d relocs recomp_alloc failed: %d", 
        tab->id, tab->entryCount * sizeof(s32));
    tab->relocations = relocs;

    u32 *newTab = mmAlloc(tab->sizeBytes, COLOUR_TAG_GREY, NULL);
    extfs_assert(newTab != NULL, "[extfs] rebuild_tab %d newTab malloc failed: %d", 
        tab->id, tab->sizeBytes);
    tab->rebuiltEntries = newTab;

    u32 newOffset = 0;
    s32 hasEndMarker = FALSE;

    for (u32 i = 0; i < tab->entryCount; i++) {
        u32 originalOffset = tab->originalEntries[i];
        u32 originalSize = tab->originalEntries[i + 1] - originalOffset;

        newTab[i] = newOffset;
        // Calculate relocations to convert new ROM offsets to original ROM offsets for unmodified files
        relocs[i] = (s32)originalOffset - (s32)newOffset;

        if (tab->replacements[i].data != NULL) {
            newOffset += tab->replacements[i].sizeBytes;
        } else {
            newOffset += originalSize;
        }
    }

    // TODO: originalEntries can be freed

    newTab[tab->entryCount] = newOffset;
    if (tab->hasEndMarker) {
        newTab[tab->entryCount + 1] = 0xFFFFFFFF;
    }

    // Merge stride data
    if (tab->stride > 1) {
        for (u32 i = 0; i < tab->entryCount; i += tab->stride) {
            if (tab->replacements[i].data == NULL) {
                continue;
            }
    
            u32 fullStrideSize = tab->rebuiltEntries[i + tab->stride] - tab->rebuiltEntries[i];
            void *mergedData = recomp_alloc(fullStrideSize);
            extfs_assert(mergedData != NULL, "[extfs] merge_tab_ext_strides %d mergedData recomp_alloc failed: %d", tab->id, fullStrideSize);
    
            u32 sizeLeft = fullStrideSize;
            for (s32 k = 0; k < tab->stride; k++) {
                TabExtEntry *entry = &tab->replacements[i + k];
    
                bcopy(entry->data, mergedData, entry->sizeBytes);
                recomp_free(entry->data);
    
                entry->data = mergedData;
                entry->maxReadSizeBytes = sizeLeft;
    
                mergedData = (void*)((u32)mergedData + entry->sizeBytes);
                sizeLeft -= entry->sizeBytes;
            }
    
            extfs_log("[extfs] Merged data for %s/%d-%d.\n", tab->name, i, i + tab->stride);
        }
    } else {
        for (u32 i = 0; i < tab->entryCount; i++) {
            TabExtEntry *entry = &tab->replacements[i];
            if (entry->data != NULL) {
                entry->maxReadSizeBytes = entry->sizeBytes;
            }
        }
    }

    extfs_log("[extfs] Rebuilt tab %s.\n", tab->name);
}

_Bool tab_ext_get_rebuilt_entries(TabExt *tab, void **outRebuiltEntries) {
    if (tab->rebuiltEntries != NULL) {
        *outRebuiltEntries = tab->rebuiltEntries;
        return TRUE;
    }

    return FALSE;
}

_Bool tab_ext_get_rebuilt_size(TabExt *tab, u32 *outSize) {
    if (tab->replacements != NULL) {
        *outSize = tab->sizeBytes;
        return TRUE;
    }

    return FALSE;
}

static s32 tab_find_idx(const TabExt *tab, u32 offset) {
    for (u32 i = 0; i < tab->entryCount; i++) {
        u32 start = tab->rebuiltEntries[i];
        u32 end = tab->rebuiltEntries[i + 1];

        if (offset >= start && offset < end) {
            return i;
        }
    }

    return -1;
}

_Bool tab_ext_try_read_bin(TabExt *tab, void *dst, u32 offset, u32 size) {
    if (tab->rebuiltEntries == NULL) {
        return FALSE;
    }

    // Find tab index of the file referenced by the offset
    s32 tabIdx = tab_find_idx(tab, offset);
    if (tabIdx == -1) {
        extfs_error_box("[extfs] Couldn't find tab index for offset 0x%x in %s. Bad things to follow!\n", offset, tab->name);
        // We don't have the correct ROM address to read, just return all zeroes
        bzero(dst, size);
        return TRUE;
    }

    // Find the replacement for the file, if any
    TabExtEntry *replacement = &tab->replacements[tabIdx];

    if (replacement->data == NULL) {
        // Adjust ROM address back to original
        s32 reloc = tab->relocations[tabIdx];
        fst_ext_read_from_file(tab->binId, dst, offset + reloc, size);
    } else {
        // Read from replacement file
        u32 relativeOffset = offset - tab->rebuiltEntries[tabIdx];
        u32 availableSize = replacement->maxReadSizeBytes - relativeOffset;
        if (size > availableSize) {
            extfs_error_box("[extfs] Replacement not big enough for %s[%d]! Requested size: %x, Available size: %x",
                tab->name, tabIdx, size, availableSize);
        }

        bcopy((void*)((u32)replacement->data + relativeOffset), dst, size);

        extfs_log("[extfs] Using replacement for %s[%d]\n", tab->name, tabIdx);
    }

    return TRUE;
}
