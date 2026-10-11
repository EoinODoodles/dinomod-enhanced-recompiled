#include "configs.h"
#include "math_util.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "PR/os.h"
#include "dll.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/engine/84_camnormal.h"
#include "dlls/engine/85_attentioncam.h"
#include "dlls/engine/86_cam1stperson.h"
#include "dlls/objects/210_player.h"
#include "game/gamebits.h"
#include "game/objects/object_id.h"
#include "sys/dll.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/print.h"

#include "recomp/dlls/engine/84_camnormal_recomp.h"

typedef struct {
    f32 unk0;
    f32 unk4;
    f32 unk8;
    f32 unkC;
    f32 unk10;
    f32 unk14;
    f32 unk18;
    f32 unk1C;
    f32 unk20;
    f32 unk24;
    f32 unk28;
    f32 unk2C;
    f32 unk30;
    f32 unk34;
    f32 unk38;
    f32 unk3C;
    f32 unk40;
    f32 unk44;
    f32 unk48;
    f32 unk4C;
    f32 unk50;
    f32 unk54;
    f32 unk58;
    f32 unk5C;
    f32 unk60;
    f32 unk64;
    f32 unk68;
    f32 unk6C;
    f32 unk70;
    f32 unk74;
    f32 unk78;
    f32 unk7C;
    s16 unk80;
    s16 unk82;
    s16 unk84;
    s16 unk86;
    s16 unk88;
    s16 unk8A;
    f32 unk8C;
    f32 unk90;
    f32 unk94;
    f32 unk98;
    f32 unk9C;
    f32 unkA0;
    f32 unkA4;
    s32 unkA8;
    u8 unkAC;
    u8 unkAD;
    u8 unkAE;
    u8 _unkAF[0xB0 - 0xAF];
} CamNormal;

extern CamNormal* sState;

// #define DEBUG_LOG_FOLLOW_Z
static s32 rsLogZPrevYaw = 0;          //Used to check how quickly player's turning
static s32 rsLogZHoldTimer = 0;        //How long Z's been held (eventually caps out)
static u8 rsLogZPlayedWhoosh = FALSE;  //Whether a "whoosh" was played due to a big camera swing, during the current button hold
static u8 rsLogZFlags = 0;             //Whether a "whoosh" was played due to a big camera swing, during the current button hold
static f32 rsLogZOrbitRadius = 0;      //Current orbital radius
static f32 rsLogZOrbitY = 0;           //Current orbital height
static s32 rsLogZOrbitYaw = 0;         //Current orbital angle
static s32 rsLogZGoalYaw = 0;          //Goal orbital angle (behind the player, plus a bias based on how sharply they're turning)
static s16 rsLogZOrbitYawSpeed = 0;    //Easing: rate of change of orbit yaw
static f32 rsLogZOrbitRadiusSpeed = 0; //Easing: rate of change of orbit radius
static f32 rsLogZOrbitYSpeed = 0;      //Easing: rate of change of orbit height
#define CAM_BEHIND_BUTTON_TAP_THRESHOLD 10
#define CAM_BEHIND_MAX_EASE_TIME 40
#define CAM_BEHIND_FINISH_MOVE_TIME 60
#define CAM_BEHIND_FINISH_ANGLE_THRESHOLD (DEGREES_TO_ANGLE16(1))
#define CAM_BEHIND_FINISH_ANGLE_SPEED_THRESHOLD (DEGREES_TO_ANGLE16(1)/4)

typedef enum {
    LogFollowZ_FLAG_Initialised_Orbit = 1,
    LogFollowZ_FLAG_Played_Whoosh = 2,
} LogFollowZ_Flags;

//TODO: move to playerUtil
static _Bool isPlayerOnLog(Object* player) {
    Object* vehicle;
    if (player == NULL) {
        return FALSE;
    }

    vehicle = ((DLL_210_Player*)player->dll)->vtbl->get_vehicle(player);
    if (vehicle && vehicle->id == OBJ_BWLog) {
        return TRUE;
    }

    return FALSE;
}

/** 
  * Used to keep the camera behind the log while holding Z, sort of like holding R in Banjo-Kazooie! 
  * Helpful if you wanted to lock in for some racing challenge (if there was one for the log).
  */
