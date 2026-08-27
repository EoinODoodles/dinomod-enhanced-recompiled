#include "configs.h"
#include "custom_gamebits.h"
#include "engine/5_AMSEQ.h"
#include "macros.h"
#include "modding.h"
#include "object_util.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "PR/os.h"
#include "dll.h"
#include "game/objects/object.h"
#include "game/gamebits.h"
#include "sys/dll.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/menu.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"

#include "recomp/dlls/engine/5_AMSEQ_recomp.h"
#include "recomp/dlls/objects/779_WCLevelControl_recomp.h"

// #define DEBUG_ACT1
// #define DEBUG_MUSIC

//TEMPORARY DEFINES
#define WCLevelControl_obj_Setup WCLevelControl_setup
#define WCLevelControl_obj_Control WCLevelControl_control
#define WCLevelControl_obj_Free WCLevelControl_free
#define WCLevelControl_obj_GetDataSize WCLevelControl_get_data_size
#define WCLevelControl_animCallback WCLevelControl_anim_callback
#define WCLevelControl_handleAct1 WCLevelControl_handle_act1
#define WCLevelControl_handleAct2 WCLevelControl_handle_act2
#define WCLevelControl_sunPuzzleReset WCLevelControl_sun_puzzle_init_hard
#define WCLevelControl_moonPuzzleReset WCLevelControl_moon_puzzle_init_hard

#define BIT_WC_Sun_Pressure_Switch_Active 0x7ED
#define BIT_WC_Sun_Beacon_Raised 0x7EF
#define BIT_WC_Sun_Beacon_Lit 0x7F9
#define BIT_WC_SlabDoor_Sun_Symbol_Lit 0x7F7

#define BIT_WC_Moon_Pressure_Switch_Active 0x7EE
#define BIT_WC_Moon_Beacon_Raised 0x7F0
#define BIT_WC_Moon_Beacon_Lit 0x7FA
#define BIT_WC_SlabDoor_Moon_Symbol_Lit 0x802

#define BIT_WC_SlabDoor_Opened 0x813
#define BIT_WC_Boss_Door_Opened 0x819
#define BIT_WC_King_EarthWalker_Cage_Opened 0x7F5
#define BIT_WC_King_EarthWalker_Rescued 0x1DF

#define BIT_WC_Sun_Pushblock_Puzzle_Reset 0x808
#define BIT_WC_Moon_Pushblock_Puzzle_Reset 0x809
#define BIT_WC_Sun_Pushblock_Puzzle_Progress 0x810
#define BIT_WC_Moon_Pushblock_Puzzle_Progress 0x811
#define BIT_WC_Sun_Aperture_Opened 0x812
#define BIT_WC_Moon_Aperture_Opened 0x813

#define BIT_WC_Sun_Temple_Illusory_Wall_Switch_Pressed 0x205
#define BIT_WC_Sun_Temple_Maze_Timed_Challenge_Switch_Pressed 0x2B1
#define BIT_WC_Sun_Temple_Maze_Timed_Challenge_Door_Opened 0x274
#define BIT_WC_Played_Seq_179_Sun_Temple_Maze_Timed_Challenge_Intro 0x204
#define BIT_WC_Sun_Temple_Maze_Illusory_Wall_1_Shown 0x226
#define BIT_WC_Sun_Temple_Maze_Illusory_Wall_2_Hidden 0x2A6
#define BIT_WC_Sun_Temple_Maze_Illusory_Wall_3_Hidden 0x206
#define BIT_WC_Sun_Temple_Maze_Illusory_Wall_4_Hidden 0x25F
#define BIT_WC_Sun_Temple_Maze_Solved 0x2A5
//END OF TEMPORARY DEFINES

typedef struct {
    f32 timer;
    u8 state;
    u8 flags;
    u8 previousState;
    /* RECOMP */
    u16 extraFlags;
    s32 challengeMusicTime;     //how long the challenge music has been playing for (outside of sequences)
    s32 musicFadeTimer;         //timer for fading out the challenge music when the player idles on the switch for a long while
    s32 musicPlayerNo;          //musicPlayerID for the challenge music
} WCLevelControl_Data;

typedef enum {
    FLAG_1_Entered_ObjSeq = 0x1,
    FLAG_2_Pressure_Switch_Challenge_Started = 0x2,
    FLAG_4_Sun_Beacon_Lit = 0x4,
    FLAG_8_Moon_Beacon_Lit = 0x8,
    FLAG_10_Sun_Aperture_Opened = 0x10,
    FLAG_20_Moon_Aperture_Opened = 0x20,
    FLAG_40_Sun_Temple_Maze_Solved = 0x40
} WCLevelControl_Flags;

