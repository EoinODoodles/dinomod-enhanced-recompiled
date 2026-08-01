#include "modding.h"

#include "game/objects/object.h"

#include "recomp/dlls/objects/781_WCpressureswitch_recomp.h"

//TEMPORARY DEFINES
#define WCPressureSwitch_obj_GetModelFlags WCpressureswitch_get_model_flags
#define WCPressureSwitch_addObject WCpressureswitch_add_object
//END OF TEMPORARY DEFINES

typedef struct {
f32 x;
f32 z;
} CoordXZ;

typedef struct {
/*00*/ u32 soundHandle;
/*04*/ s8 pressed;
/*05*/ s8 state;
/*08*/ Object* objectsOnSwitch[10];
/*30*/ CoordXZ objCoords[10];
} PressureSwitch_Data;

// Prevents the pressure switches' object arrays from overflowing and crashing (originally by MusicalProgrammer)
RECOMP_PATCH void WCPressureSwitch_addObject(Object* self, Object* objectOnSwitch) {
    PressureSwitch_Data *objdata = self->data;
    u8 objectIndex;
    
    //@recomp: fix loop condition, and check if object already in list
    for (objectIndex = 0; objectIndex < 10; objectIndex++){
        if (objdata->objectsOnSwitch[objectIndex] == NULL || 
            objdata->objectsOnSwitch[objectIndex] == objectOnSwitch){
            break;
        }
    }
    
    objdata->objectsOnSwitch[objectIndex] = objectOnSwitch;    
    objdata->objCoords[objectIndex].x = objectOnSwitch->srt.transl.x;
    objdata->objCoords[objectIndex].z = objectOnSwitch->srt.transl.z;
}

/* Ensure both models are loaded, so the Moon switch's polyHits work */
RECOMP_PATCH u32 WCPressureSwitch_obj_GetModelFlags(Object* self) {
    return MODFLAGS_1;
}
