#include "dll.h"
#include "dlls/objects/210_player.h"
#include "game/gamebits.h"
#include "modding.h"

#include "PR/ultratypes.h"
#include "dlls/objects/469_ECSHshrine.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/gfx/modgfx.h"
#include "sys/map_enums.h"
#include "sys/objects.h"

#include "recomp/dlls/objects/469_ECSHshrine_recomp.h"
#include "sys/objtype.h"
#include "sys/rand.h"

/*0x40*/ extern Object *dShrine;

extern void ECSHshrine_handle_messages(Object *self);

#define NO_CHOICE_YET -1

#define WHISPERS_VOL_MIN 12
#define WHISPERS_VOL_MAX 70
#define WHISPERS_VOL_INIT WHISPERS_VOL_MIN

#define CORRIDOR_VOL_MIN 1
#define CORRIDOR_VOL_MAX 70
#define CORRIDOR_VOL_INIT 30

typedef enum {
    Shrine_MUSICTRACK_Whispering = 2,
    Shrine_MUSICTRACK_Corridor = 3
} ECSHshrine_MusicTracks;

/*0x0*/ extern Vec2f dCupCoords[];
/*0x30*/ extern s16 dCupSlots[];
/*0x3C*/ extern Texture *dTexture;
/*0x40*/ extern Object *dShrine;

