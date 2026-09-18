#include "common_objsetups.h"
#include "configs.h"
#include "math_util.h"
#include "modding.h"
#include "recomputils.h"

#include "PR/os.h"
#include "common.h"
#include "dlls/objects/210_player.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"

#include "recomp/dlls/objects/423_DFbarrel_recomp.h"

//TEMPORARY DEFINES
#define DFbarrel_handleMovement DFbarrel_handle_movement
//END OF TEMPORARY DEFINES

typedef struct {
    Pickup pickup;
    u8 damage;                  //Damage accumulated by the barrel (explodes if it's damaged at all, though!)
    u8 framesSinceDetonation;   //Seems intended to count up to deleting the barrel after it explodes, but it's deleted immediately anyway
    s32 _unusedC;
    f32 accelerationX;          //Acceleration from DFriverflow currents
    f32 accelerationZ;          //Acceleration from DFriverflow currents
    f32 velocityX;              //Copy of barrel's velocity, for flow/bouyancy calcs
    f32 velocityY;              //Copy of barrel's velocity, for flow/bouyancy calcs
    f32 velocityZ;              //Copy of barrel's velocity, for flow/bouyancy calcs
} DFBarrel_Data;

RECOMP_PATCH void DFbarrel_handleMovement(Object* self) {
    DFBarrel_Data* objData;
    Object* riverFlow;
    Object** objects;
    s32 count;
    s32 i;
    f32 delta;
    f32 dy;
    f32 dz;
    f32 range;
    f32 diffMagnitude;
    TrackHeightResult** trackResult;
    Vec3f position;
    f32 minDiffMagnitude;
    f32 yDiff;
    /* RECOMP */
    f32 groundY = -9999.0f;
    f32 waterY;
    f32 speed;
    
    //@recomp: this is a slight vertical offset for the barrel's waterline when floating in water, to give it
    //a better sense of bouyancy - in December 2000 the waterline used to be at the very bottom of the barrel!
    #define WATERLINE_HEIGHT 7.5f

    objData = self->data;

    //@recomp: optionally have the barrel drift towards and stay at the crack in Discovery Falls' whirlpool cave
    u8 doRegularMove = TRUE;
    if (self->mapID == MAP_DISCOVERY_FALLS && configs_GetDFWhirlpoolAssist()) {
        #define ATTRACT_DISTANCE 100.0f
        Object* whirlpoolCrack = objGetObjectByUID(0x00002620);
        f32 distance;
        if (whirlpoolCrack) {
            distance = vec3DistanceXZ(&self->globalPosition, &whirlpoolCrack->globalPosition);

            if (distance < ATTRACT_DISTANCE) {
                doRegularMove = FALSE;

                self->srt.transl.x = dampedSmoothToFrom(self->srt.transl.x, whirlpoolCrack->globalPosition.x, &objData->velocityX, 300.0f);
                self->srt.transl.z = dampedSmoothToFrom(self->srt.transl.z, whirlpoolCrack->globalPosition.z, &objData->velocityZ, 300.0f);
                self->srt.transl.y += self->velocity.y;
            }
        }
    }

    //Get swept away by DFriverflow objects
    if (doRegularMove) {
        objData->accelerationX = objData->accelerationZ = 0.0f;

        for (objects = objGetAllOfType(OBJTYPE_Riverflow, &count), i = 0; i < count; i++) {
            riverFlow = objects[i];
            dy = riverFlow->srt.transl.y - self->srt.transl.y;
            if ((dy <= 200.0f) && (dy >= -200.0f)) {
                delta = riverFlow->srt.transl.x - self->srt.transl.x;
                dz = riverFlow->srt.transl.z - self->srt.transl.z;
                delta = sqrtf(SQ(delta) + SQ(dz));
                
                range = ((DFriverflow_Setup*)riverFlow->setup)->range * 1.5f;
                if (delta < range) {
                    delta = (range - delta) / range;
                    delta *= riverFlow->srt.scale * 10.0f;
                    
                    objData->accelerationX += mathSinfInterp(riverFlow->srt.yaw) * delta;
                    objData->accelerationZ += mathCosfInterp(riverFlow->srt.yaw) * delta;
                }
            }
        }
        
        if (count != 0) {
            objData->accelerationX /= count;
            objData->accelerationZ /= count;
            objData->velocityX -= (objData->accelerationX * 0.05f);
            objData->velocityZ -= (objData->accelerationZ * 0.05f);
            objData->velocityX *= 0.99f;
            objData->velocityZ *= 0.99f;
            
            delta = sqrtf(SQ(objData->velocityX) + SQ(objData->velocityZ));
            if (delta > 0.85f) {
                objData->velocityX *= 0.85f / delta;
                objData->velocityZ *= 0.85f / delta;
            }
        }
    }

    //Float on water
    {
        objData->velocityY = self->velocity.y;
        
        //Find the nearest water plane
        count = trackGetHeight(self, self->srt.transl.x, self->srt.transl.y, self->srt.transl.z, &trackResult, 0, 0);
        if (count) {
            delta = 10000.0f;
            waterY = 10000.0f;
            
            for (i = 0; i < count; i++){
                if (trackResult[i]->unk14 == 14) { //water 
                    yDiff = (self->srt.transl.y + WATERLINE_HEIGHT) - trackResult[i]->y;
                    if (yDiff >= 0) {
                        diffMagnitude = yDiff;
                    } else {
                        diffMagnitude = -yDiff;
                    }
                    
                    if (delta >= 0) {
                        minDiffMagnitude = delta;
                    } else {
                        minDiffMagnitude = -delta;
                    }
                    
                    if (diffMagnitude < minDiffMagnitude) {
                        delta = yDiff;
                        waterY = trackResult[i]->y; //@recomp: store waterY
                    }
                }
            }

            //@recomp: find the closest ground under the water plane too
            for (i = 0; i < count; i++){
                if ((trackResult[i]->unk14 != 0xE) && 
                    (trackResult[i]->y < waterY) && 
                    (groundY < trackResult[i]->y)
                ) {
                    groundY = trackResult[i]->y;
                }
            }

            //Correlate velocityY with barrel's depth underwater
            if ((delta != 10000.0f) && (delta < 0)) {
                objData->velocityY += (-delta * 0.1f);
                objData->velocityY *= 0.8f;
            }

            //@recomp: limit velocityY, so the barrel doesn't rocket into the next calendar year when released underwater
            if (self->velocity.y > 1.5f) {
                self->velocity.y = 1.5f;
            }

            //@recomp: fix framerate dependency on y (the x/z velocity components are premultiplied by gUpdateRate prior to objMove)
            self->velocity.y *= gUpdateRateF / 2.0f;
        }

        self->velocity.y = objData->velocityY;
    }

    //@recomp: spin slightly while being swept away in Discovery Falls' whirlpool
    if (self->mapID == MAP_DISCOVERY_FALLS && (self->velocity.x || self->velocity.z)) {
        speed = sqrtf(SQ(self->velocity.x) + SQ(self->velocity.z));
        s32 newYaw = self->srt.yaw + speed * 125;
        CIRCLE_WRAP(newYaw);
        self->srt.yaw = newYaw;
    }

    position.x = self->srt.transl.x;
    position.y = self->srt.transl.y;
    position.z = self->srt.transl.z;

    //@recomp: move velocity assignment here, to work with dampedSmoothToFrom when the attractor option's enabled
    self->velocity.x = objData->velocityX * gUpdateRateF;
    self->velocity.z = objData->velocityZ * gUpdateRateF;

    if (doRegularMove) {
        objMove(self, self->velocity.x, self->velocity.y, self->velocity.z); 
    }

    //@recomp: snap to the ground in shallow water
    if (self->srt.transl.y < groundY) {
        self->srt.transl.y = groundY;
    }

    trackGetLineIntersect(&position, &self->srt.transl, 10.0f, 0, NULL, self, 8, -1, 0xFF, 0);
}
