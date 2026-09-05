#include "modding.h"
#include "object_util.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "common.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/210_player.h"
#include "game/objects/object.h"
#include "sys/gfx/modgfx.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "sys/objmsg.h"
#include "sys/objtype.h"
#include "sys/print.h"

#include "recomp/dlls/objects/433_DFSH_Shrine_recomp.h"

//TEMPORARY DEFINES
#define DFShrine_obj_Control DFShrine_control
#define DFShrine_obj_GetDataSize DFShrine_get_data_size
#define DFShrine_animCallback DFShrine_anim_callback
#define DFShrine_processObjMessages DFShrine_process_obj_messages

#define BIT_DFSH_ObjCreator_Stop 0x589
#define BIT_DF_Shrine_Activate_ObjCreator_1 0xF6
#define BIT_DF_Shrine_Activate_ObjCreator_2 0xF7
#define BIT_DF_Shrine_Activate_ObjCreator_3 0xF8
#define BIT_DF_Shrine_Activate_ObjCreator_4 0xF9
#define BIT_DF_Shrine_SharpClaw_Drop_Magic_Gems 0xFC
#define BIT_DF_Shrine_SharpClaw_Defeated 0x1E7

#define dll_player(obj) (((DLL_210_Player*)obj->dll)->vtbl)
//END OF TEMPORARY DEFINES

// #define DEBUG_TEST_OF_COMBAT

typedef struct {
/*00*/ s16 testStartRange;
/*02*/ s16 stateCooldown;
/*04*/ s16 whisperVolume;
/*06*/ s16 whisperVolumeRate;
/*08*/ s16 bgmVolume;
/*0A*/ s16 bgmVolumeRate;
/*0C*/ s16 modGfxRing;
/*0E*/ s16 testTimer;
/*10*/ u8 numSharpClawDefeated;
/*11*/ s8 state;
/*12*/ u8 seqValue;
/*13*/ u8 battleRound; //@recomp: repurpose for the combat waves
/*14*/ u8 startedBgMusic;
} DFShrine_Data;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 _unk18;
/*1A*/ s16 testStartRange;
} DFShrine_Setup;

typedef enum {
    DFShrine_STATE_0_Waiting,
    DFShrine_STATE_1_Test_Start,
    DFShrine_STATE_2_Test_Active,
    DFShrine_STATE_3_Test_Finished,
    DFShrine_STATE_4_Grant_Spirit,
    DFShrine_STATE_5_Warp_Away,
    DFShrine_STATE_6, //Unused
    DFShrine_STATE_7_Finished,
    DFShrine_STATE_8_Reset
} DFShrine_States;

extern void DFShrine_processObjMessages(Object* self);

extern Texture* data_0;

