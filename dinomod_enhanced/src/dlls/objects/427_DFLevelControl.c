#include "dll.h"
#include "modding.h"
#include "recomp/dlls/objects/427_DFlevelcontrol_recomp.h"
#include "recomputils.h"

#include "objects/427_DFLevelControl.h"

#include "common.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "sys/math.h"
#include "sys/print.h"

#include "recomp/dlls/objects/427_DFlevelcontrol_recomp.h"

//TEMPORARY DEFINES
#define DFlevelcontrol_obj_Setup DFlevelcontrol_setup
#define DFlevelcontrol_obj_Control DFlevelcontrol_control
#define DFlevelcontrol_obj_GetDataSize DFlevelcontrol_get_data_size
#define DFlevelcontrol_initialise DFlevelcontrol_func_388

#define dll_gplay (gDLL_29_Gplay->vtbl)
//END OF TEMPORARY DEFINES

typedef struct {
/*00*/ u8 state;
/*01*/ u8 unk1;
/*02*/ u8 mapID;
/*03*/ u8 unk3;
/* RECOMP */
s8 prevObjGroup2Loaded;  //BWC rope
s8 prevObjGroup14Loaded; //Upper Falls rope
} DFlevelcontrol_Data;

typedef enum {
    DFLevelControl_STATE_0_Shrine_Door_Closed,
    DFLevelControl_STATE_1_Shrine_Door_Unlocked,
    DFLevelControl_STATE_2_Finished
} DFLevelControl_States;

extern void DFlevelcontrol_initialise(Object* self);

/* Handles Kyte's custom rope objGroups, ensuring they're synced with their original objGroup's load state and the rope's state gamebit. */
static void DFlevelcontrol_syncRopeObjGroups(DFlevelcontrol_Data* objData) {
    s8 objGroup2Loaded;
    s8 objGroup14Loaded;

    if (objData == NULL) {
        return;
    }

    objGroup2Loaded = dll_gplay->get_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup2_Lower_Falls);
    objGroup14Loaded = dll_gplay->get_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup14_Middle_and_Upper_Falls);

    //Rope near BWC
    if (objData->prevObjGroup2Loaded != objGroup2Loaded) {
        if (objGroup2Loaded) {
            if (mainGetBits(BIT_DF_Kyte_Secured_Rope_Near_BWC)) {
                dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_Rope_BWC_Attached, TRUE);
                dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_Rope_BWC_Detached, FALSE);
            } else {
                dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_Rope_BWC_Attached, FALSE);
                dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_Rope_BWC_Detached, TRUE);
            }
        } else {
            dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_Rope_BWC_Attached, FALSE);
            dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_Rope_BWC_Detached, FALSE);
        }

        objData->prevObjGroup2Loaded = objGroup2Loaded;
    }

    //Rope near Upper Falls
    if (objData->prevObjGroup14Loaded != objGroup14Loaded) {
        if (objGroup14Loaded) {
            if (mainGetBits(BIT_DF_Kyte_Secured_Rope_Upper_Falls)) {
                dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_Rope_Upper_Falls_Attached, TRUE);
                dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_Rope_Upper_Falls_Detached, FALSE);
            } else {
                dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_Rope_Upper_Falls_Attached, FALSE);
                dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_Rope_Upper_Falls_Detached, TRUE);
            }
        } else {
            dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_Rope_Upper_Falls_Attached, FALSE);
            dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_Rope_Upper_Falls_Detached, FALSE);
        }

        objData->prevObjGroup14Loaded = objGroup14Loaded;
    }
}

RECOMP_PATCH void DFlevelcontrol_obj_Setup(Object* self, ObjSetup* setup, s32 reset) {
    DFlevelcontrol_Data* objdata;

    objdata = self->data;
    if (mainGetBits(BIT_10D)) {
        objdata->state = DFLevelControl_STATE_2_Finished;
    } else {
        objdata->state = DFLevelControl_STATE_0_Shrine_Door_Closed;
    }

    objdata->unk1 = mainGetBits(BIT_342);
    mainSetBits(BIT_8DE, 1 - objdata->unk1);

    objdata->mapID = -1;

    //@recomp: handle Kyte's ropes
    DFlevelcontrol_syncRopeObjGroups(objdata);
}

RECOMP_PATCH void DFlevelcontrol_obj_Control(Object* self) {
    DFlevelcontrol_Data* objdata;
    Object* player;

    objdata = self->data;
    player = objGetPlayer();
    dll_amSfx->WaterFallsControl();

    //@recomp: handle Kyte's ropes
    DFlevelcontrol_syncRopeObjGroups(objdata);

    //Run the level's initialisation function when the player enters the map
    if (objdata->mapID != MAP_DISCOVERY_FALLS) {
        if (mapWorldXZToMapID(player->srt.transl.x, player->srt.transl.z) != MAP_DISCOVERY_FALLS) {
            return;
        }

        DFlevelcontrol_initialise(self);
    }
    objdata->mapID = mapWorldXZToMapID(player->srt.transl.x, player->srt.transl.z);

    if ((objdata->unk1 == FALSE) && (mainGetBits(BIT_342))) {
        mainSetBits(BIT_Kyte_Flight_Curve, 70);
        mainSetBits(BIT_8DE, FALSE);
        objdata->unk1 = TRUE;
    }

    //Shrine Door State Machine
    switch (objdata->state) {
    case DFLevelControl_STATE_0_Shrine_Door_Closed:
        if (mainGetBits(BIT_DF_Shrine_Door_Light_Activated_One) &&
            mainGetBits(BIT_DF_Shrine_Door_Light_Activated_Two) &&
            mainGetBits(BIT_DF_Shrine_Door_Light_Activated_Three) &&
            mainGetBits(BIT_DF_Shrine_Door_Light_Activated_Four)) {
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mapID, DF_ObjGroup11_Shrine_Door, TRUE);
            objdata->state++;
        }
        break;
    case DFLevelControl_STATE_1_Shrine_Door_Unlocked:
        if (gDLL_29_Gplay->vtbl->get_obj_group_status(self->mapID, DF_ObjGroup11_Shrine_Door)) {
            mainSetBits(BIT_4A1, TRUE);
            objdata->state++;
        }
        break;
    }
}

/* Extend objData */
RECOMP_PATCH u32 DFlevelcontrol_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(DFlevelcontrol_Data);
}
