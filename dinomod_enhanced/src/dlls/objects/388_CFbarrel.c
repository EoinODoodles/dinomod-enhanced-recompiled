
#include "modding.h"
#include "player_util.h"

#include "game/objects/interaction_arrow.h"
#include "game/objects/object_id.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/engine/53_movelib.h"
#include "dlls/engine/54_pickup.h"
#include "dlls/objects/210_player.h"
#include "sys/intersect.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/objmsg.h"
#include "sys/print.h"
#include "sys/rand.h"
#include "dll.h"

#include "recomp/dlls/objects/388_CFbarrel_recomp.h"

typedef struct {
/*00*/ Pickup pickup;
/*0C*/ Object* mobileMap;
/*10*/ u8 _unk10;
/*11*/ u8 isHeld;
/*12*/ u8 hits; // how many times the barrel has been hit
/*13*/ u8 explodedTime; // how long it's been since the barrel exploded
/*14*/ s32 respawnTimer; // time remaining until respawn
/*18*/ Vec3f targVelocity;
/*24*/ f32 unk24;
/*28*/ f32 unk28;
/*2C*/ f32 unk2C;
/*30*/ s16 unk30;
/*32*/ s8 unk32;
/*34*/ s32 unk34;
/*38*/ s16 unk38;
/*3A*/ s16 unk3A;
/*3C:0*/ u8 isLevelObj : 1; // part of the level, has a respawn point, etc
/*3D*/ u8 flags;
/*3E:0*/ u8 inWindLift : 1;
/*3E:1*/ u8 unk3E_1 : 1;
} CFbarrel_Data;

enum CFbarrelFlags {
    CFBARREL_Physics_Enabled = 0x1
};

void CFbarrel_doPhysics(Object* self);
void CFbarrel_checkHit(Object* self);
void CFbarrel_moveToAttractor(Object* self, s16 a1, s16 a2);
void CFbarrel_processMesgLoop(Object* self);
void CFbarrel_setWindLiftState(Object* self, u8 a1);

RECOMP_HOOK_DLL(CFbarrel_obj_Setup) void CFbarrel_obj_Setup_hook(Object* self, ObjSetup* setup, s32 reset) {
    // @recomp: CFbarrel obj update already handles this and the automatic set can conflict in a way
    //          that makes the barrel not detect the hitlines that force an exit from windlifts
    self->srt.flags |= OBJFLAG_MANUAL_PREV_POSITIONS;
}