/* Fix SharpClaw respawning endlessly during the test and hanging around afterwards */
RECOMP_PATCH void DFShrine_obj_Control(Object* self) {
    static u8 sFirstTick = TRUE;
    DFShrine_Data* objdata = self->data;
    Object* player = objGetPlayer();
    DLL_IModgfx* modgfx;
    f32 playerDoorZDist;
    f32 playerDoorDist;
    Object* door;
    s16 volume;
    s16 i;
    Object** baddieList;
    s32 baddieCount;

    baddieCount = 0;
    playerDoorDist = 1000.0f;

    if (sFirstTick) {
        self->globalPosition.x = self->srt.transl.x;
        self->globalPosition.y = self->srt.transl.y;
        self->globalPosition.z = self->srt.transl.z;
        mainSetBits(BIT_DB_Entered_Shrine_2, TRUE);
        // Start whispers
        gDLL_5_AMSEQ->vtbl->play_ex(2, 0x2B, 0x50, 1, 0);
    }

    // @debug code: restore the player's magic and ensure they have the Forcefield Spell
    if (sFirstTick) {
        dll_player(player)->add_magic(player, 20);
        mainSetBits(BIT_Spell_Forcefield, TRUE);
        sFirstTick = FALSE;
    }

    // Process obj messages
    DFShrine_processObjMessages(self);

    // Modulate background whispers
    if (objdata->whisperVolumeRate != 0) {
        objdata->whisperVolume += objdata->whisperVolumeRate;
        if (objdata->whisperVolume <= 12) {
            objdata->whisperVolume = 12;
            objdata->whisperVolumeRate = 0;
        }
        else if (objdata->whisperVolume >= 70) {
            objdata->whisperVolume = 70;

            objdata->whisperVolumeRate = 0;
        }
        gDLL_5_AMSEQ->vtbl->set_volume(2, objdata->whisperVolume);
    }

    // Modulate background music
    if (objdata->bgmVolumeRate != 0) {
        objdata->bgmVolume += objdata->bgmVolumeRate;
        if ((objdata->bgmVolume <= 1) && (objdata->bgmVolumeRate <= 0)) {
            objdata->bgmVolume = 1;
            objdata->bgmVolumeRate = 0;
        } 
        else if ((objdata->bgmVolume >= 70) && (objdata->bgmVolumeRate >= 0)) {
            objdata->bgmVolume = 70;
            objdata->bgmVolumeRate = 0;
        }
        gDLL_5_AMSEQ->vtbl->set_volume(3, objdata->bgmVolume);
    }

    // Handle cooldown between states and return early
    if (objdata->stateCooldown > 0) {
        objdata->stateCooldown -= gUpdateRate;
        if (objdata->stateCooldown <= 0) {
            objdata->stateCooldown = 0;
            // Start background music if not already started
            if (objdata->startedBgMusic == FALSE) {
                gDLL_5_AMSEQ->vtbl->play_ex(3, 0x2C, 0x50, objdata->bgmVolume, 0);
                objdata->startedBgMusic = TRUE;
            }
        }
        return;
    }

    // Modulate background music and whispers as player passes the shrine door
    door = objGetNearestTypeTo(OBJTYPE_Door, player, &playerDoorDist);
    if ((door != NULL) && (playerDoorDist < 300.0f) && (playerDoorDist > 100.0f)) {
        playerDoorZDist = door->srt.transl.z - player->srt.transl.z;
        if (playerDoorZDist <= 0.0f) {
            if (playerDoorZDist < 0.0f) {
                playerDoorZDist *= -1.0f;
            }
            if (objdata->bgmVolume != 30) {
                objdata->bgmVolume = 30;
            }
            volume = (s16) (objdata->bgmVolume * ((playerDoorZDist - 100.0f) / 200.0f));
            if (volume < 1) {
                volume = 1;
            }
            gDLL_5_AMSEQ->vtbl->set_volume(3, volume);
            volume = (s16) (objdata->whisperVolume * ((200.0f - (playerDoorZDist - 100.0f)) / 200.0f));
            if (volume < 1) {
                volume = 1;
            }
            gDLL_5_AMSEQ->vtbl->set_volume(2, volume);
        }
    }

    //Shrine State Machine
    switch (objdata->state) {
    case DFShrine_STATE_0_Waiting:
        if (vec3Distance(&self->globalPosition, &player->globalPosition) < objdata->testStartRange) {
            mainSetBits(BIT_DFSH_ObjCreator_Stop, FALSE);
            objdata->state = DFShrine_STATE_1_Test_Start;
            mainSetBits(BIT_DB_Entered_Shrine_3, 0);
            gDLL_3_Animation->vtbl->start_obj_sequence(0, self, -1);

            modgfx = dllLoad(DLL_ID_147, 1);
            modgfx->vtbl->func0(self, 0, 0, 1, -1, 0);
            dllFree(modgfx);

            modgfx = dllLoad(DLL_ID_148, 1);
            modgfx->vtbl->func0(self, 0, 0, 1, -1, 0);
            dllFree(modgfx);

            mainSetBits(BIT_DB_Entered_Shrine_1, 0);
            gDLL_14_Modgfx->vtbl->func7(&objdata->modGfxRing);
        }
    default:
        break;
    case DFShrine_STATE_1_Test_Start:
        //Wait for sequence to advance state
        if (objdata->seqValue == 1) {
            objdata->state = DFShrine_STATE_2_Test_Active;
            objdata->stateCooldown = 160;
            objdata->testTimer = 6000;
            /* RECOMP */
            objdata->battleRound = 0;
        }
        break;
    case DFShrine_STATE_2_Test_Active:
        //@recomp: Pause the test timer while a SharpClaw is dying (so you can't fail while waiting for the last SharpClaw to finish dying)
        if (mainGetBits(BIT_DF_Shrine_SharpClaw_Defeated) == FALSE && 
            (player->stateFlags & OBJSTATE_IN_SEQ) == FALSE //Pause the timer while the player's in sequence (like being revived by a Duster)
        ) {
            objdata->testTimer -= gUpdateRate;
        }
        diPrintf("TIMER %d\n", objdata->testTimer);

#ifdef DEBUG_TEST_OF_COMBAT
        diPrintf("battleRound: %d\n", objdata->battleRound);
        diPrintf("numSharpClawDefeated: %d\n", objdata->numSharpClawDefeated);
#endif

        //@recomp: rework the combat waves (three rounds, fighting 1, then 2, then 4 SharpClaw)
        switch (objdata->battleRound) {
        case 0:
            mainSetBits(BIT_DF_Shrine_Activate_ObjCreator_1, TRUE);
            objdata->battleRound++;
            return;
        case 1: //Fighting one SharpClaw
            if (objdata->numSharpClawDefeated == 1) {
                mainSetBits(BIT_DF_Shrine_Activate_ObjCreator_1, TRUE);
                mainSetBits(BIT_DF_Shrine_Activate_ObjCreator_2, TRUE);
                objdata->battleRound++;
                objdata->numSharpClawDefeated = 0;
                return;
            }
            break;
        case 2: //Fighting two SharpClaw
            if (objdata->numSharpClawDefeated == 2) {
                mainSetBits(BIT_DF_Shrine_Activate_ObjCreator_1, TRUE);
                mainSetBits(BIT_DF_Shrine_Activate_ObjCreator_2, TRUE);
                mainSetBits(BIT_DF_Shrine_Activate_ObjCreator_3, TRUE);
                mainSetBits(BIT_DF_Shrine_Activate_ObjCreator_4, TRUE);
                objdata->battleRound++;
                objdata->numSharpClawDefeated = 0;
                return;
            }
            break;
        case 3: //Fighting four SharpClaw
            if (objdata->numSharpClawDefeated == 4) {
                // Pass
                objdata->state = DFShrine_STATE_3_Test_Finished;
                objdata->stateCooldown = 200;

                //@recomp: don't play LightFoot guitar track (must be a mistake!)
                // gDLL_5_AMSEQ->vtbl->play_ex(3, 0xE, 0x50, (u8)objdata->bgmVolume, 0); 
                objdata->bgmVolumeRate = -1; //@recomp: fade out Test of Combat theme

                //@recomp: play success chime (TODO: maybe better without it, to preserve the eeriness?)
                dll_amSfx->Play(self, SOUND_B01_Success_Chime, VOLUME_PERCENT(60), NULL, NULL, 0, NULL);
                return;
            }
            break;
        }
        
        //@recomp: rework the fail condition, so you can't run out of time while waiting for the last SharpClaw to finish dying
        if (mainGetBits(BIT_DF_Shrine_SharpClaw_Defeated) == FALSE && objdata->testTimer <= 0) {
            // Fail
            objdata->state = DFShrine_STATE_3_Test_Finished;
        }
        break;
    case DFShrine_STATE_3_Test_Finished:
        if (objdata->numSharpClawDefeated != 4) {
            // Test failed
            // Delete remaining SharpClaws
            baddieList = objGetAllOfType(OBJTYPE_Baddie, &baddieCount);
            for (i = 0; i < baddieCount; i++) {
                objFreeObject(baddieList[i]);
            }

            objdata->stateCooldown = 0;
            gDLL_5_AMSEQ->vtbl->play_ex(3, 0x35, 0x50, (u8) objdata->bgmVolume, 0);
            objdata->bgmVolumeRate = 1;
            gDLL_3_Animation->vtbl->start_obj_sequence(2, self, -1);
            objdata->stateCooldown = 10;
        } else if (dll_player(player)->get_spirit_bits(player, PLAYER_SPIRIT_1)) {
            // Test passed, but the player is already carrying the spirit
            objdata->bgmVolume = 1;
            gDLL_5_AMSEQ->vtbl->play_ex(3, 0x2C, 0x50, (u8) objdata->bgmVolume, 0);
            objdata->bgmVolumeRate = 1;
            mainSetBits(BIT_DB_Entered_Shrine_3, 1);
            objdata->state = DFShrine_STATE_5_Warp_Away;
        } else {
            // Test passed, play spirit cutscene
            objdata->stateCooldown = 0;
            mainSetBits(BIT_DB_Entered_Shrine_1, 0);
            gDLL_3_Animation->vtbl->start_obj_sequence(1, self, -1);
            objdata->stateCooldown = 10;
        }
        break;
    case DFShrine_STATE_4_Grant_Spirit:
        // Spirit granted
        mainSetBits(BIT_125, FALSE);
        objdata->state = DFShrine_STATE_5_Warp_Away;
        dll_player(player)->set_spirit_bits(player, PLAYER_SPIRIT_1, TRUE);
        gDLL_29_Gplay->vtbl->set_act(MAP_WARLOCK_MOUNTAIN, 2);
        break;
    case DFShrine_STATE_5_Warp_Away:
        // Warp out
        if (mainGetBits(BIT_Shrine_Do_Exit_Warp) == FALSE) {
            mainSetBits(BIT_Shrine_Do_Exit_Warp, TRUE);
        }
        mainSetBits(BIT_DB_Entered_Shrine_2, 0);
        mainSetBits(BIT_DB_Entered_Shrine_3, 1);
        objdata->state = DFShrine_STATE_7_Finished;
        break;
    case DFShrine_STATE_8_Reset:
        // Reset
        objdata->state = DFShrine_STATE_0_Waiting;
        objdata->seqValue = 0;
        objdata->stateCooldown = 400;
        objdata->numSharpClawDefeated = 0;
        mainSetBits(BIT_DB_Entered_Shrine_3, 1);
        mainSetBits(BIT_125, 0);
        mainSetBits(BIT_DB_Entered_Shrine_1, 1);
        mainSetBits(BIT_DB_Entered_Shrine_2, 1);
        mainSetBits(BIT_DF_Shrine_SharpClaw_Defeated, FALSE);
        mainSetBits(BIT_DF_Shrine_Activate_ObjCreator_1, FALSE);
        mainSetBits(BIT_DF_Shrine_Activate_ObjCreator_2, FALSE);
        mainSetBits(BIT_DF_Shrine_Activate_ObjCreator_3, FALSE);
        mainSetBits(BIT_DF_Shrine_Activate_ObjCreator_4, FALSE);
        mainSetBits(BIT_FA, FALSE);
        mainSetBits(BIT_FB, FALSE);
        mainSetBits(BIT_DFSH_ObjCreator_Stop, TRUE);
        objdata->testTimer = 0;
        /* RECOMP */
        objdata->battleRound = 0;

        modgfx = dllLoad(DLL_ID_122, 1);
        objdata->modGfxRing = modgfx->vtbl->func0(self, 0, 0, 0x402, -1, 0);
        dllFree(modgfx);
        break;
    }
}