typedef enum {
    CUSTOM_FLAG_Act1_Gamebits_Set = 1,                                   //Act 1's important gamebits were given their expected values when WCLevelControl entered Act 2
    CUSTOM_FLAG_Top_Up_Timer = 2,                                        //Pressure switch challenge's timer will be refilled
    CUSTOM_FLAG_Player_on_Pressure_Switch = 4,                           //The player is standing on the current pressure switch challenge's switch
    CUSTOM_FLAG_Challenge_Music_Started = 8,                             //The timed challenge music was played from the start during the current timed challenge
    CUSTOM_FLAG_Challenge_Music_Too_Far_Ahead = 0x10,                    //Challenge music is too far ahead of the timer countdown (due to topping up the timer when standing on it again) - challenge music will fade out, then stop, then play from the beginning
    CUSTOM_FLAG_Challenge_Music_Fully_Faded_Out = 0x20,                  //Challenge music has fully faded out, and is ready to be replayed when the player leaves the switch (or the Walled City theme can play in the meantime, until they leave the switch)
    CUSTOM_FLAG_Stop_Challenge_Music = 0x40,                             //Challenge music will stop immediately - used when the challenge's completed successfully, or when swapping between Sun/Moon challenges
    CUSTOM_FLAG_Swapped_Back_to_Walled_City_Music = 0x80,                //Played Walled City's music after the timed challenge music stopped playing
    CUSTOM_FLAG_Played_Walled_City_Music_While_Idling_on_Switch = 0x100  //Played Walled City's music while the timed challenge music is waiting to restart but the player has yet to leave the switch
} WCLevelControl_CustomFlags;

#define MUSIC_FLAGS (\
    CUSTOM_FLAG_Challenge_Music_Started | \
    CUSTOM_FLAG_Challenge_Music_Too_Far_Ahead | \
    CUSTOM_FLAG_Challenge_Music_Fully_Faded_Out | \
    CUSTOM_FLAG_Stop_Challenge_Music | \
    CUSTOM_FLAG_Swapped_Back_to_Walled_City_Music | \
    CUSTOM_FLAG_Played_Walled_City_Music_While_Idling_on_Switch\
)

typedef enum {
    STATE_0_Idle,

    //Act 1 states
    STATE_1_Sun_Beacon_Timed_Challenge,
    STATE_2_Moon_Beacon_Timed_Challenge,
    STATE_3_WCSlabDoor_Opened,

    //Act 2 states
    STATE_4_Sun_Maze_Timed_Challenge,
    STATE_5_Unused, //Maybe a placeholder for a challenge in the Moon Temple room with the hole in the wall?
    STATE_6_Sun_Maze_Setup
} WCLevelControl_States;

extern int WCLevelControl_animCallback(Object* self, Object* overrideObj, AnimObj_Data* animData, s8 prevCallbackValue);
extern void WCLevelControl_handleAct1(Object* self, WCLevelControl_Data* objData);
extern void WCLevelControl_handleAct2(Object* self, WCLevelControl_Data* objData);
extern void WCLevelControl_sunPuzzleReset(void);
extern void WCLevelControl_moonPuzzleReset(void);

#define PRESSURE_SWITCH_CHALLENGE_SECONDS (60)

/* Tops the pressure switch challenges' timers back up to 60 seconds, 
   used as a quality-of-life feature when the player is still standing on the switch. */
static void WCLevelControl_refillTimer(Object* self) {
    #define ROUND_TO_SIXTY 2
    #define CHALLENGE_END_TIME ((PRESSURE_SWITCH_CHALLENGE_SECONDS * 60) + ROUND_TO_SIXTY)
    extern f32 D_800A7D70;
    WCLevelControl_Data* objData;

    objData = self->data;

    s32 timerValue = D_800A7D70;
    if (timerValue < CHALLENGE_END_TIME) {
        timerValue += gUpdateRate * 40;
        if (timerValue > CHALLENGE_END_TIME) {
            timerValue = CHALLENGE_END_TIME;
            objData->extraFlags &= ~CUSTOM_FLAG_Top_Up_Timer;
        }

        D_800A7D70 = timerValue;
    }
}