void logFollowZ_putCamBehind(Cam* cam, Object* player, s32 easeTimer) {
    f32 unk0;
    f32 unk4;
    f32 unk8;
    f32 unkC;
    f32 distance;
    f32 sin;
    f32 cos;
    Vec3f initialPos;
    Vec3f goalPos;
    AABBs32 aaabb;
    TrackIntersectResult result;
    CamControl_Module* camnormal;
    s32 yawDiff;

    camnormal = gDLL_2_Camera->vtbl->get_camnormal_module();
    if (camnormal == NULL) {
        return;
    }
    ((DLL_84_camnormal*)camnormal->dll)->vtbl->func7(
        &unk0, &unk4, &unk8, 0, &unkC);

    //Store the player's initial yaw
    if ((rsLogZFlags & LogFollowZ_FLAG_Initialised_Orbit) == FALSE) {
        rsLogZPrevYaw = player->srt.yaw;
    }

    //Calculate orbital yaw goal, adding a bias based on how much the player's turning, so 
    //that the camera points more towards where the player wants to go.
    if (joyGetButtons(0) & Z_TRIG) { //NOTE: goal only updated if the player's actually holding Z, so the release ends more smoothly
        yawDiff = player->srt.yaw - rsLogZPrevYaw;
        CIRCLE_WRAP(yawDiff);
        if (gUpdateRate) {
            yawDiff /= gUpdateRate;
        }
        rsLogZGoalYaw = player->srt.yaw + 30 * yawDiff;
        CIRCLE_WRAP(rsLogZGoalYaw);
    }

    //Get the camera's initial orbital yaw around the player
    if ((rsLogZFlags & LogFollowZ_FLAG_Initialised_Orbit) == FALSE) {
        rsLogZFlags |= LogFollowZ_FLAG_Initialised_Orbit;

        //Get camera's orbital angle
        rsLogZOrbitYaw = mathAtan2(cam->srt.transl.x - player->globalPosition.x, cam->srt.transl.z - player->globalPosition.z);

        //Get initial lateral radius too, and initial orbit height
        {
            goalPos.x = cam->srt.transl.x - player->globalPosition.x;
            goalPos.z = cam->srt.transl.z - player->globalPosition.z;
            rsLogZOrbitRadius = sqrtf(SQ(goalPos.x) + SQ(goalPos.z));
            rsLogZOrbitY = cam->srt.transl.y - player->globalPosition.y;
        }

        //Play a "whoosh" if there's a big angular jump between the current and goal yaw, similar to `attentioncam`
        if ((rsLogZFlags & LogFollowZ_FLAG_Played_Whoosh) == FALSE) {
            yawDiff = rsLogZGoalYaw - rsLogZOrbitYaw;
            CIRCLE_WRAP(yawDiff);
            if (yawDiff < 0) {
                yawDiff *= -1;
            }
            if (yawDiff > M_45_DEGREES) {
                dll_amSfx->Play(NULL, SOUND_1E, MAX_VOLUME, NULL, NULL, 0, NULL);
                rsLogZFlags |= LogFollowZ_FLAG_Played_Whoosh;
            }
        }
    }

#ifdef DEBUG_LOG_FOLLOW_Z
    diPrintf("currentYaw: %4x\n", rsLogZOrbitYaw);
    diPrintf("goalYaw: %4x\n", rsLogZGoalYaw);
#endif

    //Limit ease timer
    if (easeTimer > CAM_BEHIND_MAX_EASE_TIME) {
        easeTimer = CAM_BEHIND_MAX_EASE_TIME;
    }

    //Ease the orbital yaw around behind the player
    rsLogZOrbitYaw = dampedSmoothAngleToFrom(rsLogZOrbitYaw, rsLogZGoalYaw, &rsLogZOrbitYawSpeed, easeTimer);
    
    //Ease the orbital radius/height too
    distance = sqrtf(SQ(unk4) - SQ(unk8));
    rsLogZOrbitRadius = dampedSmoothToFrom(rsLogZOrbitRadius, distance, &rsLogZOrbitRadiusSpeed, easeTimer);
    rsLogZOrbitY = dampedSmoothToFrom(rsLogZOrbitY, unkC + unk8, &rsLogZOrbitYSpeed, easeTimer);

    //Calculate goal coords, based on current orbital yaw
    sin = mathSinfInterp(rsLogZOrbitYaw);
    cos = mathCosfInterp(rsLogZOrbitYaw);
    goalPos.x = player->globalPosition.x + rsLogZOrbitRadius * sin;
    goalPos.y = player->globalPosition.y + rsLogZOrbitY;
    goalPos.z = player->globalPosition.z + rsLogZOrbitRadius * cos;

    //Do an intersect check to keep the camera's goal coords inbounds
    initialPos.x = player->globalPosition.x;
    initialPos.y = player->globalPosition.y + unkC;
    initialPos.z = player->globalPosition.z;
    result.unk40[0] = 4.5f;
    result.unk50[0] = -1;
    result.unk54[0] = 3;
    trackIntersectBuildAABB(&aaabb, &initialPos, &goalPos, &result.unk40[0], 1);
    trackIntersectBroadphase(player, &aaabb, 0);
    trackGetIntersect(player, initialPos.f, goalPos.f, 1, &result, 0);

    cam->srt.transl.x = goalPos.x;
    cam->srt.transl.y = goalPos.y;
    cam->srt.transl.z = goalPos.z;

    //Aim camera yaw at the player (like `attentioncam`)
    {
        f32 dx;
        f32 dy;
        f32 dz;
        f32 distance2D;
        gDLL_2_Camera->vtbl->get_player_to_camera_distances(cam, &dx, &dy, &dz, &distance2D, 0.0f);
        s32 yawDiff = (-mathAtan2f(dx, dz) - (cam->srt.yaw & 0xFFFF)) + M_180_DEGREES;
        CIRCLE_WRAP(yawDiff);
        cam->srt.yaw += yawDiff;
    }

    //Store yaw to get the turn speed on the next tick
    rsLogZPrevYaw = player->srt.yaw;
}

