#include "math_util.h"
#include "modding.h"

#include "dll.h"
#include "dlls/objects/210_player.h"

#include "recomp/dlls/engine/100_camspellaim_recomp.h"
#include "sys/print.h"

//TEMPORARY DEFINES
#define camspellaim_Control camspellaim_func_64
#define camspellaim_Free camspellaim_func_2C8

#define dPlayerOffsetY data_0
#define sOrbitDistanceInitial bss_0
#define sOrbitDistance bss_4
//END OF TEMPORARY DEFINES

/*0x0*/ extern f32 dPlayerOffsetY;

/*0x0*/ extern f32 sOrbitDistanceInitial;
/*0x4*/ extern f32 sOrbitDistance;

/* Very similar to `camspellaim_calculateIntersectDistance`, but the camera's goal coordinates are just
   brought inbounds instead (along the radius between the vertically offset player origin and the goal coords). */
static void camspellaim_calculateIntersect(Cam* cam, Object* player, Vec3f* coords) {
    f32 dx;
    f32 dz;
    Vec3f initial;
    Vec3f goal;
    AABBs32 aabb;
    TrackIntersectResult result;

    goal.x = coords->x;
    goal.y = coords->y;
    goal.z = coords->z;
    initial.x = player->globalPosition.x;
    initial.y = player->globalPosition.y + dPlayerOffsetY;
    initial.z = player->globalPosition.z;
    result.unk50[0] = -1;
    result.unk54[0] = 4;
    result.unk40[0] = 4.5f;
    trackIntersectBuildAABB(&aabb, &initial, &goal, &result.unk40[0], 1);
    trackIntersectBroadphase(player, &aabb, 1);
    if (trackGetIntersect(player, initial.f, goal.f, 1, &result, 0)) {
        coords->x = goal.x;
        coords->y = goal.y;
        coords->z = goal.z;
    }
}

/* Stop the camera clipping through walls, and fade out the player when the camera's really close */
RECOMP_PATCH void camspellaim_Control(Cam* cam) {
    Object* player;
    f32 temp_ft5;
    f32 distanceLateral;
    f32 distanceVertical;
    f32 sinYaw;
    f32 cosYaw;
    f32 cosPitch;
    f32 sinPitch;
    s16 yawSpeed;
    s16 pitchSpeed;
    Vec3f orbitOrigin;

    player = cam->player;

    distanceVertical = sOrbitDistanceInitial;
    cam->highlightFlags |= 2;

    sOrbitDistance = distanceVertical;

    ((DLL_210_Player*)player->dll)->vtbl->func62(player, &yawSpeed, &pitchSpeed);
    yawSpeed = ((yawSpeed >> 1) - player->srt.yaw) + M_180_DEGREES;
    pitchSpeed >>= 1;
    orbitOrigin.x = player->srt.transl.x;
    orbitOrigin.y = player->srt.transl.y + dPlayerOffsetY;
    orbitOrigin.z = player->srt.transl.z;
    
    yawSpeed -= (cam->srt.yaw & 0xFFFF);
    CIRCLE_WRAP(yawSpeed);
    cam->srt.yaw += (yawSpeed * gUpdateRate) >> 3;

    pitchSpeed -= (cam->srt.pitch & 0xFFFF);
    CIRCLE_WRAP(pitchSpeed);
    cam->srt.pitch += (pitchSpeed * gUpdateRate) >> 3;

    sinYaw = mathSinfInterp((cam->srt.yaw - M_90_DEGREES));
    cosYaw = mathCosfInterp((cam->srt.yaw - M_90_DEGREES));
    cosPitch = mathCosfInterp(cam->srt.pitch);
    sinPitch = mathSinfInterp(cam->srt.pitch);

    distanceVertical = sOrbitDistance;
    distanceLateral = distanceVertical * cosPitch;

    cam->srt.transl.x = orbitOrigin.x + (distanceLateral * cosYaw);
    cam->srt.transl.y = orbitOrigin.y + (distanceVertical * sinPitch);
    cam->srt.transl.z = orbitOrigin.z + (distanceLateral * sinYaw);

    //@recomp: check intersect continuously to avoid the camera swinging out through walls
    camspellaim_calculateIntersect(cam, player, &cam->srt.transl);

    //@recomp: fade out the player when really close
    if (vec3DistanceXZSquared(&orbitOrigin, &cam->srt.transl) < SQ(20)) {
        #define MIN_OPACITY 30

        if (player->opacity > MIN_OPACITY) {
            player->opacity = UINT_SAFE_SUBTRACT_WITH_FLOOR(player->opacity, gUpdateRate * 16, MIN_OPACITY);
        }
    } else {
        if (player->opacity < OBJECT_OPACITY_MAX) {
            player->opacity = UINT_SAFE_ADD(player->opacity, gUpdateRate * 16, 0xFF);
        }
    }
}

/* Make sure player's opacity is restored when exiting the camera */
RECOMP_PATCH void camspellaim_Free(Cam* cam) {
    if (cam->player) {
        cam->player->opacity = OBJECT_OPACITY_MAX;
    }
}