/* Ensures that Act 1's important gamebits have their expected values at the start of Act 2 */
static void WCLevelControl_setAct1Finished(Object* self, WCLevelControl_Data* objData) {
    static s16 rsWCAct1Gamebits[] = {
        BIT_WC_Moon_Pressure_Switch_Active,
        BIT_WC_Moon_Beacon_Lit,
        BIT_WC_SlabDoor_Moon_Symbol_Lit,
        BIT_WC_Sun_Pressure_Switch_Active,
        BIT_WC_Sun_Beacon_Lit,
        BIT_WC_SlabDoor_Sun_Symbol_Lit,
        BIT_WC_SlabDoor_Opened,
        BIT_WC_Silver_Tooth,
        BIT_WC_Gold_Tooth,
        BIT_WC_Used_Silver_Tooth,
        BIT_WC_Used_Gold_Tooth,
        BIT_WC_Boss_Door_Opened,
        BIT_WC_King_EarthWalker_Cage_Opened,
        BIT_WC_King_EarthWalker_Rescued
    };

    u32 i;

    for (i = 0; i < ARRAYCOUNT(rsWCAct1Gamebits); i++) {
        mainSetBits(rsWCAct1Gamebits[i], TRUE);
    }

    objData->extraFlags |= CUSTOM_FLAG_Act1_Gamebits_Set;
}

/* Restore Act 1's state*/
static void WCLevelControl_setupAct1(Object* self, WCLevelControl_Data* objData) {
    //Set this flag so the TriggerPlane on the way into Walled City is what starts Walled City's music
    //TODO: maybe remove this? It's kind of cool without it: the music starts off quiet as you go through
    //the gateway in SwapStone Hollow, and then it gets louder when you reach the TriggerPlane!
    objData->extraFlags |= CUSTOM_FLAG_Swapped_Back_to_Walled_City_Music;

    //Check if Pressure Switch challenges are already finished
    if (mainGetBits(BIT_WC_SlabDoor_Opened) || 
        (mainGetBits(BIT_WC_Moon_Beacon_Lit) && mainGetBits(BIT_WC_Sun_Beacon_Lit))
    ) {
        objData->state = STATE_3_WCSlabDoor_Opened;
        return;
    }

    //Stop timed challenges if they start as the level loads
    if (mainGetBits(BIT_WC_Moon_Beacon_Lit) == FALSE && mainGetBits(BIT_WC_Moon_Pressure_Switch_Active)) {
        mainSetBits(BIT_WC_Moon_Pressure_Switch_Active, FALSE);
        mainSetBits(BIT_WC_Moon_Beacon_Raised, FALSE);
    }
    if (mainGetBits(BIT_WC_Sun_Beacon_Lit) == FALSE && mainGetBits(BIT_WC_Sun_Pressure_Switch_Active)) {
        mainSetBits(BIT_WC_Sun_Pressure_Switch_Active, FALSE);
        mainSetBits(BIT_WC_Sun_Beacon_Raised, FALSE);
    }
}

RECOMP_PATCH void WCLevelControl_obj_Setup(Object* self, ObjSetup* setup, s32 reset) {
    WCLevelControl_Data* objData;
    /* RECOMP */
    s32 act;

    objData = self->data;
    self->animCallback = WCLevelControl_animCallback;

    WCLevelControl_sunPuzzleReset();
    WCLevelControl_moonPuzzleReset();

    if (mainGetBits(BIT_WC_Moon_Beacon_Lit)) {
        objData->flags |= FLAG_8_Moon_Beacon_Lit;
    }
    if (mainGetBits(BIT_WC_Sun_Beacon_Lit)) {
        objData->flags |= FLAG_4_Sun_Beacon_Lit;
    }
    if (mainGetBits(BIT_WC_Moon_Aperture_Opened)) {
        objData->flags |= FLAG_20_Moon_Aperture_Opened;
    }
    if (mainGetBits(BIT_WC_Sun_Aperture_Opened)) {
        objData->flags |= FLAG_10_Sun_Aperture_Opened;
    }
    if (mainGetBits(BIT_WC_Sun_Temple_Maze_Solved)) {
        objData->flags |= FLAG_40_Sun_Temple_Maze_Solved;
    }

    objAddObjectType(self, OBJTYPE_LevelControl);

    mainSetBits(BIT_WC_Sun_Temple_Maze_Illusory_Wall_1_Shown, TRUE);
    mainSetBits(BIT_WC_Sun_Temple_Maze_Illusory_Wall_2_Hidden, TRUE);
    mainSetBits(BIT_WC_Sun_Temple_Maze_Illusory_Wall_3_Hidden, TRUE);
    mainSetBits(BIT_WC_Sun_Temple_Maze_Illusory_Wall_4_Hidden, TRUE);
    
    act = gDLL_29_Gplay->vtbl->get_act(self->mapID);
    if (act <= 1) {
        //@recomp: Restore Act 1's state
        WCLevelControl_setupAct1(self, objData);
    } else if ((objData->extraFlags & CUSTOM_FLAG_Act1_Gamebits_Set) == FALSE) {
        //@recomp: set Act 1's gamebits when the map loads in Act 2 or higher
        WCLevelControl_setAct1Finished(self, objData);
    }

    //@recomp: initialise musicPlayerNo
    objData->musicPlayerNo = -1;
}

