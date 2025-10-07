#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "sys/dll.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"

#include "recomp/dlls/_asm/287_recomp.h"

typedef struct {
f32 x;
f32 z;
} CoordXZ;

typedef struct {
s32 unk0;
s8 unk4;
s8 unk5[0x8 - 0x5];
Object* objectsOnSwitch[10];
CoordXZ objCoords[10];
} PressureSwitchState;

typedef struct {
ObjCreateInfo base;
s16 unk18;
} PressureSwitchCreateInfo;

// Prevents the pressure switches' object arrays from overflowing and crashing (originally by MusicalProgrammer)
RECOMP_PATCH void dll_287_func_628(Object* self, Object* objectOnSwitch) {
    PressureSwitchState *state = self->state;
    u8 objectIndex;
    
    //@recomp: fix loop condition, and check if object already in list
    for (objectIndex = 0; objectIndex < 10; objectIndex++){
        if (state->objectsOnSwitch[objectIndex] == NULL || 
            state->objectsOnSwitch[objectIndex] == objectOnSwitch){
            break;
        }
    }
    
    state->objectsOnSwitch[objectIndex] = objectOnSwitch;    
    state->objCoords[objectIndex].x = objectOnSwitch->srt.transl.x;
    state->objCoords[objectIndex].z = objectOnSwitch->srt.transl.z;
}
