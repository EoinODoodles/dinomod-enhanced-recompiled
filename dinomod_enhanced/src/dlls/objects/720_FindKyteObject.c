#include "modding.h"
#include "recomputils.h"
#include "common_objsetups.h"

#include "common.h"
#include "dlls/objects/common/sidekick.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/print.h"

#include "recomp/dlls/objects/720_FindKyteObject_recomp.h"

// #define DEBUG_KYTE_TIMEOUT

//TEMPORARY DEFINES
#define FindKyteObject_obj_Control FindKyteObject_control
#define FindKyteObject_obj_GetDataSize FindKyteObject_get_data_size
#define EnableCommand enable_command
//END OF TEMPORARY DEFINES

typedef struct {
/*00*/ f32 timer;
/*04*/ s32 flightCurve;
/*08*/ u8 state;
/*0C*/ CurveSetup* curveSetup;
/* RECOMP */
/*10*/ u8 timerStarted;
} FindKyteObject_Data;

typedef enum {
    FindKyteObject_STATE_0_Finding_CurveSetup,
    FindKyteObject_STATE_1_Show_Find_Command_When_Nearby,
    FindKyteObject_STATE_2_Using_Find,
    FindKyteObject_STATE_3_Finished
} FindKyteObject_States;

RECOMP_PATCH void FindKyteObject_obj_Control(Object* self) {
    CurveSetup* curveSetup;
    FindKyteObject_Setup* setup;
    Object* kyte;
    Object* player;
    f32 dist;
    s16 gamebit;
    FindKyteObject_Data* objdata;

    objdata = self->data;
    setup = (FindKyteObject_Setup*)self->setup;

#ifdef DEBUG_KYTE_TIMEOUT
    diPrintf("FindKyteObject_obj_Control | state: %d\n", objdata->state);
#endif

    switch (objdata->state) {
    case FindKyteObject_STATE_0_Finding_CurveSetup:
        curveSetup = gDLL_25->vtbl->func_2A50(self, setup->kyteFlightCurve);
        objdata->curveSetup = curveSetup;
        if (curveSetup) {
#ifdef DEBUG_KYTE_TIMEOUT
            recomp_printf("curve (%x) usedBit: %x (%d)\n", 
                curveSetup->uID, 
                curveSetup->type22.usedBit, 
                mainGetBits(curveSetup->type22.usedBit)
            );
#endif
            objdata->state = FindKyteObject_STATE_1_Show_Find_Command_When_Nearby;

            //@recomp: reset timer tracker
            objdata->timerStarted = FALSE;
        }
        break;
    case FindKyteObject_STATE_1_Show_Find_Command_When_Nearby:
        //If the curveSetup's "used" gamebit is specified and set, set Kyte's flight curve and go back to State 0
        gamebit = objdata->curveSetup->type22.usedBit;
        if ((gamebit != NO_GAMEBIT) && (mainGetBits(gamebit))) {
            mainSetBits(BIT_Kyte_Flight_Curve, objdata->flightCurve);
            objdata->state = FindKyteObject_STATE_0_Finding_CurveSetup;
            break;
        }

        //If Kyte's around and the player's near the object, enable the Find command option
        kyte = objGetSidekick();
        if (kyte) {
            player = objGetPlayer();
            if (setup->checkDistance2D) {
                dist = vec3DistanceXZSquared(&player->globalPosition, &self->globalPosition);
            } else {
                dist = vec3DistanceSquared(&player->globalPosition, &self->globalPosition);
            }

            if (dist <= SQ(setup->findRange * 2)) {
                //Enable Find command option
                ((DLL_ISidekick*)kyte->dll)->vtbl->EnableCommand(kyte, Sidekick_Command_INDEX_1_Find);

                //Advance state if Find command was used
                if (gDLL_1_cmdmenu->vtbl->was_this_item_used(Sidekick_Command_INDEX_1_Find)) {
                    objdata->flightCurve = mainGetBits(BIT_Kyte_Flight_Curve);
                    mainSetBits(BIT_Kyte_Flight_Curve, setup->kyteFlightCurve);

                    //Optionally set Kyte's talk sequence
                    if (setup->kyteTalkSeq != 0) {
                        mainSetBits(BIT_Kyte_Flight_Talk_Sequence, setup->kyteTalkSeq);
                    }
                    
                    objdata->timer = setup->timerSeconds * 60.0f;
                    objdata->state = FindKyteObject_STATE_2_Using_Find;

                    //@recomp: set up timer
                    if ((setup->flags & FindKyteObject_FLAG_4_Start_Timeout_When_Kyte_is_Nearby) == FALSE) {
                        objdata->timerStarted = TRUE;
                    }
                }
            }
        }
        break;
    case FindKyteObject_STATE_2_Using_Find:
        //@recomp: optionally wait until Kyte's nearby to start the timer
        if (objdata->timerStarted) {
            objdata->timer -= gUpdateRateF;
        } else if (setup->flags & FindKyteObject_FLAG_4_Start_Timeout_When_Kyte_is_Nearby) {
            kyte = objGetSidekick();
            if (kyte && (vec3DistanceXZSquared(&self->globalPosition, &kyte->globalPosition) < SQ(setup->findRange * 4))) {
                objdata->timerStarted = TRUE;
            }
        }

#ifdef DEBUG_KYTE_TIMEOUT
        diPrintf("timer: %f\n", &objdata->timer);
#endif

        //Revert to State 1 after a few seconds, or once the curveSetup's "used" gamebit is set
        if ((objdata->timer <= 0.0f) || 
            ((gamebit = objdata->curveSetup->type22.usedBit, (gamebit != NO_GAMEBIT)) && mainGetBits(gamebit))
        ) {
            //@recomp: optionally set the gamebit after the timer expired (if Kyte took too long)
            if (setup->flags & FindKyteObject_FLAG_2_Set_Gamebit_After_Timeout) {
                kyte = objGetSidekick();
                if (kyte && objdata->curveSetup && objdata->timer <= 0.0f) {
                    mainSetBits(objdata->curveSetup->type22.usedBit, TRUE);
                }
            }

            //Clear Kyte's talk sequence, if specified
            if (setup->kyteTalkSeq != 0) {
                mainSetBits(BIT_Kyte_Flight_Talk_Sequence, -1);
            }

            //Revert state
            objdata->state = FindKyteObject_STATE_1_Show_Find_Command_When_Nearby;

            //Set Kyte's flight curve
            if (mainGetBits(BIT_Kyte_Flight_Curve) == setup->kyteFlightCurve) {
                mainSetBits(BIT_Kyte_Flight_Curve, objdata->flightCurve);
            }
        }
        break;
    case FindKyteObject_STATE_3_Finished: //Inaccessible?
        break;
    }
}

/* Extend objData */
RECOMP_PATCH u32 FindKyteObject_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(FindKyteObject_Data);
}
