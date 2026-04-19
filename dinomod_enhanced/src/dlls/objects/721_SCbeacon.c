#include "modding.h"
#include "recomputils.h"
#include "player_util.h"

#include "objects/227_tumbleweed.h"

#include "common.h"
#include "PR/ultratypes.h"
#include "sys/gfx/modgfx.h"
#include "game/objects/interaction_arrow.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/common/sidekick.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/227_tumbleweed.h"
// #include "dlls/objects/721_SCbeacon.h"

#include "recomp/dlls/objects/227_tumbleweed_recomp.h"
// #include "recomp/dlls/objects/721_SCbeacon_recomp.h"

#include "recomp/dlls/_asm/721_recomp.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"

typedef struct {
    ObjSetup base;
    u8 playerRange;
    u8 unused19;
    u16 kyteCurveID;
    s8 unused1C;
    s8 unused1D;
    s8 unused1E;
    u8 yaw;
} SCbeacon_Setup;

typedef struct {
    u8 state;
    u8 flags;                   //Tracks twigs being added to bowl, and LightAction use (when lit)
    u8 prevFlags;               //Value of flags on previous flame, for comparison
    s32 seqIdxPlaceTwigs;       //When carrying Tumbleweed near beacon (@bug: doesn't play?)
    s32 seqIdxLightTwigs;       //When Flame used
    CurveSetup* kyteCurve;      
    s16 gamebitTwigs;           //Set when twigs placed in bowl
    s16 gamebitLit;             //Set when beacon lit
    Object* unk14;              //Set to 0 in setup, but otherwise unused
    //RECOMP
    u8 prevState;               //`state` on previous tick, for comparison
    u8 playSuccess;             //Flag a sound to play when beacon lit (handled this way to avoid sound call getting "eaten"?)
    u8 destroyTumbleweed;       //Flag that the player's held Tumbleweed should be deleted
    Object* heldTumbleweed;     //The Tumbleweed Object held by the player
    u32 soundHandleBurn1;
    u32 soundHandleBurn2;
} SCbeacon_Data_Extended;

typedef enum {
    SCbeacon_STATE_Initial = 0,
    SCbeacon_STATE_Bowl_Empty = 1,
    SCbeacon_STATE_Twigs_in_Bowl = 2,
    SCbeacon_STATE_Lighting = 3,
    SCbeacon_STATE_Lit = 4
} SCbeacon_States;

typedef enum {
    SCbeacon_FLAG_0 = 0,
    SCbeacon_FLAG_Add_Tumbleweed = 1,
    SCbeacon_FLAG_Emit_Light = 2
} SCbeacon_Flags;

typedef enum {
    SCbeacon_MODEL_Bowl_Empty = 0,
    SCbeacon_MODEL_Twigs_in_Bowl = 1
} SCbeacon_ModelIndices;

typedef enum {
    SCbeacon_SEQIDX_Placing_Twigs = 0, //doesn't play?
    SCbeacon_SEQIDX_Lighting_Twigs = 1
} SCbeacon_SeqIndices;

typedef enum {
    SCbeacon_Near_Golden_Plains = 0x28CE,
    SCbeacon_Near_Pond_with_Pole = 0x28CF,
    SCbeacon_Near_Discovery_Falls = 0x28D0
} SCbeacon_UIDs;

extern void dll_721_func_814(Object* self);

static void SCbeacon_flame_sounds_start(Object* self){
    SCbeacon_Data_Extended* objData;
    if (!self){
        return;
    }

    objData = self->data;
    if (!objData){
        return;
    }

    if (!objData->soundHandleBurn1) {
        gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_50a_Fire_Burning_Low_Loop, MAX_VOLUME, &objData->soundHandleBurn1, NULL, 0, NULL);
    }
    if (!objData->soundHandleBurn2) {
        gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_50b_Fire_Burning_High_Loop, MAX_VOLUME, &objData->soundHandleBurn2, NULL, 0, NULL);
    }
}

static void SCbeacon_flame_sounds_stop(Object* self){
    SCbeacon_Data_Extended* objData;
    if (!self){
        return;
    }

    objData = self->data;
    if (!objData){
        return;
    }

    if (objData->soundHandleBurn1) {
        gDLL_6_AMSFX->vtbl->func_A1C(objData->soundHandleBurn1);
        objData->soundHandleBurn1 = 0;
    }
    if (objData->soundHandleBurn2) {
        gDLL_6_AMSFX->vtbl->func_A1C(objData->soundHandleBurn2);
        objData->soundHandleBurn2 = 0;
    }
}

