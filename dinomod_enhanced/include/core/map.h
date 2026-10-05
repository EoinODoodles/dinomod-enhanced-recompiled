#pragma once

#include "PR/ultratypes.h"
#include "game/objects/object.h"

void blockAddLODAnimator(Object* lodAnimator);
void blockRemoveLODAnimator(Object* lodAnimator);

typedef enum {
    BlockTexScrollAdd_REUSE_Match_by_Texture = 1,
    BlockTexScrollAdd_REUSE_Match_by_USpeedA = 2,
    BlockTexScrollAdd_REUSE_Match_by_VSpeedA = 4,
    BlockTexScrollAdd_REUSE_Match_by_USpeedB = 8,
    BlockTexScrollAdd_REUSE_Match_by_VSpeedB = 0x10,
    BlockTexScrollAdd_REUSE_Match_by_WidthA = 0x20,
    BlockTexScrollAdd_REUSE_Match_by_HeightA = 0x40,
    BlockTexScrollAdd_REUSE_Match_by_WidthB = 0x80,
    BlockTexScrollAdd_REUSE_Match_by_HeightB = 0x100,
} BlockTexScrollAdd_RecycleOptions;

#define BlockTexScrollAdd_REUSE_Match_by_SpeedsA (BlockTexScrollAdd_REUSE_Match_by_USpeedA | BlockTexScrollAdd_REUSE_Match_by_VSpeedA)
#define BlockTexScrollAdd_REUSE_Match_by_SpeedsB (BlockTexScrollAdd_REUSE_Match_by_USpeedB | BlockTexScrollAdd_REUSE_Match_by_VSpeedB)
#define BlockTexScrollAdd_REUSE_Match_by_DimensionsA (BlockTexScrollAdd_REUSE_Match_by_WidthA | BlockTexScrollAdd_REUSE_Match_by_HeightA)
#define BlockTexScrollAdd_REUSE_Match_by_DimensionsB (BlockTexScrollAdd_REUSE_Match_by_WidthB | BlockTexScrollAdd_REUSE_Match_by_HeightB)
#define BlockTexScrollAdd_REUSE_Match_by_A (BlockTexScrollAdd_REUSE_Match_by_SpeedsA | BlockTexScrollAdd_REUSE_Match_by_DimensionsA)
#define BlockTexScrollAdd_REUSE_Match_by_B (BlockTexScrollAdd_REUSE_Match_by_SpeedsB | BlockTexScrollAdd_REUSE_Match_by_DimensionsB)

void blockTexscrollSetWithTexture(u32 scrollerID, s32 uSpeedA, s32 vSpeedA, s32 widthA, s32 heightA, s32 uSpeedB, s32 vSpeedB, s32 widthB, s32 heightB, Texture* texture);
s32 blockTexscrollAddWithTexture(s32 uSpeedA, s32 vSpeedA, s32 widthA, s32 heightA, s32 uSpeedB, s32 vSpeedB, s32 widthB, s32 heightB, Texture* texture, u16 recycleOptions);
Texture* blockTexscrollGetTexture(u8 scrollHandlerID);