/* Checks if the camera should follow the player while holding Z on the log */
static _Bool logFollowZ_shouldFollowCamBeUsed(u8 configEnabled, Cam* cam, Object* player) {
    //Always false if the Z-button log follow config is off
    if (configEnabled == FALSE) {
        return FALSE;
    }

    //Check if attentioncam's cam flag condition is met
    if (cam->srt.flags & OBJFLAG_UNK_4) {
        return FALSE;
    }

    //Don't do anything if the player's off the log
    if (isPlayerOnLog(player) == FALSE) {
        return FALSE;
    }

#ifdef DEBUG_LOG_FOLLOW_Z
    diPrintf("rsLogZHoldTimer: %d\n", rsLogZHoldTimer);
    diPrintf("rsLogZOrbitYaw: %x\n", rsLogZOrbitYaw & 0xFFFF);
    diPrintf("rsLogZOrbitYawSpeed: %x\n", rsLogZOrbitYawSpeed & 0xFFFF);
#endif

    //Check if the player's holding Z while on the log
    if (joyGetButtons(0) & Z_TRIG) {
        return TRUE;
    }

    //Use the log follow camera if a significant camera move is still active (only within the first second of holding Z)
    if ((rsLogZHoldTimer > CAM_BEHIND_BUTTON_TAP_THRESHOLD) && (rsLogZHoldTimer < CAM_BEHIND_FINISH_MOVE_TIME)) {
        //Continue camera move if there's still significant yaw speed
        s32 yawDiff = rsLogZOrbitYawSpeed;
        if (yawDiff < 0) {
            yawDiff *= -1;
        }
        if (yawDiff > CAM_BEHIND_FINISH_ANGLE_SPEED_THRESHOLD) {
            return TRUE;
        }

        //Continue camera move if there's still a significant difference between current/goal orbital yaw
        yawDiff = (rsLogZGoalYaw & 0xFFFF) - (rsLogZOrbitYaw & 0xFFFF);
        CIRCLE_WRAP(yawDiff);
        if (yawDiff < 0) {
            yawDiff *= -1;
        }

#ifdef DEBUG_LOG_FOLLOW_Z
        diPrintf("yawDiff: %d\n", yawDiff & 0xFFFF);
#endif

        if (yawDiff > CAM_BEHIND_FINISH_ANGLE_THRESHOLD) {
            return TRUE;
        }
    }

    return FALSE;
}

/** Resets everything to do with the log follow camera
  * @param checkFlags - When nonzero, only reset if the log follow camera has initialised (i.e. Z was held longer than a button tap)
  */
static void logFollowZ_reset(_Bool checkFlags) {
    if (checkFlags && (rsLogZFlags & LogFollowZ_FLAG_Initialised_Orbit) == FALSE) {
        return;
    }
    rsLogZHoldTimer = 0;
    rsLogZFlags = 0;
    rsLogZOrbitYawSpeed = 0;
    rsLogZOrbitRadiusSpeed = 0;
    rsLogZOrbitYSpeed = 0;
}

/* If the Z-button log follow camera is being used, checks if the player held and released the Z button quickly
   enough for it to be considered a button tap (which causes `attentioncam` to be used as usual, while on the log) */
