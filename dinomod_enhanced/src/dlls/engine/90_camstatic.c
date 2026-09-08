#include "configs.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "core/map.h"
#include "common_objsetups.h"
#include "dll.h"
// #include "dlls/engine/90_camstatic.h"
#include "dlls/objects/210_player.h"
// #include "dlls/objects/715_StaticCamera.h"
#include "game/gamebits.h"
#include "sys/dll.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/memory.h"
#include "sys/print.h"

#include "recomp/dlls/engine/90_camstatic_recomp.h"

// #define DEBUG_EASE

//TEMPORARY DEFINES
typedef struct {
    s32 cameraID;   //The cameraID tag to search for in StaticCamera objects' setup structs
    u8 previousCameraEasesIn; //Don't use the StaticCamera's own easing function for the initial ease in
} CamStatic_Params;

#define camstatic_setupEase camstatic_func_5D4
#define camstatic_ease camstatic_func_798
#define camstatic_findStaticCamera camstatic_func_C04
//END OF TEMPORARY DEFINES

#define PLAYER_CENTRE_HEIGHT 20

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

extern void camstatic_setupEase(Cam* cam, Vec3f* staticCamCoords, s32 goalYaw, s32 goalPitch, s32 goalRoll, f32 goalFov);
extern s32 camstatic_ease(Cam* cam, u8 flags);
extern Object* camstatic_findStaticCamera(f32 x, f32 y, f32 z, s32 cameraID, s32 controlNo);

/* Add option to aim at player's centre rather than their origin. */
RECOMP_PATCH void camstatic_func_18(Cam* cam, s32 arg1, CamStatic_Params* data) {
    Object* staticCam;
    StaticCamera_Setup* camSetup;
    f32 dx;
    f32 dy;
    f32 dz;
    Object* player;
    s16 yaw;
    s16 pitch;
    s16 roll;
    f32 fov;

    player = cam->player;

    sState = mmAlloc(sizeof(CamStatic), ALLOC_TAG_CAM_COL, ALLOC_NAME("camstatic"));
    sState->easeInFinished = TRUE;
    sState->cameraLost = FALSE;
    
    staticCam = camstatic_findStaticCamera(player->srt.transl.x, player->srt.transl.y, player->srt.transl.z, data->cameraID, OBJCONTROL_StaticCamera);
    if (staticCam == NULL) {
        sState->cameraLost = TRUE;
        return;
    }

    sState->obj = staticCam;
    camSetup = (StaticCamera_Setup*)staticCam->setup;
    dx = staticCam->globalPosition.x - player->globalPosition.x;
    dy = staticCam->globalPosition.y - player->globalPosition.y;
    dz = staticCam->globalPosition.z - player->globalPosition.z;

    //@recomp: optionally aim at centre of player
    if (camSetup->flags & CamStatic_FLAG_Aim_at_Player_Centre) {
        dy -= PLAYER_CENTRE_HEIGHT;
    }

    if (camSetup->flags & CamStatic_FLAG_Aim_Yaw_at_Player) {
        yaw = M_180_DEGREES - mathAtan2f(dx, dz);
    } else {
        yaw = camSetup->yaw + M_180_DEGREES;
    }

    if (camSetup->flags & CamStatic_FLAG_Aim_Pitch_at_Player) {
        pitch = (mathAtan2f(dy, sqrtf(SQ(dx) + SQ(dz))) & 0xFFFF & 0xFFFF) - camSetup->pitch;
    } else {
        pitch = camSetup->pitch;
    }

    if (camSetup->flags & CamStatic_FLAG_Use_Player_Roll) {
        roll = player->srt.roll;
    } else {
        roll = camSetup->roll;
    }

    fov = camSetup->fov;

    if (data->previousCameraEasesIn == FALSE) {
        //StaticCamera manages the initial ease in
        camstatic_setupEase(cam, &staticCam->globalPosition, yaw, pitch, roll, fov);
    } else {
        cam->srt.transl.x = staticCam->globalPosition.x;
        cam->srt.transl.y = staticCam->globalPosition.y;
        cam->srt.transl.z = staticCam->globalPosition.z;
        cam->srt.yaw = yaw;
        cam->srt.pitch = pitch;
        cam->srt.roll = roll;
        cam->fov = fov;
    }
}

/* Add option to aim at player's centre rather than their origin. */
RECOMP_PATCH void camstatic_func_278(Cam* cam) {
    StaticCamera_Setup* camSetup;
    s32 rollDiff;
    s32 pitchDiff;
    f32 dx;
    f32 dy;
    f32 dz;
    Object* player;
    s32 easeFinished;

    if (sState->cameraLost) {
        gDLL_2_Camera->vtbl->change_camera_module(DLL_ID_CAMNORMAL, FALSE, 1, 0, NULL, 0, Cam_Ease_All);
        return;
    }

    player = cam->player;
    camSetup = (StaticCamera_Setup*)sState->obj->setup;

    if (!(camSetup->flags & CamStatic_FLAG_Aim_Yaw_at_Player)) {
        cam->srt.yaw = camSetup->yaw + M_180_DEGREES;
    }
    if (!(camSetup->flags & CamStatic_FLAG_Aim_Pitch_at_Player)) {
        cam->srt.pitch = camSetup->pitch;
    }
    if (!(camSetup->flags & CamStatic_FLAG_Use_Player_Roll)) {
        cam->srt.roll = camSetup->roll;
    }

    cam->srt.transl.x = sState->obj->globalPosition.x;
    cam->srt.transl.y = sState->obj->globalPosition.y;
    cam->srt.transl.z = sState->obj->globalPosition.z;
    cam->fov = camSetup->fov;

    //Apply easing
    if (sState->easeInFinished == FALSE) {
        easeFinished = camstatic_ease(cam, camSetup->flags);
        if (easeFinished) {
            sState->easeInFinished = TRUE;
        }
    }

    dx = cam->srt.transl.x - player->globalPosition.x;
    dy = cam->srt.transl.y - player->globalPosition.y;
    dz = cam->srt.transl.z - player->globalPosition.z;

    //@recomp: optionally aim at centre of player
    if (camSetup->flags & CamStatic_FLAG_Aim_at_Player_Centre) {
        dy -= PLAYER_CENTRE_HEIGHT;
    }

    if (camSetup->flags & CamStatic_FLAG_Aim_Yaw_at_Player) {
        cam->srt.yaw = M_180_DEGREES - mathAtan2f(dx, dz);
    }

    if (camSetup->flags & CamStatic_FLAG_Aim_Pitch_at_Player) {
        pitchDiff = (mathAtan2f(dy, sqrtf(SQ(dx) + SQ(dz))) - camSetup->pitch) - (cam->srt.pitch & 0xFFFF);
        CIRCLE_WRAP(pitchDiff);
        cam->srt.pitch += (pitchDiff * gUpdateRate) >> 3;
    }

    if (camSetup->flags & CamStatic_FLAG_Use_Player_Roll) {
        rollDiff = cam->srt.roll - (player->srt.roll & 0xFFFF);
        CIRCLE_WRAP(rollDiff);
        cam->srt.roll += (rollDiff * gUpdateRate) >> 3;
    }
}

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
