#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "sys/dll.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"

#include "recomp/dlls/objects/287_pressureswitch_recomp.h"

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
RECOMP_PATCH void pressureswitch_add_object(Object* self, Object* objectOnSwitch) {
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
