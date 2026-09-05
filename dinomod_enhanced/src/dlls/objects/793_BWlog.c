#include "configs.h"
#include "modding.h"
#include "recompconfig.h"

#include "dlls/objects/common/vehicle.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/418_DFriverflow.h"
#include "dlls/objects/419_DFdockpoint.h"
#include "dlls/engine/27.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "dll.h"

#include "recomp/dlls/objects/793_BWlog_recomp.h"

typedef enum {
    BWLog_STATE_0_Main,
    BWLog_STATE_1_Roll_Left,
    BWLog_STATE_2_Roll_Right,
    BWLog_STATE_3_Placeholder1,
    BWLog_STATE_4_Placeholder2
} BWlog_States;

typedef struct {
/*000*/ DLL27_Data collider;
/*260*/ Vec3f endPoints[2];
/*278*/ Vec3f velocity[2];
/*290*/ f32 powerZ[2]; // forward/backward
/*298*/ f32 powerX[2]; // lateral
/*2A0*/ Vec4f rollCurve; //Roll speed spline
/*2B0*/ f32 rollSpeed; // wobble (smoothed)
/*2B4*/ f32 targetWaterYOffset;
/*2B8*/ f32 paddleTimer; // move timer (when > 0, move forward)
/*2BC*/ f32 rollTimer;
/*2C0*/ f32 rollAcceleration; // wobble
/*2C4*/ s32 _unk2C4;
/*2C8*/ f32 rollCurveProgress;
/*2CC*/ f32 tValueRoll;
/*2D0*/ f32 flowX[2]; //DFriverflow objects' combined push strength in X (values for both ends of log)
/*2D8*/ f32 flowY[2]; //DFriverflow objects' combined push strength in Y (values for both ends of log)
/*2E0*/ f32 flowZ[2]; //DFriverflow objects' combined push strength in Z (values for both ends of log)
/*2E8*/ u8 _unk2E8[0x2F8 - 0x2E8];
/*2F8*/ f32 joyATimer;
/*2FC*/ f32 paddlePower;
/*300*/ f32 unk300[2];
/*308*/ f32 soundPitch;
/*30C*/ f32 soundVolume;
/*310*/ f32 riverflowMagnitude;
/*314*/ u32 soundHandle; //Controls rushing water sound loop
/*318*/ s32 rollAngle; // when rolling, the current roll rotation
/*31C*/ u16 wiggleYOffsets[2]; // y offset for each end point to pitch the log up/down to simulate small waves
/*320*/ u16 joyPressed; // controller buttons pressed
/*322*/ s16 joyStickX; // joystick x
/*324*/ s16 joyStickY; // joystick y
/*326*/ s16 soundPitchPhase;
/*328*/ s16 soundVolumePhase;
/*32A*/ u8 state; // roll state (0 = not rolling, 1 = left, 2 = right)
/*32B*/ u8 playerVehicleAnim;
/*32C*/ u8 joyARecentTap; // a pressed (turns off automatically after a time or if a is pressed again)
/*32D*/ u8 unk32D; // bitfield of which side of the log is touching terrain (0x1 = front, 0x2 = back, 0x3 = both)
/*32E*/ u8 mountState; // see VehicleMountState
/*32F*/ u8 _unk32F[0x338 - 0x32F];
/*338*/ Object *dockpoint;
} BWlog_Data;

// @recomp: Custom setup
typedef struct {
    ObjSetup setup;
    s8 startRotation;
} BWlog_Setup;

extern Vec3f dLocalEndpointCoords[2];
extern f32 dCollisionRadii[2];
extern u8 dColliderParams[2];

extern void BWlog_handleWater(Object* self, BWlog_Data* objdata, s32 arg2);
extern void BWlog_handleControlsAButton(Object* self, BWlog_Data* objdata);
extern void BWlog_startRoll(Object* self, BWlog_Data* objdata, s32 arg2);
extern void BWlog_handleRoll(Object* self, BWlog_Data* objdata);
extern void BWlog_handleUnknownState(Object* self, BWlog_Data* objdata);
extern void BWlog_handlePaddleMotion(Object* self, BWlog_Data* objdata);
extern void BWlog_findRiverflows(Object* self, BWlog_Data* objdata);
extern void BWlog_handleSounds(Object* arg0, BWlog_Data* arg1);
extern void BWlog_handleFx(Object* self, BWlog_Data* objdata);

// @recomp: Copy of DFlog's anim callback
static int recomp_BWlog_animcallback(Object *self, Object *a1, AnimObj_Data *a2, s8 a3) {
    func_800267A4(self);
    return 0;
}

