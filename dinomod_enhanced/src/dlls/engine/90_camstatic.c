#include "modding.h"

#include "dlls/engine/2_camcontrol.h"

#include "recomp/dlls/engine/90_camstatic_recomp.h"

typedef struct {
    Object* obj;        //StaticCamera object
    u8 _unk4[0x8 - 0x4];
    f32 x;              //Ease's initial value
    f32 goalX;          //Ease's goal value
    f32 y;              //Ease's initial value
    f32 goalY;          //Ease's goal value
    f32 z;              //Ease's initial value
    f32 goalZ;          //Ease's goal value
    f32 yaw;            //Ease's initial value
    f32 goalYaw;        //Ease's goal value
    f32 pitch;          //Ease's initial value
    f32 goalPitch;      //Ease's goal value
    f32 roll;           //Ease's initial value
    f32 goalRoll;       //Ease's goal value
    f32 fov;            //Ease's initial value
    f32 goalFov;        //Ease's goal value
    Vec4f easeSpline;
    f32 easedDistance;  //Distance travelled so far while easing
    f32 goalDistance;   //Total distance that will be travelled, from the ease's start to end position
    u8 _unk58[0xF4 - 0x58];
    u8 easeInFinished;  //Ease into StaticCamera has finished
    u8 cameraLost;      //No StaticCamera found, swapping back to CamNormal
} CamStatic;

/*0x0*/ extern CamStatic* sState;

RECOMP_HOOK_DLL(camstatic_func_278) void camstatic_func_278_hook(Cam* cam) {
    // @recomp: If the static camera object unloads, switch back to the normal camera instead of referencing a dangling pointer
    if (sState->obj == NULL || (sState->obj->stateFlags & OBJSTATE_DESTROYED)) {
        sState->obj = NULL;
        sState->cameraLost = TRUE;
    }
}
