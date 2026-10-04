#pragma once

#include "PR/ultratypes.h"
#include "game/objects/object.h"

void blockAddLODAnimator(Object* lodAnimator);
void blockRemoveLODAnimator(Object* lodAnimator);

void blockTexscrollSetWithTexture(u32 scrollerID, s32 uSpeedA, s32 vSpeedA, s32 widthA, s32 heightA, s32 uSpeedB, s32 vSpeedB, s32 widthB, s32 heightB, Texture* texture);
s32 blockTexscrollAddWithTexture(s32 uSpeedA, s32 vSpeedA, s32 widthA, s32 heightA, s32 uSpeedB, s32 vSpeedB, s32 widthB, s32 heightB, Texture* texture);
s32 blockTexscrollAddByTexture(s32 uSpeedA, s32 vSpeedA, s32 widthA, s32 heightA, s32 uSpeedB, s32 vSpeedB, s32 widthB, s32 heightB, Texture* texture);
Texture* blockTexscrollGetTexture(u8 scrollHandlerID);
