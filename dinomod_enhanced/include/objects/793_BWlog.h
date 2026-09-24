#pragma once

#include "PR/ultratypes.h"
#include "dlls/engine/27.h"
#include "game/objects/object.h"

typedef enum {
    BWLog_STATE_0_Main,
    BWLog_STATE_1_Roll_Left,
    BWLog_STATE_2_Roll_Right,
    BWLog_STATE_3_Placeholder1,
    BWLog_STATE_4_Placeholder2
} BWlog_States;

// @recomp: Edited version of struct
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
/*2C4*/ u8 _unk2C4[3];
/*2C7*/ u8 flipState; //@recomp: repurpose unused field, applies visual flip to the log via seqJoint (for mounting log "backwards")
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
