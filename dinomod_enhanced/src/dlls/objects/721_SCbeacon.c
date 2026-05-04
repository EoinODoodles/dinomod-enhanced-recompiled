#include "modding.h"
#include "recomputils.h"
#include "player_util.h"

#include "objects/227_tumbleweed.h"

#include "common.h"
#include "PR/ultratypes.h"
#include "game/objects/interaction_arrow.h"
#include "sys/gfx/modgfx.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "sys/segment_1050.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/common/sidekick.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/227_tumbleweed.h"
#include "dlls/objects/721_SCbeacon.h"

#include "recomp/dlls/objects/227_tumbleweed_recomp.h"
#include "recomp/dlls/objects/721_SC_beacon_recomp.h"

#define DEBUG_BEACON FALSE

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

extern void SCbeacon_attempt_to_light(Object* self);

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
        gDLL_6_AMSFX->vtbl->play(self, SOUND_50a_Fire_Burning_Low_Loop, MAX_VOLUME, &objData->soundHandleBurn1, NULL, 0, NULL);
    }
    if (!objData->soundHandleBurn2) {
        gDLL_6_AMSFX->vtbl->play(self, SOUND_50b_Fire_Burning_High_Loop, MAX_VOLUME, &objData->soundHandleBurn2, NULL, 0, NULL);
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
        gDLL_6_AMSFX->vtbl->stop(objData->soundHandleBurn1);
        objData->soundHandleBurn1 = 0;
    }
    if (objData->soundHandleBurn2) {
        gDLL_6_AMSFX->vtbl->stop(objData->soundHandleBurn2);
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
RECOMP_PATCH void SCbeacon_control(Object* self) {
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

    #if DEBUG_BEACON 
    {
        diPrintf("soundHandleBurn1: %d\n", objData->soundHandleBurn1);
        diPrintf("soundHandleBurn2: %d\n", objData->soundHandleBurn2);
    }
    #endif

    playerDistSQ = vec3_distance_xz_squared(&player->globalPosition, &self->globalPosition);
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
            SCbeacon_attempt_to_light(self); //will only be successful here if twigs gamebit also set
        }
        break;
    case SCbeacon_STATE_Bowl_Empty:

        sidekick = get_sidekick();
        if (sidekick && (self->unkAF & ARROW_FLAG_4_Highlighted)) {
            //Show Flame command option
            ((DLL_ISidekick*)sidekick->dll)->vtbl->func14(sidekick, 4);

            //Check if Flame command was selected
            if (gDLL_1_cmdmenu->vtbl->was_this_item_used(Sidekick_Command_INDEX_4_Flame)) {
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

                #if DEBUG_BEACON
                recomp_printf("PLACING: setting up silent delete!\n");
                #endif

                //Play deposit sequence
                gDLL_3_Animation->vtbl->func17(objData->seqIdxPlaceTwigs, self, -1);
            }
        }
        break;
    case SCbeacon_STATE_Twigs_in_Bowl:
        //@recomp: destroy the Tumbleweed that was being held
        if (objData->destroyTumbleweed) {
            #if DEBUG_BEACON
            recomp_printf("IN BOWL: destroyTumbleweed? %d\n", objData->destroyTumbleweed);
            #endif

            if (objData->heldTumbleweed && !(objData->heldTumbleweed->stateFlags & OBJSTATE_DESTROYED)){
                #if DEBUG_BEACON
                recomp_printf("IN BOWL: deleting Tumbleweed!\n");
                #endif
                
                obj_destroy_object(objData->heldTumbleweed);
            }
            objData->destroyTumbleweed = FALSE;

            #if DEBUG_BEACON
            recomp_printf("IN BOWL: stop player carry!\n");
            #endif

            playerUtil_stop_carrying(player);
        }

        sidekick = get_sidekick();
        if (sidekick && (self->unkAF & ARROW_FLAG_4_Highlighted)) {
            //Show Flame command option
            ((DLL_ISidekick*)sidekick->dll)->vtbl->func14(sidekick, 4);

            //Check if Flame command was selected
            if (gDLL_1_cmdmenu->vtbl->was_this_item_used(Sidekick_Command_INDEX_4_Flame)) {
                main_set_bits(BIT_Kyte_Flight_Curve, objSetup->kyteCurveID);
            }
        }
        break;
    case SCbeacon_STATE_Lighting:
        sidekick = get_sidekick();
        if (vec3_distance_xz_squared(&sidekick->globalPosition, &self->globalPosition) <= 2500.0f) {
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
        gDLL_6_AMSFX->vtbl->play(player, SOUND_6E6_Interaction_Refused, MAX_VOLUME, NULL, NULL, 0, NULL);
    //Play success sound when lit
    } else if (objData->playSuccess && (objData->prevState == SCbeacon_STATE_Lit) && (objData->state == SCbeacon_STATE_Lit)){
        gDLL_6_AMSFX->vtbl->play(player, 0xB01, 0x60, NULL, NULL, 0, NULL);
        objData->playSuccess = FALSE;
    }

    SCbeacon_handle_flame_sounds(self, objData, playerDistSQ, interactRangeSQ);
    
    objData->prevFlags = objData->flags;
    //@recomp: store previous state as well
    objData->prevState = objData->state;
}

/** Stop sounds, free soundHandles */
RECOMP_PATCH void SCbeacon_free(Object* self, s32 arg1) {
    gDLL_14_Modgfx->vtbl->func5(self);
    gDLL_13_Expgfx->vtbl->func5(self);
    obj_free_object_type(self, OBJTYPE_48);

    //@recomp: soundHandles
    SCbeacon_flame_sounds_stop(self);
}

/** Play success jingle when puzzle complete */
RECOMP_PATCH int SCbeacon_handle_kyte_flame_seqs(Object* self, s32 finishLighting) {
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
            SCbeacon_attempt_to_light(self); //will always be successful here

            //Set up success sound
            objData->playSuccess = TRUE;

            //Warp to Discovery Falls' entrance when all beacons are lit, and play pool-draining sequence
            if (main_get_bits(BIT_SC_Beacon_Lit_1) && 
                main_get_bits(BIT_SC_Beacon_Lit_2) && 
                main_get_bits(BIT_SC_Beacon_Lit_3)
            ) {
                main_set_bits(BIT_SC_All_Beacons_Lit, 1);

                //@recomp: play success jingle
                gDLL_6_AMSFX->vtbl->play(NULL, SOUND_B89_Puzzle_Solved, 0x60, NULL, NULL, 0, NULL);

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
RECOMP_PATCH void SCbeacon_attempt_to_light(Object* self) {
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
RECOMP_PATCH int SCbeacon_anim_callback(Object* self, Object* override, AnimObj_Data* animData, s8 arg3) {
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
    
    playerDistanceSQ = vec3_distance_xz_squared(&player->globalPosition, &self->globalPosition);
    interactRangeSQ = SQ(objSetup->playerRange);
    playerIsNearby = (playerDistanceSQ <= interactRangeSQ);

    switch (objData->state) {
    case SCbeacon_STATE_Lighting:
        sidekick = get_sidekick();
        if (vec3_distance_xz_squared(&sidekick->globalPosition, &self->globalPosition) <= 2500.0f) {
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
RECOMP_PATCH u32 SCbeacon_get_data_size(Object* self, s32 arg1) {
    return sizeof(SCbeacon_Data_Extended);
}