RECOMP_PATCH void BWlog_obj_Setup(Object *self, BWlog_Setup *setup, s32 arg2) {
    BWlog_Data *objdata = (BWlog_Data*)self->data;
    s32 i;

    // @recomp: Register anim callback. DFlog was swapped with BWlog in dinomod but BWlog is missing
    //          an anim callback, but there's some sequences that assume one is set up to adjust collision.
    self->animCallback = recomp_BWlog_animcallback;

    gDLL_27->vtbl->init(&objdata->collider, 
        DLL27FLAG_NONE, 
        DLL27FLAG_8000000 | DLL27FLAG_40000 | DLL27FLAG_20 | DLL27FLAG_2 | DLL27FLAG_1, 
        DLL27MODE_1);
    gDLL_27->vtbl->setup_terrain_collider(&objdata->collider, 2, dLocalEndpointCoords, dCollisionRadii, dColliderParams);
    // @recomp: Add block hits collider
    gDLL_27->vtbl->setup_hits_collider(&objdata->collider, 2, dLocalEndpointCoords, dCollisionRadii, 8);
    objdata->collider.boundsYExtension = 100;
    objAddObjectType(self, OBJTYPE_Vehicle);
    objdata->wiggleYOffsets[1] = 0x2000;
    objdata->targetWaterYOffset = 15.0f;
    
    self->shadow->flags |= (OBJ_SHADOW_FLAG_WATER_SURFACE | OBJ_SHADOW_FLAG_TOP_DOWN | OBJ_SHADOW_FLAG_USE_OBJ_YAW | OBJ_SHADOW_FLAG_CUSTOM_COLOR | OBJ_SHADOW_FLAG_CUSTOM_DIR);
    self->shadow->r = 0xFF;
    self->shadow->g = 0xFF;
    self->shadow->b = 0xFF;
    self->shadow->a = 0x7F;

    for (i = 0; i < 2; i++) {
        objdata->endPoints[i].x = self->srt.transl.x;
        objdata->endPoints[i].y = self->srt.transl.y;
        objdata->endPoints[i].z = self->srt.transl.z;
    }

    // @recomp: Do the same hit info setup as DFlog. Helps with cases where an Override takes control
    //          over the log instantly after being spawned.
    self->objhitInfo->unk58 |= 1;
    self->objhitInfo->unk58 |= 4;
    // @recomp: Initialize DLL 27 state. In cases where an Override takes control away from the log right
    //          after the log spawns, sometimes this state won't be initialized with the log's current position.
    //          This is a problem with the totem puzzle log, where the log will warp away a bit after regaining
    //          control if this state is not set up right.
    gDLL_27->vtbl->reset(self, &objdata->collider);

    // @recomp: Support start yaw via setup
    self->srt.yaw = setup->startRotation << 8;
}

// This is yoinked and modified from dll_27_func_1D60. Looks at the log collisions and rotates the log
// such that it reacts to moving along a wall. This effect can also be enabled by setting the DLL 27
// flag 0x8000, but the built-in version of this applies a very weak spin.
// static void wall_react(Object* arg0, DLL27_Data* arg1) {
//     f32 temp;
//     f32 f2;
//     f32 f0;
//     s32 var_s1_2;
//     s32 var_s0_2;
//     f32 f14;
//     u8 temp_t7;
//     s8 var_a1;
//     f32 f12;
//     s32 i;
//     SRT spDC;
//     f32 temp2;
//     f32 spC8[4];
//     f32 spB8[4];
//     f32 spA8[4];
//     MtxF sp68;
//     Vec3f globalPosition;

//     for (s32 i = 0; i < 2; i++) {
//         // Note: Don't do anything if we're not touching a wall, otherwise we get a weird wiggle at higher fps
//         Vec3f *array = NULL;
//         if (i == 0) {
//             if (!(arg1->unk25C & 3)) {
//                 continue;
//             }
//             array = arg1->unk38;
//         } else {
//             if (!(arg1->hitsTouchBits & 3)) {
//                 continue;
//             }
//             array = arg1->unk110;
//         }

//         temp_t7 = (arg1->numTestPoints >> 4);
//         globalPosition.x = 0.0f;
//         globalPosition.y = 0.0f;
//         globalPosition.z = 0.0f;
//         for (i = 0; i < (temp_t7*3); i+=3) {
//             globalPosition.x += array[0].f[i];
//             globalPosition.y += array[0].f[i+1];
//             globalPosition.z += array[0].f[i+2];
//         }
//         VECTOR_SCALE(globalPosition, 1.0f / temp_t7);

