#include "common_objsetups.h"
#include "configs.h"
#include "custom_gamebits.h"
#include "math_util.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "core/map.h"
#include "dll.h"
#include "dlls/engine/84_camnormal.h"
#include "dlls/objects/210_player.h"
#include "game/gamebits.h"
#include "sys/dll.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/memory.h"
#include "sys/objects.h"
#include "sys/print.h"

#include "recomp/dlls/engine/93_camclimb_recomp.h"

// #define DEBUG_EASE

//TEMPORARY DEFINES
#define camclimb_setup camclimb_func_18
#define camclimb_control camclimb_func_340
#define camclimb_ease camclimb_func_63C

#define M_10_DEGREES (M_5_DEGREES * 2)

typedef struct {
    s8 unk0;
    s8 easeDuration;
    s8 distance;
    s8 pitchOffset;
    s8 maxY;
    s8 minY;
} CamClimb_Params;
//END OF TEMPORARY DEFINES

typedef struct {
    f32 desiredDistance;    //Desired lateral distance from the player (eased value)
    f32 distance;           //Camera's lateral distance from the player (actual current value)
    f32 speedY;             //Rate of change of y offset
    f32 minY;               //Lower bound for camera's y difference relative to the player
    f32 maxY;               //Upper bound for camera's y difference relative to the player
    f32 distanceInitial;    //For easing: y difference relative to the player (start)
    f32 distanceGoal;       //For easing: y difference relative to the player (end)
    f32 minYInitial;        //For easing: y offset lower bound (start)
    f32 minYGoal;           //For easing: y offset lower bound (end)
    f32 maxYInitial;        //For easing: y offset upper bound (start)
    f32 maxYGoal;           //For easing: y offset upper bound (end)
    s16 easeTimer;          //Decrementing ease timer
    s16 easeDuration;       //Length of ease in
    u16 pitchOffset;        //Additive camera pitch value
    u16 pitchOffsetInitial; //For easing: pitch offset (start)
    s16 pitchOffsetGoal;    //For easing: pitch offset (end)
    /* RECOMP */
    s16 orbitYaw;
    s16 orbitSpeed;
    f32 speedX;
    f32 speedZ;
} CamClimb;

/*0x0*/ extern CamClimb* sState;

extern void camclimb_ease(Cam* cam);

RECOMP_PATCH void camclimb_setup(Cam* cam, s32 mode, CamClimb_Params* data) {
    s32 _pad;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 distance;
    s32 _pad2;
    f32 distanceMin;
    f32 distanceMax;
    f32 minY;
    f32 maxY;
    f32 currentY;
    CamControl_Module* camnormal;

    if (sState == NULL) {
        sState = mmAlloc(sizeof(CamClimb), ALLOC_TAG_CAM_COL, ALLOC_NAME("camclimb"));
    }
    
    if ((mode != 1) && (mode == 2)) {
        //Cut to camera
        sState->pitchOffsetInitial = sState->pitchOffset;
        sState->minYInitial = sState->minY;
        sState->maxYInitial = sState->maxY;
        sState->distanceInitial = sState->desiredDistance;
        sState->pitchOffsetGoal = data->pitchOffset * M_1_DEGREE_F;
        sState->minYGoal = data->minY;
        sState->maxYGoal = data->maxY;
        sState->distanceGoal = data->distance;
        sState->easeTimer = data->easeDuration;
        sState->easeDuration = data->easeDuration;
        return;
    }
    bzero(sState, sizeof(CamClimb));

    camnormal = gDLL_2_Camera->vtbl->get_camnormal_module();
    ((DLL_84_camnormal*)camnormal->dll)->vtbl->func7(&distanceMin, &distanceMax, &minY, &maxY, &currentY);
    gDLL_2_Camera->vtbl->get_player_to_camera_distances(cam, &dx, &dy, &dz, &distance, sState->pitchOffset);

    sState->pitchOffsetInitial = currentY;
    sState->minYInitial = minY;
    sState->maxYInitial = maxY;
    sState->distanceInitial = distance;
    sState->pitchOffsetGoal = M_10_DEGREES;
    sState->minYGoal = 0.0f;
    sState->maxYGoal = 0.0f;
    sState->distanceGoal = (distanceMax + distanceMin) * 0.5f;
    sState->easeTimer = 60;
    sState->easeDuration = 60;
    sState->distance = distance;
    sState->speedY = 0.05f;

    //@recomp: handle yaw differently
    {
        sState->orbitSpeed = 0;
        Object* player = objGetPlayer();
        if (player) {
            sState->orbitYaw = player->srt.yaw; //Snap to initial yaw

            //Snap to initial position
            cam->srt.transl.x = (mathSinfInterp(player->srt.yaw) * sState->distance) + player->srt.transl.x;
            cam->srt.transl.z = (mathCosfInterp(player->srt.yaw) * sState->distance) + player->srt.transl.z;
        }
    }
}

