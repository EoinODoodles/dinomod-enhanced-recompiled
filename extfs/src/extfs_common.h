#pragma once

#include "PR/ultratypes.h"
#include "sys/fs.h"

extern Fs *gFST;
extern u32 gLastFSTIndex;
extern s32 __fstAddress;
extern s32 __file1Address;

extern void read_from_rom(u32 romAddr, u8* dst, s32 size);

extern void bcopy_recomp(const void *src, void *dst, int length);
#define bcopy bcopy_recomp

extern void bzero_recomp(void *, int);
#define bzero bzero_recomp

#define EXTFS_ON_INIT_CALLBACK RECOMP_CALLBACK(".", _extfs_on_init)
#define EXTFS_ON_COMMIT_CALLBACK RECOMP_CALLBACK(".", _extfs_on_commit)

typedef enum {
    EXTFS_STAGE_UNINITIALIZED,
    EXTFS_STAGE_FST_REPLACEMENTS,
    EXTFS_STAGE_SUBFILE_REPLACEMENTS,
    EXTFS_STAGE_MODIFICATIONS,
    EXTFS_STAGE_COMMITTED
} ExtFsLoadStage;

extern ExtFsLoadStage extfsLoadStage;

void extfs_assert(_Bool condition, const char *fmt, ...);
void extfs_log(const char *fmt, ...);
void extfs_log_error(const char *fmt, ...);
void extfs_error_box(const char *fmt, ...);