//         spDC.yaw = -arg0->srt.yaw;
//         spDC.pitch = -arg0->srt.pitch;
//         spDC.roll = -arg0->srt.roll;
//         spDC.scale = 1;
//         spDC.transl.x = -globalPosition.x;
//         spDC.transl.y = -globalPosition.y;
//         spDC.transl.z = -globalPosition.z;
//         mathRpyXyzMtx(&sp68, &spDC);
//         for (var_s0_2 = 0, i = 0; i < temp_t7; i++) {
//             mathMtxXFMF(&sp68, 
//                             array[var_s0_2].x, array[var_s0_2].y, array[var_s0_2].z, 
//                             &spC8[i], &spB8[i], &spA8[i]);
//             var_s0_2++;
//         }

//         var_s1_2 = 0;
//         var_s0_2 = 1;
//         var_a1 = 1;

//         f0 = spC8[0] + spC8[var_s1_2];
//         f2 = spA8[0] + spA8[var_s1_2];
//         temp = spC8[var_s0_2] + spC8[var_a1];
//         f12 = f0 - temp;
//         temp2 = spA8[var_s0_2] + spA8[var_a1];
//         f14 = f2 - temp2;
//         arg0->srt.yaw += (s16) ((mathAtan2f(f12, f14) & 0xFFFF) + 0x8000);
//     }
// }

