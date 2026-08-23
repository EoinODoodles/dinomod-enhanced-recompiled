#include "dll.h"
#include "dlls/engine/6_amsfx.h"
#include "modding.h"
#include "recomputils.h"
#include "math_util.h"

#include "sys/dll.h"
#include "sys/gfx/animseq.h"
#include "sys/intersect.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objexpr.h"
#include "sys/objhits.h"

#include "recomp/dlls/_asm/789_recomp.h"

//TEMPORARY DEFINES
#define WCTempleDial_obj_Control dll_789_obj_Control
#define WCTempleDial_setBlockIconFrames dll_789_func_51C

#define BIT_WC_Sun_Temple_Dial_Hit_Sunrise_Switch 0x2F8
#define BIT_WC_Sun_Temple_Dial_Hit_Midday_Switch 0x2D1
#define BIT_WC_Sun_Temple_Dial_Hit_Sunset_Switch 0x2D2

#define BIT_WC_Moon_Temple_Dial_Hit_Crescent_Moon_Switch 0x203
#define BIT_WC_Moon_Temple_Dial_Hit_Half_Moon_Switch 0x2EC
#define BIT_WC_Moon_Temple_Dial_Hit_Full_Moon_Switch 0x2EF
//END OF TEMPORARY DEFINES

typedef struct {
    ObjSetup base;
    s8 yaw;
    s8 modelIdx;
    s16 unk1A;
    s16 unk1C;
    s16 gamebitFinished;
} WCTempleDial_Setup;

typedef struct {
    f32 rotateSpeed;
    f32 rotateSpeedGoal;
    u8 switchFlags;
    u8 dialFlags;
    f32* rotateSpeedGoals;
    s16* switchGamebits;
} WCTempleDial_Data;

typedef enum {
    WCTempleDial_MODEL_Sun,
    WCTempleDial_MODEL_Moon
} WCTempleDial_Models;

typedef enum {
    WCTempleDial_FLAG_Stopped = 1 //Puzzle solved
} WCTempleDial_Flags;

typedef enum {
    WCTempleDial_SWITCH_1 = 1,
    WCTempleDial_SWITCH_2 = 2,
    WCTempleDial_SWITCH_3 = 4
} WCTempleDial_SwitchFlags;

#define ALL_THREE_SWITCHES (\
    WCTempleDial_SWITCH_1 |\
    WCTempleDial_SWITCH_2 |\
    WCTempleDial_SWITCH_3\
)

/*0x0*/ static s16 dSunGamebits[] = {
    BIT_WC_Sun_Temple_Dial_Hit_Sunrise_Switch, 
    BIT_WC_Sun_Temple_Dial_Hit_Midday_Switch, 
    BIT_WC_Sun_Temple_Dial_Hit_Sunset_Switch
};
/*0x8*/ static s16 dMoonGamebits[] = {
    BIT_WC_Moon_Temple_Dial_Hit_Crescent_Moon_Switch,
    BIT_WC_Moon_Temple_Dial_Hit_Half_Moon_Switch,
    BIT_WC_Moon_Temple_Dial_Hit_Full_Moon_Switch
};
/*0x10*/ extern f32 dSunRotateSpeedGoals[];
/*0x1C*/ extern f32 dMoonRotateSpeedGoals[];

extern void WCTempleDial_setBlockIconFrames(Object* self, u8 iconStates);

RECOMP_PATCH void WCTempleDial_obj_Control(Object* self) {
    s32 switchIdx;
    s32 j;
    s32 wrongSwitchPressed;
    WCTempleDial_Data* objData;
    WCTempleDial_Setup* objSetup;

    objData = self->data;
    objSetup = (WCTempleDial_Setup*)self->setup;
    
    //When the puzzle's finished, update the sun/moon icons and return early
    if (objData->dialFlags & WCTempleDial_FLAG_Stopped) {
        WCTempleDial_setBlockIconFrames(self, objData->switchFlags);
        return;
    }
    
    //Accelerate/decelerate to meet goal rotation speed
    objData->rotateSpeed += (objData->rotateSpeedGoal - objData->rotateSpeed) * 0.01f * gUpdateRateF;

    //Rotate
    self->srt.roll += gUpdateRateF * objData->rotateSpeed;

    //Check if the 3 switches have been pressed
    for (switchIdx = 0; switchIdx < 3; switchIdx++) {
        //Check if the gamebit for each of the inactive switches becomes set
        if (!(objData->switchFlags & (1 << switchIdx)) && mainGetBits(objData->switchGamebits[switchIdx])) {
            //Check if the switch was pressed in the wrong order
            wrongSwitchPressed = FALSE;
            for (j = 0; j < switchIdx; j++) {
                if ((objData->switchFlags & (1 << j)) == FALSE) {
                    wrongSwitchPressed = TRUE;
                    break;
                }
            }
            
            if (wrongSwitchPressed) {
                //Reset all the switches
                for (j = 0; j < 3; j++) {
                    mainSetBits(objData->switchGamebits[j], 0);
                }
                
                gDLL_6_AMSFX->vtbl->Play(self, SOUND_912_Object_Refused, MAX_VOLUME, NULL, NULL, 0, NULL);
                
                //Reset back to initial state and rotation speed
                objData->switchFlags = 0;
                objData->rotateSpeedGoal = objData->rotateSpeedGoals[0];
                break;
            } else {
                //Update switch flags and rotation speed
                objData->switchFlags |= (1 << switchIdx);
                if (switchIdx == 0) {
                    objData->rotateSpeedGoal = objData->rotateSpeedGoals[1];
                } else if (switchIdx == 1) {
                    objData->rotateSpeedGoal = objData->rotateSpeedGoals[2];
                }

                //@recomp: save the full "puzzle solved" jingle until all three switches are pressed
                if (objData->switchFlags == ALL_THREE_SWITCHES) {
                    gDLL_6_AMSFX->vtbl->Play(self, SOUND_798_Puzzle_Solved, MAX_VOLUME, NULL, NULL, 0, NULL);
                } else {
                    //Smaller "success!" chime as you progress through the switch sequence
                    gDLL_6_AMSFX->vtbl->Play(self, SOUND_B01_Success_Chime, MAX_VOLUME, NULL, NULL, 0, NULL);
                }
            }
        }
    }
    
    //Update the three sun/moon icons to match the dial's state
    WCTempleDial_setBlockIconFrames(self, objData->switchFlags);
    
    //Check if the puzzle is finished
    if (objData->switchFlags == ALL_THREE_SWITCHES) {
        mainSetBits(objSetup->gamebitFinished, TRUE);
        objData->dialFlags |= WCTempleDial_FLAG_Stopped;
    }
}
