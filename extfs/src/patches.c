#include "modding.h"

#include "sys/fs.h"
#include "sys/memory.h"

#include "extfs_common.h"
#include "fst_ext.h"
#include "files/blocks_ext.h"
#include "files/hits_ext.h"
#include "files/maps_ext.h"
#include "files/models_ext.h"
#include "files/objects_ext.h"
#include "files/screens_ext.h"

RECOMP_DECLARE_EVENT(extfs_on_load_fst_replacements());
RECOMP_DECLARE_EVENT(extfs_on_load_replacements());
RECOMP_DECLARE_EVENT(extfs_on_load_modifications());

RECOMP_DECLARE_EVENT(_extfs_on_init());
RECOMP_DECLARE_EVENT(_extfs_on_commit());

RECOMP_HOOK_RETURN("init_filesystem") void fs_init_hook() {
    extfsLoadStage = EXTFS_STAGE_FST_REPLACEMENTS;
    extfs_on_load_fst_replacements();

    _extfs_on_init();

    extfsLoadStage = EXTFS_STAGE_SUBFILE_REPLACEMENTS;
    extfs_on_load_replacements();

    extfsLoadStage = EXTFS_STAGE_MODIFICATIONS;
    extfs_on_load_modifications();

    extfsLoadStage = EXTFS_STAGE_COMMITTED;
    _extfs_on_commit();
}

RECOMP_PATCH void *read_alloc_file(u32 id, u32 a1)
{
    void *data;

    if (id > (u32)gFST->fileCount)
        return NULL;

    // @recomp: Extended filesystem patches
    switch (id) {
        case BLOCKS_TAB:
            if (blocks_ext_try_read_tab(&data)) return data;
            break;
        case HITS_TAB:
            if (hits_ext_try_read_tab(&data)) return data;
            break;
        case MAPS_TAB:
            if (maps_ext_try_read_tab(&data)) return data;
            break;
        case MODELS_TAB:
            if (models_ext_try_read_tab(&data)) return data;
            break;
        case OBJECTS_TAB:
            if (objects_ext_try_read_tab(&data)) return data;
            break;
        case SCREENS_TAB:
            if (screens_ext_try_read_tab(&data)) return data;
            break;
    }

    // @recomp: Rewrite to use fst_ext
    u32 size = fst_ext_get_file_size(id);

    data = mmAlloc(size, COLOUR_TAG_GREY, NULL);
    if (data == NULL)
        return NULL;

    fst_ext_read_from_file(id, data, 0, size);

    return data;
}

RECOMP_PATCH s32 read_file(u32 id, void *dest)
{
    if (id > (u32)gFST->fileCount)
        return NULL;

    // @recomp: Rewrite to use fst_ext
    u32 size = fst_ext_get_file_size(id);
    fst_ext_read_from_file(id, dest, 0, size);

    return size;
}

RECOMP_PATCH s32 read_file_region(u32 id, void *dst, u32 offset, s32 size)
{
    if (size == 0 || id > (u32)gFST->fileCount)
        return 0;

    // @recomp: Extended filesystem patches
    switch (id) {
        case BLOCKS_BIN:
            if (blocks_ext_try_read_bin(dst, offset, size)) return size;
            break;
        case HITS_BIN:
            if (hits_ext_try_read_bin(dst, offset, size)) return size;
            break;
        case MAPS_BIN:
            if (maps_ext_try_read_bin(dst, offset, size)) return size;
            break;
        case MODELS_BIN:
            if (models_ext_try_read_bin(dst, offset, size)) return size;
            break;
        case OBJECTS_BIN:
            if (objects_ext_try_read_bin(dst, offset, size)) return size;
            break;
        case SCREENS_BIN:
            if (screens_ext_try_read_bin(dst, offset, size)) return size;
            break;
    }

    // @recomp: Rewrite to use fst_ext
    gLastFSTIndex = id + 1;

    fst_ext_read_from_file(id, dst, offset, size);

    return size;
}

// TODO: deal with file_get_romaddr

RECOMP_PATCH s32 get_file_size(u32 id)
{
    if (id > (u32)gFST->fileCount)
        return NULL;

    // @recomp: Extended filesystem patches
    u32 size;
    switch (id) {
        case BLOCKS_BIN:
            if (blocks_ext_try_get_tab_size(&size)) return size;
            break;
        case HITS_BIN:
            if (hits_ext_try_get_tab_size(&size)) return size;
            break;
        case MAPS_BIN:
            if (maps_ext_try_get_tab_size(&size)) return size;
            break;
        case MODELS_BIN:
            if (models_ext_try_get_tab_size(&size)) return size;
            break;
        case OBJECTS_BIN:
            if (objects_ext_try_get_tab_size(&size)) return size;
            break;
        case SCREENS_BIN:
            if (screens_ext_try_get_tab_size(&size)) return size;
            break;
    }

    // @recomp: Rewrite to use fst_ext
    return fst_ext_get_file_size(id);
}
