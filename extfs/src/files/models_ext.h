#pragma once

#include "PR/ultratypes.h"

_Bool models_ext_try_read_tab(void **outTab);
_Bool models_ext_try_read_bin(void *dst, u32 offset, u32 size);
_Bool models_ext_try_get_tab_size(u32 *outSize);
