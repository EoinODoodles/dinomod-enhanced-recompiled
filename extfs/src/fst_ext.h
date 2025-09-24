#pragma once

#include "PR/ultratypes.h"

u32 fst_ext_get_file_size(s32 fileID);
void fst_ext_read_from_file(s32 fileID, void *dst, u32 offset, u32 size);
