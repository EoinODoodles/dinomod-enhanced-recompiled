#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "sys/dll.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"

#include "recomp/dlls/_asm/781_recomp.h"

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
RECOMP_PATCH void dll_781_func_5DC(Object* self, Object* objectOnSwitch) {
    PressureSwitchState *state = self->state;
    u8 objectIndex;
    
    objectIndex = 0;
    //@recomp: fix broken loop condition
    while (state->objectsOnSwitch[objectIndex] && objectIndex < 4){ 
        objectIndex++; 
    }
    
    state->objectsOnSwitch[objectIndex] = objectOnSwitch;    
    state->objCoords[objectIndex].x = objectOnSwitch->srt.transl.x;
    state->objCoords[objectIndex].z = objectOnSwitch->srt.transl.z;
}
