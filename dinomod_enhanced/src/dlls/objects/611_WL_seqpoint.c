#include "modding.h"

#include "custom_gamebits.h"

#include "dlls/objects/210_player.h"
#include "game/gamebits.h"
#include "recomputils.h"
#include "sys/gfx/animseq.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/objprint.h"
#include "sys/print.h"
#include "macros.h"
#include "dll.h"

#include "recomp/dlls/objects/611_WL_seqpoint_recomp.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 rotation; // yaw >> 8
/*19*/ s8 triggerCondition;
/*1A*/ s16 triggerRange;
/*1C*/ s16 seqno; // object seq index
/*1E*/ s16 conditionBit; // if not -1, bit that determines if the seqpoint is enabled. required value depends on the trigger condition
/*20*/ s16 triggeredBit; // if not -1, this bit is set to 1 once seqpoint is triggered. prevents re-trigger if bit is already 1
} WL_seqpoint_Setup;

typedef struct {
/*00*/ f32 triggerRange;
/*04*/ s16 conditionBit;
/*06*/ s16 triggeredBit;
/*08*/ s16 seqno;
/*0A*/ s16 unkA; // unused
/*0C*/ u8 lastMesg; // last seq message
/*0D*/ u8 triggered;
/*0E*/ s8 triggerCondition;
} WL_seqpoint_Data;

enum WL_seqpoint_Sequence {
    WL_SEQPOINT_SEQ_0_Spirit_1_Release = 0,
    WL_SEQPOINT_SEQ_1 = 1,
    WL_SEQPOINT_SEQ_4_Randorn_Warns_Krystal_Of_Skeetlas = 4,
    WL_SEQPOINT_SEQ_5_Krystal_Randorn_Talk_About_Sabre = 5,
    WL_SEQPOINT_SEQ_6_Spirit_3_Release = 6,
    WL_SEQPOINT_SEQ_10_Spirit_4_Release = 10,
    WL_SEQPOINT_SEQ_11_Randorn_Asks_For_Food = 11,
    WL_SEQPOINT_SEQ_13_Randorn_Explains_Arrival = 13,
    WL_SEQPOINT_SEQ_14_Spirit_2_Release = 14,
    WL_SEQPOINT_SEQ_15_Spirit_5_Release = 15,
    WL_SEQPOINT_SEQ_16_Tricky_Senses_RockFall = 16,
    WL_SEQPOINT_SEQ_17_Spirit_6_Release = 17,
    WL_SEQPOINT_SEQ_18_SpiritCrystal_Talks_To_Sabre_1 = 18,
    WL_SEQPOINT_SEQ_19_SpiritCrystal_Talks_To_Sabre_2 = 19,
    WL_SEQPOINT_SEQ_21_DF_Spirit_Conversation = 21
};