static void SCbeacon_handle_flame_sounds(Object* self, SCbeacon_Data_Extended* objData, u32 playerDistSQ, u32 interactRangeSQ) {
    //Do nothing if the beacon isn't lit (or it's the first tick after lighting)
    if (!(objData->state == SCbeacon_STATE_Lit && objData->prevState == SCbeacon_STATE_Lit)){
        return;
    }
    
    //Stop flame sounds when at a distance (TO-DO: figure out why this sometimes continues playing anyway!)
    if (objData->soundHandleBurn1 || objData->soundHandleBurn2){
        if (playerDistSQ > interactRangeSQ*16){ //stop sounds at 4x interact radius
            SCbeacon_flame_sounds_stop(self);
        }
    }
    //Restart flame sounds when nearby
    if (!(objData->soundHandleBurn1 && objData->soundHandleBurn2)){
        if (playerDistSQ < interactRangeSQ*4){ //start sounds at 2x interact radius
            SCbeacon_flame_sounds_start(self);
        }
    }
}

/**
  * Fix ability to place Tumbleweeds into the beacon bowl, and add some polish.
  *
  * Stop/restart flame sounds based on player distance.
  */
RECOMP_PATCH void dll_721_control(Object* self) {
    SCbeacon_Data_Extended* objData;
    SCbeacon_Setup* objSetup;
    Object* sidekick;
    Object* player;
    u32 playerDistSQ;
    u32 interactRangeSQ;
    u8 playerIsNearby;

    objData = self->data;
    objSetup = (SCbeacon_Setup*)self->setup;
    player = get_player();

    diPrintf("soundHandleBurn1: %d\n", objData->soundHandleBurn1);
    diPrintf("soundHandleBurn2: %d\n", objData->soundHandleBurn2);
    
    playerDistSQ = vec3_distance_xz_squared(&player->positionMirror, &self->positionMirror);
    interactRangeSQ = SQ(objSetup->playerRange);
    playerIsNearby = (playerDistSQ <= interactRangeSQ);
    
    objData->flags &= ~SCbeacon_FLAG_Emit_Light;

    //Add a Tumbleweed to the bowl when flagged
    if (objData->flags & SCbeacon_FLAG_Add_Tumbleweed) {
        objData->state = SCbeacon_STATE_Twigs_in_Bowl;
        self->modelInstIdx = SCbeacon_MODEL_Twigs_in_Bowl;
        main_set_bits(objData->gamebitTwigs, 1);
        objData->flags &= ~SCbeacon_FLAG_Add_Tumbleweed;
    }

    switch (objData->state) {
    case SCbeacon_STATE_Initial:
        //Restore state based on gamebits
        self->modelInstIdx = SCbeacon_MODEL_Bowl_Empty;
        objData->state = SCbeacon_STATE_Bowl_Empty;

        if (main_get_bits(objData->gamebitTwigs)) {
            self->modelInstIdx = SCbeacon_MODEL_Twigs_in_Bowl;
            objData->state = SCbeacon_STATE_Twigs_in_Bowl;
        }

        if (main_get_bits(objData->gamebitLit)) {
            dll_721_func_814(self); //will only be successful here if twigs gamebit also set
        }
        break;
    case SCbeacon_STATE_Bowl_Empty:

        sidekick = get_sidekick();
        if (sidekick && (self->unkAF & ARROW_FLAG_4_Highlighted)) {
            //Show Flame command option
            ((DLL_ISidekick*)sidekick->dll)->vtbl->func14(sidekick, 4);

            //Check if Flame command was selected
            if (gDLL_1_UI->vtbl->func_DF4(4)) {
                main_set_bits(BIT_Kyte_Flight_Curve, objSetup->kyteCurveID);
            }
        }

        //@recomp: use a different function to get the ID of the player's held object
        {
            Object* heldObject;
            Player_Data* playerData;

            //Do nothing if the player is far away or holding nothing
            if (!playerIsNearby || (((DLL_210_Player*)player->dll)->vtbl->func10(player, &heldObject) == 0)){
                break;
            }

            //Play a depositing sequence when the player is holding a Tumbleweed
            if (heldObject && heldObject->id == OBJ_Tumbleweed1twig){
                objData->state = SCbeacon_STATE_Twigs_in_Bowl;
                objData->heldTumbleweed = heldObject;
                objData->destroyTumbleweed = TRUE;
                tumbleweed_set_silent_delete(objData->heldTumbleweed, TRUE);
                tumbleweed_stop_being_carried(objData->heldTumbleweed);
                recomp_eprintf("PLACING: setting up silent delete!\n");

                //Play deposit sequence
                gDLL_3_Animation->vtbl->func17(objData->seqIdxPlaceTwigs, self, -1);
            }
        }
        break;
    case SCbeacon_STATE_Twigs_in_Bowl:
        //@recomp: destroy the Tumbleweed that was being held
        if (objData->destroyTumbleweed) {
            recomp_eprintf("IN BOWL: destroyTumbleweed? %d\n", objData->destroyTumbleweed);

            if (objData->heldTumbleweed && !(objData->heldTumbleweed->unkB0 & 0x40)){
                recomp_eprintf("IN BOWL: deleting Tumbleweed!\n");
                obj_destroy_object(objData->heldTumbleweed);
            }
            objData->destroyTumbleweed = FALSE;

            recomp_eprintf("IN BOWL: stop player carry!\n");
            playerUtil_stop_carrying(player);
        }

        sidekick = get_sidekick();
        if (sidekick && (self->unkAF & ARROW_FLAG_4_Highlighted)) {
            //Show Flame command option
            ((DLL_ISidekick*)sidekick->dll)->vtbl->func14(sidekick, 4);

            //Check if Flame command was selected
            if (gDLL_1_UI->vtbl->func_DF4(4)) {
                main_set_bits(BIT_Kyte_Flight_Curve, objSetup->kyteCurveID);
            }
        }
        break;
    case SCbeacon_STATE_Lighting:
        sidekick = get_sidekick();
        if (vec3_distance_xz_squared(&sidekick->positionMirror, &self->positionMirror) <= 2500.0f) {
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_425, NULL, 2, -1, NULL); //create smoke
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_426, NULL, 2, -1, NULL); //create embers
        }
        break;
    case SCbeacon_STATE_Lit:
        if (playerIsNearby) {
            objData->flags |= SCbeacon_FLAG_Emit_Light;
        }
        
        //Use LightActions to emit a warm glow (or remove it)
        if ((objData->flags & SCbeacon_FLAG_Emit_Light) && !(objData->prevFlags & SCbeacon_FLAG_Emit_Light)) {
            func_80000450(self, self, 0x59, 0, 0, 0);
        } else if (!(objData->flags & SCbeacon_FLAG_Emit_Light) && (objData->prevFlags & SCbeacon_FLAG_Emit_Light)) {
            func_80000450(self, self, 0x5A, 0, 0, 0);
        }

        //Create smoke
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_425, NULL, 2, -1, NULL);
        break;
    }

    //@recomp: Play refusal sound after trying to light beacon without adding kindling
    if ((objData->prevState == SCbeacon_STATE_Lighting) && (objData->state == SCbeacon_STATE_Bowl_Empty)){
        gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_6E6_Interaction_Refused, MAX_VOLUME, NULL, NULL, 0, NULL);
    //Play success sound when lit
    } else if (objData->playSuccess && (objData->prevState == SCbeacon_STATE_Lit) && (objData->state == SCbeacon_STATE_Lit)){
        gDLL_6_AMSFX->vtbl->play_sound(player, 0xB01, 0x60, NULL, NULL, 0, NULL);
        objData->playSuccess = FALSE;
    }

    SCbeacon_handle_flame_sounds(self, objData, playerDistSQ, interactRangeSQ);
    
    objData->prevFlags = objData->flags;
    //@recomp: store previous state as well
    objData->prevState = objData->state;
}

