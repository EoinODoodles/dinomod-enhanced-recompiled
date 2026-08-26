#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "dlls/objects/210_player.h"
#include "game/objects/object_id.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"

#include "recomp/dlls/objects/786_WCTempleLift_recomp.h"

//TEMPORARY DEFINES
#define WCTempleLift_obj_Control dll_786_control
//END OF TEMPORARY DEFINES

typedef struct {
    f32 cooldownTimer;
    u8 state;
    /* RECOMP */
    s8 playerCheckInterval;
} WCTempleLift_Data;

typedef struct {
    ObjSetup base;
    s8 yaw;
} WCTempleLift_Setup;

typedef enum {
    Lift_STATE_Down,
    Lift_STATE_Up
} WCTempleLift_States;

typedef enum {
    WCTempleLift_OBJSEQ0_Up,
    WCTempleLift_OBJSEQ1_Down
} WCTempleLift_ObjSeqs;

RECOMP_PATCH void WCTempleLift_obj_Control(Object* self) {
    #define LIFT_MOVE_COOLDOWN 300
    #define LIFT_TERRAIN_TYPE 0x21
    WCTempleLift_Data* objdata;
    Object* player;
    s32 i;
    Object* obj;

    objdata = self->data;

    //Wait at least 5 seconds between lift movements
    objdata->cooldownTimer -= gUpdateRateF;
    if (objdata->cooldownTimer < 0.0f) {
        objdata->cooldownTimer = 0.0f;
    }

    //Move up/down when the player stands on the lift
    if (objdata->state == Lift_STATE_Down) {
        if (self->polyhits->unk10F > 0) {
            for (i = 0; i < self->polyhits->unk10F; i++) {
                obj = self->polyhits->unk100[i];
                if (obj->id == OBJ_Sabre) {
                    player = objGetPlayer();
                    if ((objdata->cooldownTimer <= 0.0f) && (((DLL_210_Player*)player->dll)->vtbl->func70(player) == LIFT_TERRAIN_TYPE)) {
                        gDLL_3_Animation->vtbl->start_obj_sequence(WCTempleLift_OBJSEQ0_Up, self, -1);
                        objdata->state = Lift_STATE_Up;
                        objdata->cooldownTimer = LIFT_MOVE_COOLDOWN;
                    }
                }
            }
        }
    } else {
        if (self->polyhits->unk10F > 0) {
            for (i = 0; i < self->polyhits->unk10F; i++) {
                obj = self->polyhits->unk100[i];
                if (obj->id == OBJ_Sabre) {
                    player = objGetPlayer();
                    if ((objdata->cooldownTimer <= 0.0f) && (((DLL_210_Player*)player->dll)->vtbl->func70(player) == LIFT_TERRAIN_TYPE)) {
                        gDLL_3_Animation->vtbl->start_obj_sequence(WCTempleLift_OBJSEQ1_Down, self, -1);
                        objdata->state = Lift_STATE_Down;
                        objdata->cooldownTimer = LIFT_MOVE_COOLDOWN;
                    }
                }
            }
        }

        //@recomp: move back down if the player is at the bottom of the temple (i.e. they fell off, or some desync occurred)
        if (objdata->playerCheckInterval == 0) {
            objdata->playerCheckInterval = 120;

            player = objGetPlayer();
            if (player) {
                if (-1130 < player->globalPosition.y && player->globalPosition.y < -878) {
                    gDLL_3_Animation->vtbl->start_obj_sequence(WCTempleLift_OBJSEQ1_Down, self, 1); //Mask out other actors
                    objdata->state = Lift_STATE_Down;
                    objdata->cooldownTimer = LIFT_MOVE_COOLDOWN;
                }
            }
        } else {
            objdata->playerCheckInterval -= gUpdateRate;
            if (objdata->playerCheckInterval < 0) {
                objdata->playerCheckInterval = 0;
            }
        }
    }
}
