#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "dlls/engine/53_movelib.h"
#include "game/gametexts.h"
#include "game/objects/setups/51_Generic.h"
#include "game/objects/interaction_arrow.h"
#include "sys/math.h"
#include "sys/objanim.h"
#include "sys/objexpr.h"

#include "recomp/dlls/objects/507_SHqueenearthwalker_recomp.h"

#define DEBUG_MUSHROOMS FALSE

typedef struct {
    u8 state;
    u8 numMushroomsEaten;
    u8 unk2;
    u8 unk3;
    /** RECOMP */
    u8 wasMenuOpen;
    MoveLibData moveData;
    HeadAnimation headAnim;
} SHqueenearthwalker_Data;

typedef enum {
    SHqueenearthwalker_STATE_1_Initial = 1,
    SHqueenearthwalker_STATE_2_Before_Meeting_Sabre,
    SHqueenearthwalker_STATE_3_Needing_White_Mushrooms,
    SHqueenearthwalker_STATE_4_Healed,
    SHqueenearthwalker_STATE_5_Discussing_Force_Point_Temples,
    SHqueenearthwalker_STATE_6_Discussing_SpellStones,
    SHqueenearthwalker_STATE_7_After_SpellStone_Sequence
} SHqueenearthwalker_States;

/** @recomp: for checking which White Mushrooms are collected */
static s16 mushroomCollectionGamebits[10] = {
    BIT_13,
    BIT_F4,
    BIT_F5,
    BIT_15C,
    BIT_177,
    BIT_460,
    BIT_461,
    BIT_462,
    BIT_463,
    BIT_464
};

/** Allows game progress to continue if there's a desync between the White Mushroom collection gamebits
  * and the White Mushroom inventory gamebit (a commonly-reported bug) */
static void handle_mushroom_gamebit_contradictions(void) {
    u8 mushroomsCollected = 0;
    u8 mushroomsGiven;
    u8 mushroomsHeld;
    u8 i;

    //Check how many White Mushrooms have been collected (and no longer appear in the world)
    for (i = 0; i < ARRAYCOUNT(mushroomCollectionGamebits); i++){
        if (mainGetBits(mushroomCollectionGamebits[i])){
            mushroomsCollected++;
        }
    }

    //Get inventory mushrooms count and Queen's received mushroom count
    mushroomsHeld = mainGetBits(BIT_Inventory_White_Mushrooms);
    mushroomsGiven = mainGetBits(BIT_SH_Queen_EW_White_Mushrooms_Eaten);

    if (mushroomsHeld > ARRAYCOUNT(mushroomCollectionGamebits)){
        mushroomsHeld = ARRAYCOUNT(mushroomCollectionGamebits);
    }
    if (mushroomsGiven > ARRAYCOUNT(mushroomCollectionGamebits)){
        mushroomsGiven = ARRAYCOUNT(mushroomCollectionGamebits);
    }

    //Handle contradictions
    if (mushroomsCollected != (mushroomsHeld + mushroomsGiven)){
        #if DEBUG_MUSHROOMS
        {
            recomp_printf("Contradiction found (White Mushrooms)!\n");
            recomp_printf("Total collected: %d\n", mushroomsCollected);
            recomp_printf("Total held: %d\n", mushroomsHeld);
            recomp_printf("Total given: %d\n", mushroomsGiven);
            recomp_printf("...fixing:\n");
        }
        #endif

        mainSetBits(BIT_Inventory_White_Mushrooms, mushroomsCollected - mushroomsGiven);
        mainSetBits(BIT_SH_Queen_EW_White_Mushrooms_Eaten, mushroomsGiven);

        #if DEBUG_MUSHROOMS
        {
            recomp_printf("Total collected: %d\n", mushroomsCollected);
            recomp_printf("Total held: %d\n", mushroomsCollected - mushroomsGiven);
            recomp_printf("Total given: %d\n", mushroomsGiven);
        }
        #endif
        
        return;
    }

    recomp_printf("No contradictions found for White Mushrooms!\n");
}

extern int SHqueenearthwalker_animCallback(Object* self, Object* animObj, AnimObj_Data* animData, s8 prevCallbackValue);