RECOMP_PATCH int WL_seqpoint_anim_callback(Object* actor, Object* animObj, AnimObj_Data* animObjData, s8 a3) {
    WL_seqpoint_Data* objdata = actor->data;
    Object* player = get_player();
    s32 i;
    
    animObjData->unk7A = -1;
    animObjData->unk62 = 0;
    // @recomp: Fix loop condition to avoid reading out of bounds of the messages array and handling
    //          message indices that weren't actually set.
    for (i = 0; i < animObjData->messageCount; i++) {
        switch (objdata->seqno) {
        case WL_SEQPOINT_SEQ_21_DF_Spirit_Conversation:
            switch (animObjData->messages[i]) {
            case 2:
                main_set_bits(BIT_125, 1);
                break;
            case 3:
                main_set_bits(BIT_125, 0);
                break;
            case 10:
                ((DLL_210_Player*)player->dll)->vtbl->set_spirit_bits(player, PLAYER_SPIRIT_1, TRUE);
                main_set_bits(BIT_Play_Seq_0170_WM_Return_to_Randorn_Quan_Ata_Lachu_Speaks, 1);
                break;
            }
            break;
        case WL_SEQPOINT_SEQ_19_SpiritCrystal_Talks_To_Sabre_2:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
                if (animObjData->messages[i] == 1) {
                    main_set_bits(objdata->conditionBit, 0);
                }
            }
            break;
        case WL_SEQPOINT_SEQ_18_SpiritCrystal_Talks_To_Sabre_1:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
                if (animObjData->messages[i] == 1) {
                    gDLL_29_Gplay->vtbl->set_act(MAP_CAPE_CLAW, 9);
                    main_set_bits(BIT_367, 1);
                    main_set_bits(BIT_368, 1);
                    main_set_bits(BIT_Play_Seq_020D, 0);
                }
            }
            break;
        case WL_SEQPOINT_SEQ_17_Spirit_6_Release:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
                switch (animObjData->messages[i]) {
                case 1:
                    main_set_bits(BIT_1D2, 1);
                    break;
                case 2:
                    main_set_bits(BIT_1D2, 0);
                    break;
                case 3:
                    main_set_bits(BIT_WM_Spirit_Release_Effect, 1);
                    break;
                case 4:
                    main_set_bits(BIT_WM_Spirit_Release_Effect, 0);
                    ((DLL_210_Player*)player->dll)->vtbl->set_spirit_bits(player, PLAYER_SPIRIT_7, FALSE);
                    main_set_bits(BIT_222, 1);
                    break;
                }
            }
            break;
        case WL_SEQPOINT_SEQ_16_Tricky_Senses_RockFall:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
            }
            switch (objdata->lastMesg) {
            case 1:
                diPrintf("\t\tWhat is it ? ...\n");
                break;
            case 2:
                diPrintf("\t\tI Can Feel Something Strange !\n");
                break;
            }
            break;
        case WL_SEQPOINT_SEQ_15_Spirit_5_Release:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
                switch (animObjData->messages[i]) {
                case 1:
                    main_set_bits(BIT_1CF, 1);
                    break;
                case 2:
                    main_set_bits(BIT_1CF, 0);
                    break;
                case 3:
                    main_set_bits(BIT_WM_Spirit_Release_Effect, 1);
                    break;
                case 4:
                    main_set_bits(BIT_WM_Spirit_Release_Effect, 0);
                    main_set_bits(BIT_221, 1);
                    ((DLL_210_Player*)player->dll)->vtbl->set_spirit_bits(player, PLAYER_SPIRIT_6, FALSE);
                    main_set_bits(BIT_31D, 1);
                    main_set_bits(BIT_31E, 1);
                    main_set_bits(BIT_31C, 1);
                    break;
                }
            }
            break;
        case WL_SEQPOINT_SEQ_14_Spirit_2_Release:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
                switch (animObjData->messages[i]) {
                case 1:
                    main_set_bits(BIT_125, 1);
                    break;
                case 2:
                    main_set_bits(BIT_125, 0);
                    break;
                case 3:
                    main_set_bits(BIT_WM_Spirit_Release_Effect, 1);
                    break;
                case 4:
                    main_set_bits(BIT_WM_Spirit_Release_Effect, 0);
                    main_set_bits(BIT_21F_Spirit_Collected, 1);
                    ((DLL_210_Player*)player->dll)->vtbl->set_spirit_bits(player, PLAYER_SPIRIT_2, FALSE);
                    main_set_bits(BIT_231, 1);
                    main_set_bits(BIT_232, 1);
                    main_set_bits(BIT_Spirit_2_Release_Sabre, 1);
                    break;
                }
            }
            break;
        case WL_SEQPOINT_SEQ_0_Spirit_1_Release:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
                switch (animObjData->messages[i]) {
                case 1:
                    main_set_bits(BIT_125, 1);
                    break;
                case 2:
                    main_set_bits(BIT_125, 0);
                    break;
                case 3:
                    main_set_bits(BIT_WM_Spirit_Release_Effect, 1);
                    break;
                case 5:
                    main_set_bits(BIT_Set_During_Spirit_Release_1, 1);
                    break;
                case 4:
                    main_set_bits(BIT_WM_Spirit_Release_Effect, 0);
                    main_set_bits(BIT_Set_During_Spirit_Release_1, 1);
                    ((DLL_210_Player*)player->dll)->vtbl->set_spirit_bits(player, PLAYER_SPIRIT_1, FALSE);
                    main_set_bits(BIT_231, 1);
                    main_set_bits(BIT_232, 1);
                    main_set_bits(BIT_Play_Seq_0180_Release_Spirit_1, 1);
                    break;
                }
            }
            break;
        case WL_SEQPOINT_SEQ_6_Spirit_3_Release:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
                switch (animObjData->messages[i]) {
                case 1:
                    main_set_bits(BIT_MMP_GP_Shrine_Spirit_Light_Beams, 1);
                    break;
                case 2:
                    main_set_bits(BIT_MMP_GP_Shrine_Spirit_Light_Beams, 0);
                    break;
                case 3:
                    main_set_bits(BIT_WM_Spirit_Release_Effect, 1);
                    break;
                case 4:
                    main_set_bits(BIT_WM_Spirit_Release_Effect, 0);
                    main_set_bits(BIT_21C, 1);
                    ((DLL_210_Player*)player->dll)->vtbl->set_spirit_bits(player, PLAYER_SPIRIT_3, FALSE);
                    main_set_bits(BIT_231, 1);
                    main_set_bits(BIT_232, 1);
                    main_set_bits(BIT_317, 1);
                    break;
                }
            }
            break;
        case WL_SEQPOINT_SEQ_10_Spirit_4_Release:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
                switch (animObjData->messages[i]) {
                case 1:
                    main_set_bits(BIT_143, 1);
                    break;
                case 2:
                    main_set_bits(BIT_143, 0);
                    break;
                case 3:
                    main_set_bits(BIT_WM_Spirit_Release_Effect, 1);
                    break;
                case 4:
                    main_set_bits(BIT_WM_Spirit_Release_Effect, 0);
                    main_set_bits(BIT_21D, 1);
                    ((DLL_210_Player*)player->dll)->vtbl->set_spirit_bits(player, PLAYER_SPIRIT_4, FALSE);
                    main_set_bits(BIT_2F5, 1);
                    main_set_bits(BIT_2F6, 1);
                    main_set_bits(BIT_318, 1);
                    break;
                }
            }
            break;
        case WL_SEQPOINT_SEQ_1:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
            }
            break;
        case WL_SEQPOINT_SEQ_4_Randorn_Warns_Krystal_Of_Skeetlas:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
            }
            switch (objdata->lastMesg) {
            case 1:
                diPrintf("\t\tRandorn...  Are You There ??\n");
                break;
            case 2:
                diPrintf("\t\tBe Careful..Skeetas Everywhere !\n");
                break;
            case 3:
                diPrintf("\t\tStay Close to a Torch!\n");
                diPrintf("\t\tLight Kills Them\n");
                break;
            }
            break;
        case WL_SEQPOINT_SEQ_5_Krystal_Randorn_Talk_About_Sabre:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
            }
            switch (objdata->lastMesg) {
            case 1:
                diPrintf("\t\tKrystal! How Is My Son?\n");
                break;
            case 2:
                diPrintf("\t\tSabre Is Just Glad Your Alive\n");
                break;
            case 3:
                diPrintf("\t\tWhen His Brother Died I blamed Myself\n");
                diPrintf("\t\tI Couldn't Face Being A Father\n");
                diPrintf("\t\tUntil I Found You My Dear Girl\n");
                break;
            case 4:
                diPrintf("\t\tHe Understands Why You Left Him\n");
                break;
            case 5:
                diPrintf("\t\tHe Is A Good Son\n");
                diPrintf("\t\tI've Yet To See Him To Apologise\n");
                break;
            case 6:
                diPrintf("\t\tHe Has Already Forgiven You\n");
                diPrintf("\t\tWhen This Is Over You Shall Be Reunited!\n");
                break;
            }
            break;
        case WL_SEQPOINT_SEQ_11_Randorn_Asks_For_Food:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
            }
            switch (objdata->lastMesg) {
            case 1:
                diPrintf("\t\tWhat's happening to you Randorn?\n");
                break;
            case 2:
                diPrintf("\t\tThere's not much energy left in this Mountain\n");
                diPrintf("\t\tI'm dying..I need food\n");
                break;
            }
            break;
        case WL_SEQPOINT_SEQ_13_Randorn_Explains_Arrival:
            if (animObjData->messages[i] != 0) {
                objdata->lastMesg = animObjData->messages[i];
            }
            switch (objdata->lastMesg) {
            case 1:
                diPrintf("\t\tDo You Remember How We Got Here?\n");
                break;
            case 2:
                diPrintf("\t\tOf Course I Remember...\n");
                diPrintf("\t\tBack On Our Own Planet We Found A\n");
                diPrintf("\t\tStrange Room In The Lost City..\n");
                diPrintf("\t\tThere We Uncovered A Message From The\n");
                diPrintf("\t\tKing EarthWalker, Begging Us To Help Him.\n");
                break;
            case 3:
                diPrintf("\t\tAnd Then I Entered A Wormhole\n");
                diPrintf("\t\tAnd Arrived On Dinosaur Planet\n");
                break;
            case 4:
                diPrintf("\t\tYou Followed Later With Sabre\n");
                break;
            case 5:
                diPrintf("\t\tYes, We Ended Up In A Chamber Beneath\n");
                diPrintf("\t\tOne Of The Earthwalker Temples\n");
                break;
            case 6:
                diPrintf("\t\tUnless You Can Find A Cure For Me\n");
                diPrintf("\t\tI'm Not Going To Last Much Longer\n");
                break;
            case 7:
                diPrintf("\t\tBut What Can We Do\n");
                break;
            case 8:
                diPrintf("\t\tThere Must Be A Similiar Room Here\n");
                diPrintf("\t\tThat Will Allow Us To Return\n");
                diPrintf("\t\tEither You Or Sabre Must Find It\n");
                break;
            }
            break;
        // @recomp: Disable Blue SnowHorn hit after GP spirit release
        case 29:
            switch (animObjData->messages[i]) {
            case 4:
                main_set_bits(DINOMOD_BIT_92D_Blue_SnowHorn_HitAnimator, 1);
                ((DLL_210_Player*)player->dll)->vtbl->set_spirit_bits(player, PLAYER_SPIRIT_8, FALSE);
                break;
            }
            break;
        }
        animObjData->messages[i] = 0;
    }
    return 0;
}