RECOMP_PATCH void CFbarrel_obj_Control(Object* self) {
    CFbarrel_Data* objdata = self->data;
    
    if (objdata->respawnTimer != 0) {
        objdata->respawnTimer -= gUpdateRate;
        // @bug: won't ever be true if the respawn timer is divisible by the update rate
        if (objdata->respawnTimer <= 0) { // @recomp: fix condition
            objdata->respawnTimer = 0;
            objdata->explodedTime = 0;
            objdata->hits = 0;
            objdata->flags |= CFBARREL_Physics_Enabled;
            obj_func_80023C6C(self); // @recomp: Reset lockdata on respawn since we modify it in another patch
        }
        return;
    }

    if ((self->parent != NULL) && (self->parent->id == OBJ_DR_PushCart)) {
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
    } else {
        self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
    }

    if (mapWorldCoordsToBlockIndex(self->globalPosition.x, self->globalPosition.y, self->globalPosition.z) == -1) {
        // not in a loaded block
        return;
    }

    if (objdata->explodedTime != 0) {
        objdata->explodedTime += gUpdateRate;
        objdata->unk24 = (objdata->unk2C * (f32) objdata->explodedTime) + 1.0f;
        func_8002683C(self, (s16) objdata->unk24, (s16) (-objdata->unk24 * 0.5f), (s16) (objdata->unk24 * 0.5f));
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_353, NULL, 0, -1, NULL);
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_353, NULL, 0, -1, NULL);
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_353, NULL, 0, -1, NULL);
        if (objdata->explodedTime > 20) {
            objdata->inWindLift = FALSE;
            // Move to "create point" node
            ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func7(0x1A, &self->srt);
            STUBBED_PRINTF(" Should has Set position  %i %f %f \n", 0x1A, &self->srt.transl.x, &self->srt.transl.z);
            self->srt.transl.x += (f32) mathRnd(-30, 30) * 0.1f;
            self->srt.transl.z += (f32) mathRnd(-30, 30) * 0.1f;
            // @recomp: Also update prev position or else a wall might get in the way of the teleport
            self->prevLocalPosition.x = self->srt.transl.x;
            self->prevLocalPosition.y = self->srt.transl.y;
            self->prevLocalPosition.z = self->srt.transl.z;
            bzero(&objdata->targVelocity, sizeof(Vec3f));
            bzero(&self->velocity, sizeof(Vec3f));
            if (objdata->isLevelObj) {
                // prep for respawn
                mapSaveObject(self->setup, self->mapID, self->srt.transl.x, self->srt.transl.y, self->srt.transl.z);
                objdata->respawnTimer = 600;
                func_80026160(self);
                func_8002683C(self, 8, -2, 0x19);
            } else {
                // no respawn, just disable
                objDisable(self);
                func_800267A4(self);
                self->srt.flags |= OBJSTATE_PRINT_DISABLED;
            }
        }
    } else {
        if (gDLL_54_pickup->vtbl->control(self, &objdata->pickup) == PICKUP_NotHeld) {
            if (objdata->isHeld) {
                objAddObjectType(self, OBJTYPE_WindLiftable);
            }
            objdata->isHeld = FALSE;
            func_8002674C(self);
            if (objdata->flags & CFBARREL_Physics_Enabled) {
                CFbarrel_doPhysics(self);
            }
            CFbarrel_checkHit(self);
        } else {
            objdata->flags |= CFBARREL_Physics_Enabled;
            objdata->isHeld = TRUE;
            objdata->unk3E_1 = TRUE;
            // @recomp: Disable voxmap lock icon check after getting picked up for the first time. It's too easy
            //          to place the barrel partway in a wall...
            obj_func_80023BF8(self, 0, 0, 0, 0, 0x6);
        }
        CFbarrel_processMesgLoop(self);
        if (objdata->inWindLift) {
            self->unkAF |= ARROW_FLAG_8_No_Targetting;
            if (objdata->unk3E_1 && objdata->inWindLift) {
                objdata->targVelocity.x = self->velocity.x;
                objdata->targVelocity.y = self->velocity.y;
                objdata->targVelocity.z = self->velocity.z;
                objdata->targVelocity.y = 0.0f;
                objdata->unk3E_1 = FALSE;
            }
        }
        if (objdata->isLevelObj) {
            mapSaveObject(self->setup, self->mapID, self->srt.transl.x, self->srt.transl.y, self->srt.transl.z);
        }
    }
}

RECOMP_PATCH void CFbarrel_obj_Update(Object* self) {
    CFbarrel_Data* objdata = self->data;
    f32 vel[3];
    TrackLineIntersectResult result;

    if (objdata->explodedTime == 0 && objdata->respawnTimer == 0) {
        if (objdata->mobileMap != NULL) {
            trackIntersect_func_8005B5B8(self, objdata->mobileMap, 1);
            objdata->mobileMap = NULL;
        }
        if (objdata->inWindLift) {
            diPrintf(" floating ");
            vel[0] = self->srt.transl.x - self->prevLocalPosition.x;
            vel[1] = self->srt.transl.y - self->prevLocalPosition.y;
            vel[2] = self->srt.transl.z - self->prevLocalPosition.z;
            vel[0] *= 0.99f * (1.0f / gUpdateRateF);
            vel[1] *= 0.99f * (1.0f / gUpdateRateF);
            vel[2] *= 0.99f * (1.0f / gUpdateRateF);
            objdata->targVelocity.x += vel[0];
            objdata->targVelocity.y += vel[1];
            objdata->targVelocity.z += vel[2];
            vel[1] = 0.0f;
            objdata->flags |= CFBARREL_Physics_Enabled;
            objdata->targVelocity.x *= 0.5f;
            objdata->targVelocity.y = 0.0f;
            objdata->targVelocity.z *= 0.5f;
        }
        if (!objdata->isHeld) {
            if (trackGetLineIntersect(&self->prevLocalPosition, &self->srt.transl, 4.0f, 1, &result, self, 8, -1, 0xFF, 0) != 0) {
                STUBBED_PRINTF(" Line IDNO %i \n", result.unk51);
                if (objdata->inWindLift && result.unk51 == 3) {
                    // animator ID 3 hitlines are a failsafe to exit the windlift
                    CFbarrel_setWindLiftState(self, FALSE);
                } else {
                    // @recomp: Cap speed here or else the following code can result in super high velocities
                    //          if the barrel is ping-ponging off of two close walls.
                    self->velocity.x = CLAMP_EXPR(self->velocity.x * -1.1f, -5.0f, 5.0f);
                    self->velocity.z = CLAMP_EXPR(self->velocity.z * -1.1f, -5.0f, 5.0f);
                    objdata->targVelocity.x = CLAMP_EXPR(objdata->targVelocity.x * -1.1f, -5.0f, 5.0f);
                    objdata->targVelocity.z = CLAMP_EXPR(objdata->targVelocity.z * -1.1f, -5.0f, 5.0f);
                    // self->velocity.x *= -1.1f;
                    // self->velocity.z *= -1.1f;
                    // objdata->targVelocity.x *= -1.1f;
                    // objdata->targVelocity.z *= -1.1f;
                    STUBBED_PRINTF(" Hit Line ");
                }
            }
        }
        self->prevLocalPosition.x = self->srt.transl.x;
        self->prevLocalPosition.y = self->srt.transl.y;
        self->prevLocalPosition.z = self->srt.transl.z;
        // @recomp: A patch above disables the automatic set of prevGlobalPosition, so do it ourselves
        self->prevGlobalPosition.x = self->globalPosition.x;
        self->prevGlobalPosition.y = self->globalPosition.y;
        self->prevGlobalPosition.z = self->globalPosition.z;
    }
}

