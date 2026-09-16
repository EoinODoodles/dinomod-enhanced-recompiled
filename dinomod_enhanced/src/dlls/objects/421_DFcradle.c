#include "PR/os.h"
#include "math_util.h"
#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "dlls/objects/347_texscroll2.h"
// #include "dlls/objects/421_DFcradle.h"

#include "recomp/dlls/objects/421_DFcradle_recomp.h"

// #define DEBUG_CRADLE

//TEMPORARY DEFINES
typedef struct {
    /* 0000 */ f32 unk0;
    /* 0004 */ f32 unk4; // curveProgress? (lerp t-value from 0 to 100?)
    /* 0008 */ f32 unk8;
    /* 000C */ f32 unkC;
    /* 0010 */ s32 unk10;
    /* 0014 */ f32 unk14[20];
    /* 0064 */ f32 unk64;
    /* 0068 */ Vec3f unk68; //curve point (at current tValue)
    /* 0074 */ Vec3f unk74; //curve tangent (at current tValue)
    /* 0080 */ s32 unk80;
    /* 0084 */ f32* unk84;
    /* 0088 */ f32* unk88;
    /* 008C */ f32* unk8C;
    /* 0090 */ s32 numControlPoints;
    /* 0094 */ SplineFunc splineFunc;
    /* 0098 */ SplineConverterFunc splineConverterFunc;
} UnkCurvesStruct_Split;

typedef struct {
    ObjSetup base;
    u8 roll;
    u8 pitch;
    u8 yaw;
} DFCradle_Setup;

typedef struct {
    UnkCurvesStruct_Split curves;
    f32 speed;
    f32* splineX;
    f32* splineY;
    f32* splineZ;
    f32 pulleyValLower; //Related to the rotating middle pulley closer to SwapStone Circle entrance
    f32 pulleyValUpper; //Related to the rotating middle pulley closer to the shrine entrance
    s8 enabled;         //Cradle has been powered (Kyte activated turbine lever)
    s8 direction;       //1 or -1
    u8 pauseTimer;
    u8 prevCradleStation;
    s8 soundTimer; //Randomised interval between rope straining sounds
} DFCradle_Data;

#define DFCradle_obj_Control DFCradle_control
#define DFCradle_getStationNumber DFCradle_func_99C

#define dll_TexScroll2(obj) (((DLL_347_texscroll2*)obj->dll)->vtbl)

#define BIT_DF_Cradle_Moving_Down 0x1C
//END OF TEMPORARY DEFINES

#define BIT_DF_Cradle_Reverse_Direction BIT_DF_Cradle_Moving_Down

/*0x0*/ extern s16 sSoundIDs[];
/*0x8*/ extern u32 dTexscrollUIDs[];

extern u8 DFCradle_getStationNumber(Object* self, f32 x, f32 z, u8 isPlayer);

/**
  * - Fix framerate dependencies.
  * - Adjust the way the direction-reversing gamebit is handled, so the Projectile Switches hopefully work as intended (or at least do something helpful!)
  */