RECOMP_PATCH void BWlog_obj_Control(Object* self) {
    BWlog_Data* objdata;
    f32 damp;
    f32 vel[3];
    Vec3f vec;
    SRT srt;
    MtxF logMtx;
    MtxF pitchYawMtx;
    MtxF invPitchYawMtx;
    f32 sp9C;
    f32 distance;
    DFdockpoint_Setup* dockpointSetup;
    s32 i;

    // @recomp: Run objhit func just like DFlog does. Resets the state modified by the anim callback.
    func_8002674C(self);

    objdata = (BWlog_Data*)self->data;
    
    distance = 10000.0f;
    objdata->dockpoint = objGetNearestTypeTo(OBJTYPE_Dockpoint, self, &distance);
    if (objdata->dockpoint != NULL) {
        dockpointSetup = (DFdockpoint_Setup*)objdata->dockpoint->setup;
        distance = vec3Distance(&self->globalPosition, &objdata->dockpoint->globalPosition);
        // Reduce velocity while near a dockpoint
        if (objdata->mountState == VEHICLE_Mounted) {
            damp = 0.95f;
        } else {
            damp = 0.5f;
        }
        if (distance < dockpointSetup->range) {
            for (i = 0; i < 2; i++) {
                objdata->velocity[i].x *= damp;
                objdata->velocity[i].z *= damp;
            }
        } else {
            objdata->dockpoint = NULL;
        }
    }

    BWlog_findRiverflows(self, objdata);

    if (objdata->mountState == VEHICLE_Mounted) {
        BWlog_handleControlsAButton(self, objdata);

        switch (objdata->state) {
        case BWLog_STATE_1_Roll_Left:
        case BWLog_STATE_2_Roll_Right:
            BWlog_handleRoll(self, objdata);
            break;
        case BWLog_STATE_3_Placeholder1:
        case BWLog_STATE_4_Placeholder2:
            BWlog_handleUnknownState(self, objdata); //Placeholder for other left/right state? Maybe getting hurt?
            break;
        default:
            BWlog_handlePaddleMotion(self, objdata);
            break;
        }

        // @recomp: Don't apply turn to yaw directly to behave more like DFLog (turn logic moved to below code)
        //self->srt.yaw -= (s32) ((f32) objdata->unk322 * (60.0f - ((f32) objdata->unk324 * 0.05f)) * 0.1f * gUpdateRateF) & 0xFFFF & 0xFFFF;
    }

    srt.yaw = self->srt.yaw;
    srt.pitch = self->srt.pitch;
    srt.roll = self->srt.roll;
    srt.scale = 1.0f;
    srt.transl.x = self->srt.transl.x;
    srt.transl.y = self->srt.transl.y;
    srt.transl.z = self->srt.transl.z;
    mathYprXyzMtx(&logMtx, &srt);
    srt.roll = 0;
    srt.transl.x = 0;
    srt.transl.y = 0;
    srt.transl.z = 0;
    mathYprXyzMtx(&pitchYawMtx, &srt);
    srt.yaw = -srt.yaw;
    srt.pitch = -srt.pitch;
    mathRpyXyzMtx(&invPitchYawMtx, &srt);

    for (i = 0; i < 2; i++) {
        // Recalculate log end points from current matrix
        mathMtxXFMF(&logMtx, 
                       dLocalEndpointCoords[i].x, dLocalEndpointCoords[i].y, dLocalEndpointCoords[i].z, 
                       &objdata->endPoints[i].x, &objdata->endPoints[i].y, &objdata->endPoints[i].z);
        // Do water physics for this side of the log
        BWlog_handleWater(self, objdata, i);
        // Factor in riverflow influence
        mathMtxXFMF(&invPitchYawMtx, 
                       objdata->flowX[i], 0.0f, objdata->flowZ[i], 
                       &vel[0], &vel[1], &vel[2]);
        // @recomp: Don't cut riverflow strength in half (many of the game's riverflows were designed for DFlog
        //          which is affected by riverflows much more than BWLog). Note that this does make the Blackwater
        //          Canyon riverflows feel stronger and may need to be tweaked for that level.
        // vel[0] *= -0.5f;
        // vel[2] *= 0.5f;
        vel[0] *= -1.0f;
        objdata->riverflowMagnitude = sqrtf(SQ(vel[2]) + SQ(vel[0]));
        // Factor in paddle forward power
        vel[2] += objdata->paddlePower;

        // @recomp: Apply turn by rotating the front/end log points (like DFLog rather than altering yaw directly)
        if (objdata->mountState == VEHICLE_Mounted) {
            f32 turn = (f32) objdata->joyStickX * (60.0f - ((f32) objdata->joyStickY * 0.05f)) / 60.0f * 0.01f;
            // Reduce turning effectiveness with increased pitch to help avoid fast spiralling when going down waterfalls
            f32 pitchDamp = (f32)self->srt.pitch / (f32)M_90_DEGREES;
            if (pitchDamp < 0) {
                pitchDamp = -pitchDamp;
            }
            turn *= (1.0f - MIN(pitchDamp, 1.0f));
            if (i == 1) {
                vel[0] -= turn;
            } else {
                vel[0] += turn;
            }
        }

        // @recomp: Add roll movement here instead of forcing unk298. Note: Doing this here makes rolling affected
        //          by riverflows, which helps prevent escaping intended areas. This shouldn't have too much of
        //          an effect on the Blackwater Canyon rapids.
        if (objdata->state == BWLog_STATE_1_Roll_Left) {
            vel[0] += -2.5f;
        } else if (objdata->state == BWLog_STATE_2_Roll_Right) {
            vel[0] += 2.5f;
        }

        // Interpolate to target forward/lateral powers
        objdata->powerZ[i] += ((vel[2] - objdata->powerZ[i]) * gUpdateRateF * 0.1f);
        // @recomp: Allow lateral movement while rolling (we changed how roll movement is applied)
        //if (objdata->state == BWLog_STATE_0_Main) {
            objdata->powerX[i] += ((vel[0] - objdata->powerX[i]) * gUpdateRateF * 0.1f);
        //}
        // Convert powers to velocity in world space
        mathMtxXFMF(&pitchYawMtx, 
                       objdata->powerX[i], 0.0f, -objdata->powerZ[i], 
                       &objdata->velocity[i].x, &sp9C, &objdata->velocity[i].z);
        // @recomp: Clamp pitch acceleration to avoid launches at extreme angles.
        //          There might be a better way to patch this, but this works pretty much perfectly.
        CLAMP(objdata->velocity[i].y, -10.0f, 10.0f);
        // Apply velocity
        vel[0] = objdata->velocity[i].x * gUpdateRateF;
        vel[1] = objdata->velocity[i].y * gUpdateRateF;
        vel[2] = objdata->velocity[i].z * gUpdateRateF;
        objdata->endPoints[i].x = objdata->endPoints[i].x + vel[0];
        objdata->endPoints[i].y = objdata->endPoints[i].y + vel[1];
        objdata->endPoints[i].z = objdata->endPoints[i].z + vel[2];
    }

    // Set object position to average of end point positions
    VECTOR_ADD(objdata->endPoints[0], objdata->endPoints[1], vec);
    self->srt.transl.x = vec.f[0] * 0.5f;
    self->srt.transl.y = vec.f[1] * 0.5f;
    self->srt.transl.z = vec.f[2] * 0.5f;
    // Align object pitch with pitch of end points line
    VECTOR_SUBTRACT(objdata->endPoints[1], objdata->endPoints[0], vec);
    self->srt.pitch = -mathAtan2f(vec.f[1], sqrtf(SQ(vec.f[2]) + SQ(vec.f[0])));
    // @recomp: Clamp pitch to avoid reaching exactly 90 degrees (clamps to ~87 degrees).
    //          This allows the player to move forward slightly even when vertical.
    CLAMP(self->srt.pitch, -0x3E00, 0x3E00);
    // @recomp: Calculate yaw from log front/back points (like DFLog)
    self->srt.yaw = mathAtan2f(vec.f[0], vec.f[2]);
    // Collider updates
    gDLL_27->vtbl->func_1E8(self, &objdata->collider, gUpdateRateF);
    gDLL_27->vtbl->func_5A8(self, &objdata->collider);
    gDLL_27->vtbl->func_624(self, &objdata->collider, gUpdateRateF);
    // @recomp: When not rolling, rotate log when bumping into walls
    // TODO: this is fun but it messes up going down waterfalls. will need some changes before we can enable it
    // if (objdata->state == BWLog_STATE_0_Main) {
    //     wall_react(self, &objdata->unk0);
    // }
    
    BWlog_handleSounds(self, objdata);
    BWlog_handleFx(self, objdata);
}

