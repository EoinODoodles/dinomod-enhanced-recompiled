#pragma once

#include "modding.h"

#include "PR/ultratypes.h"

#define EXTFS_ON_LOAD_FST_REPLACEMENTS_CALLBACK RECOMP_CALLBACK("extfs", extfs_on_load_fst_replacements)
#define EXTFS_ON_LOAD_REPLACEMENTS_CALLBACK RECOMP_CALLBACK("extfs", extfs_on_load_replacements)
#define EXTFS_ON_LOAD_MODIFICATIONS_CALLBACK RECOMP_CALLBACK("extfs", extfs_on_load_modifications)

/* ----- FST ---------------------*/
RECOMP_IMPORT("extfs", void extfs_set_fst_file_replacement(s32 fileID, const void *data, u32 sizeBytes))

/* ----- BLOCKS ---------------------*/
RECOMP_IMPORT("extfs", void extfs_blocks_set_replacement(s32 blockTabIdx, const void *data, u32 sizeBytes))
RECOMP_IMPORT("extfs", void *extfs_blocks_get(s32 blockTabIdx, u32 *outSize))
RECOMP_IMPORT("extfs", void *extfs_blocks_resize(s32 blockTabIdx, u32 newSize))

/* ----- HITS ---------------------*/
RECOMP_IMPORT("extfs", void extfs_hits_set_replacement(s32 hitsTabIdx, const void *data, u32 sizeBytes))
RECOMP_IMPORT("extfs", void *extfs_hits_get(s32 hitsTabIdx, u32 *outSize))
RECOMP_IMPORT("extfs", void *extfs_hits_resize(s32 hitsTabIdx, u32 newSize))

/* ----- MAPS ---------------------*/
RECOMP_IMPORT("extfs", void extfs_maps_set_replacement(s32 mapID, s32 segment, const void *data, u32 sizeBytes))
RECOMP_IMPORT("extfs", void *extfs_maps_get(s32 mapID, s32 segment, u32 *outSize))
RECOMP_IMPORT("extfs", void *extfs_maps_resize(s32 mapID, s32 segment, u32 newSize))

/* ----- MODELS ---------------------*/
RECOMP_IMPORT("extfs", void extfs_models_set_replacement(s32 modelTabIdx, const void *data, u32 sizeBytes))
RECOMP_IMPORT("extfs", void *extfs_models_get(s32 modelTabIdx, u32 *outSize))
RECOMP_IMPORT("extfs", void *extfs_models_resize(s32 modelTabIdx, u32 newSize))

/* ----- OBJECTS ---------------------*/
RECOMP_IMPORT("extfs", void extfs_objects_set_replacement(s32 objectTabIdx, const void *data, u32 sizeBytes))
RECOMP_IMPORT("extfs", void *extfs_objects_get(s32 objectTabIdx, u32 *outSize))
RECOMP_IMPORT("extfs", void *extfs_objects_resize(s32 objectTabIdx, u32 newSize))

/* ----- SCREENS ---------------------*/
RECOMP_IMPORT("extfs", void extfs_screens_set_replacement(s32 screenTabIdx, const void *data, u32 sizeBytes))
RECOMP_IMPORT("extfs", void *extfs_screens_get(s32 screenTabIdx, u32 *outSize))
RECOMP_IMPORT("extfs", void *extfs_screens_resize(s32 screenTabIdx, u32 newSize))
