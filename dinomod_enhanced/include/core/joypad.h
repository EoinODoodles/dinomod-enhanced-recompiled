#pragma once

#include "PR/ultratypes.h"

#define MAX_BUFFERED_CONT_SNAPSHOTS 4

void joyDisableStick(int port);
u16 joyGetButtonsRaw(int port);