RECOMP_PATCH void BWlog_handleWater(Object* self, BWlog_Data* objdata, s32 side) {
    f32 temp_fa1;
    f32 temp_ft5;
    f32 temp_fv0;
    f32 sp60;
    s32 sp50;
    f32 volume;
    f32 var_fv1;
    s32 i;
    f32 targetY;

    // @recomp: Use floor position when water is not detected
    f32 floor = objdata->collider.waterYList[side];
    s32 foundWater = TRUE;
    if (floor <= -100000.0f) {
        floor = objdata->collider.floorYList[side];
        foundWater = FALSE;
    }

    targetY = objdata->targetWaterYOffset + floor;
    if ((objdata->endPoints[side].y + 30.0f) < targetY) {
        diPrintf("Water too high\n");
    }
    // @recomp: Don't apply the pitch wiggle if not actually in water. Avoids the log wiggling
    //          when it's up on the CClogpush cliff.
    if (foundWater) {
        targetY += (mathSinfInterp(objdata->wiggleYOffsets[side]) * 1.5f);
        objdata->wiggleYOffsets[side] += (gUpdateRateF * 512.0f);
    }

    sp60 = targetY - objdata->endPoints[side].y;
    if ((sp60 > 0.0f) && (objdata->unk300[side] < 0.0f)) {
        volume = objdata->velocity[side].y * 127.0f;
        if (volume < 0.0f) {
            volume = -volume;
        }
        if (volume > 127.0f) {
            volume = 127.0f;
        }
        if (volume > 20.0f) {
            dll_amSfx->Play(self, SOUND_A75, (u8)volume, NULL, NULL, 0, NULL);
        }
    } else if ((sp60 < 0.0f) && (objdata->unk300[side] > 0.0f)) {
        volume = objdata->velocity[side].y * 127.0f;
        if (volume < 0.0f) {
            volume = -volume;
        }
        if (volume > 127.0f) {
            volume = 127.0f;
        }
        if (volume > 20.0f) {
            dll_amSfx->Play(self, SOUND_A74, (u8)volume, NULL, NULL, 0, NULL);
        }
    }

    objdata->unk300[side] = sp60;
    if (sp60 > 25.0f) {
        sp60 = 25.0f;
    }
    if (sp60 < 0.0f) {
        sp60 = 0.0f;
    }
    objdata->velocity[side].y += ((sp60 / 15.0f) * 0.15f * gUpdateRateF);
    objdata->velocity[side].y -= (0.1f * gUpdateRateF);
    i = (s32) gUpdateRate;
    diPrintf("[%d]=%f\n", side, &sp60);
    if (sp60 > 0.0f) {
        if (objdata->velocity[side].y < 0.0f) {
            var_fv1 = sp60 / 25.0f;
            temp_ft5 = 1.0f + var_fv1;
            if (var_fv1 > 1.0f) {
                var_fv1 = 1.0f;
            }
            var_fv1 = (1.0f - var_fv1);
            temp_fa1 = (0.007000029f * var_fv1) + 0.988f;
            while (i--) {
                temp_fv0 = objdata->velocity[side].y;
                if (temp_fv0 > 0/*.0f*/) {
                    var_fv1 = temp_fv0;
                } else {
                    var_fv1 = -temp_fv0;
                }
                objdata->velocity[side].y = temp_fv0 - (temp_fv0 * var_fv1 * 0.1f * temp_ft5);
                objdata->powerZ[side] *= temp_fa1;
                objdata->powerX[side] *= temp_fa1;
                objdata->paddlePower *= 0.99f;
            }
        } else {
            var_fv1 = sp60 / 25.0f;
            temp_ft5 = 1.0f + var_fv1;
            if (var_fv1 > 1.0f) {
                var_fv1 = 1.0f;
            }
            var_fv1 = (1.0f - var_fv1);
            temp_fa1 = (0.007000029f * var_fv1) + 0.988f;
            while (i--) {
                temp_fv0 = objdata->velocity[side].y;
                if (temp_fv0 > 0/*.0f*/) {
                    var_fv1 = temp_fv0;
                } else {
                    var_fv1 = -temp_fv0;
                }
                objdata->velocity[side].y = temp_fv0 - (temp_fv0 * var_fv1 * 0.1f * temp_ft5);
                objdata->powerZ[side] *= temp_fa1;
                objdata->powerX[side] *= temp_fa1;
                objdata->paddlePower *= 0.99f;
            }
        }
    } else {
        while (i--) {
            objdata->velocity[side].y *= 0.994f;
            objdata->powerZ[side] *= 0.995f;
            objdata->powerX[side] *= 0.99f;
            objdata->paddlePower *= 0.99f;
        }
    }
}