/**
  * - Fix a bug where you suddenly get blinded by intense fog when visiting Walled City (originally by MusicalProgrammer)
  * TODO: An alternative fix could be to edit the EnvFxAction that has the bugged fog distances (0x149)
  *
  * - Set Act 1's gamebits when the level's in Act 2.
  */
RECOMP_PATCH void WCLevelControl_obj_Control(Object *self) {
    WCLevelControl_Data *objData = self->data;
    f32 time;
    u8 act;

    if (self->unkDC == 0) {
        envfxAction(self, self, 0x1FB, 0);
        envfxAction(self, self, 0x1FC, 0);
        // envfxAction(self, self, 0x149, 0); //@recomp: stop blinding fog
        lfxAction(self, self, 0x97, 0, 0, 0);
        lfxAction(self, self, 0x24F, 0, 0, 0);
        self->unkDC = 1;
    }

    act = gDLL_29_Gplay->vtbl->get_act(self->mapID);
    if ((act == 1) || (act != 2)) {
        WCLevelControl_handleAct1(self, objData);
    } else {
        //@recomp: set important Act 1 gamebits, just in case
        if ((objData->extraFlags & CUSTOM_FLAG_Act1_Gamebits_Set) == FALSE) {
            WCLevelControl_setAct1Finished(self, objData);
        }

        WCLevelControl_handleAct2(self, objData);
    }

    //Check if night-time
    if (gDLL_7_Newday->vtbl->func8(&time)) {
        mainSetBits(BIT_7F3, 1);
        mainSetBits(BIT_7F1, 0);
    } else {
        mainSetBits(BIT_7F3, 0);
        mainSetBits(BIT_7F1, 1);
    }
}

