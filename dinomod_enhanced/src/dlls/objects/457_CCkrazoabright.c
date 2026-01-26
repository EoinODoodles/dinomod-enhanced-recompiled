#include "modding.h"

#include "common.h"

#include "recomp/dlls/objects/457_CCkrazoabright_recomp.h"

typedef struct {
    f32 timer;          //manages delay and blending strength for updating the Krazoa symbol
    f32 iconBlendTimer; //manages crossfading two sets of icon textures on Kyte's lever columns
    u8 updateNeeded;
    u8 state;
    u8 prevPoints[6];   //Booleans: apply colour to each point of courtyard's Krazoa symbol (clockwise from south-east point)
    u8 pointsLit[6];
} CCkrazoabright_Data;

typedef enum {
    STATE_Initialise = 0,
    STATE_Lever_Puzzle_One = 1,
    STATE_Lighting_Lanterns = 2,
    STATE_Krazoa_Tablet_Quest = 3,
    STATE_Lever_Puzzle_Two = 4,
    STATE_Finished = 5
} CCkrazoabright_States;

/** Change completed Cape Claw Puzzle #2 sequence to 04 (originally by MusicalProgrammer) */
RECOMP_PATCH void CCkrazoabright_handle_lever_puzzle_2(Object* self, CCkrazoabright_Data* objData) {
    s32 index;

    if (objData->updateNeeded) {
        //Handle resetting levers and gamebits after each pull
        objData->timer += gUpdateRateF;
        if (objData->timer > 300.0f) {
            main_set_bits(BIT_636, 0);
            main_set_bits(BIT_CC_Courtyard_Kyte_Pulled_4th_Lever, 0);
            main_set_bits(BIT_638, 0);
            main_set_bits(BIT_CC_Courtyard_Kyte_Pulled_3rd_Lever, 0);
            main_set_bits(BIT_4D5, 0);
            main_set_bits(BIT_CC_Courtyard_Kyte_Pulled_2nd_Lever, 0);
            main_set_bits(BIT_637, 0);
            main_set_bits(BIT_CC_Courtyard_Kyte_Pulled_1st_Lever, 0);
            objData->updateNeeded = FALSE;
        }
    } else {
        //Check if Kyte pulls levers (positions noted from left, while facing towards ocean)
        if (main_get_bits(BIT_CC_Courtyard_Kyte_Pulled_4th_Lever)) {
            objData->pointsLit[2] = objData->prevPoints[2] ^ 1;
            objData->pointsLit[3] = objData->prevPoints[3] ^ 1;
            objData->pointsLit[4] = objData->prevPoints[4] ^ 1;
            objData->updateNeeded = TRUE;
        } else if (main_get_bits(BIT_CC_Courtyard_Kyte_Pulled_3rd_Lever)) {
            objData->pointsLit[3] = objData->prevPoints[3] ^ 1;
            objData->pointsLit[4] = objData->prevPoints[4] ^ 1;
            objData->pointsLit[5] = objData->prevPoints[5] ^ 1;
            objData->updateNeeded = TRUE;
        } else if (main_get_bits(BIT_CC_Courtyard_Kyte_Pulled_2nd_Lever)) {
            objData->pointsLit[5] = objData->prevPoints[5] ^ 1;
            objData->pointsLit[0] = objData->prevPoints[0] ^ 1;
            objData->pointsLit[1] = objData->prevPoints[1] ^ 1;
            objData->pointsLit[2] = objData->prevPoints[2] ^ 1;
            objData->updateNeeded = TRUE;
        } else if (main_get_bits(BIT_CC_Courtyard_Kyte_Pulled_1st_Lever)) {
            objData->pointsLit[0] = objData->prevPoints[0] ^ 1;
            objData->pointsLit[1] = objData->prevPoints[1] ^ 1;
            objData->pointsLit[3] = objData->prevPoints[3] ^ 1;
            objData->pointsLit[4] = objData->prevPoints[4] ^ 1;
            objData->updateNeeded = TRUE;
        }

        //Show overhead view of Krazoa symbol
        if (objData->updateNeeded) {
            for (index = 0; index < 6; index++){
                if (!objData->pointsLit[index]){ 
                    break; 
                }
            }
            
            //Check if all points of the Krazoa symbol are lit
            if (index == 6) {
                // gDLL_3_Animation->vtbl->func17(5, self, -1);
                gDLL_3_Animation->vtbl->func17(4, self, -1); //@recomp: change sequence index
                objData->state = STATE_Finished;
                objData->timer = 0.0f;
                for (index = 0; index < 6; index++) { objData->pointsLit[index] = 0; }
                return;
            }

            gDLL_3_Animation->vtbl->func17(2, self, -1);
            objData->timer = 0.0f;
        }
    }
}