/** Stop sounds, free soundHandles */
RECOMP_PATCH void dll_721_free(Object* self, s32 arg1) {
    gDLL_14_Modgfx->vtbl->func5(self);
    gDLL_13_Expgfx->vtbl->func5(self);
    obj_free_object_type(self, OBJTYPE_48);

    //@recomp: soundHandles
    SCbeacon_flame_sounds_stop(self);
}

typedef enum {
    BIT_SC_Beacon_Lit_1 = 0x82,
    BIT_SC_Beacon_Lit_2 = 0x83,
    BIT_SC_Beacon_Lit_3 = 0x84,
    BIT_SC_All_Beacons_Lit = 0x2D7, // len:1 group:2
} BeaconBits;

/** Play success jingle when puzzle complete */
RECOMP_PATCH int dll_721_func_5FC(Object* self, s32 finishLighting) {
    SCbeacon_Data_Extended* objData = self->data;
    s32 isBeingLit = FALSE;
    
    if (finishLighting == FALSE) {
        //Play the sequence where Kyte lights the beacon, and advance to "lighting" state (embers/smoke)
        gDLL_3_Animation->vtbl->func17(objData->seqIdxLightTwigs, self, -1);
        objData->state = SCbeacon_STATE_Lighting;
        isBeingLit = TRUE;

    } else if (finishLighting == TRUE) {
        //Check if the beacon is being lit (emitting embers/smoke while Kyte is using Flame on it)
        if (objData->state == SCbeacon_STATE_Lighting) {
            isBeingLit = TRUE;
        }

        //Finish lighting the beacon
        if (main_get_bits(objData->gamebitTwigs) && (main_get_bits(objData->gamebitLit) == FALSE)) {
            //Advance to "lit" state
            dll_721_func_814(self); //will always be successful here

            //Set up success sound
            objData->playSuccess = TRUE;

            //Warp to Discovery Falls' entrance when all beacons are lit, and play pool-draining sequence
            if (main_get_bits(BIT_SC_Beacon_Lit_1) && 
                main_get_bits(BIT_SC_Beacon_Lit_2) && 
                main_get_bits(BIT_SC_Beacon_Lit_3)
            ) {
                main_set_bits(BIT_SC_All_Beacons_Lit, 1);

                //@recomp: play success jingle
                gDLL_6_AMSFX->vtbl->play_sound(NULL, SOUND_B89_Puzzle_Solved, 0x60, NULL, NULL, 0, NULL);

                gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_SWAPSTONE_CIRCLE, 0, TRUE);
                warpPlayer(85, TRUE); //SC_warppoint auto-plays seq 0x77
                gDLL_29_Gplay->vtbl->set_obj_group_status(MAP_SWAPSTONE_CIRCLE, 3, FALSE);
            }
        } else {
            objData->state = SCbeacon_STATE_Bowl_Empty;
        }
    }
    
    return isBeingLit;
}

