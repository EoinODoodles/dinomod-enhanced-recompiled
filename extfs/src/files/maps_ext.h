#pragma once

#include "PR/ultratypes.h"

_Bool maps_ext_try_read_tab(void **outTab);
_Bool maps_ext_try_read_bin(void *dst, u32 offset, u32 size);
_Bool maps_ext_try_get_tab_size(u32 *outSize);