RECOMP_PATCH void SHqueenearthwalker_obj_Setup(Object* self, Generic_Setup* setup, s32 reset) {
    SHqueenearthwalker_Data* objdata;

    objdata = self->data;
    self->srt.yaw = setup->yaw << 8;
    self->animCallback = SHqueenearthwalker_animCallback;
    objdata->numMushroomsEaten = mainGetBits(BIT_SH_Queen_EW_White_Mushrooms_Eaten);
    objdata->state = mainGetBits(BIT_SH_Queen_EW_Quest_Progress);

    // @recomp: Let the queen turn her head a little when the player approaches
    s16 yawRangeDeg[] = { 0, 25 };
    s16 pitchRangeDeg[] = { 0, 8 };
    mainCreateTempDLL(DLL_ID_MOVELIB);
    ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func2(self, &objdata->moveData, 
        /*pitch offset*/-0x1000, 
        /*turn range*/0x31C6, 
        /*numjoints*/2);
    ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func6(&objdata->moveData, yawRangeDeg, pitchRangeDeg, 2);
    
    objdata->moveData.unk4A9 |= 2;
}

RECOMP_PATCH void SHqueenearthwalker_obj_Control(Object* self) {
    SHqueenearthwalker_Data* objdata;
    s32 prevState;

    objdata = self->data;
    prevState = objdata->state;
    self->unkAF &= ~ARROW_FLAG_8_No_Targetting;

    //Handle animation
    {
        // @recomp: Switch animation state after being healed
        if (objdata->state >= SHqueenearthwalker_STATE_7_After_SpellStone_Sequence) {
            ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func0(self, &objdata->moveData);
            objExprEyeIdle(self, &objdata->headAnim);

            if (self->curModAnimId != 8) {
                objAnimSet(self, 8, 0.0f, 0);
            }
            objAnimAdvance(self, 0.005f, gUpdateRate, NULL);
        } else {
            // @recomp: Close eyes when not healed yet
            TextureAnimator* eyelidR = objExprGetTexAnimator(self, HEAD_ANIMATION_TAG_Eyelid_R, 0);
            TextureAnimator* eyelidL = objExprGetTexAnimator(self, HEAD_ANIMATION_TAG_Eyelid_L, 0);
            eyelidR->frame = MIN(eyelidR->frame + 0x100, 0x200);
            eyelidL->frame = MIN(eyelidL->frame + 0x100, 0x200);

            if (self->curModAnimId != 1) {
                objAnimSet(self, 1, 0.0f, 0);
            }
            objAnimAdvance(self, 0.005f, gUpdateRate, NULL);
        }
    }

    //State Machine
    switch (objdata->state) {
    case SHqueenearthwalker_STATE_1_Initial:
        //Advance state automatically
        objdata->state = SHqueenearthwalker_STATE_2_Before_Meeting_Sabre;
        break;
    case SHqueenearthwalker_STATE_2_Before_Meeting_Sabre:
        //Advance to the White Mushrooms quest state when talked to
        if (self->unkAF & ARROW_FLAG_1_Interacted) {
            joyDisableButtons(0, A_BUTTON);
            gDLL_3_Animation->vtbl->start_obj_sequence(1, self, -1);
            mainSetBits(BIT_SH_Move_Thorntail_Blocking_Hollow_Log, 1);
            objdata->state = SHqueenearthwalker_STATE_3_Needing_White_Mushrooms;
        }
        break;
    case SHqueenearthwalker_STATE_3_Needing_White_Mushrooms:
        if (self->unkAF & ARROW_FLAG_4_Highlighted) {
            //@recomp: when inventory opened, handle White Mushroom contradictions
            if (gDLL_1_cmdmenu->vtbl->get_page_category() == CMDMENU_CATEGORY_3_Items){
                if (!objdata->wasMenuOpen){
                    recomp_printf("CHECKING MUSHROOMS!\n");
                    handle_mushroom_gamebit_contradictions();
                }
                objdata->wasMenuOpen = TRUE;
            } else {
                objdata->wasMenuOpen = FALSE;
            }

            if (gDLL_1_cmdmenu->vtbl->was_this_item_used(BIT_Inventory_White_Mushrooms) != 0) {
                joyDisableButtons(0, A_BUTTON);
                objdata->numMushroomsEaten += mainGetBits(BIT_Inventory_White_Mushrooms);
                // @recomp: Require all ten white mushrooms instead of just one. (originally by MusicalProgrammer)
                if (objdata->numMushroomsEaten < 10) {
                    //Play a mushroom eating sequence
                    gDLL_3_Animation->vtbl->start_obj_sequence(3, self, -1);
                } else {
                    //Finish the quest after enough White Mushrooms have been fed
                    objdata->state = SHqueenearthwalker_STATE_4_Healed;
                    dll_task->MarkTaskCompleted(TASK_NUM(GAMETEXT_0FF_Task_011));
                    mainSetBits(BIT_SH_Move_Thorntail_Blocking_Swapstone, 1);
                }

                mainSetBits(BIT_Inventory_White_Mushrooms, 0);
                mainSetBits(BIT_SH_Queen_EW_White_Mushrooms_Eaten, objdata->numMushroomsEaten);
            } else if (self->unkAF & ARROW_FLAG_1_Interacted) {
                //Chat with the player
                joyDisableButtons(0, A_BUTTON);
                gDLL_3_Animation->vtbl->start_obj_sequence(4, self, -1);
            }
        }
        break;
    case SHqueenearthwalker_STATE_4_Healed:
        //Start a chain of sequences where the Queen asks Sabre to save Dinosaur Planet
        gDLL_3_Animation->vtbl->start_obj_sequence(2, self, -1);
        break;
    case SHqueenearthwalker_STATE_5_Discussing_Force_Point_Temples:
        gDLL_3_Animation->vtbl->start_obj_sequence(6, self, -1);
        break;
    case SHqueenearthwalker_STATE_6_Discussing_SpellStones:
        gDLL_3_Animation->vtbl->start_obj_sequence(7, self, -1);
        objdata->state = SHqueenearthwalker_STATE_7_After_SpellStone_Sequence;
        break;
    case SHqueenearthwalker_STATE_7_After_SpellStone_Sequence:
        break;
    default:
        //Advance state automatically
        objdata->state = SHqueenearthwalker_STATE_1_Initial;
        break;
    }

    if (prevState != objdata->state) {
        mainSetBits(BIT_SH_Queen_EW_Quest_Progress, objdata->state);
    }
}

