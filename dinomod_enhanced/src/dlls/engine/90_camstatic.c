#include "configs.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "core/map.h"
#include "dll.h"
#include "dlls/engine/90_camstatic.h"
#include "dlls/objects/210_player.h"
// #include "dlls/objects/715_StaticCamera.h"
#include "game/gamebits.h"
#include "sys/dll.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/print.h"

#include "recomp/dlls/engine/90_camstatic_recomp.h"

// #define DEBUG_EASE

//TEMPORARY DEFINES
typedef struct {
    ObjSetup base;
    u8 cameraID;
    u8 unk19;
    u8 fov;
    u8 flags;
    s16 yaw;
    s16 pitch;
    s16 roll;
} StaticCamera_Setup;

typedef enum {
    CamStatic_FLAG_Aim_Yaw_at_Player = 1,
    CamStatic_FLAG_Aim_Pitch_at_Player = 2,
    CamStatic_FLAG_Use_Player_Roll = 4
} CamStatic_Flags;

#define camstatic_ease camstatic_func_798
//END OF TEMPORARY DEFINES

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

/* Add missing FOV easing, to prevent a jarring pop when switching between cameras with different FOVs. */
RECOMP_PATCH s32 camstatic_ease(Cam* cam, u8 flags) {
    f32 tValue;
    f32 speed;

    sState->goalX = cam->srt.transl.x;
    sState->goalY = cam->srt.transl.y;
    sState->goalZ = cam->srt.transl.z;
    sState->goalYaw = cam->srt.yaw;
    sState->goalPitch = cam->srt.pitch;
    sState->goalRoll = cam->srt.roll;
    sState->goalFov = cam->fov;

    tValue = sState->easedDistance / sState->goalDistance;
    if (tValue > 1.0f) {
        tValue = 1.0f;
    }

#ifdef DEBUG_EASE
    diPrintf("camStatic ease: %f\n", &tValue);
#endif

    speed = curvesHermite(sState->easeSpline.f, tValue, NULL);
    if (speed < 0.2f) {
        speed = 0.2f;
    }

    sState->easedDistance += speed * gUpdateRateF;
    tValue = sState->easedDistance / sState->goalDistance;
    if (tValue > 1.0f) {
        tValue = 1.0f;
    }

    cam->srt.transl.x = curvesLinear(&sState->x, tValue, NULL);
    cam->srt.transl.y = curvesLinear(&sState->y, tValue, NULL);
    cam->srt.transl.z = curvesLinear(&sState->z, tValue, NULL);

    if (((sState->yaw - sState->goalYaw) > M_180_DEGREES) || ((sState->yaw - sState->goalYaw) < -M_180_DEGREES)) {
        if (sState->yaw < 0.0f) {
            sState->yaw += M_360_DEGREES - 1;
        } else if (sState->goalYaw < 0.0f) {
            sState->goalYaw += M_360_DEGREES - 1;
        }
    }

    if (((sState->pitch - sState->goalPitch) > M_180_DEGREES) || ((sState->pitch - sState->goalPitch) < -M_180_DEGREES)) {
        if (sState->pitch < 0.0f) {
            sState->pitch += M_360_DEGREES - 1;
        } else if (sState->goalPitch < 0.0f) {
            sState->goalPitch += M_360_DEGREES - 1;
        }
    }

    if (((sState->roll - sState->goalRoll) > M_180_DEGREES) || ((sState->roll - sState->goalRoll) < -M_180_DEGREES)) {
        if (sState->roll < 0.0f) {
            sState->roll += M_360_DEGREES - 1;
        } else if (sState->goalRoll < 0.0f) {
            sState->goalRoll += M_360_DEGREES - 1;
        }
    }

    if (!(flags & CamStatic_FLAG_Aim_Yaw_at_Player)) {
        cam->srt.yaw = curvesLinear(&sState->yaw, tValue, NULL);
    }

    if (!(flags & CamStatic_FLAG_Aim_Pitch_at_Player)) {
        cam->srt.pitch = curvesLinear(&sState->pitch, tValue, NULL);
    }

    if (!(flags & CamStatic_FLAG_Use_Player_Roll)) {
        cam->srt.roll = curvesLinear(&sState->roll, tValue, NULL);
    }

    //@recomp: ease FOV as well
    cam->fov = curvesLinear(&sState->fov, tValue, NULL);

    return tValue >= 1.0f;
}