RECOMP_PATCH void WCLevelControl_handleAct1(Object* self, WCLevelControl_Data* objData) {
    /* RECOMP */
    _Bool switchesQOL = configs_GetWCPressureSwitchQOL();

    if (objData->flags & FLAG_2_Pressure_Switch_Challenge_Started) {
        return;
    }

    objData->previousState = objData->state;

#ifdef DEBUG_ACT1
    diPrintf("WCLevelControl: handling act1. State: %d\n", objData->state);

    diPrintf("MOON)\t[Switch: %d] [Beacon | Raised: %d, Lit: %d]\n", 
        mainGetBits(BIT_WC_Moon_Pressure_Switch_Active),
        mainGetBits(BIT_WC_Moon_Beacon_Raised),
        mainGetBits(BIT_WC_Moon_Beacon_Lit)
    );

    diPrintf("SUN)\t[Switch: %d] [Beacon | Raised: %d, Lit: %d]\n", 
        mainGetBits(BIT_WC_Sun_Pressure_Switch_Active),
        mainGetBits(BIT_WC_Sun_Beacon_Raised),
        mainGetBits(BIT_WC_Sun_Beacon_Lit)
    );

    #ifdef DEBUG_MUSIC
    diPrintf("musicPlayerNo: %d\n", objData->musicPlayerNo);
    #endif

    diPrintf("extraFlags: %d %d %d %d %d %d %d %d %d\n", 
        (objData->extraFlags & CUSTOM_FLAG_Played_Walled_City_Music_While_Idling_on_Switch) != 0,
        (objData->extraFlags & CUSTOM_FLAG_Swapped_Back_to_Walled_City_Music) != 0,
        (objData->extraFlags & CUSTOM_FLAG_Stop_Challenge_Music) != 0,
        (objData->extraFlags & CUSTOM_FLAG_Challenge_Music_Fully_Faded_Out) != 0,
        (objData->extraFlags & CUSTOM_FLAG_Challenge_Music_Too_Far_Ahead) != 0,
        (objData->extraFlags & CUSTOM_FLAG_Challenge_Music_Started) != 0,
        (objData->extraFlags & CUSTOM_FLAG_Player_on_Pressure_Switch) != 0,
        (objData->extraFlags & CUSTOM_FLAG_Top_Up_Timer) != 0,
        (objData->extraFlags & CUSTOM_FLAG_Act1_Gamebits_Set) != 0
    );
#endif

    switch (objData->state) {
    case STATE_1_Sun_Beacon_Timed_Challenge:
        if (objData->flags & FLAG_1_Entered_ObjSeq) {
            //Start a 60 second timed challenge
            menu_func_8000F64C(0x11, PRESSURE_SWITCH_CHALLENGE_SECONDS);
            menu_func_8000F6CC();
            WCLevelControl_refillTimer(self);
        } else if (mainGetBits(BIT_WC_Sun_Beacon_Lit)) {
            //Success! Play a sequence when the player lights the beacon
            objData->flags |= FLAG_4_Sun_Beacon_Lit;
            menu_func_8000FAC8();

            //Play a different sequence (WCSlabDoor opening) if both beacons are now lit
            if (mainGetBits(BIT_WC_Moon_Beacon_Lit)) {
                gDLL_3_Animation->vtbl->start_obj_sequence(0, self, -1);
                objData->state = STATE_3_WCSlabDoor_Opened;
            } else {
                gDLL_3_Animation->vtbl->start_obj_sequence(1, self, -1);
                objData->state = STATE_0_Idle;
            }

            //@recomp: stop challenge music immediately
            objData->extraFlags |= CUSTOM_FLAG_Stop_Challenge_Music;
        } else if (menu_func_8000FB1C()) { 
            //Failure... reset the gamebits and revert state
            mainSetBits(BIT_WC_Sun_Beacon_Raised, FALSE);
            mainSetBits(BIT_WC_Sun_Pressure_Switch_Active, FALSE);
            objData->state = STATE_0_Idle;
        } else if (switchesQOL && mainGetBits(BIT_WC_Moon_Pressure_Switch_Active) && !mainGetBits(BIT_WC_Moon_Beacon_Lit)) { 
            //@recomp: allow swapping to the other puzzle (standing on Moon switch while Sun challenge is active)
            mainSetBits(BIT_WC_Sun_Beacon_Raised, FALSE);
            mainSetBits(BIT_WC_Sun_Pressure_Switch_Active, FALSE);
            objData->state = STATE_0_Idle;

            //@recomp: Clear timer, for swapping between moon/sun challenges
            menu_func_8000FAC8();
        } else if (switchesQOL && mainGetBits(DINOMOD_BIT_92F_WC_Player_on_Sun_Pressure_Switch)) {
            //@recomp: Refill the timer while the player's still standing on it
            objData->extraFlags |= (CUSTOM_FLAG_Top_Up_Timer | CUSTOM_FLAG_Player_on_Pressure_Switch);
        } else {
            objData->extraFlags &= ~CUSTOM_FLAG_Player_on_Pressure_Switch;
        }
        break;
    case STATE_2_Moon_Beacon_Timed_Challenge:
        if (objData->flags & FLAG_1_Entered_ObjSeq) {
            //Start a 60 second timed challenge
            menu_func_8000F64C(0x11, PRESSURE_SWITCH_CHALLENGE_SECONDS);
            menu_func_8000F6CC();
            WCLevelControl_refillTimer(self);
        } else if (mainGetBits(BIT_WC_Moon_Beacon_Lit)) {
            //Success! Play a sequence when the player lights the beacon
            objData->flags |= FLAG_8_Moon_Beacon_Lit;
            menu_func_8000FAC8();

            //Play a different sequence (WCSlabDoor opening) if both beacons are now lit
            if (mainGetBits(BIT_WC_Sun_Beacon_Lit)) {
                gDLL_3_Animation->vtbl->start_obj_sequence(0, self, -1);
                objData->state = STATE_3_WCSlabDoor_Opened;
            } else {
                gDLL_3_Animation->vtbl->start_obj_sequence(1, self, -1);
                objData->state = STATE_0_Idle;
            }

            //@recomp: stop challenge music immediately
            objData->extraFlags |= CUSTOM_FLAG_Stop_Challenge_Music;
        } else if (menu_func_8000FB1C()) {
            //Failure... reset the gamebits and revert state
            mainSetBits(BIT_WC_Moon_Beacon_Raised, FALSE);
            mainSetBits(BIT_WC_Moon_Pressure_Switch_Active, FALSE);
            objData->state = STATE_0_Idle;
        } else if (switchesQOL && mainGetBits(BIT_WC_Sun_Pressure_Switch_Active) && !mainGetBits(BIT_WC_Sun_Beacon_Lit)) {
            //@recomp: allow swapping to the other puzzle (standing on Sun switch while Moon challenge is active)
            mainSetBits(BIT_WC_Moon_Beacon_Raised, FALSE);
            mainSetBits(BIT_WC_Moon_Pressure_Switch_Active, FALSE);
            objData->state = STATE_0_Idle;

            //@recomp: Clear timer, for swapping between moon/sun challenges
            menu_func_8000FAC8();
        } else if (switchesQOL && mainGetBits(DINOMOD_BIT_92E_WC_Player_on_Moon_Pressure_Switch)) {
            //@recomp: Refill the timer while the player's still standing on it
            objData->extraFlags |= (CUSTOM_FLAG_Top_Up_Timer | CUSTOM_FLAG_Player_on_Pressure_Switch);
        } else {
            objData->extraFlags &= ~CUSTOM_FLAG_Player_on_Pressure_Switch;
        }
        break;
    case STATE_3_WCSlabDoor_Opened:
        break;
    default:
        //Start a timed challenge when one of the pressure switches is pressed
        if (!(objData->flags & FLAG_4_Sun_Beacon_Lit) && mainGetBits(BIT_WC_Sun_Pressure_Switch_Active)) {
            //Sun switch
            mainSetBits(BIT_WC_Sun_Beacon_Raised, TRUE);
            objData->state = STATE_1_Sun_Beacon_Timed_Challenge;
            objData->flags |= FLAG_2_Pressure_Switch_Challenge_Started;
            objData->timer = 70.0f;
            objData->extraFlags &= ~MUSIC_FLAGS;
            objData->extraFlags |= CUSTOM_FLAG_Stop_Challenge_Music; //If it's currently playing
        } else if (!(objData->flags & FLAG_8_Moon_Beacon_Lit) && mainGetBits(BIT_WC_Moon_Pressure_Switch_Active)) {
            //Moon switch
            mainSetBits(BIT_WC_Moon_Beacon_Raised, TRUE);
            objData->state = STATE_2_Moon_Beacon_Timed_Challenge;
            objData->flags |= FLAG_2_Pressure_Switch_Challenge_Started;
            objData->timer = 70.0f;
            objData->extraFlags &= ~MUSIC_FLAGS;
            objData->extraFlags |= CUSTOM_FLAG_Stop_Challenge_Music; //If it's currently playing
        }
        break;
    }

    //@recomp: Handle music
    if (objData->state < STATE_3_WCSlabDoor_Opened || (objData->extraFlags & CUSTOM_FLAG_Swapped_Back_to_Walled_City_Music) == FALSE) {
        #define TIMED_CHALLENGE_ACTIVE (objData->state == STATE_1_Sun_Beacon_Timed_Challenge || objData->state == STATE_2_Moon_Beacon_Timed_Challenge)
        #define BASE_VOL 40
        #define MAX_DESYNC_SECONDS 5

        s32 volume;

        //Handle stopping the challenge music immediately (when challenge completed successfully, or when swapping between challenges)
        if (objData->extraFlags & CUSTOM_FLAG_Stop_Challenge_Music) {
            if (objData->musicPlayerNo >= 0) {
                gDLL_5_AMSEQ2->vtbl->stop(objData->musicPlayerNo);
                objData->musicPlayerNo = -1;
            }
            objData->extraFlags &= ~CUSTOM_FLAG_Stop_Challenge_Music;
        }

        //Check if the challenge music has stopped playing
        if (objData->musicPlayerNo >= 0) {
            if (AMSEQ_hasMusicPlayerStopped(objData->musicPlayerNo)) {
                objData->musicPlayerNo = -1;
            }
        }

        //Start the timed challenge music
        if (TIMED_CHALLENGE_ACTIVE && 
            (objData->extraFlags & CUSTOM_FLAG_Challenge_Music_Started) == FALSE && 
            (objData->musicPlayerNo < 0)
        ) {
            objData->musicPlayerNo = gDLL_5_AMSEQ2->vtbl->set(NULL, 0x106, 0, 0, 0);
            objData->challengeMusicTime = 0;
            objData->musicFadeTimer = 0;
            objData->extraFlags |= CUSTOM_FLAG_Challenge_Music_Started;
        }

        //Start Walled City's music (when idling on the switch)
        else if (TIMED_CHALLENGE_ACTIVE && 
            (objData->extraFlags & CUSTOM_FLAG_Played_Walled_City_Music_While_Idling_on_Switch) == FALSE &&
            (objData->extraFlags & CUSTOM_FLAG_Challenge_Music_Too_Far_Ahead) &&
            (objData->extraFlags & CUSTOM_FLAG_Player_on_Pressure_Switch) &&
            (objData->extraFlags & CUSTOM_FLAG_Challenge_Music_Fully_Faded_Out)
        ) {
            gDLL_5_AMSEQ2->vtbl->set(NULL, 0x104, 0, 0, 0);
            objData->extraFlags |= CUSTOM_FLAG_Played_Walled_City_Music_While_Idling_on_Switch;
        }

        //Start Walled City's music (when timed challenge is over)
        else if (TIMED_CHALLENGE_ACTIVE == FALSE && 
            (objData->extraFlags & CUSTOM_FLAG_Swapped_Back_to_Walled_City_Music) == FALSE &&
            (objData->musicPlayerNo < 0)
        ) {
            gDLL_5_AMSEQ2->vtbl->set(NULL, 0x104, 0, 0, 0);
            objData->extraFlags |= CUSTOM_FLAG_Swapped_Back_to_Walled_City_Music;
        }

        //Check if the challenge music is too far ahead of the timer, when topping up the timer
        else if (TIMED_CHALLENGE_ACTIVE &&
            (objData->extraFlags & CUSTOM_FLAG_Challenge_Music_Too_Far_Ahead) == FALSE &&
            (objData->musicPlayerNo >= 0) &&
            (objData->extraFlags & CUSTOM_FLAG_Top_Up_Timer) &&
            (objData->challengeMusicTime > (MAX_DESYNC_SECONDS * 60))
        ) {
            objData->extraFlags |= CUSTOM_FLAG_Challenge_Music_Too_Far_Ahead;
        }

        //Handle fading out the challenge music (when idling on the switch too long) and restarting it when the player leaves the switch
        if ((objData->extraFlags & CUSTOM_FLAG_Challenge_Music_Too_Far_Ahead) &&
            (objData->musicPlayerNo >= 0)
        ) {
            objData->musicFadeTimer += gUpdateRate;

            //Set volume
            if ((objData->extraFlags & CUSTOM_FLAG_Challenge_Music_Fully_Faded_Out) == FALSE) {
                volume = BASE_VOL - (objData->musicFadeTimer/4);
                if (volume > BASE_VOL) {
                    volume = BASE_VOL;
                }  else if (volume <= 0) {
                    volume = 0;
                    objData->extraFlags |= CUSTOM_FLAG_Challenge_Music_Fully_Faded_Out;
                }
                gDLL_5_AMSEQ2->vtbl->set_volume(objData->musicPlayerNo, volume);
            }

            //Restart the challenge music when the music's fully faded out and the player's left the switch
            if ((objData->extraFlags & CUSTOM_FLAG_Challenge_Music_Fully_Faded_Out) && 
                (objData->extraFlags & CUSTOM_FLAG_Player_on_Pressure_Switch) == FALSE
            ) {
                gDLL_5_AMSEQ2->vtbl->stop(objData->musicPlayerNo);
                objData->musicPlayerNo = -1;
                objData->extraFlags &= ~MUSIC_FLAGS;
            }
        }

        //Keep track of how long the challenge music's been playing
        if (objData->musicPlayerNo >= 0) {
            objData->challengeMusicTime += gUpdateRate;
            if (objData->challengeMusicTime > PRESSURE_SWITCH_CHALLENGE_SECONDS * 60) {
                objData->challengeMusicTime = PRESSURE_SWITCH_CHALLENGE_SECONDS * 60;
            }
        }
    }

    //@recomp: top up the challenge timer if the player is still on the switch
    {
        if (!switchesQOL || (objData->state != STATE_1_Sun_Beacon_Timed_Challenge && objData->state != STATE_2_Moon_Beacon_Timed_Challenge)) {
            objData->extraFlags &= ~CUSTOM_FLAG_Top_Up_Timer;
        }
        if (objData->extraFlags & CUSTOM_FLAG_Top_Up_Timer) {
            WCLevelControl_refillTimer(self);
        }
    }

    objData->flags &= ~FLAG_1_Entered_ObjSeq;
}