RECOMP_PATCH void SHqueenearthwalker_obj_Print(Object* self, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    SHqueenearthwalker_Data* objdata = self->data;
    
    if (visibility) {
        objprintDrawModel(self, gdl, mtxs, vtxs, pols, 1.0f);
        // @recomp:
        ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func3(self, &objdata->moveData, /*attachpoint*/0);
    }
}

RECOMP_PATCH void SHqueenearthwalker_obj_Free(Object* self, s32 onlySelf) {
    // @recomp:
    mainRemoveTempDLL(DLL_ID_MOVELIB);
}

RECOMP_PATCH u32 SHqueenearthwalker_obj_GetDataSize(Object* self, u32 a1) {
    return sizeof(SHqueenearthwalker_Data);
}

static void recomp_SHqueenearthwalker_animEndCallback(Object* self, Object* animObj, AnimObj_Data* animData) {
    // Reset head/neck joints after cutscene ends
    s32* boneList = objExpr_func_800349B0();
    for (s32 i = 0; i < 2; i++){
        SeqJoint* joint = objExpr_func_80034804(self, boneList[i]);
        if (joint != NULL){
            joint->pitch = 0;
            joint->yaw = 0;
            joint->roll = 0;
        }
    }
}

RECOMP_HOOK_DLL(SHqueenearthwalker_animCallback) void SHqueenearthwalker_animCallback_hook(Object* self, Object* animObj, AnimObj_Data* animData, s8 prevCallbackValue) {
    // @recomp: Set custom anim end callback
    animData->unkF4 = recomp_SHqueenearthwalker_animEndCallback;
}