/* Fix framerate dependencies */
RECOMP_PATCH void ECSHshrine_control(Object* self) {
    ECSHshrine_Data* objdata = self->data;
    Object* player = objGetPlayer();
    f32 dz;
    f32 tempCupZ;
    f32 tempCupX;
    s32 sp4C[] = {0, 0};
    DLL_IModgfx* modGfxDLL;
    Object* door;
    f32 objectDistance;
    s16 volume;
    s16 tempCupIndex;
    s32 i;

    objectDistance = 1000.0f;
    ECSHshrine_handle_messages(self);
    mainSetBits(BIT_DB_Entered_Shrine_2, 1);

    //Update music volume (whispers)
    if (objdata->musWhipersVolSpeed != 0) {
        objdata->musWhispersVol += objdata->musWhipersVolSpeed;
        if (objdata->musWhispersVol <= WHISPERS_VOL_MIN) {
            objdata->musWhispersVol = WHISPERS_VOL_MIN;
            objdata->musWhipersVolSpeed = 0;
        } else if (objdata->musWhispersVol >= WHISPERS_VOL_MAX) {
            objdata->musWhispersVol = WHISPERS_VOL_MAX;
            objdata->musWhipersVolSpeed = 0;
        }
        gDLL_5_AMSEQ->vtbl->set_volume(Shrine_MUSICTRACK_Whispering, objdata->musWhispersVol);
    }

    //Update music volume (corridor ambience)
    if (objdata->musCorridorVolSpeed != 0) {
        objdata->musCorridorVol += objdata->musCorridorVolSpeed;
        if ((objdata->musCorridorVol <= CORRIDOR_VOL_MIN) && (objdata->musCorridorVolSpeed <= 0)) {
            objdata->musCorridorVol = CORRIDOR_VOL_MIN;
            objdata->musCorridorVolSpeed = 0;
        } else if ((objdata->musCorridorVol >= CORRIDOR_VOL_MAX) && (objdata->musCorridorVolSpeed >= 0)) {
            objdata->musCorridorVol = CORRIDOR_VOL_MAX;
            objdata->musCorridorVolSpeed = 0;
        }
        gDLL_5_AMSEQ->vtbl->set_volume(Shrine_MUSICTRACK_Corridor, objdata->musCorridorVol);
    }

    //Start corridor music (and return early)
    if (objdata->musicPlayTimer > 0) {
        objdata->musicPlayTimer -= gUpdateRate;
        if (objdata->musicPlayTimer <= 0) {
            objdata->musicPlayTimer = 0;
            if (objdata->musicStarted == FALSE) {
                gDLL_5_AMSEQ->vtbl->play_ex(3, 44, 0x50, objdata->musCorridorVol, 0);
                objdata->musicStarted = TRUE;
            }
        }
        return;
    }

    //Crossfade music volumes as player approaches door
    door = objGetNearestTypeTo(OBJTYPE_Door, player, &objectDistance);
    if ((door != NULL) && (objectDistance < 300.0f) && (objectDistance > 100.0f)) {
        dz = door->srt.transl.z - player->srt.transl.z;

        //If player ahead of door (in shrine corridor)
        if (dz <= 0.0f) {
            //Get absolute distance along z
            if (dz < 0.0f) {
                dz *= -1.0f;
            }

            if (objdata->musCorridorVol != CORRIDOR_VOL_INIT) {
                objdata->musCorridorVol = CORRIDOR_VOL_INIT;
            }

            //Cross-fade music tracks as player approaches door
            volume = ((f32) objdata->musCorridorVol * ((dz - 100.0f) / 200.0f));
            if (volume < 1) {
                volume = 1;
            }
            gDLL_5_AMSEQ->vtbl->set_volume(Shrine_MUSICTRACK_Corridor, volume); //fade out corridor music

            volume = ((f32) objdata->musWhispersVol * ((200.0f - (dz - 100.0f)) / 200.0f));
            if (volume < 1) {
                volume = 1;
            }
            gDLL_5_AMSEQ->vtbl->set_volume(Shrine_MUSICTRACK_Whispering, volume); //fade in Krazoa whispering
        }
    }

    //STATE MACHINE
    switch (objdata->state) {
    case ECShrine_STATE_Waiting:
        if (vec3Distance(&self->globalPosition, &player->globalPosition) < objdata->testStartRadius) {
            objdata->state = ECShrine_STATE_Test_Start;
            mainSetBits(BIT_DB_Entered_Shrine_3, 0);
            gDLL_3_Animation->vtbl->start_obj_sequence(0, self, -1);

            modGfxDLL = dllLoad(DLL_ID_147, 1);
            modGfxDLL->vtbl->func0(self, 2, NULL, 1, -1, NULL);
            dllFree(modGfxDLL);

            modGfxDLL = dllLoad(DLL_ID_148, 1);
            modGfxDLL->vtbl->func0(self, 0, NULL, 1, -1, NULL);
            dllFree(modGfxDLL);

            mainSetBits(BIT_DB_Entered_Shrine_1, 0);
            gDLL_14_Modgfx->vtbl->func7(&objdata->modGfxCircle);
        }
        break;
    case ECShrine_STATE_Test_Start:
        //Wait for sequence to advance state
        if (objdata->seqValue == 1) {
            objdata->state = ECShrine_STATE_Cup_Game_Start;
            objdata->musicPlayTimer = 200;
            objdata->substate = Cup_STATE_Rise_Up;
            dll_amSfx->Play(NULL, SOUND_33E_Reverse_Magic_Hiss, MAX_VOLUME, NULL, NULL, 0, NULL);
            objdata->delayTimer = 0;
        }
        break;
    case ECShrine_STATE_Cup_Game_Start:
        objdata->state = ECShrine_STATE_Cups_Round_1;
        objdata->musicPlayTimer = 80;
        objdata->substate = Cup_STATE_Round_Start;
        objdata->delayTimer = 40 * 2; //@recomp: fix framerate dependency
        objdata->shuffles = 16;
        objdata->spiritCup = mathRnd(0, 5);
        gDLL_3_Animation->vtbl->start_obj_sequence(3, self, -1);
        break;
    case ECShrine_STATE_Cups_Round_1:
    case ECShrine_STATE_Cups_Round_2:
    case ECShrine_STATE_Cups_Round_3:
        if (objdata->delayTimer > 0) {
            objdata->delayTimer -= gUpdateRate; //@recomp: fix framerate dependency
            if (objdata->delayTimer < 0) {
                objdata->delayTimer = 0;
            }
        } else {
            switch (objdata->substate) {
            case Cup_STATE_Round_Start:
                objdata->substate = Cup_STATE_Shuffle;
                objdata->delayTimer = 40 * 2; //@recomp: fix framerate dependency
                objdata->musicPlayTimer = 60;
                break;
            case Cup_STATE_Round_End:
                objdata->substate = Cup_STATE_Round_Start;
                objdata->delayTimer = 40 * 2; //@recomp: fix framerate dependency
                objdata->musicPlayTimer = 60;
                break;
            case Cup_STATE_Sink_Down:
                objdata->substate = Cup_STATE_Underground;
                objdata->delayTimer = 40 * 2; //@recomp: fix framerate dependency
                objdata->musicPlayTimer = 60;
                break;
            case Cup_STATE_Shuffle:
                //Shuffle the cups
                objdata->shuffles--;
                if (objdata->shuffles <= 0) {
                    objdata->substate = Cup_STATE_Await_Choice;
                    objdata->choiceTimeout = 1000;
                } else {
                    objdata->substate = Cup_STATE_Stopped;
                    objdata->delayTimer = 2 * 2; //@recomp: fix framerate dependency

                    //Pick one of 8 different ways of shuffling cups (some options only used in later rounds)
                    if (objdata->state == ECShrine_STATE_Cups_Round_1) {
                        i = mathRnd(0, 1);
                    } else if (objdata->state == ECShrine_STATE_Cups_Round_2) {
                        i = mathRnd(0, 5);
                    } else {
                        i = mathRnd(0, 7);
                    }

                    //Perform the chosen shuffle
                    if (i == 0) {
                        //Swap all cup slots clockwise
                        for (i = 0; i < 6; i++) {
                            dCupSlots[i]++;
                            if (dCupSlots[i] > 5) {
                                dCupSlots[i] = 0;
                            }
                        }
                    } else if (i == 1) {
                        //Swap all cup slots anticlockwise
                        for (i = 0; i < 6; i++) {
                            dCupSlots[i]--;
                            if (dCupSlots[i] < 0) {
                                dCupSlots[i] = 5;
                            }
                        }
                    } else if (i == 2) {
                        //Swap slots 0, 2, 4 clockwise
                        tempCupIndex = dCupSlots[0];
                        dCupSlots[0] = dCupSlots[2];
                        dCupSlots[2] = dCupSlots[4];
                        dCupSlots[4] = tempCupIndex;
                    } else if (i == 3) {
                        //Swap slots 0, 2, 4 anticlockwise
                        tempCupIndex = dCupSlots[4];
                        dCupSlots[4] = dCupSlots[0];
                        dCupSlots[0] = dCupSlots[2];
                        dCupSlots[2] = tempCupIndex;
                    } else if (i == 4) {
                        //Swap slots 1, 3, 5 clockwise
                        tempCupIndex = dCupSlots[1];
                        dCupSlots[1] = dCupSlots[3];
                        dCupSlots[3] = dCupSlots[5];
                        dCupSlots[5] = tempCupIndex;
                    } else if (i == 5) {
                        //Swap slots 1, 3, 5 anticlockwise
                        tempCupIndex = dCupSlots[5];
                        dCupSlots[5] = dCupSlots[1];
                        dCupSlots[1] = dCupSlots[3];
                        dCupSlots[3] = tempCupIndex;
                    } else if (i == 6) {
                        //Swap slots 1, 2, 4, 5 clockwise (using coords rather than index)
                        tempCupX = dCupCoords[1].x;
                        tempCupZ = dCupCoords[1].y;
                        dCupCoords[1].x = dCupCoords[2].x;
                        dCupCoords[1].y = dCupCoords[2].y;
                        dCupCoords[2].x = dCupCoords[4].x;
                        dCupCoords[2].y = dCupCoords[4].y;
                        dCupCoords[4].x = dCupCoords[5].x;
                        dCupCoords[4].y = dCupCoords[5].y;
                        dCupCoords[5].x = tempCupX;
                        dCupCoords[5].y = tempCupZ;
                    } else if (i == 7) {
                        //Swap slots 1, 2, 4, 5 anticlockwise (using coords rather than index)
                        tempCupX = dCupCoords[5].x;
                        tempCupZ = dCupCoords[5].y;
                        dCupCoords[5].x = dCupCoords[4].x;
                        dCupCoords[5].y = dCupCoords[4].y;
                        dCupCoords[4].x = dCupCoords[2].x;
                        dCupCoords[4].y = dCupCoords[2].y;
                        dCupCoords[2].x = dCupCoords[1].x;
                        dCupCoords[2].y = dCupCoords[1].y;
                        dCupCoords[1].x = tempCupX;
                        dCupCoords[1].y = tempCupZ;
                    }
                }
                break;
            case Cup_STATE_Stopped:
                objdata->substate = Cup_STATE_Moving;
                objdata->delayTimer = 12 * 2; //@recomp: fix framerate dependency
                return;
            case Cup_STATE_Moving:
                objdata->substate = Cup_STATE_Move_Finished;
                objdata->delayTimer = 0;
                return;
            case Cup_STATE_Move_Finished:
                objdata->substate = Cup_STATE_Shuffle;
                objdata->delayTimer = 0;
                return;
            case Cup_STATE_Await_Choice:
                if (objdata->choiceWasCorrect == FALSE) {
                    objdata->state = ECShrine_STATE_Test_Failure;
                    gDLL_5_AMSEQ->vtbl->play_ex(3, 0x35, 0x50, (u8) objdata->musCorridorVol, 0);
                    objdata->musCorridorVolSpeed = 1;
                    gDLL_3_Animation->vtbl->start_obj_sequence(2, self, -1);
                    objdata->musicPlayTimer = 10;
                    objdata->substate = Cup_STATE_Sink_Down;
                    dll_amSfx->Play(NULL, SOUND_33E_Reverse_Magic_Hiss, MAX_VOLUME, NULL, NULL, 0, NULL);
                } else if (objdata->choiceWasCorrect == TRUE) {
                    if (objdata->state == ECShrine_STATE_Cups_Round_1) {
                        //Go to round 2
                        objdata->spiritCup = mathRnd(0, 5);
                        objdata->state = ECShrine_STATE_Cups_Round_2;
                        objdata->substate = Cup_STATE_Round_End;
                        objdata->musicPlayTimer = 150;
                        objdata->delayTimer = 12 * 2; //@recomp: fix framerate dependency
                        objdata->shuffles = 10;
                        objdata->choiceWasCorrect = NO_CHOICE_YET;
                        dll_amSfx->Play(NULL, SOUND_344_Chime, MAX_VOLUME, NULL, NULL, 0, NULL);
                        gDLL_3_Animation->vtbl->start_obj_sequence(3, self, -1);
                    } else if (objdata->state == ECShrine_STATE_Cups_Round_2) {
                        //Go to round 3
                        objdata->spiritCup = mathRnd(0, 5);
                        objdata->state = ECShrine_STATE_Cups_Round_3;
                        objdata->substate = Cup_STATE_Round_End;
                        objdata->musicPlayTimer = 150;
                        objdata->delayTimer = 12 * 2; //@recomp: fix framerate dependency
                        objdata->shuffles = 16;
                        objdata->choiceWasCorrect = NO_CHOICE_YET;
                        dll_amSfx->Play(NULL, SOUND_344_Chime, MAX_VOLUME, NULL, NULL, 0, NULL);
                        gDLL_3_Animation->vtbl->start_obj_sequence(3, self, -1);
                    } else {
                        //Test won!
                        objdata->musicPlayTimer = 130;
                        objdata->state = ECShrine_STATE_Test_Successful;
                        objdata->choiceWasCorrect = 0;
                        objdata->substate = Cup_STATE_Sink_Down;
                        dll_amSfx->Play(NULL, SOUND_344_Chime, MAX_VOLUME, NULL, NULL, 0, NULL);
                        dll_amSfx->Play(NULL, SOUND_33E_Reverse_Magic_Hiss, MAX_VOLUME, NULL, NULL, 0, NULL);
                    }
                } else {
                    //Automatically win if the player takes too long choosing (@bug? Possibly supposed to fail instead)
                    objdata->choiceTimeout -= gUpdateRate;
                    if (objdata->choiceTimeout <= 0) {
                        objdata->musicPlayTimer = 130;
                        objdata->state = ECShrine_STATE_Test_Successful;
                        objdata->choiceWasCorrect = FALSE;
                        objdata->substate = Cup_STATE_Sink_Down;
                        dll_amSfx->Play(NULL, SOUND_344_Chime, MAX_VOLUME, NULL, NULL, 0, NULL);
                        dll_amSfx->Play(NULL, SOUND_33E_Reverse_Magic_Hiss, MAX_VOLUME, NULL, NULL, 0, NULL);
                    }
                }
                break;
            }
        }
        break;
    case ECShrine_STATE_Test_Successful:
        //Check if player has Spirit #2 (Diamond Bay: Test of Strength) @bug?: should check for Spirit #4?
        if (((DLL_210_Player*)player->dll)->vtbl->get_spirit_bits(player, PLAYER_SPIRIT_2) != 0) {
            objdata->musCorridorVol = 1;
            gDLL_5_AMSEQ->vtbl->play_ex(3, 0x2C, 0x50, (u8) objdata->musCorridorVol, 0);
            objdata->musCorridorVolSpeed = 1;
            mainSetBits(BIT_DB_Entered_Shrine_3, 1);
            objdata->state = ECShrine_STATE_Warp_Away;
        } else {
            mainSetBits(BIT_DB_Entered_Shrine_1, 0);
            gDLL_5_AMSEQ->vtbl->play_ex(3, 0x2C, 0x50, (u8) objdata->musCorridorVol, 0);
            objdata->musCorridorVolSpeed = 1;
            gDLL_3_Animation->vtbl->start_obj_sequence(1, self, -1);
            objdata->state = ECShrine_STATE_Grant_Spirit;
        }
        break;
    case ECShrine_STATE_Grant_Spirit:
        mainSetBits(BIT_143, 0);
        objdata->state = ECShrine_STATE_Warp_Away;
        ((DLL_210_Player*)player->dll)->vtbl->set_spirit_bits(player, PLAYER_SPIRIT_4, TRUE);
        gDLL_29_Gplay->vtbl->set_act(MAP_WARLOCK_MOUNTAIN, 5);
        break;
    case ECShrine_STATE_Warp_Away:
        if (mainGetBits(BIT_Shrine_Do_Exit_Warp) == 0) {
            mainSetBits(BIT_Shrine_Do_Exit_Warp, 1);
        }
        mainSetBits(BIT_DB_Entered_Shrine_2, 0);
        mainSetBits(BIT_DB_Entered_Shrine_3, 0);
        objdata->state = ECShrine_STATE_Finished;
        break;
    case ECShrine_STATE_Test_Failure:
        //Reset shrine
        objdata->state = ECShrine_STATE_Waiting;
        objdata->unk10 = 0;
        objdata->shuffles = 0;
        objdata->substate = Cup_STATE_Stopped;
        objdata->choiceWasCorrect = NO_CHOICE_YET;
        objdata->delayTimer = 0;
        objdata->spiritCup = 0;
        objdata->seqValue = 0;
        objdata->musicPlayTimer = 400;
        
        mainSetBits(BIT_DB_Entered_Shrine_3, 1);
        mainSetBits(BIT_DB_Entered_Shrine_1, 1);
        mainSetBits(BIT_DB_Entered_Shrine_2, 1);

        modGfxDLL = dllLoad(DLL_ID_122, 1);
        objdata->modGfxCircle = modGfxDLL->vtbl->func0(self, 2, NULL, 0x402, -1, NULL);
        dllFree(modGfxDLL);
        break;
    case ECShrine_STATE_Finished:
        break;
    }
}

/** 
  * Exports the shrine's state and the cup minigame's substate value, and the index of the cup currently holding the Krazoa Spirit.
  */
RECOMP_PATCH void ECSHshrine_get_minigame_state(s32* shrineStatesArray, u8* cupWithSpirit) {
    Object* object;
    ECSHshrine_Data* objdata;

    object = dShrine;
    if (object != NULL) {
        objdata = dShrine->data;
        *cupWithSpirit = objdata->spiritCup;

        //@recomp: export the main shrine state value too
        shrineStatesArray[0] = objdata->state;
        shrineStatesArray[1] = objdata->substate;
    }
}