RECOMP_PATCH void DFCradle_obj_Control(Object* self) {
    DFCradle_Data* objData;
    Object** objects;
    Object* player;
    s32 count;
    s32 idx;
    u8 doDirectionReverse;
    u8 cradleStation;
    u8 playerStation;
    f32 dx;
    f32 dy;
    f32 dz;
    u8 doReverseAfterStopping;
    Object* texscroll;
    u8 i;
    s8 scrollSpeed;

    objData = self->data;
    
#ifdef DEBUG_CRADLE
    diPrintf("Cradle direction: %d\n", objData->direction);
    diPrintf("Cradle speed: %f\n", &objData->speed);
    diPrintf("Curve tValue: %1.3f\n", &objData->curves.unk0);
#endif

    //Handle cradle's motion
    if (objData->enabled) {

        //@recomp: handle changing direction by gamebit (hitting the nearby Projectile Switches)
        if (mainGetBits(BIT_DF_Cradle_Reverse_Direction)) {
            mainSetBits(BIT_DF_Cradle_Reverse_Direction, FALSE);
            objData->direction *= -1;
#ifdef DEBUG_CRADLE
            recomp_printf("Changing direction by gamebit! %d -> %d\n", -objData->direction, objData->direction);
#endif
        }

        //Play rope straining noise at random intervals
        if ((objData->soundTimer -= gUpdateRate) < 0) { //@recomp: fix framerate dependency
            dll_amSfx->Play(self, sSoundIDs[mathRnd(0, 2)], MAX_VOLUME, NULL, NULL, 0, NULL);
            objData->soundTimer = mathRnd(40, 60);
        }
        
        if (objData->pauseTimer) {
            //@recomp: fix framerate dependency
            if (objData->pauseTimer > gUpdateRate) {
                objData->pauseTimer -= gUpdateRate;
            } else {
                objData->pauseTimer = 0;
            }
        }
        
        player = objGetPlayer();
        if (player == NULL) {
            return;
        }

        //Check if the cradle should reverse back to the player once it reaches the next pulley
        {
            playerStation = DFCradle_getStationNumber(self, player->globalPosition.x, player->globalPosition.z, TRUE);
            cradleStation = DFCradle_getStationNumber(self, self->globalPosition.x,   self->globalPosition.z,   FALSE);

            doDirectionReverse = ((playerStation < cradleStation) && (objData->direction > 0)) || //Cradle moving upwards and the player's at a lower station
                                ((playerStation > cradleStation) && (objData->direction < 0));   //Cradle moving downwards and the player's at a higher station
        }

        //Get the player's distance from the cradle
        dx = self->globalPosition.x - player->globalPosition.x;
        dy = self->globalPosition.y - player->globalPosition.y;
        dz = self->globalPosition.z - player->globalPosition.z;
        dx = SQ(dx) + SQ(dy) + SQ(dz);
        
        //Pause when reaching a pulley (longer pause if the player's close)
        if (!objData->pauseTimer && (cradleStation != objData->prevCradleStation)) {
            if (dx < SQ(200)) {
                objData->pauseTimer = 40;
            } else {
                objData->pauseTimer = 10;
            }
        }
        objData->prevCradleStation = cradleStation;

        /* Automatically reverse direction to move back towards the player when all these conditions are met:
         - the player isn't on the cradle
         - the cradle's pauseTimer just reset (finished waiting after slowing down to stop at a pulley)
         - the cradle's moving down but the player's at a higher station, or moving up and the player's at a lower station
         */
        if ((objData->pauseTimer == 10) && (self != player->parent) && doDirectionReverse) {
            objData->direction = -objData->direction;

            //@recomp: don't set gamebit here
            // if (objData->direction > 0) {
            //     mainSetBits(BIT_DF_Cradle_Moving_Down, FALSE);
            // } else {
            //     mainSetBits(BIT_DF_Cradle_Moving_Down, TRUE);
            // }
        }
        
        if (objData->pauseTimer <= 10) {
            objects = objGetAllOfType(OBJTYPE_Pulley, &count);

            //Rotate pulley objects
            for (idx = 0; idx < count; idx++) {
                objects[idx]->srt.yaw -= objData->speed * 500.0f * objData->direction * gUpdateRateF * (((objects[idx]->stateFlags & 1) * 2) - 1);
            }                
            
            //@recomp: handle reversing direction by gamebit earlier in the function
            {
                //Set the cradle's speed based on distance to the nearest pulley
                s32 minDistance = 10000;
                s32 distance; 
                
                for (idx = 0; idx < count; idx++) {
                    distance = vec3Distance(&self->globalPosition, &objects[idx]->globalPosition);
                    if (minDistance > distance) {
                        minDistance = distance;
                    }
                }
                
                if (minDistance <= 20) {
                    minDistance = 20;
                }
                if (minDistance >= 120) {
                    minDistance = 120;
                }
                
                objData->speed = (((minDistance - 20.0f) * 1.8f) / 100.0f) + 0.2f;
            }
            
            scrollSpeed = (objData->speed * -53.0f) * objData->direction;

            //Handle curves
            {
                //@TODO: investigate why the curve tValue sometimes jumps slightly just by reversing the sign of curves_func_800053B0's arg1

                if (curves_func_800053B0((CurvesStruct*)&objData->curves, -scrollSpeed / 53.0f)) {
                    //Reverse direction when reaching the end of the pulley path
                    objData->direction = -objData->direction;
                    objData->speed = 0/*0.0f*/;
                    
                    if (dx < SQ(200)) {
                        objData->pauseTimer = 60;
                    } else {
                        objData->pauseTimer = 20;
                    }

                    //@recomp: don't set gamebit here
                    // if (objData->direction > 0) {
                    //     mainSetBits(BIT_DF_Cradle_Moving_Down, FALSE);
                    // } else {
                    //     mainSetBits(BIT_DF_Cradle_Moving_Down, TRUE);
                    // }
                } else if ((objData->curves.unk74.x != 0/*.0f*/) || (objData->curves.unk74.z != 0/*.0f*/)) {
                    //Set yaw using the curve tangent
                    self->srt.yaw = mathAtan2f(objData->curves.unk74.x, objData->curves.unk74.z) + M_180_DEGREES;
                }

                //Set position using the current point on the curve
                self->srt.transl.x = objData->curves.unk68.x;
                self->srt.transl.y = objData->curves.unk68.y;
                self->srt.transl.z = objData->curves.unk68.z;
            }
        } else {
            scrollSpeed = 0;
        }

        //Animate cradle ropes
        for (i = 0; i < 4; i++) {
            texscroll = objGetObjectByUID(dTexscrollUIDs[i]);
            if (texscroll != NULL) {
                dll_TexScroll2(texscroll)->change_scroll_speed(texscroll, scrollSpeed);
            }
        }
        return;
    }

    //Check if the cradle has been powered
    objData->enabled = mainGetBits(BIT_DF_Cradle_Powered);
    
    if (objData->speed > 0.0f) {
        objData->speed -= 0.02f;
    } else {
        objData->speed = 0.0f;
    }
}
