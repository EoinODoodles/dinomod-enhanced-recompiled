#include "modding.h"
#include "common_objsetups.h"

#include "PR/ultratypes.h"
#include "PR/gbi.h"
#include "dlls/engine/6_amsfx.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "recomputils.h"
#include "sys/asset.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/pi.h"
#include "types.h"
#include "dll.h"

#include "recomp/dlls/objects/275_WarpPoint_recomp.h"

//TEMPORARY DEFINES
#define WarpPoint_obj_Control WarpPoint_control
#define WarpPoint_animCallback WarpPoint_anim_callback
#define NO_WARP_ID -1
//END OF TEMPORARY DEFINES

// #define BLOCK_WARP

typedef enum {
    WarpPoint_FLAG_1_Arrival_Sequence_Played = 1,
    WarpPoint_FLAG_2_Departure_Sequence_Played = 2,
    WarpPoint_FLAG_4_AnimCallback_Finished = 4,
    WarpPoint_FLAG_8_Warp_Queued = 8
} WarpPoint_CustomFlags;

typedef struct {
/*00*/ s16 warpDelayTimer;
/*02*/ s16 gamebit;                 //arrival sequence
/*04*/ s16 objectSeqIndex;          //arrival sequence
/*08*/ f32 range;
/*0C*/ u8 flags;    //@recomp: field repurposed as flags instead of Boolean
} WarpPoint_Data;

typedef enum {
    WarpPoint_OBJSEQ_0, //0x81 (outbound) (FADE NOT CHECKED) Warp immediately via objSeq message, has unseen player anims too (curling into ball?)
    WarpPoint_OBJSEQ_1, //0x82 (inbound) (no fade) Player curled and rolling in midair, uncurls and lands
    WarpPoint_OBJSEQ_2, //0x15E (inbound) (FADE NOT CHECKED) Player spat out during Galadon battle #1
    WarpPoint_OBJSEQ_3, //0x196 (inbound) (FADE NOT CHECKED) Player landing in Galadon's stomach
    WarpPoint_OBJSEQ_4, //0x15F (inbound) (FADE NOT CHECKED) Player spat out during Galadon battle #2
    WarpPoint_OBJSEQ_5, //0x1C3 (inbound) (FADE NOT CHECKED) Rocky swapping to Sabre (unused early draft?)
    WarpPoint_OBJSEQ_6, //0x2CE (inbound) (fade in) Sabre and Tricky thrown out of DarkIce Mines
    WarpPoint_OBJSEQ_7, //0x382 (inbound) (no fade) Sabre falling through vertical chute and crashing to ground in Lower Dragon Rock
    WarpPoint_OBJSEQ_8, //0x429 (inbound) (fade in) Sabre and Tricky walking towards camera 
    WarpPoint_OBJSEQ_9  //0x447 (inbound) (fade in) Player walks in towards camera, camera cuts behind them
} WarpPoint_ObjSeqs;

