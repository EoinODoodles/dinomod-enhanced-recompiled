#pragma once

#include "PR/ultratypes.h"
#include "sys/fonts.h"

void dll_73_set_fontIDs(FontID light, FontID dark);
FontID dll_73_get_fontID_light(void);
FontID dll_73_get_fontID_dark(void);
void dll_73_set_font_colours(u32 light, u32 dark);
void dll_73_enable_drop_shadow(u8 enable);
void dll_73_set_drop_shadow_colour(u32 colour);
void dll_73_set_drop_shadow_position(s8 x, s8 y);
