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
#include "dlls/objects/210_player.h"

#include "recomp/dlls/objects/528_SC_totemstrength_recomp.h"

#define ANGLE_TO_PIT 0x1DDC 
#define ANGULAR_RANGE (ANGLE_TO_PIT*2)
#define ANGULAR_RANGE_F ((f32)ANGULAR_RANGE)

#define YAW_NEUTRAL -0x2900
#define YAW_LOSE (YAW_NEUTRAL + ANGLE_TO_PIT)
#define YAW_WIN  (YAW_NEUTRAL - ANGLE_TO_PIT)

#define YAW_SPEED_MAX 40.0f
#define YAW_SPEED_DELTA 2.0f

typedef struct {
    ObjSetup base;
} SCTotemStrength_Setup;

typedef struct {
    Object* lightFoot;         //Muscle LightFoot
    f32 unk4;                  //Set to 0 during initial state, but otherwise unused
    f32 yawSpeed;              //Rate of change of rotation (lowered by A button presses)
    s32 yaw;                   //Overall rotation value, applied via a sequence function
    s32 pushSeq;               //Used to control the overall rotation of the sequence
    s16 unused14;
    s16 state;                 //State Machine value
    u32 soundHandleCreak;      //For the wood creaking sound loop
    u32 soundHandleLightFoot;  //Sound handle for the LightFoot's random sounds
    u8 flags;                  //Tracks whether strength game is active, whether contestants can push, and whether to reset
    Vec3f home;                //Initial position
    f32 soundTimerKrystal;     //Random delay between effort sounds (shorter interval when Krystal losing)
    f32 soundTimerLF;          //Random delay between effort sounds (shorter interval when Krystal winning)
} SCTotemStrength_Data;

typedef enum {
    SCTotemStrength_FLAG_None = 0,
    SCTotemStrength_FLAG_Pushing_Enabled = 1,
    SCTotemStrength_FLAG_Strength_Game_Active = 2,
    SCTotemStrength_FLAG_Reset = 4
} SCTotemStrength_Flags;

typedef enum {
    SCTotemStrength_STATE_Initial = 0,
    SCTotemStrength_STATE_Pushing = 1,
    SCTotemStrength_STATE_Won = 2,
    SCTotemStrength_STATE_Lost = 3
} SCTotemStrength_States;

typedef enum {
    SCTotemStrength_SEQCMD_1_Enable_Pushing = 1,
    SCTotemStrength_SEQCMD_2_Initialise_Strength_Game = 2,
    SCTotemStrength_SEQCMD_3_Set_Level_State_3 = 3,
    SCTotemStrength_SEQCMD_4_Set_Level_State_4 = 4,
    SCTotemStrength_SEQCMD_5_Refill_Player_Health_Magic = 5
} SCTotemStrength_SeqCommands;

/*0x0*/ extern u16 dSoundsLightFoot[3];
/*0x8*/ extern u16 dSoundsKrystal[3];

extern void SCTotemStrength_set_level_state(Object* self, u8 value);

static u32 rsFramesSinceLastAutoTap = 0;
static u8 rsLightFootReversals = 0;
static s8 rsPushDirection = 0;

#define ASSIST_TAP_INTERVAL 11
#define ASSIST_REDUCTION 0.75f

/* 
    At N64 framerates (gUpdate of ~6 in LightFoot Village), one in four button taps accidentally gets
    turned into 3 consecutive taps. So at low framerates, 4 taps are on (on average) treated as ~6 taps.
    To account for this: the button repeating bug is fixed and the tap strength is multiplied by 1.5 (6/4)
*/
#define FRAMERATE_ADJUST 1.5f

/**
  * - Handle framerate dependency issues (which made things much harder at smoother framerates).
  * - Add accessibility options for button tapping gameplay.
  */