/* For the roll control mode config: checks whether the current control mode currently allows rolling. */
static _Bool BWlog_checkRollControlCondition(Object* self, BWlog_Data* objdata) {
    u8 logRollMode = configs_LogRollMode();

    switch (logRollMode) {
    case RECOMP_LOG_ROLL_ENABLED:
        return TRUE;
    case RECOMP_LOG_ROLL_DISABLED:
        return FALSE;
    case RECOMP_LOG_ROLL_WHEN_TILTING_BACK:
        if (objdata->joyStickY <= -35) { //-70 when fully down
            return TRUE;
        }
        break;
    }

    return FALSE;
}

/* For the roll cooldown config: checks whether the cooldown has finished (or returns TRUE if there's no cooldown). */
static _Bool BWlog_checkRollCooldownCondition(Object* self, BWlog_Data* objdata) {
    u8 logRollCooldown = configs_LogRollCooldown();

    switch (logRollCooldown) {
    case RECOMP_LOG_COOLDOWN_NONE:
        return TRUE;
    case RECOMP_LOG_COOLDOWN_QUICK:
        if (objdata->state == BWLog_STATE_0_Main) {
            return TRUE;
        } else if (objdata->state == BWLog_STATE_1_Roll_Left || objdata->state == BWLog_STATE_2_Roll_Right) {
            if (objdata->rollTimer > 28.0f) { //48 when complete
                return TRUE;
            }
        }
        break;
    case RECOMP_LOG_COOLDOWN_FULL:
        if (objdata->state == BWLog_STATE_0_Main) {
            return TRUE;
        }
    }

    return FALSE;
}

RECOMP_PATCH void BWlog_handleControlsAButton(Object* self, BWlog_Data* objdata) {
    s32 doubleTappedA;

    objdata->joyPressed = joyGetPressed(0);
    objdata->joyStickX = joyGetStickX(0);
    objdata->joyStickY = joyGetStickY(0);

    //Decrement timer (used for detecting A button double-tap)
    objdata->joyATimer -= gUpdateRateF;
    if (objdata->joyATimer <= 0.0f) {
        objdata->joyARecentTap = FALSE;
        objdata->joyATimer = 0.0f;
    }

    //Check for A button presses
    doubleTappedA = FALSE;
    if (objdata->joyPressed & A_BUTTON) {
        if (objdata->joyARecentTap) {
            //If A button was already tapped recently, register this A press as a double-tap
            doubleTappedA = TRUE;
            objdata->joyARecentTap = FALSE;
            objdata->joyATimer = 0.0f;
        } else {
            //Otherwise, remember that there was a recent A-tap and start a timer until it's forgotten
            objdata->joyARecentTap = TRUE;
            objdata->joyATimer = 15.0f;
        }
    }

    // @recomp: Disable rolling (unless renabled via an option)
    if (doubleTappedA && 
        BWlog_checkRollControlCondition(self, objdata) && //Check whether current roll control mode currently allows rolling
        BWlog_checkRollCooldownCondition(self, objdata)   //Check whether the rolling cooldown has finished (or there's no cooldown)
    ) {
        //Roll when double-tapping A
        if (objdata->joyStickX > 20) {
            BWlog_startRoll(self, objdata, FALSE);
            objdata->state = BWLog_STATE_2_Roll_Right;
            return;
        }
        if (objdata->joyStickX < -20) {
            BWlog_startRoll(self, objdata, TRUE);
            objdata->state = BWLog_STATE_1_Roll_Left;
        }
      // @recomp: If enabled, allow the A button to be held instead of requiring repeated tapping
    } else if ((objdata->joyPressed & A_BUTTON) || (configs_LogCanHoldA() && joyGetButtons(0) & A_BUTTON)) {
        //Paddle with single A-press
        objdata->paddleTimer = 30.0f;
    }
}