RECOMP_PATCH void CFbarrel_doPhysics(Object* self) {
    CFbarrel_Data* objdata = self->data;
    f32 height;
    f32 bestHeight;
    s16 angle;
    s32 numResults;
    s32 floors[4];
    s32 bestResultIdx;
    s32 i;
    s32 numFloors;
    s32 resultIdx;
    f32 dirX;
    f32 dirY;
    f32 dirZ;
    f32 smallestHeight;
    TrackHeightResult** results;
    s16 sp98[] = {-1, 0, 0, 1, 1, 0, 0, -1};
    s16 sp90[] = {2, 3, 0, 1};
    Object* obj;
    // @recomp: new vars
    s32 numCloseFloors = 0;
    _Bool anyFloorsBelow = FALSE;

    self->srt.yaw = 0;
    if (objdata->targVelocity.y > 0.01f) {
        objdata->targVelocity.y -= 0.1f; // @bug: framerate dependent
        self->velocity.x = objdata->targVelocity.x * gUpdateRateF;
        self->velocity.y = objdata->targVelocity.y * gUpdateRateF;
        self->velocity.z = objdata->targVelocity.z * gUpdateRateF;
        objMove(self, self->velocity.x, self->velocity.y, self->velocity.z);
        self->srt.roll += (objdata->targVelocity.y * 500.0f);
        return;
    }
    smallestHeight = 0.0f;
    angle = 0;
    numFloors = 0;
    for (i = 0; i < 4; i++) {
        dirX = (10.0f * mathSinfInterp(angle)) * mathCosfInterp(self->srt.roll);
        dirZ = (10.0f * mathCosfInterp(angle)) * mathCosfInterp(self->srt.pitch);
        dirY = (10.0f * mathSinfInterp(self->srt.roll) * mathSinfInterp(angle)) 
            + (10.0f * mathSinfInterp(self->srt.pitch) * mathCosfInterp(angle));

        angle += 0x3FD2; // almost 90 degrees

        numResults = trackGetHeight(self, 
                                self->srt.transl.x + dirX, 
                                self->srt.transl.y + dirY,
                                self->srt.transl.z + dirZ, 
                                &results, 0, 1);
        if (numResults != 0) {
            bestHeight = 10000.0f;

            for (resultIdx = 0; resultIdx < numResults; resultIdx++) {
                height = self->srt.transl.y - results[resultIdx]->y;
                if (
                    (height > 0.0f && height < ABS_EXPR(bestHeight)) || 
                    (height <= 0.0f && height > -20.0f)
                ) {
                    bestHeight = height;
                    bestResultIdx = resultIdx;
                }
                // @recomp: Record whether there is anything below us at all
                if (height >= 0.0f) {
                    anyFloorsBelow = TRUE;
                }
                // @recomp: Also track floors that are below us but pretty close
                if (height > 0.0f && height < 2.0f) {
                    numCloseFloors++;
                }
            }
          
            if (bestHeight <= 0.0f) { // wait until we pass a floor
                if (bestHeight < smallestHeight) {
                    smallestHeight = bestHeight;
                }
                objdata->targVelocity.y = 0.0f;
                if (bestHeight > 0.0f) {
                    floors[numFloors] = i;
                } else {
                    floors[numFloors] = sp90[i];
                }
                numFloors += 1;
            }
        }
    }
    if (numFloors != 0) {
        // zip up to floor
        self->velocity.y = -smallestHeight;
    }
    if (numFloors == 0) {
        //objdata->targVelocity.y -= 0.1f;
        self->velocity.y = objdata->targVelocity.y * gUpdateRateF;
        self->velocity.x = objdata->targVelocity.x * gUpdateRateF;
        self->velocity.z = objdata->targVelocity.z * gUpdateRateF;
        objdata->targVelocity.y -= 0.1f; // @recomp: moved to prevent falling when windlift unloads
        if (objdata->targVelocity.y < -0.2f) {
            CFbarrel_moveToAttractor(self, objdata->unk38, objdata->unk3A);
        }
        // @recomp: Check if we're in the void and if so just respawn
        if (!anyFloorsBelow) {
            objdata->explodedTime = 1;
        }
    }
    if (numFloors > 0) {
        STUBBED_PRINTF(" Landed On World Obj ");
        objdata->flags &= ~CFBARREL_Physics_Enabled;
        // @bug: bestResultIdx might not be valid here, the results array could have changed by now
        obj = results[bestResultIdx]->obj;
        if (obj != NULL) {
            if ((obj->def->flags & OBJDEF_IS_MOBILE_MAP) && !(obj->def->flags & OBJDEF_MOBILE_MAP_NEVER_PLAYER_PARENT)) {
                objdata->mobileMap = obj;
                STUBBED_PRINTF(" ob Obj %x  Defno %i \n\n", obj, obj->id);
            }
        }
        objdata->targVelocity.x = 0.0f;
        objdata->targVelocity.y = 0.0f;
        objdata->targVelocity.z = 0.0f;
        self->srt.roll = 0;
        self->srt.pitch = 0;
    }
    if (objdata->inWindLift && numFloors >= 3) { // @recomp: only require 3 test points instead of 4
        // all 4 test points reported a floor, so we're not in a windlift anymore even if
        // the windlift hasn't said so yet
        CFbarrel_setWindLiftState(self, FALSE);
    }
    // @recomp: If in a windlift and there's enough floors that are "close enough", then we're probably
    //          outside of the windlift. This is almost certainly the floor of the blown up entrance
    //          to the "treasure" windlift in front of the throne room. The angled floor there doesn't
    //          play nice with the vanilla floor check and the windlift radius extends out there slightly.
    else if (objdata->inWindLift && numCloseFloors >= 3) {
        CFbarrel_setWindLiftState(self, FALSE);
    }
    objMove(self, self->velocity.x, self->velocity.y, self->velocity.z);
}

RECOMP_PATCH void CFbarrel_processMesgLoop(Object* self) {
    u32 mesgID = 0;
    void* mesgArg = NULL;
    
    while (objRecvMesg(self, &mesgID, NULL, &mesgArg) != 0) {
        // @recomp: Don't allow barrels to be taken into the CRF courtyard
        if (mesgID == 15 || mesgID == 16) {
            // The treasure windlift has a very large number for an ID but the others have normal IDs <= 5
            s32 windLiftID = ((s32)mesgArg & 0xFF);
            if (windLiftID <= 5) {
                // where do you think you're going...
                Object* player = objGetPlayer();
                Object* held;
                if (((DLL_210_Player*)player->dll)->vtbl->func10(player, &held) && held == self) {
                    playerUtil_stop_carrying(player);
                }
                CFbarrel_Data* objdata = self->data;
                objdata->explodedTime = 1;
            }
        }
        switch (mesgID) {
        case 15:
            CFbarrel_setWindLiftState(self, TRUE);
            break;
        case 16:
            CFbarrel_setWindLiftState(self, FALSE);
            break;
        }
    }
}
