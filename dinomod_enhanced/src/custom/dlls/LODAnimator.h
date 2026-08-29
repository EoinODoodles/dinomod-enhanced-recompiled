#pragma once

#include "game/objects/object.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 gridOffsetX;
/*19*/ s8 gridOffsetZ;
/*1A*/ s8 animatorID;
} LODAnimator_Setup;

void LODAnimator_ctor(void* dll);
void LODAnimator_dtor(void* dll);

DLL_INTERFACE(DLL_LODAnimator) {
/*:*/ DLL_INTERFACE_BASE(DLL_IObject);
/*07*/ void (*UpdateShapes)(Object* self, s32 showLOD);
/*08*/ s32 (*GetNearbyBlockIdx)(Object* self);
/*09*/ void (*ClearNearbyBlock)(Object* self);
/*10*/ s32 (*GetOwnBlockIdx)(Object* self);
/*11*/ void (*ClearOwnBlock)(Object* self);
/*12*/ void (*CheckIfNearbyBlockAppeared)(Object* self);
};

#define dll_lodAnimator(obj) (((DLL_LODAnimator*)obj->dll)->vtbl)

extern DLL_LODAnimator_Vtbl DLL_LODAnimator_vtbl;