RECOMP_PATCH void BWlog_startRoll(Object* self, BWlog_Data* objdata, s32 arg2) {
    if (arg2 != 0) {
        objdata->rollCurve.x = 1500.0f;
        objdata->rollCurve.y = 500.0f;
        objdata->rollCurve.z = 2000.0f;
        objdata->rollCurve.w = 4000.0f;
        objdata->playerVehicleAnim = 2;
        // @recomp: Don't force lateral movement for rolling here, we'll factor in this speed in the normal movement code
        // objdata->powerX[0] = -2.5f;
        // objdata->powerX[1] = -2.5f;
    } else {
        objdata->rollCurve.x = -1500.0f;
        objdata->rollCurve.y = -500.0f;
        objdata->rollCurve.z = -2000.0f;
        objdata->rollCurve.w = -4000.0f;
        objdata->playerVehicleAnim = 3;
        // @recomp: Ditto
        // objdata->powerX[0] = 2.5f;
        // objdata->powerX[1] = 2.5f;
    }

    objdata->rollAngle = 0;
    objdata->rollTimer = 0.0f;
    objdata->rollSpeed = objdata->rollCurve.x;
    objdata->rollCurveProgress = 0.0f;
    objdata->tValueRoll = 0.0f;
}

RECOMP_PATCH void BWlog_handleSounds(Object* self, BWlog_Data* objdata) {
    u8 bumpSide;
    u8 colliderFlags;
    s32 volume;

    if (objdata->soundHandle == 0) {
        dll_amSfx->Play(self, SOUND_A77, MAX_VOLUME, &objdata->soundHandle, NULL, 0, NULL);
    } else {
        //Adjust sound volume sinusoidally
        objdata->soundVolume = objdata->riverflowMagnitude * 127.0f;
        objdata->soundVolume += mathSinfInterp(objdata->soundVolumePhase) * 30.0f;
        if (objdata->soundVolume < 30.0f) {
            objdata->soundVolume = 30.0f;
        } else if (objdata->soundVolume > 127.0f) {
            objdata->soundVolume = 127.0f;
        }
        dll_amSfx->SetVol(objdata->soundHandle, objdata->soundVolume);

        //Adjust sound pitch sinusoidally
        objdata->soundPitch = ((objdata->unk300[0] + objdata->unk300[1]) * 0.5f) / 25.0f;
        if (objdata->soundPitch < 0.0f) {
            objdata->soundPitch = 0.0f;
        }
        objdata->soundPitch = 1.0f - objdata->soundPitch;
        objdata->soundPitch = (objdata->soundPitch * 0.2f) + 0.2f;
        objdata->soundPitch += mathSinfInterp(objdata->soundPitchPhase) * 0.1f;
        dll_amSfx->SetPitch(objdata->soundHandle, objdata->soundPitch);

        objdata->soundPitchPhase += gUpdateRate << 8;
        objdata->soundVolumePhase += gUpdateRate << 9;
    }

    //Play bump sound when colliding
    volume = 0;
    // @recomp: Also consider block hit touches
    colliderFlags = (objdata->collider.hitsTouchBits | objdata->collider.unk25C) & 3;
    bumpSide = colliderFlags & (colliderFlags ^ objdata->unk32D);
    if (bumpSide & 1) {
        volume = (s32) ((sqrtf(SQ(objdata->powerZ[0]) + SQ(objdata->powerX[0])) * 127.0f) / 0.95f);
    }
    if (bumpSide & 2) {
        if (volume > ((sqrtf(SQ(objdata->powerZ[1]) + SQ(objdata->powerX[1])) * 127.0f) / 0.95f)) {
            volume = (s32) (f32) volume; // what
        } else {
            volume = (s32) ((sqrtf(SQ(objdata->powerZ[1]) + SQ(objdata->powerX[1])) * 127.0f) / 0.95f);
        }
    }
    if (volume > 10) {
        if (volume > MAX_VOLUME) {
            volume = MAX_VOLUME;
        }
        dll_amSfx->Play(self, SOUND_76D_Log_Bump, volume, NULL, NULL, 0, NULL);
    }

    objdata->unk32D = colliderFlags;
}

