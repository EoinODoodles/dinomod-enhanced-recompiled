#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "sys/main.h"
#include "sys/segment_1050.h"
#include "sys/segment_1460.h"

#include "recomp/dlls/objects/408_IMIceMountain_recomp.h"

typedef struct {
    u8 state;
    u8 flags;
    s8 warpCounter;
} IMIceMountain_Data;

typedef enum {
    STATE_0,
    STATE_Rescue_Tricky,
    STATE_Race_Start_Sequence,
    STATE_Race,
    STATE_Race_Won,
    STATE_Race_Lost,
    STATE_Intro_Sequence
} IMIceMountain_State;

typedef enum {
    IMICEMOUNTAIN_FLAG_1 = 0x1
} IMIceMountain_Flag;

typedef enum {
    IM_ObjGroup0_AnimTricky = 0,
    IM_ObjGroup1 = 1,                           //Empty
    IM_ObjGroup2_AnimSpacecraft = 2,
    IM_ObjGroup3_Cave_Jetbikes_SharpClaw = 3,   //Player Jetbike, SharpClaw & Jetbike
    IM_ObjGroup4_Cave_Jetbikes_SharpClaw = 4,   //SharpClaw & Jetbike (closer to door)
    IM_ObjGroup5_Summit_Main = 5,
    IM_ObjGroup6_Track_Icicles = 6,             //Objects for the first stretch with icicle-covered arches
    IM_ObjGroup7_Summit_Jetbike_Sequences = 7   //Summit: SharpClaw racing sequences
} IM_ObjectGroups;

typedef enum {
    IM_Act1 = 1,
    IM_Act2 = 2,
    IM_Act3 = 3,
    IM_Act4 = 4
} IM_Acts;

extern void IMIceMountain_do_race(Object *self, IMIceMountain_Data *objdata);

/** 
  * Make sure the cave door is shut during the summit's initial gameplay.
  * (Hopefully allows Sabre's intro sequence to be skipped without consequences!) 
  */
RECOMP_PATCH void IMIceMountain_do_act1(Object *self) {
    IMIceMountain_Data *objdata;

    objdata = self->data;
    switch (objdata->state) {
    case STATE_Intro_Sequence:
        if (main_get_bits(BIT_Played_Seq_0063_IM_Sabre_Intro)) {
            objdata->state = STATE_Rescue_Tricky;
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, IM_ObjGroup2_AnimSpacecraft, 0);

            //@recomp: make sure Tricky's cave door is shut initially
            main_set_bits(BIT_382, 0);
        }
        break;
    case STATE_Rescue_Tricky:
        if (main_get_bits(BIT_IM_MultiSeq_2)) {
            objdata->state = STATE_Race_Start_Sequence;
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, IM_ObjGroup6_Track_Icicles, 1);
        }
        break;
    case STATE_Race_Start_Sequence:
        if (main_get_bits(BIT_IM_MultiSeq_3)) {
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, IM_ObjGroup0_AnimTricky, 0);
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, IM_ObjGroup5_Summit_Main, 0);
        }
        if (main_get_bits(BIT_IM_Race_Started)) {
            objdata->state = STATE_Race;
        }
        if (self->unkDC == 0) {
            func_80000860(self, self, 0xA3, 0);
            func_80000860(self, self, 0x9E, 0);
            func_80000860(self, self, 0x119, 0);
            func_80000450(self, self, 0x15B, 0, 0, 0);
            func_80000450(self, self, 0x15C, 0, 0, 0);
            func_80000450(self, self, 0x17C, 0, 0, 0);
            func_80000450(self, self, 0x17B, 0, 0, 0);
            gDLL_12_Minic->vtbl->func6(1);
            self->unkDC = 1;
        }
        break;
    case STATE_Race:
        IMIceMountain_do_race(self, objdata);
        break;
    case STATE_Race_Won:
        if (objdata->flags & IMICEMOUNTAIN_FLAG_1) {
            gDLL_29_Gplay->vtbl->set_map_setup(self->mapID, 2);
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, IM_ObjGroup3_Cave_Jetbikes_SharpClaw, 0);
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, IM_ObjGroup4_Cave_Jetbikes_SharpClaw, 0);
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, IM_ObjGroup6_Track_Icicles, 0);
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, IM_ObjGroup7_Summit_Jetbike_Sequences, 0);
            objdata->state = STATE_0;
        }
        break;
    case STATE_Race_Lost:
        if (objdata->flags & IMICEMOUNTAIN_FLAG_1) {
            objdata->warpCounter = 2;
        }

        if (objdata->warpCounter > 0) {
            objdata->warpCounter--;
            if (objdata->warpCounter == 0) {
                // race will restart after warp
                warpPlayer(WARP_IM_RACE_START, FALSE);
            }
        }
        break;
    case STATE_0:
        break;
    }
}