/* Play the usual post-Krazoa Test music */
RECOMP_PATCH int DFShrine_animCallback(Object* self, Object* animObj, AnimObj_Data* animData, s8 prevCallbackValue) {
    DFShrine_Data* objData = self->data;
    s32 i;
    
    animData->unk62 = 0;

    if (objData->bgmVolumeRate != 0) {
        objData->bgmVolume += objData->bgmVolumeRate;
        if ((objData->bgmVolume < 2) && (objData->bgmVolumeRate <= 0)) {
            objData->bgmVolume = 1;
            objData->bgmVolumeRate = 0;
        } else if ((objData->bgmVolume >= 70) && (objData->bgmVolumeRate >= 0)) {
            objData->bgmVolume = 70;
            objData->bgmVolumeRate = 0;
        }
        gDLL_5_AMSEQ->vtbl->set_volume(3, objData->bgmVolume);
    }

    for (i = 0; i < animData->messageCount; i++) {
        s32 message = animData->messages[i];
        switch (message) {
            case 1:
                envfxAction(self, self, 0xC3, 0);
                break;
            case 2:
                if (*D_80092A7C == -1) {
                    envfxAction(self, self, 0x14, 0);
                } else {
                    envfxAction(self, self, *D_80092A7C, 0);
                }
                break;
            case 3:
                objData->seqValue = 1;
                break;
            case 4:
                objData->state = DFShrine_STATE_4_Grant_Spirit;
                objData->seqValue = 2;
                mainSetBits(BIT_DB_Entered_Shrine_3, 1);
                mainSetBits(BIT_125, 0);
                mainSetBits(BIT_DB_Entered_Shrine_1, 1);
                objData->bgmVolumeRate = -3;
                break;
            case 5:
                objData->state = DFShrine_STATE_8_Reset;
                objData->seqValue = 3;
                objData->bgmVolumeRate = -3;
                mainSetBits(BIT_DB_Entered_Shrine_3, 1);
                break;
            case 6:
                mainSetBits(BIT_125, 1);
                break;
            case 7:
                mainSetBits(BIT_125, 0);
                objData->bgmVolumeRate = -3;
                break;
            case 9:
                mainSetBits(BIT_DB_Triggered_In_Shrine_Spirit_Cutscene, 1);
                if (data_0 == NULL) {
                    data_0 = blockTexanimGetTex(1);
                }
                break;
            case 8:
                mainSetBits(BIT_DB_Entered_Shrine_2, 1);
                break;
            case 10:
                objData->bgmVolume = 100;
                gDLL_5_AMSEQ->vtbl->play_ex(3, 0x2D, 0x50, (u8) objData->bgmVolume, 0);
                break;
            case 11:
                objData->bgmVolume = 100;
                //@recomp: change music to usual post-Krazoa Test theme
                gDLL_5_AMSEQ->vtbl->play_ex(3, 0x2C, 0x50, (u8) objData->bgmVolume, 0); 
                break;
        }
        animData->messages[i] = 0;
    }

    return 0;
}

/* Use messages to detect when a SharpClaw dies instead of a gamebit, to avoid problems when multiple SharpClaw die at once. */
RECOMP_PATCH void DFShrine_processObjMessages(Object* self) {
    Object* sender;
    u32 mesgID;
    void* mesgArg;
    DFShrine_Data* objdata = self->data;
    
    mesgArg = NULL;
    while (objRecvMesg(self, &mesgID, &sender, &mesgArg)) {
        switch (mesgID) {
        case 0x30005:
            objdata->whisperVolumeRate = -3;
            break;
        case 0x30006:
            objdata->whisperVolumeRate = 0x10;
            break;

        //@recomp: handle new message
        case 0xDEAD:
            objdata->numSharpClawDefeated++;
            mainSetBits(BIT_DF_Shrine_SharpClaw_Defeated, FALSE);
            break;
        }
    }
}
