#pragma once

#include "PR/ultratypes.h"

typedef enum {
    PauseBlock_Off,
    PauseBlock_Next_Tick_Only,
    PauseBlock_On_Until_Removed
} PauseBlockingStates;

void main_block_pausing(PauseBlockingStates value);