static _Bool logFollowZ_buttonHoldBecameButtonTap(u8 configEnabled) {
    u16 heldBtns;

    //Always false if the Z-button log follow config is off
    if (configEnabled == FALSE) {
        return FALSE;
    }

    heldBtns = joyGetButtons(0);

    //Check if Z is still being held (no button tap)
    if (heldBtns & Z_TRIG) {
        return FALSE;
    }

    //Check if the Z hold timer was released within a short enough time to be considered a tap
    if (0 < rsLogZHoldTimer && rsLogZHoldTimer <= CAM_BEHIND_BUTTON_TAP_THRESHOLD) {
        logFollowZ_reset(FALSE);
        return TRUE;
    }

    return FALSE;
}

/** Increments the Z button timer, and activates the log follow camera if Z's held long enough not to be considered a button tap. */
static void logFollowZ_tick(Cam* cam, Object* player) {
    //Tick the Z hold timer
    rsLogZHoldTimer += gUpdateRate;
    if (rsLogZHoldTimer > CAM_BEHIND_FINISH_MOVE_TIME) {
        rsLogZHoldTimer = CAM_BEHIND_FINISH_MOVE_TIME;
    }

#ifdef DEBUG_LOG_FOLLOW_Z
    if (rsLogZHoldTimer > CAM_BEHIND_BUTTON_TAP_THRESHOLD) {
        diPrintf("Z FOLLOW ACTIVATED!\n");
    } else {
        diPrintf("Z FOLLOW TIMER: %d\n", rsLogZHoldTimer);
    }
#endif

    //If the player's holding Z long enough for it not to be considered a button tap, follow the player
    if (rsLogZHoldTimer > CAM_BEHIND_BUTTON_TAP_THRESHOLD) {
        logFollowZ_putCamBehind(cam, player, rsLogZHoldTimer);
    }
}

/* (Optionally) keep the camera behind the player while holding Z on the log. 
 * TODO: maybe this could be revisited, converting the Z log follow stuff into a new camera module DLL? */
RECOMP_PATCH void camnormal_func_26E4(Cam* cam, Object* player) {
    u16 btns;
    AttentionCam_Params attentionCam;
    Cam1stPerson_Params cam1stPerson;
    u8 gamebitValue;
    /* RECOMP */
    u8 logFollowZEnabled = configs_LogFollowZ();
    u8 logFollowZInUse = FALSE;

    if (player->animObj) {
        return;
    }

    btns = joyGetPressed(0);

    if (((cam->highlight != NULL) || (cam->srt.flags & OBJFLAG_UNK_2)) && 
            !(cam->highlightFlags & 2) && 
            (((gamebitValue = mainGetBits(BIT_4AD), (gamebitValue == 0)) && (btns & Z_TRIG)) || ((gamebitValue != 0) && (btns & R_TRIG)) || (cam->targetFlags & 2)) && 
            (player->controlNo == OBJCONTROL_Player) && 
            (((DLL_210_Player*)player->dll)->vtbl->func60(player))) {
        gDLL_2_Camera->vtbl->change_camera_module(DLL_ID_CAMLOCKON, TRUE, 0, sizeof(&cam->highlight), &cam->highlight, 0x3C, Cam_Ease_All);
    } else if ((btns & U_CBUTTONS) && !(cam->highlightFlags & 1)) {
        cam1stPerson.unk0 = sState->unk4;
        cam1stPerson.unk4 = sState->unk8;
        cam1stPerson.unk8 = (s16) (u32) sState->unk8C;
        gDLL_2_Camera->vtbl->change_camera_module(DLL_ID_CAM1STPERSON, TRUE, 0, sizeof(cam1stPerson), &cam1stPerson, 0, Cam_Ease_All);
    } else if ((logFollowZInUse = logFollowZ_shouldFollowCamBeUsed(logFollowZEnabled, cam, player))) { //@recomp: handle optional Z-button log follow
        logFollowZ_tick(cam, player);
    } else if (
        (btns & Z_TRIG || logFollowZ_buttonHoldBecameButtonTap(logFollowZEnabled)) && //@recomp: handle optional Z-button log follow (Z hold becoming Z tap)
        !(cam->srt.flags & OBJFLAG_UNK_4)
    ) {
        attentionCam.unk0 = 5;
        attentionCam.unk2 = 1;
        attentionCam.unk3 = 1;
        gDLL_2_Camera->vtbl->change_camera_module(DLL_ID_ATTENTIONCAM, TRUE, 0, sizeof(attentionCam), &attentionCam, 0, Cam_Ease_All);
    }

    //@recomp: handle optional Z-button log follow
    if (logFollowZEnabled && (logFollowZInUse == FALSE)) {
        logFollowZ_reset(TRUE);
    }
}