/**
  * - Adds a way to bring the camera closer to the player (when gamebit `DINOMOD_BIT_96C_CamClimb_Closer` is set)
  * - Adds easing as the player moves between differently-angled climbable surfaces.
  */
RECOMP_PATCH void camclimb_control(Cam* cam) {
    Object* player;
    s32 angleDiff;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 distance;
    f32 maxY;
    f32 minY;
    /* RECOMP */
    u8 initialEaseFinished = sState->easeTimer == 0;
    u8 useCloserCamera = mainGetBits(DINOMOD_BIT_96C_CamClimb_Closer);
    u8 angleEaseDivisor = initialEaseFinished ? 32 : 6;
    f32 distEaseSpeed = initialEaseFinished ? 0.0125f : 0.05f;
    f32 newX;
    f32 newZ;

    player = cam->player;

    camclimb_ease(cam);

    //Set camera Y (eased)
    {
        maxY = player->srt.transl.y + sState->maxY;
        minY = player->srt.transl.y + sState->minY;

        //@recomp: raise camera slightly when closer
        if (useCloserCamera) {
            maxY += 20.0f;
            minY += 20.0f;
        }

        if (minY > cam->srt.transl.y) {
            dy = minY - cam->srt.transl.y;
        } else if (maxY < cam->srt.transl.y) {
            dy = maxY - cam->srt.transl.y;
        } else {
            dy = 0.0f;
        }
        dy *= (sState->speedY * gUpdateRateF);
        cam->srt.transl.y += dy;
    }
    
    //Set camera X and Z from orbit yaw and desired camera distance
    {
        //@recomp: add a way to bring the camera closer during the climb (to avoid situations where it'd clip out of surroundings)
        if (useCloserCamera) {
            distance = (sState->desiredDistance * 0.75f) - sState->distance;
        } else {
            distance = sState->desiredDistance - sState->distance;
        }
        distance *= distEaseSpeed * gUpdateRateF;
        sState->distance += distance;

        //@recomp: use easing for orbital yaw 
        //(to avoid sudden snap when the player moves across to a differently-angled climb surface)
        angleDiff = player->srt.yaw - sState->orbitYaw;
        if (initialEaseFinished && angleDiff) {
            sState->orbitYaw = dampedSmoothAngleToFrom(sState->orbitYaw, player->srt.yaw, &sState->orbitSpeed, 30);
        
            //Ease position too, just during the angle ease
            newX = (mathSinfInterp(sState->orbitYaw) * sState->distance) + player->srt.transl.x;
            newZ = (mathCosfInterp(sState->orbitYaw) * sState->distance) + player->srt.transl.z;

            if (newX != cam->srt.transl.x) {
                cam->srt.transl.x = dampedSmoothToFrom(cam->srt.transl.x, newX, &sState->speedX, 5);
            }
            if (newZ != cam->srt.transl.z) {
                cam->srt.transl.z = dampedSmoothToFrom(cam->srt.transl.z, newZ, &sState->speedZ, 5);
            }
        } else {
            cam->srt.transl.x = (mathSinfInterp(player->srt.yaw) * sState->distance) + player->srt.transl.x;
            cam->srt.transl.z = (mathCosfInterp(player->srt.yaw) * sState->distance) + player->srt.transl.z;
        }
    }

    gDLL_2_Camera->vtbl->get_player_to_camera_distances(cam, &dx, &dy, &dz, &distance, 0.0f);
    
    //Set aim yaw
    angleDiff = -mathAtan2f(dx, dz) - (cam->srt.yaw & 0xFFFF);
    angleDiff += M_180_DEGREES;
    CIRCLE_WRAP(angleDiff);
    if (initialEaseFinished) {
        cam->srt.yaw += (s32) (angleDiff * gUpdateRate) / angleEaseDivisor; //@recomp: use easing
    } else {
        cam->srt.yaw += angleDiff;
    }
    
    //Set pitch
    if (useCloserCamera) {
        angleDiff = (mathAtan2f(dy, distance) - sState->pitchOffset - DEGREES_TO_ANGLE16(12.5)) - (cam->srt.pitch & 0xFFFF);
    } else {
        angleDiff = (mathAtan2f(dy, distance) - sState->pitchOffset) - (cam->srt.pitch & 0xFFFF);
    }
    CIRCLE_WRAP(angleDiff);
    if (initialEaseFinished) {
        cam->srt.pitch += (s32) (angleDiff * gUpdateRate) / angleEaseDivisor;
    } else {
        cam->srt.pitch += angleDiff; //@recomp: no pitch ease after initial ease in
    }
}
