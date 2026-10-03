#include "configs.h"
#include "custom_gamebits.h"
#include "modding.h"
#include "recomputils.h"

#include "objects/427_DFLevelControl.h"

#include "common.h"
#include "dll.h"
#include "dlls/objects/210_player.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "sys/math.h"
#include "sys/objects.h"
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
/*01*/ u8 openedWhirlpoolCave;
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
            if (mainGetBits(BIT_DF_Played_Seq_0016_Kyte_Secures_Rope_Near_BWC)) {
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
            if (mainGetBits(BIT_DF_Played_Seq_000F_Kyte_Secures_Rope_Upper_Falls)) {
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

/* Handles the various config modes for DFmoondoor */
static void DFlevelcontrol_handleMoonDoor(void) {
    u8 mode;
    
    //Return early if the door's already open
    if (mainGetBits(DINOMOD_BIT_972_DF_Open_Door_to_MMP) || 
        mainGetBits(DINOMOD_BIT_973_DF_Opened_Door_to_MMP)
    ) {
        //Make sure Kyte can fly through the open door
        if (mainGetBits(DINOMOD_BIT_975_DF_Kyte_Can_Fly_Through_Door_to_MMP) == FALSE) {
            mainSetBits(DINOMOD_BIT_975_DF_Kyte_Can_Fly_Through_Door_to_MMP, TRUE);
        }
        return;
    }

    mode = configs_GetDFMoonDoorMode();

    //Hide the door if the config has it switched off
    if (mode == DFMOONDOOR_MODE_OFF) {
        if (dll_gplay->get_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_MMP_Door)) {
            dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_MMP_Door, FALSE); 
        }

        //Make sure Kyte can fly through where the door would be
        if (mainGetBits(DINOMOD_BIT_975_DF_Kyte_Can_Fly_Through_Door_to_MMP) == FALSE) {
            mainSetBits(DINOMOD_BIT_975_DF_Kyte_Can_Fly_Through_Door_to_MMP, TRUE);
        }
        return;
    }

    //Otherwise, show the door if the tunnel objGroup is loaded
    if (dll_gplay->get_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_Tunnel_to_MMP)) {
        dll_gplay->set_obj_group_status(MAP_DISCOVERY_FALLS, DF_ObjGroup_MMP_Door, TRUE); 
    }

    //Check if the player passed through the TriggerPlane in front of the door
    if (mainGetBits(DINOMOD_BIT_974_DF_Query_Open_Door_to_MMP)) {
        mainSetBits(DINOMOD_BIT_974_DF_Query_Open_Door_to_MMP, FALSE);

        //Open the door if the config's conditions are met
        switch (mode) {
        case DFMOONDOOR_MODE_ON_IMMEDIATE:
            mainSetBits(DINOMOD_BIT_972_DF_Open_Door_to_MMP, TRUE);
            mainSetBits(DINOMOD_BIT_975_DF_Kyte_Can_Fly_Through_Door_to_MMP, TRUE);
            break;
        case DFMOONDOOR_MODE_ON_SPIRIT_1:
            if (mainGetBits(BIT_DF_Played_Seq_003C_Krystal_Returns_From_The_Shrine)) {
                mainSetBits(DINOMOD_BIT_972_DF_Open_Door_to_MMP, TRUE);
                mainSetBits(DINOMOD_BIT_975_DF_Kyte_Can_Fly_Through_Door_to_MMP, TRUE);
            }
            break;
        case DFMOONDOOR_MODE_ON_MMP_KEY:
            if (mainGetBits(BIT_CRF_Prison_Key_2)) { //TODO: this is MMP's key, but it's mislabelled in the enum
                mainSetBits(DINOMOD_BIT_972_DF_Open_Door_to_MMP, TRUE);
                mainSetBits(DINOMOD_BIT_975_DF_Kyte_Can_Fly_Through_Door_to_MMP, TRUE);
            }
            break;
        }
    }

    //If the door isn't open, make sure Kyte can't fly through it
    if (!(mainGetBits(DINOMOD_BIT_972_DF_Open_Door_to_MMP) || mainGetBits(DINOMOD_BIT_973_DF_Opened_Door_to_MMP))) {
        if (mainGetBits(DINOMOD_BIT_975_DF_Kyte_Can_Fly_Through_Door_to_MMP)) {
            mainSetBits(DINOMOD_BIT_975_DF_Kyte_Can_Fly_Through_Door_to_MMP, FALSE);
        }
    }
}

RECOMP_PATCH void DFlevelcontrol_obj_Setup(Object* self, ObjSetup* setup, s32 reset) {
    DFlevelcontrol_Data* objdata = self->data;

    if (mainGetBits(BIT_DF_Shrine_Door_Opened)) {
        objdata->state = DFLevelControl_STATE_2_Finished;
    } else {
        objdata->state = DFLevelControl_STATE_0_Shrine_Door_Closed;

        //@recomp: hide the SharpClaw behind the Shrine door, if it's not open yet
        mainSetBits(BIT_DF_Defeated_Shrine_Entrance_SharpClaw, TRUE);
    }

    objdata->openedWhirlpoolCave = mainGetBits(BIT_DF_Demolition_Cave_Destroyed_Whirlpool_Wall_4);
    mainSetBits(BIT_DF_8DE, 1 - objdata->openedWhirlpoolCave);

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

    //@recomp: handle DFmoondoor
    DFlevelcontrol_handleMoonDoor();

    //Run the level's initialisation function when the player enters the map
    if (objdata->mapID != MAP_DISCOVERY_FALLS) {
        if (mapWorldXZToMapID(player->srt.transl.x, player->srt.transl.z) != MAP_DISCOVERY_FALLS) {
            return;
        }

        DFlevelcontrol_initialise(self);
    }
    objdata->mapID = mapWorldXZToMapID(player->srt.transl.x, player->srt.transl.z);

    if ((objdata->openedWhirlpoolCave == FALSE) && (mainGetBits(BIT_DF_Demolition_Cave_Destroyed_Whirlpool_Wall_4))) {
        mainSetBits(BIT_Kyte_Flight_Curve, 70);
        mainSetBits(BIT_DF_8DE, FALSE);
        objdata->openedWhirlpoolCave = TRUE;
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

            //@recomp: unhide the SharpClaw inside the Shrine entrance
            mainSetBits(BIT_DF_Defeated_Shrine_Entrance_SharpClaw, FALSE);
        }
        break;
    case DFLevelControl_STATE_1_Shrine_Door_Unlocked:
        if (gDLL_29_Gplay->vtbl->get_obj_group_status(self->mapID, DF_ObjGroup11_Shrine_Door)) {
            mainSetBits(BIT_DF_Seq_0035_Shrine_Door_Opens, TRUE);
            objdata->state++;
        }
        break;
    }
}

/* Extend objData */
RECOMP_PATCH u32 DFlevelcontrol_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(DFlevelcontrol_Data);
}