/* Play "puzzle solved" sound when successfully completing the Sun Temple maze */
RECOMP_PATCH void WCLevelControl_handleAct2(Object* self, WCLevelControl_Data* objData) {
    u8 isNightTime;
    u8 pushblocksPlaced;
    f32 time;

    isNightTime = gDLL_7_Newday->vtbl->func8(&time);

    switch (objData->state) {
        case STATE_6_Sun_Maze_Setup:
            //Start a 60 second timed challenge
            gDLL_5_AMSEQ2->vtbl->set(NULL, 0x106, 0, 0, 0);
            menu_func_8000F64C(0x11, 60);
            menu_func_8000F6CC();
            objData->state = STATE_4_Sun_Maze_Timed_Challenge;
            break;
        case STATE_4_Sun_Maze_Timed_Challenge:
            //Check if the timer ended
            if (menu_func_8000FB1C()) {
                gDLL_5_AMSEQ2->vtbl->set(NULL, 0x104, 0, 0, 0);
                if (mainGetBits(BIT_WC_Sun_Temple_Maze_Solved)) {
                    //Success!
                    objData->flags |= FLAG_40_Sun_Temple_Maze_Solved;

                    //@recomp: Play success jingle
                    dll_amSfx->Play(self, SOUND_798_Puzzle_Solved, VOLUME_PERCENT(33), NULL, NULL, 0, NULL);
                } else {
                    //Failure...
                    mainSetBits(BIT_WC_Sun_Temple_Maze_Timed_Challenge_Door_Opened, FALSE);
                    mainSetBits(BIT_WC_Sun_Temple_Maze_Timed_Challenge_Switch_Pressed, FALSE);
                    mainSetBits(BIT_WC_Sun_Temple_Maze_Illusory_Wall_1_Shown, TRUE);
                    mainSetBits(BIT_WC_Sun_Temple_Maze_Illusory_Wall_2_Hidden, TRUE);
                    mainSetBits(BIT_WC_Sun_Temple_Maze_Illusory_Wall_3_Hidden, TRUE);
                    mainSetBits(BIT_WC_Sun_Temple_Maze_Illusory_Wall_4_Hidden, TRUE);
                }
                objData->state = STATE_0_Idle;
            } else {
                //@recomp: check if the player reached the goal, and end the timer early
                if (mainGetBits(BIT_WC_Sun_Temple_Maze_Solved)) {
                    //Success!
                    objData->flags |= FLAG_40_Sun_Temple_Maze_Solved;
                    objData->state = STATE_0_Idle;

                    //Clear the timer
                    menu_func_8000FAC8();

                    //Play success jingle
                    dll_amSfx->Play(self, SOUND_798_Puzzle_Solved, VOLUME_PERCENT(33), NULL, NULL, 0, NULL);

                    //Switch back to Walled City music
                    gDLL_5_AMSEQ2->vtbl->set(NULL, 0x104, 0, 0, 0);
                }
            }
            break;
        default:
            if ((objData->flags & FLAG_40_Sun_Temple_Maze_Solved) == FALSE) {
                //Start the maze's timed challenge when the wall switch is pressed (just to the left when entering)
                if (mainGetBits(BIT_WC_Sun_Temple_Maze_Timed_Challenge_Switch_Pressed) && 
                    mainGetBits(BIT_WC_Played_Seq_179_Sun_Temple_Maze_Timed_Challenge_Intro)
                ) {
                    mainSetBits(BIT_WC_Sun_Temple_Maze_Illusory_Wall_1_Shown, FALSE);
                    mainSetBits(BIT_WC_Sun_Temple_Maze_Illusory_Wall_2_Hidden, FALSE);
                    mainSetBits(BIT_WC_Sun_Temple_Maze_Illusory_Wall_3_Hidden, FALSE);
                    mainSetBits(BIT_WC_Sun_Temple_Maze_Illusory_Wall_4_Hidden, FALSE);
                    mainSetBits(BIT_WC_Sun_Temple_Maze_Timed_Challenge_Door_Opened, TRUE);
                    objData->state = STATE_6_Sun_Maze_Setup;
                }
            }
            break;
    }

    //Handle the Sun Pushblock Puzzle
    if ((objData->flags & FLAG_10_Sun_Aperture_Opened) == FALSE) {
        pushblocksPlaced = mainGetBits(BIT_WC_Sun_Pushblock_Puzzle_Progress);
        if (pushblocksPlaced == 4) {
            mainSetBits(BIT_WC_Sun_Aperture_Opened,  TRUE);
            objData->flags |= FLAG_10_Sun_Aperture_Opened;
        } else if (isNightTime || mainGetBits(BIT_WC_Sun_Pushblock_Puzzle_Reset)) {
            WCLevelControl_sunPuzzleReset();
        }
    }

    //Handle the Moon Pushblock Puzzle
    if ((objData->flags & FLAG_20_Moon_Aperture_Opened) == FALSE) {
        pushblocksPlaced = mainGetBits(BIT_WC_Moon_Pushblock_Puzzle_Progress);
        if (pushblocksPlaced == 4) {
            mainSetBits(BIT_WC_Moon_Aperture_Opened, TRUE);
            objData->flags |= FLAG_20_Moon_Aperture_Opened;
        } else if (!isNightTime || mainGetBits(BIT_WC_Moon_Pushblock_Puzzle_Reset)) {
            WCLevelControl_moonPuzzleReset();
        }
    }

    objData->flags &= ~FLAG_1_Entered_ObjSeq;
}

/* Free musicPlayerNo */
RECOMP_PATCH void WCLevelControl_obj_Free(Object* self, s32 onlySelf) {
    /* RECOMP */
    WCLevelControl_Data* objData = self->data;
    
    objFreeObjectType(self, OBJTYPE_LevelControl);

    //@recomp: stop challenge music
    if (objData->musicPlayerNo >= 0) {
        gDLL_5_AMSEQ2->vtbl->stop(objData->musicPlayerNo);
        objData->musicPlayerNo = -1;
    }

    //@recomp: stop challenge timer
    switch (objData->state) {
    case STATE_1_Sun_Beacon_Timed_Challenge:
    case STATE_2_Moon_Beacon_Timed_Challenge:
    //TODO: Act 2's maze challenge timer states
        menu_func_8000FAC8();
        break;
    }
}

/* Extend objData */
RECOMP_PATCH u32 WCLevelControl_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(WCLevelControl_Data);
}
