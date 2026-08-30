#pragma once

#include "game/objects/object.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 gridOffsetX;  //The x position offset (in worldSpace grid units) to the Block that needs an LOD stand-in
/*19*/ s8 gridOffsetZ;  //The z position offset (in worldSpace grid units) to the Block that needs an LOD stand-in
/*1A*/ s8 animatorID;   //animatorID for the local block's LOD shapes, which will be shown/hidden as the nearby Block disappears/appears
/*1B*/ u8 options;      //See `LODAnimator_Options`
} LODAnimator_Setup;

typedef enum {
    LODAnimator_OPTION_1_Show_LOD_on_Unload = 1 //Show the LOD when the LODAnimator unloads (only if the LODAnimator is closer to the target Block than the player is)
} LODAnimator_Options;

void LODAnimator_ctor(void* dll);
void LODAnimator_dtor(void* dll);

DLL_INTERFACE(DLL_LODAnimator) {
/*:*/ DLL_INTERFACE_BASE(DLL_IObject);
/*07*/ s32 (*UpdateShapes)(Object* self, s32 showLOD);
/*08*/ s32 (*GetNearbyBlockIdx)(Object* self);
/*09*/ void (*ClearNearbyBlock)(Object* self);
/*10*/ s32 (*GetOwnBlockIdx)(Object* self);
/*11*/ void (*ClearOwnBlock)(Object* self);
/*12*/ void (*CheckIfNearbyBlockAppeared)(Object* self);
};

#define dll_lodAnimator(obj) (((DLL_LODAnimator*)obj->dll)->vtbl)

extern DLL_LODAnimator_Vtbl DLL_LODAnimator_vtbl;
