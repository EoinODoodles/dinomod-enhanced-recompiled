#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "sys/objanim.h"

#include "recomp/dlls/objects/507_SHqueenearthwalker_recomp.h"

#define DEBUG_MUSHROOMS FALSE

typedef struct {
    u8 questProgress;
    u8 eatenWhiteMushrooms;
    u8 unk2;
    u8 unk3;
    /** RECOMP */
    u8 wasMenuOpen;
} SHqueenearthwalker_Data;

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
static void handle_mushroom_gamebit_contradictions(){
    u8 mushroomsCollected = 0;
    u8 mushroomsGiven;
    u8 mushroomsHeld;
    u8 i;

    //Check how many White Mushrooms have been collected (and no longer appear in the world)
    for (i = 0; i < ARRAYCOUNT(mushroomCollectionGamebits); i++){
        if (main_get_bits(mushroomCollectionGamebits[i])){
            mushroomsCollected++;
        }
    }

    //Get inventory mushrooms count and Queen's received mushroom count
    mushroomsHeld = main_get_bits(BIT_Inventory_White_Mushrooms);
    mushroomsGiven = main_get_bits(BIT_SH_Queen_EW_White_Mushrooms_Eaten);

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

        main_set_bits(BIT_Inventory_White_Mushrooms, mushroomsCollected - mushroomsGiven);
        main_set_bits(BIT_SH_Queen_EW_White_Mushrooms_Eaten, mushroomsGiven);

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

RECOMP_PATCH void SHqueenearthwalker_control(Object* self) {
    SHqueenearthwalker_Data* objdata;
    s32 prevQuestProgress;

    objdata = self->data;
    prevQuestProgress = objdata->questProgress;
    self->unkAF &= ~8;
    if (self->curModAnimId != 1) {
        func_80023D30(self, 1, 0.0f, 0);
    }
    func_80024108(self, 0.005f, gUpdateRate, NULL);
    switch (objdata->questProgress) {

    case 1:
        objdata->questProgress = 2;
        break;
    case 2:
        if (self->unkAF & 1) {
            joy_set_button_mask(0, A_BUTTON);
            gDLL_3_Animation->vtbl->start_obj_sequence(1, self, -1);
            main_set_bits(BIT_SH_Move_Thorntail_Blocking_Hollow_Log, 1);
            objdata->questProgress = 3;
        }
        break;
    case 3:
        //If arrow visible above Queen
        if (self->unkAF & 4) {
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
                joy_set_button_mask(0, A_BUTTON);
                objdata->eatenWhiteMushrooms += main_get_bits(BIT_Inventory_White_Mushrooms);
                // @recomp: Require all ten white mushrooms instead of just one. (originally by MusicalProgrammer)
                if (objdata->eatenWhiteMushrooms < 10) {
                    gDLL_3_Animation->vtbl->start_obj_sequence(3, self, -1);
                } else {
                    objdata->questProgress = 4U;
                    gDLL_30_Task->vtbl->mark_task_completed(0xB);
                    main_set_bits(BIT_SH_Move_Thorntail_Blocking_Swapstone, 1);
                }
                main_set_bits(BIT_Inventory_White_Mushrooms, 0);
                main_set_bits(BIT_SH_Queen_EW_White_Mushrooms_Eaten, objdata->eatenWhiteMushrooms);
            } else if (self->unkAF & 1) {
                joy_set_button_mask(0, A_BUTTON);
                gDLL_3_Animation->vtbl->start_obj_sequence(4, self, -1);
            }
        }
        break;
    case 4:
        gDLL_3_Animation->vtbl->start_obj_sequence(2, self, -1);
        break;
    case 5:
        gDLL_3_Animation->vtbl->start_obj_sequence(6, self, -1);
        break;
    case 6:
        gDLL_3_Animation->vtbl->start_obj_sequence(7, self, -1);
        objdata->questProgress = 7;
        break;
    case 7:
        break;
    default:
        objdata->questProgress = 1;
        break;
    }
    if (prevQuestProgress != objdata->questProgress) {
        main_set_bits(BIT_SH_Queen_EW_Quest_Progress, objdata->questProgress);
    }
}

RECOMP_PATCH u32 SHqueenearthwalker_get_data_size(Object *self, u32 a1) {
    return sizeof(SHqueenearthwalker_Data);
}