RECOMP_PATCH // offset: 0xAC | func: 1 | export: 1
void WarpPoint_obj_Control(Object* self) {
    Object* player;
    WarpPoint_Setup* setup;
    WarpPoint_Data* objdata;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 dist;
    /* RECOMP */
    f32 sin;
    f32 cos;

    setup = (WarpPoint_Setup*)self->setup;
    objdata = self->data;
    player = objGetPlayer();

    //Don't allow warping until a timer has run out (for modes 1, 4)
    objdata->warpDelayTimer -= gUpdateRate;
    if (objdata->warpDelayTimer < 0) {
        objdata->warpDelayTimer = 0;
    }

    switch (setup->mode) {
    case WarpPoint_MODE_0:
        //If this is an inbound Warp Point, play the specified arrival objSeq when the player arrives nearby (variable range)
        if (D_800B4A5E > NO_WARP_ID) {
            dx = player->srt.transl.x - self->srt.transl.x;
            dy = player->srt.transl.y - self->srt.transl.y;
            dz = player->srt.transl.z - self->srt.transl.z;
            dist = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
            if (((objdata->flags & WarpPoint_FLAG_1_Arrival_Sequence_Played) == FALSE) && setup->isInboundWarp && (dist < objdata->range) && (player->parent == self->parent)) {
                gDLL_3_Animation->vtbl->start_obj_sequence(objdata->objectSeqIndex, self, -1);
                D_80092A78 = 2; //Fade in from black
                objdata->flags |= WarpPoint_FLAG_1_Arrival_Sequence_Played;
            }
        }

        //If a warpID is provided, fade to black and warp the player when they're nearby (variable range)
        if ((setup->warpID > NO_WARP_ID) && (vec3Distance(&self->globalPosition, &player->globalPosition) < objdata->range)) {
#ifndef BLOCK_WARP
            mapWarpPlayer(setup->warpID, TRUE);
#endif
        }
        break;
    case WarpPoint_MODE_1:
        dx = player->srt.transl.x - self->srt.transl.x;
        dy = player->srt.transl.y - self->srt.transl.y;
        dz = player->srt.transl.z - self->srt.transl.z;
        dist = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));

        //If this is an inbound Warp Point, play objSeq1 (player uncurling from ball) when the player arrives nearby (fixed range of 100)
        if ((D_800B4A5E > NO_WARP_ID) && setup->isInboundWarp && (dist < 100.0f) && (player->parent == self->parent)) {
            gDLL_3_Animation->vtbl->start_obj_sequence(WarpPoint_OBJSEQ_1, self, -1);
            D_80092A78 = 2; //Fade in from black
        }

        //If a warpID is provided, play objSeq0 and warp via its anim message when the player is nearby (variable range/4) (waits for timer to expire)
        if ((objdata->warpDelayTimer == 0) && (dist < setup->quarterRange)) {
            if ((setup->warpID > NO_WARP_ID) && (setup->warpID > NO_WARP_ID)) { //@bug: accidentally pasted condition twice?
                gDLL_3_Animation->vtbl->start_obj_sequence(WarpPoint_OBJSEQ_0, self, -1);
            }
        }
        break;
    case WarpPoint_MODE_2:
        if (objdata->range != 0.0f) {
            dx = player->globalPosition.x - self->globalPosition.x;
            dy = player->globalPosition.y - self->globalPosition.y;
            dz = player->globalPosition.z - self->globalPosition.z;
            dist = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
        } else {
            dist = objdata->range;
        }

        //If this is an inbound Warp Point, play the specified arrival objSeq when the player's nearby and a specified gamebit is set (variable range)
        if (mainGetBits(objdata->gamebit) && ((objdata->flags & WarpPoint_FLAG_1_Arrival_Sequence_Played) == FALSE) && setup->isInboundWarp && (dist <= objdata->range) && (player->parent == self->parent)) {
            gDLL_3_Animation->vtbl->start_obj_sequence(objdata->objectSeqIndex, self, -1);
            objdata->flags |= WarpPoint_FLAG_1_Arrival_Sequence_Played;
        } else if ((objdata->flags & WarpPoint_FLAG_1_Arrival_Sequence_Played) == TRUE) {
        //If a warpID is provided and the arrival sequence played, warp the player immediately (without fade) when they're nearby and unset the gamebit (variable range)
        //(Waits for timer to expire, but obj_Setup has the timer expire immediately when using this mode!)
            if ((mainGetBits(objdata->gamebit)) && (objdata->warpDelayTimer == 0) && (dist <= objdata->range) && (setup->warpID > NO_WARP_ID)) {
                mainSetBits(objdata->gamebit, FALSE);
#ifndef BLOCK_WARP
                mapWarpPlayer(setup->warpID, FALSE);
#endif
            }
        }
        break;
    case WarpPoint_MODE_3:
        dx = player->srt.transl.x - self->srt.transl.x;
        dy = player->srt.transl.y - self->srt.transl.y;
        dz = player->srt.transl.z - self->srt.transl.z;
        dist = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));

        //If this is an inbound Warp Point, play the specified arrival objSeq when the player's nearby and a specified gamebit is set (variable range)
        if (mainGetBits(objdata->gamebit) && ((objdata->flags & WarpPoint_FLAG_1_Arrival_Sequence_Played) == FALSE) && setup->isInboundWarp && (dist < objdata->range) && (player->parent == self->parent)) {
            mainSetBits(objdata->gamebit, FALSE);
            gDLL_3_Animation->vtbl->start_obj_sequence(objdata->objectSeqIndex, self, -1);
            objdata->flags |= WarpPoint_FLAG_1_Arrival_Sequence_Played;
        }

        //NOTE: no outbound warp in this mode (unless objectSeqIndex is 0, activating the animCallback function's warp)
        break;
    case WarpPoint_MODE_4:
        if (objdata->range != 0.0f) {
            dx = player->globalPosition.x - self->globalPosition.x;
            dy = player->globalPosition.y - self->globalPosition.y;
            dz = player->globalPosition.z - self->globalPosition.z;
            dist = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
        } else {
            dist = objdata->range;
        }

        //If this is an inbound Warp Point, play the specified arrival objSeq when the player arrives nearby (variable range)
        if ((D_800B4A5E > NO_WARP_ID) && ((objdata->flags & WarpPoint_FLAG_1_Arrival_Sequence_Played) == FALSE) && setup->isInboundWarp && (dist < objdata->range) && (player->parent == self->parent)) {
            gDLL_3_Animation->vtbl->start_obj_sequence(objdata->objectSeqIndex, self, -1);
            D_80092A78 = 2; //Fade in from black
            objdata->flags |= WarpPoint_FLAG_1_Arrival_Sequence_Played;
        }

        //If a specified gamebit is set and the timer's expired and warpID is provided, unset the gamebit and fade out and warp the player when they're nearby (variable range)
        if (mainGetBits(objdata->gamebit) && (objdata->warpDelayTimer == 0) && (dist <= objdata->range) && (setup->warpID > NO_WARP_ID)) {
            mainSetBits(objdata->gamebit, FALSE);
#ifndef BLOCK_WARP
            mapWarpPlayer(setup->warpID, TRUE);
#endif
        }
        break;
    
    case WarpPoint_CUSTOMMODE_5: //@recomp: custom mode, can play separate arrival and departure sequences when gamebits are set
        #define ZSHIFT_SCALE 16

        //Get player distance squared
        if (objdata->range != 0.0f) {
            dx = player->globalPosition.x - self->globalPosition.x;
            dy = player->globalPosition.y - self->globalPosition.y;
            dz = player->globalPosition.z - self->globalPosition.z;
            dist = SQ(dx) + SQ(dy) + SQ(dz);
        } else {
            dist = SQ(objdata->range);
        }

        //If this is an inbound Warp Point and the player warped recently, play the specified arrival objSeq when the player's nearby and a specified gamebit is set (variable range)
        if ((D_800B4A5E > NO_WARP_ID) && ((objdata->flags & WarpPoint_FLAG_1_Arrival_Sequence_Played) == FALSE) && (setup->gamebit >= (NO_GAMEBIT + 1)) && mainGetBits(setup->gamebit) && setup->isInboundWarp && (dist <= SQ(objdata->range)) && (player->parent == self->parent)) {
            //Apply optional z-shift for sequence (along objectSpace axis)
            if (setup->zShiftArrival) {
                sin = Sinf(self->srt.yaw);
                cos = Cosf(self->srt.yaw);
                self->srt.transl.x += (setup->zShiftArrival * ZSHIFT_SCALE) * sin;
                self->srt.transl.z += (setup->zShiftArrival * ZSHIFT_SCALE) * cos;
            }
            
            gDLL_3_Animation->vtbl->start_obj_sequence(setup->objectSeqIndex, self, -1);
            objdata->flags |= WarpPoint_FLAG_1_Arrival_Sequence_Played;
            break;
        } 

        //Handle the tick just after the arrival sequence finishes
        if ((objdata->flags & WarpPoint_FLAG_4_AnimCallback_Finished) &&
            (objdata->flags & WarpPoint_FLAG_1_Arrival_Sequence_Played) &&
            (objdata->flags & WarpPoint_FLAG_2_Departure_Sequence_Played) == FALSE
        ) {
            //Unset the departure gamebit
            if (setup->gamebitDepart >= (NO_GAMEBIT + 1)) {
                mainSetBits(setup->gamebitDepart, FALSE);
            }
            objdata->flags &= ~WarpPoint_FLAG_4_AnimCallback_Finished;

            //Revert position after sequence zShift
            if (setup->zShiftArrival) {
                self->srt.transl.x = setup->base.x;
                self->srt.transl.z = setup->base.z;
            }
        }
        
        //If a warpID is provided and the player is nearby (variable range) and gamebitDepart is set, play the departure sequence
        if ((setup->warpID > NO_WARP_ID) && ((objdata->flags & WarpPoint_FLAG_2_Departure_Sequence_Played) == FALSE) && (setup->gamebitDepart >= (NO_GAMEBIT + 1)) && mainGetBits(setup->gamebitDepart) && (dist <= SQ(objdata->range))) {
            //Face the other way for departure sequence
            if (setup->noRotateOnDeparture == FALSE) {
                s32 yaw = setup->yaw << 8;
                yaw += M_180_DEGREES;
                CIRCLE_WRAP(yaw);
                self->srt.yaw = yaw;
            }

            //Apply optional z-shift for sequence (along objectSpace axis)
            if (setup->zShiftDeparture) {
                sin = Sinf(self->srt.yaw);
                cos = Cosf(self->srt.yaw);
                self->srt.transl.x += (setup->zShiftDeparture * ZSHIFT_SCALE) * sin;
                self->srt.transl.z += (setup->zShiftDeparture * ZSHIFT_SCALE) * cos;
            }
            
            gDLL_3_Animation->vtbl->start_obj_sequence(setup->objectSeqIndexDepart, self, -1);
            objdata->flags |= WarpPoint_FLAG_2_Departure_Sequence_Played;
            break;
        }

        //Warp the player after the departure sequence plays and the animCallback is finished
        if ((objdata->flags & WarpPoint_FLAG_4_AnimCallback_Finished) &&
            (objdata->flags & WarpPoint_FLAG_2_Departure_Sequence_Played) &&
            (objdata->flags & WarpPoint_FLAG_8_Warp_Queued) == FALSE
        ) {
            //Unset the departure gamebit
            if (setup->gamebitDepart >= (NO_GAMEBIT + 1)) {
                mainSetBits(setup->gamebitDepart, FALSE);
            }

            //Only warp if the warpID differs from the most recently-used warp (to prevent getting stuck in a warp loop)
            if (D_800B4A5E != setup->warpID) {
#ifndef BLOCK_WARP
                mapWarpPlayer(setup->warpID, FALSE);
#endif
            }
            objdata->flags |= WarpPoint_FLAG_8_Warp_Queued;
        }
        break;
    }
}

RECOMP_PATCH int WarpPoint_animCallback(Object* self, Object* animObj, AnimObj_Data* animObjData, s8 prevCallbackValue) {
    WarpPoint_Setup* setup = (WarpPoint_Setup*)self->setup;
    /* RECOMP */
    WarpPoint_Data* objData = self->data;

    //Warp the player (with a fade-to-black) via animMessage1
    if ((setup->mode != WarpPoint_MODE_2) && (animObjData->lastMessage == 1)) {
        if (setup->warpID >= 0) {
#ifndef BLOCK_WARP
            mapWarpPlayer(setup->warpID, TRUE);
#endif
            animObjData->lastMessage = 0;
        }
    }

    //@recomp: note that the animCallback ran
    if ((objData->flags & WarpPoint_FLAG_4_AnimCallback_Finished) == FALSE) {
        objData->flags |= WarpPoint_FLAG_4_AnimCallback_Finished;
    }

    return 0;
}
