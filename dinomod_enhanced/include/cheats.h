#pragma once

#include "PR/gbi.h"
#include "types.h"

typedef enum {
    CHEAT_Big_Head,
    CHEAT_Giant_Head,
    CHEAT_Small_Head,
    CHEAT_Tiny_Head
} CheatIDs;

void cheatMessageTick(void);
void cheatMessagePrint(Gfx** gdl, Mtx** mtxs, Vertex** vtxs);
