#include "PR/os.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "configs.h"

#include "common.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/menu.h"
#include "sys/objanim.h"

#include "recomp/dlls/objects/478_DBSH_Symbol_recomp.h"

#define Y_UNDERGROUND 50.0f
#define YAW_WIN 32500
#define YAW_SPEED_MAX 80.0f

typedef struct {
    ObjSetup base;
} DBSH_Symbol_Setup;

typedef struct {
    Object* krystal;        //Phantom Krystal opponent
    f32 unk4;               //Set to 0 in setup, but otherwise unused
    f32 yawSpeed;           //Rate of change of rotation (increased by A button presses)
    f32 baseY;              //Y value of the symbol when it's fully out of the ground
    s32 yaw;                //Overall rotation value, applied via a sequence function
    s32 pushSeq;            //Used to control the overall rotation of the symbol/sequence
    s16 unused18;
    s16 delayTimer;         //Causes the control function to return early
    s16 state;              //State Machine value
    s16 magicFxTimer;       //Delay between creating pink magic particles
    s16 timeLeft;           //Time left before failing the Test of Strength (in frames)
    u32 soundHandle;        //For controlling the magic hum sound loop
    s8 testActive;          //Boolean, whether animCallback function should allow pushing yet
} DBSH_Symbol_Data;

typedef enum {
    DBSH_Symbol_STATE_Initial = 0,
    DBSH_Symbol_STATE_Rising_Up = 1,
    DBSH_Symbol_STATE_Start_Test = 2,
    DBSH_Symbol_STATE_Sinking_Down = 3
} DBSH_Symbol_States;

/*0x0*/ extern u32 dAnimOffsetSabre;   //Keeps track of Sabre's leading foot (L/R)
/*0x4*/ extern u32 dAnimOffsetKrystal; //Keeps track of Krystal's leading foot (L/R)

static u32 rsFramesSinceLastAutoTap = 0;

#define ASSIST_TAP_INTERVAL 9
#define ASSIST_REDUCTION 0.75f
#define FRAMERATE_ADJUST 0.9f

/**
  * - Fix a bug where it doesn't reset the test properly when losing.
  * - Handle intermittent framerate dependency issues (which made things harder at smoother framerates).
  * - Add accessibility options for button tapping gameplay.
  */
