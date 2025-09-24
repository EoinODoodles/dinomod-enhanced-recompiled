#include "maps_ext.h"

#include "modding.h"
#include "extfs_common.h"
#include "tab_ext.h"

#include "PR/ultratypes.h"
#include "sys/fs.h"

static TabExt tab = {
    .name = "MAPS",
    .id = MAPS_TAB,
    .binId = MAPS_BIN,
    .stride = 7
};

_Bool maps_ext_try_read_tab(void **outTab) {
    return tab_ext_get_rebuilt_entries(&tab, outTab);
}

_Bool maps_ext_try_read_bin(void *dst, u32 offset, u32 size) {
    return tab_ext_try_read_bin(&tab, dst, offset, size);
}

_Bool maps_ext_try_get_tab_size(u32 *outSize) {
    return tab_ext_get_rebuilt_size(&tab, outSize);
}

EXTFS_ON_INIT_CALLBACK void maps_ext_init() {
    tab_ext_init(&tab);
}

EXTFS_ON_COMMIT_CALLBACK void maps_ext_commit() {
    tab_ext_rebuild(&tab);
}

RECOMP_EXPORT void extfs_maps_set_replacement(s32 mapID, s32 segment, const void *data, u32 sizeBytes) {
    tab_ext_set_entry_replacement(&tab, (mapID * 7) + segment, data, sizeBytes);
}

RECOMP_EXPORT void *extfs_maps_get(s32 mapID, s32 segment, u32 *outSize) {
    return tab_ext_get_entry(&tab, (mapID * 7) + segment, outSize);
}

RECOMP_EXPORT void *extfs_maps_resize(s32 mapID, s32 segment, u32 newSize) {
    return tab_ext_resize_entry(&tab, (mapID * 7) + segment, newSize);
}