RECOMP_PATCH void BWlog_findRiverflows(Object* self, BWlog_Data* objdata) {
    s32 i;
    s32 k;
    s32 flowInfluences[2];
    s32 objListLength;
    Object* obj;
    Object** objList;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 pushRadius;
    f32 sp100;
    f32 spFC;
    f32 spF8;
    SRT srt;
    MtxF spA0;

    for (i = 0; i < 2; i++) {
        objdata->flowX[i] = 0.0f;
        objdata->flowZ[i] = 0.0f;
        flowInfluences[i] = 0;
    }

    // grabs DFriverflow instances (and possibly more)
    objList = objGetAllOfType(22, &objListLength);

    for (i = 0; i < objListLength; i++) {
        obj = objList[i];
        // @recomp: Respect riverflow log filter flag (makes BWlog behave correctly in areas where DFLog was originally used)
        if (((DFriverflow_Setup*)obj->setup)->flags & 1) {
            for (k = 0; k < 2; k++) {
                dy = obj->srt.transl.y - objdata->endPoints[k].y;
                if ((dy <= 200.0f) && (dy >= -200.0f)) {
                    dx = obj->srt.transl.x - objdata->endPoints[k].x;
                    dz = obj->srt.transl.z - objdata->endPoints[k].z;
                    dx = sqrtf(SQ(dx) + SQ(dz));
                    pushRadius = ((DFriverflow_Setup*)obj->setup)->range * 1.5f;
                    if (dx < pushRadius) {
                        dx = ((pushRadius - dx) / pushRadius);
                        dx *= (obj->srt.scale * 10.0f);
                        objdata->flowX[k] += mathSinfInterp(obj->srt.yaw) * dx;
                        objdata->flowZ[k] += mathCosfInterp(obj->srt.yaw) * dx;
                        flowInfluences[k]++;
                    }
                }
            }
        }
    }

    for (i = 0; i < 2; i++) {
        if (flowInfluences[i] != 0) {
            objdata->flowX[i] /= flowInfluences[i];
            objdata->flowZ[i] /= flowInfluences[i];
        }
    }
    
    for (i = 0; i < 2; i++) {
        srt.yaw = mathAtan2f(objdata->flowX[i], objdata->flowZ[i]);
        srt.pitch = 0;
        srt.roll = 0;
        srt.scale = 1.0f;
        srt.transl.x = 0;
        srt.transl.y = 0;
        srt.transl.z = 0;
        mathRpyXyzMtx(&spA0, &srt);
        mathMtxXFMF(&spA0, 
            objdata->collider.waterNormalXList[i], objdata->collider.waterNormalYList[i], objdata->collider.waterNormalZList[i], 
            &sp100, &spFC, &spF8);
        srt.yaw = 0;
        srt.pitch = M_90_DEGREES - mathAtan2f(spFC, spF8);
        srt.roll = -(M_90_DEGREES - mathAtan2f(spFC, sp100));
        mathRpyXyzMtx(&spA0, &srt);
        mathMtxXFMF(&spA0, 
            objdata->flowX[i], 0.0f, objdata->flowZ[i], 
            &objdata->flowX[i], &objdata->flowY[i], &objdata->flowZ[i]);
    }
}

/** Get off log in the direction of the dockpoint (helps avoid hopping off into deep water). */
RECOMP_PATCH s32 BWlog_vehicle_GetDismountSide(Object *self) {
    SRT sp88;
    MtxF sp48;
    f32 sp44;
    f32 sp40;
    f32 sp3C;
    f32 temp;
    f32 temp2;
    BWlog_Data* objdata = self->data;

    if (objdata->dockpoint != NULL) {
        sp88.yaw = self->srt.yaw + 0x4000;
        sp88.pitch = self->srt.pitch;
        sp88.roll = self->srt.roll;
        sp88.transl.x = 0.0f;
        sp88.transl.y = 0.0f;
        sp88.transl.z = 0.0f;
        sp88.scale = 1.0f;
        mathYprXyzMtx(&sp48, &sp88);
        mathMtxXFMF(&sp48, 0.0f, 0.0f, 1.0f, &sp44, &sp40, &sp3C);
        temp2 = -((self->srt.transl.x * sp44) + (sp40 * self->srt.transl.y) + (sp3C * self->srt.transl.z));
        temp = (objdata->dockpoint->srt.transl.x * sp44) + (sp40 * objdata->dockpoint->srt.transl.y) + (sp3C * objdata->dockpoint->srt.transl.z) + temp2;
        if (temp < 0) {
            return 1;
        }
    }
    return 2;
}