RECOMP_PATCH int SCTotemStrength_anim_callback(Object* self, Object* overrideObj, AnimObj_Data* animData, s8 arg3) {
    static s32 sPrevYaw;
    /* RECOMP */
    u8 rButtonTapMode = recomp_get_config_u32("button_tap_modes");
    u8 doAssistedTaps = 0;
    f32 prevPushDirection;

    s32 i;
    f32 pushFactor;
    f32 pushAbs;
    Object** objects;
    s32 index;
    s32 count;
    SCTotemStrength_Data* objData;
    f32 pushProgress;
    Object* player;

    objData = self->data;
    player = get_player();
    objData->flags |= SCTotemStrength_FLAG_Reset;
    main_set_bits(BIT_SCTotemStrength_Inactive, 0);
    
    //Handle sequence commands
    for (i = 0; i < animData->unk98; i++) {
        switch (animData->unk8E[i]) {
        case SCTotemStrength_SEQCMD_1_Enable_Pushing:
            objData->flags |= SCTotemStrength_FLAG_Pushing_Enabled;
            break;
        case SCTotemStrength_SEQCMD_2_Initialise_Strength_Game:
            objData->state = SCTotemStrength_STATE_Initial;
            objData->flags |= SCTotemStrength_FLAG_Strength_Game_Active;

            objects = get_world_objects(&index, &count);
            while (index < count) {
                objData->lightFoot = objects[index++];
                if (objData->lightFoot->id == OBJ_SC_musclelightf) {
                    index = count;
                }
            }
            
            func_80023D30(player, 0x401, 0.0f, 0);
            func_80023D30(objData->lightFoot, 0, 1.0f, 0);
            gDLL_3_Animation->vtbl->func19(0x5A, 3, 0, 0);
            break;
        case SCTotemStrength_SEQCMD_3_Set_Level_State_3:
            STUBBED_PRINTF("Enable music change\n");
            SCTotemStrength_set_level_state(self, 3);
            break;
        case SCTotemStrength_SEQCMD_4_Set_Level_State_4:
            SCTotemStrength_set_level_state(self, 4);
            break;
        case SCTotemStrength_SEQCMD_5_Refill_Player_Health_Magic:
            ((DLL_210_Player*)player->dll)->vtbl->set_health(player, 0x7F);
            ((DLL_210_Player*)player->dll)->vtbl->set_magic(player, 0xFF);
            //break;
        default:
            break;
        }
    }
    
    //Return early if the player/LightFoot can't push
    if (!(objData->flags & SCTotemStrength_FLAG_Pushing_Enabled)) {
        return 0;
    }
    
    animData->unk7A = -1;
    animData->unk62 = 0;

    //Find the Muscle LightFoot (@bug: doesn't check if already found, searches every frame)
    objects = get_world_objects(&index, &count);
    while (index < count) {
        objData->lightFoot = objects[index++];
        if (objData->lightFoot->id == OBJ_SC_musclelightf) {
            index = count; //index break
        }
    }
    
    //Start playing wood creaking loop
    if (objData->soundHandleCreak == 0) {
        objData->soundHandleCreak = gDLL_6_AMSFX->vtbl->play(self, SOUND_776_Wooden_Creaking_Loop, MAX_VOLUME, 0, 0, 0, 0);
        gDLL_6_AMSFX->vtbl->set_pitch(objData->soundHandleCreak, 0.8f);
    }

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
    //(@bug: becomes much more difficult to win at smoother framerates)
    for (i = 0; i < gUpdateRate; i++) {
        if (objData->lightFoot == NULL) {
            return 0;
        }
        
        //Get push tValue (as value from 0 to 1, i.e. from losing to winning)
        pushFactor = (objData->yaw - YAW_LOSE) / -ANGULAR_RANGE_F;

        //Get push progress (value from -1 to 1, losing to winning)
        pushProgress = (2.0f * pushFactor) + -1.0f;
        
        //Calculate LightFoot's push factor
        if (pushProgress < 0/*.0f*/) {
            pushAbs = -pushProgress;
        } else {
            pushAbs = pushProgress;
        }
        pushProgress = (pushFactor * 1.7f) + 0.2f;
        pushFactor = pushAbs * pushProgress + 1.0f;

        //@recomp: when auto-pushing, have the LightFoot run out of stamina after two comebacks
        if ((rButtonTapMode >= BUTTON_TAPPING_ASSIST_HOLD) && (rsLightFootReversals > 1)) {
            pushFactor *= 1.0f - (0.33f * rsLightFootReversals);
            if (pushFactor < 0) {
                pushFactor = 0;
            }
        }
        
        //Handle tapping A button
        /* @recomp: framerate-dependency fix
            In the unmodified game, the highest-indexed button snapshot is accidentally repeated
            when the game's running at really low framerates (when gUpdateRate is higher than 4)!
            This repetition is why the trial is easier to complete on N64.

            The game can buffer up to 4 frames' worth of button presses (MAX_BUFFERED_CONT_SNAPSHOTS).
            When gUpdateRate is higher 4 the condition below will eventually query `joy_get_released_buffered(0, i)`
            with an out-of-range snapshot index. When that happens, `joy_get_released_buffered` just returns
            the highest-indexed button snapshot again. This means there's a 25% chance a single button press
            will accidentally turn into N (gUpdateRate - 3) button presses when gUpdateRate is higher than 4!

            LightFoot Village's Strength Trial runs at an average gUpdateRate of 6 or higher on N64, 
            so the increase in difficulty at smooth framerates is very significant.
            
            To fix this (for any framerate), the button-repeating bug has been fixed and the player's
            push strength has been increased to account for 1 in 4 button presses usually being tripled.
        */
        if (doAssistedTaps || //@recomp: handle accessibility options
            ((rButtonTapMode <= BUTTON_TAPPING_ASSIST_ON) && 
                (i < MAX_BUFFERED_CONT_SNAPSHOTS) && //@recomp: don't repeat highest-indexed button snapshot
                (joy_get_released_buffered(0, i) & A_BUTTON)
            )            
        ) {
            objData->yawSpeed -= 2.5f * FRAMERATE_ADJUST;

            //@recomp: handle assisted tapping
            if (doAssistedTaps) {
                doAssistedTaps--;
            }
        }
        if (objData->yawSpeed < -YAW_SPEED_MAX) {
            objData->yawSpeed = -YAW_SPEED_MAX;
        }

        //Apply rotation speed
        if ((objData->yaw >= YAW_WIN) && (objData->yaw <= YAW_LOSE)) {
            objData->yaw += objData->yawSpeed;
        }

        //Recalculate push progress wrt. rate of change of yaw (value from -1 to 1)
        pushProgress = ((f32)sPrevYaw - (f32)objData->yaw) / YAW_SPEED_MAX;

        //Handle winning
        if (objData->yaw < YAW_WIN) {
            sPrevYaw = YAW_NEUTRAL;
            objData->state = SCTotemStrength_STATE_Won;
            func_80023D30(player, 0, 0.0f, 0);
            gDLL_3_Animation->vtbl->func18(objData->pushSeq);
            return 4;
        }
    
        //Rotate sequence using push yaw
        gDLL_3_Animation->vtbl->func28(objData->pushSeq, objData->yaw);

        //Handle losing
        if (objData->yaw > YAW_LOSE) {
            sPrevYaw = YAW_NEUTRAL;
            objData->state = SCTotemStrength_STATE_Lost;
            func_80023D30(player, 0, 0.0f, 0);
            gDLL_3_Animation->vtbl->func18(objData->pushSeq);
            return 4;
        }
        
        //Apply the LightFoot's push
        if (objData->yawSpeed < YAW_SPEED_MAX) {
            //@recomp: handle framerate dependency fix and option for reducing difficulty
            if (rButtonTapMode == BUTTON_TAPPING_ASSIST_ON) {
                objData->yawSpeed += 0.2f * pushFactor * ASSIST_REDUCTION;
            } else {
                objData->yawSpeed += 0.2f * pushFactor;
            }

            //@recomp: for assisting, track how many times the LightFoot's made a comeback
            prevPushDirection = rsPushDirection;
            if ((objData->yawSpeed > 10) && (rsPushDirection < 0)) {
                rsPushDirection = 1;
            } else if ((objData->yawSpeed < -10) && (rsPushDirection >= 0)) {
                rsPushDirection = -1;
            }
            if ((prevPushDirection < 0) && (rsPushDirection > 0)) {
                rsLightFootReversals++;
            }
        }

        //Handle player anim progress/looping
        if (func_80024108(player, ((f32)sPrevYaw - (f32)objData->yaw) / 9500.0f, gUpdateRateF, 0) != 0) {
            if ((((f32)sPrevYaw - (f32)objData->yaw) / 9500.0f) < 0.0f) {
                //Start at end of animation
                func_80023D30(player, 0x401, 1.0f, 0);
            } else {
                //Start at beginning of animation
                func_80023D30(player, 0x401, 0.0f, 0);
            }
        }
        
        //Handle LightFoot anim progress/looping
        if (func_80024108(objData->lightFoot, -(((f32)sPrevYaw - (f32)objData->yaw) / 9500.0f), gUpdateRateF, 0) != 0) {
            if (-(((f32)sPrevYaw - (f32)objData->yaw) / 9500.0f) < 0.0f) {
                //Start at end of animation
                func_80023D30(objData->lightFoot, 0, 1.0f, 0);
            } else {
                //Start at beginning of animation
                func_80023D30(objData->lightFoot, 0, 0.0f, 0);
            }
        }

        //Store previous yaw value
        sPrevYaw = objData->yaw;
    }

    //Play random Krystal sounds
    objData->soundTimerKrystal -= gUpdateRateF;
    if (objData->soundTimerKrystal < 0.0f) {
        if (pushProgress < 0.0f) {
            //More frequent when Krystal is losing
            objData->soundTimerKrystal = rand_next(40, 100);
        } else {
            //Less frequent when Krystal is winning
            objData->soundTimerKrystal = rand_next(120, 240);
        }
        gDLL_6_AMSFX->vtbl->play(self, dSoundsKrystal[rand_next(0, 2)], MAX_VOLUME, 0, 0, 0, 0);
    }
    
    //Play random LightFoot sounds
    objData->soundTimerLF -= gUpdateRateF;
    if (objData->soundTimerLF < 0.0f) {
        if (pushProgress > 0.0f) {
            //More frequent when LightFoot is losing
            objData->soundTimerLF = rand_next(40, 100);
        } else {
            //Less frequent when LightFoot is winning
            objData->soundTimerLF = rand_next(120, 240);
        }
        gDLL_6_AMSFX->vtbl->play(self, dSoundsLightFoot[rand_next(0, 2)], MAX_VOLUME, &objData->soundHandleLightFoot, 0, 0, 0);
    }
    
    //Adjust the pitch/volume of the wood creaking sound loop wrt. push progress magnitude
    if (pushProgress < 0/*.0f*/) {
        pushProgress = -pushProgress;
    }
    gDLL_6_AMSFX->vtbl->set_pitch(objData->soundHandleCreak, (pushProgress * 0.3f) + 0.85f);
    gDLL_6_AMSFX->vtbl->set_vol(objData->soundHandleCreak, (u8)(pushProgress * 0x40) + 0x20);

    return 0;
}