RECOMP_PATCH int DBSH_Symbol_anim_callback(Object* self, Object* overrideObj, AnimObj_Data* animData, s8 arg3) {
    static s32 sPrevYaw;
    /* RECOMP */
    u8 rButtonTapMode = recomp_get_config_u32("button_tap_modes");
    u8 doAssistedTaps = 0;
    
    DBSH_Symbol_Data* objData;
    Object* player;
    Object** objects;
    s32 i;
    s32 index;
    s32 count;

    objData = self->data;
    player = get_player();
    
    animData->unk7A = -1;
    animData->unk62 = 0;

    for (i = 0; i < animData->messageCount; i++) {
        if (animData->messages[i] == 1U) {
            objData->testActive = TRUE;
        }
    }
    
    if ((u8)objData->testActive == FALSE) {
        return 0;
    }

    //TEST OF STRENGTH GAMEPLAY

    //Create glowing pink magic particles
    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_27A, NULL, 0, -1, NULL);
    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_27A, NULL, 0, -1, NULL);
    
    //Play magic humming loop
    if (objData->soundHandle == 0) {
        gDLL_6_AMSFX->vtbl->play(self, SOUND_357_Magic_Hum_Loop, MAX_VOLUME, &objData->soundHandle, NULL, 0, NULL);
        gDLL_6_AMSFX->vtbl->set_pitch(objData->soundHandle, 0.8f);
    }

    //Find phantom Krystal
    if (objData->krystal == NULL) {
        for (objects = get_world_objects(&index, &count); index < count; index++) {
            objData->krystal = objects[index];
            if (objData->krystal->id == OBJ_DBSH_Krystal) {
                break;
            }
        }
    }
    
    //Decrement test timer
    objData->timeLeft -= (s16) gUpdateRateF;

    //@recomp: handle simulated button presses
    if ((rButtonTapMode == BUTTON_TAPPING_ASSIST_AUTO) ||
        ((rButtonTapMode == BUTTON_TAPPING_ASSIST_HOLD) && (joy_get_buttons(0) & A_BUTTON))
    ) {
        rsFramesSinceLastAutoTap += gUpdateRate;
        if (rsFramesSinceLastAutoTap >= ASSIST_TAP_INTERVAL) {
            doAssistedTaps = rsFramesSinceLastAutoTap/ASSIST_TAP_INTERVAL;
            rsFramesSinceLastAutoTap %= ASSIST_TAP_INTERVAL;
        }
    }

    //Handle pushing behaviour, looping for each frame skipped
    //(@bug: becomes slightly more difficult to win at smoother framerates)
    for (i = 0; i < gUpdateRate; i++) {
        if (objData->krystal == NULL) {
            return 0;
        }
        
        //Handle losing (time running out)
        if (objData->timeLeft <= 0) {
            gDLL_3_Animation->vtbl->end_obj_sequence(objData->pushSeq);
            func_8000FA2C();

            //@recomp: fix a bug where it doesn't reset the test properly
            objData->yaw = 0;
            objData->yawSpeed = 0;
            objData->timeLeft = 0;
        }

        //Handle tapping A button
        /* @recomp: framerate-dependency fix 
            In the unmodified game, the highest-indexed button snapshot is accidentally repeated
            when the game's running at really low framerates (when gUpdateRate is higher than 4)!
            This repetition is why the test is easier to complete on N64.

            The game can buffer up to 4 frames' worth of button presses (MAX_BUFFERED_CONT_SNAPSHOTS).
            When gUpdateRate is higher 4 the condition below will eventually query `joy_get_released_buffered(0, i)`
            with an out-of-range snapshot index. When that happens, `joy_get_released_buffered` just returns
            the highest-indexed button snapshot again. This means there's a 25% chance a single button press
            will accidentally turn into N (gUpdateRate - 3) button presses when gUpdateRate is higher than 4!

            The Test of Strength shrine seems to run at an average gUpdateRate of 3 or 4 on N64,
            so it's probably only occasionally susceptible to this bug. LightFoot Village's
            Strength Trial (which is programmed nearly identically to this shrine) runs at an 
            average gUpdateRate of 6 or higher on N64, so the increase in difficulty at smoother 
            framerates is much more apparent there.

            For the Krazoa Test of Strength, the opponent's push strength has been decreased very
            slightly to account for smooth framerates missing out on an intermittent doubling of presses.
        */
        if (doAssistedTaps || //@recomp: handle accessibility options
            ((rButtonTapMode <= BUTTON_TAPPING_ASSIST_ON) && 
                (i < MAX_BUFFERED_CONT_SNAPSHOTS) && //@recomp: don't repeat highest-indexed button snapshot
                (joy_get_released_buffered(0, i) & A_BUTTON)
            )            
        ) {
            objData->yawSpeed += 18.8f;
            
            //5% chance of playing random Sabre effort sound
            if (rand_next(0, 20) == 0) {
                gDLL_6_AMSFX->vtbl->play(
                    NULL, 
                    SOUND_710_Sabre_Test_of_Strength_1 + rand_next(0, 2), 
                    0x1E, 
                    NULL, 
                    NULL, 
                    0, 
                    NULL
                );
            }

            //@recomp: handle assisted tapping
            if (doAssistedTaps) {
                doAssistedTaps--;
            }
        }
        
        //Limit rotation speed in positive direction (player push)
        if (objData->yawSpeed > YAW_SPEED_MAX) {
            objData->yawSpeed = YAW_SPEED_MAX;
        }

        //Apply rotation speed
        if ((objData->yaw <= YAW_WIN) && (objData->yaw >= 0)) {
            objData->yaw += objData->yawSpeed;
        }
        
        //Handle winning
        if (objData->yaw > YAW_WIN) {
            objData->yaw = YAW_WIN;
            gDLL_3_Animation->vtbl->end_obj_sequence(objData->pushSeq);
            func_8000FA2C();
            objData->magicFxTimer = 10;
            objData->delayTimer = 20;
            func_80023D30(player, 0, 0.0f, 0);
            break;
        }
        
        //Rotate sequence using push yaw
        gDLL_3_Animation->vtbl->func28(objData->pushSeq, objData->yaw);
        
        //Handle being stopped (when at initial rotation)
        if (objData->yaw < 0) {
            objData->yaw = 0;
            
            if (objData->yawSpeed < 0.0f) {
                objData->yawSpeed = 0.0f;
            }
            
            //Use stopped animation for Sabre/Krystal
            func_80023D30(player, dAnimOffsetSabre + 0x4A, 0.0f, 0);
            func_80023D30(objData->krystal, dAnimOffsetKrystal + 0x4A, 1.0f, 0);
            
            sPrevYaw = objData->yaw;

            if (objData->yawSpeed > -300.0f) {
                //@recomp: handle framerate dependency fix and option for reducing difficulty
                if (rButtonTapMode == BUTTON_TAPPING_ASSIST_ON) {
                    objData->yawSpeed -= 10.1f * ASSIST_REDUCTION;
                } else {
                    objData->yawSpeed -= 10.1f * FRAMERATE_ADJUST;
                }
            }

            return 0;
        }

        //Apply Krystal's counter-push
        if (objData->yawSpeed > -YAW_SPEED_MAX) {          
            //@recomp: handle framerate dependency fix and option for reducing difficulty
            if (rButtonTapMode == BUTTON_TAPPING_ASSIST_ON) {
                objData->yawSpeed -= (1.8f + (0.012f * objData->yawSpeed)) * ASSIST_REDUCTION;
            } else {
                objData->yawSpeed -= 1.8f + (0.012f * objData->yawSpeed) * FRAMERATE_ADJUST;
            }
        }
    
        //Handle player anim progress/looping
        if (func_80024108(player, ((f32) objData->yaw - (f32) sPrevYaw) / 7500.0f, gUpdateRateF, NULL) != 0) {
            dAnimOffsetSabre = 1 - dAnimOffsetSabre;
            if ((((f32) objData->yaw - (f32) sPrevYaw) / 7500.0f) < 0.0f) {
                //Start at end of animation
                func_80023D30(player, dAnimOffsetSabre + 0x4A, 1.0f, 0);
            } else {
                //Start at beginning of animation
                func_80023D30(player, dAnimOffsetSabre + 0x4A, 0.0f, 0);
            }
        }
        
        //Handle phantom Krystal anim progress/looping
        if (objData->krystal && (func_80024108(objData->krystal, -((f32) objData->yaw - (f32) sPrevYaw) / 7500.0f, gUpdateRateF, NULL) != 0)) {
            dAnimOffsetKrystal = 1 - dAnimOffsetKrystal;
            if ((((f32) objData->yaw - (f32) sPrevYaw) / 7500.0f) < 0.0f) {
                //Start at beginning of animation
                func_80023D30(objData->krystal, dAnimOffsetKrystal + 0x4A, 0.0f, 0);
            } else {
                //Start at end of animation
                func_80023D30(objData->krystal, dAnimOffsetKrystal + 0x4A, 1.0f, 0);
            }
        }
        
        sPrevYaw = objData->yaw;         
    }

    //Adjust magic hum sound loop's pitch wrt. yaw
    gDLL_6_AMSFX->vtbl->set_pitch(objData->soundHandle, ((f32) objData->yaw / 97500.0f) + 0.8f);

    return 0;
}