/** Use soundHandles when playing flame loops */
RECOMP_PATCH void dll_721_func_814(Object* self) {
    s32 pad;
    SCbeacon_Setup* objSetup;
    DLL_IModgfx* modGfxDLL;
    CurveSetup* curveSetup;
    SCbeacon_Data_Extended* objData;

    objData = self->data;
    objSetup = (SCbeacon_Setup*)self->setup;
    
    if (main_get_bits(objData->gamebitTwigs)) {
        //Play burning sound loops (@recomp: use soundHandles)
        // SCbeacon_flame_sounds_start(self);
        
        //Create fire model
        gDLL_14_Modgfx->vtbl->func10(self);
        modGfxDLL = dll_load_deferred(0x100A, 1);
        modGfxDLL->vtbl->func0(self, 2, 0, 0x10004, -1, 0);
        dll_unload(modGfxDLL);
        
        //Disable targetting and advance to lit state
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
        main_set_bits(objData->gamebitLit, 1);        
        objData->state = SCbeacon_STATE_Lit;

        //Set Kyte curve gamebit
        objData->kyteCurve = gDLL_25->vtbl->func_2BC4(self, objSetup->kyteCurveID);
        if (objData->kyteCurve->type22.usedBit != NO_GAMEBIT) {
            main_set_bits(objData->kyteCurve->type22.usedBit, 1);
        }
    }
}

/**
  * Fix a bug where Tumbleweeds would suddenly appear in the beacon bowl when Kyte used Flame.
  */
RECOMP_PATCH int dll_721_func_9E0(Object* self, Object* override, AnimObj_Data* animData, s8 arg3) {
    SCbeacon_Data_Extended* objData;
    u8 playerIsNearby;
    Object* player;
    Object* sidekick;
    SCbeacon_Setup* objSetup;
    u32 interactRangeSQ;
    u32 playerDistanceSQ;

    player = get_player();
    objData = self->data;
    objSetup = (SCbeacon_Setup*)self->setup;
    
    //@recomp: check state to tell which sequence is playing (don't add twigs during Kyte's Flame sequence)
    if (objData->state != SCbeacon_STATE_Lighting){
        objData->flags |= SCbeacon_FLAG_Add_Tumbleweed;
    }
    
    playerDistanceSQ = vec3_distance_xz_squared(&player->positionMirror, &self->positionMirror);
    interactRangeSQ = SQ(objSetup->playerRange);
    playerIsNearby = (playerDistanceSQ <= interactRangeSQ);

    switch (objData->state) {
    case SCbeacon_STATE_Lighting:
        sidekick = get_sidekick();
        if (vec3_distance_xz_squared(&sidekick->positionMirror, &self->positionMirror) <= 2500.0f) {
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_425, NULL, 2, -1, NULL);
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_426, NULL, 2, -1, NULL);
        }
        break;
    case SCbeacon_STATE_Lit:
        if (playerIsNearby) {
            objData->flags |= SCbeacon_FLAG_Emit_Light;
        }
        
        if ((objData->flags & SCbeacon_FLAG_Emit_Light) && !(objData->prevFlags & SCbeacon_FLAG_Emit_Light)) {
            func_80000450(self, self, 0x59, 0, 0, 0);
        } else if (!(objData->flags & SCbeacon_FLAG_Emit_Light) && (objData->prevFlags & SCbeacon_FLAG_Emit_Light)) {
            func_80000450(self, self, 0x5A, 0, 0, 0);
        }

        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_425, NULL, 2, -1, NULL);
        break;
    }
    
    SCbeacon_handle_flame_sounds(self, objData, playerDistanceSQ, interactRangeSQ);

    objData->prevFlags = objData->flags;
    
    return 0;
}

// Extend objData
RECOMP_PATCH u32 dll_721_get_data_size(Object* self, s32 arg1) {
    return sizeof(SCbeacon_Data_Extended);
}
