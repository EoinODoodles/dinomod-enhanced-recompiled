#include "math_util.h"
#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "sys/camera.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objprint.h"
#include "sys/print.h"
#include "sys/rand.h"
#include "sys/vi.h"

#include "recomp/dlls/_asm/784_recomp.h"

// #define DEBUG_OCCLUSION

//TEMPORARY DEFINES
#define WCTrexStatue_obj_Print dll_784_obj_Print
#define WCTrexStatue_obj_Update dll_784_obj_Update
#define WCTrexStatue_obj_GetDataSize dll_784_obj_GetDataSize
#define PARTICLE_73F 0x73F
#define PARTICLE_740 0x740
//END OF TEMPORARY DEFINES

typedef enum {
    DepthFlag_1_Visible = 1,
    DepthFlag_2_New_Check_Needed = 2
} DepthFlags;

/* @recomp: custom data struct */
typedef struct {
    u8 depthFlags;
    s8 depthCheckInterval;
    s32 prevZDepthObj;
} WCTrexStatue_Data;

/* 
    Checks whether WCTrexStatue's sparkling tooth is visible and not occluded behind something else, 
    similar to WallTorch's code for this (staggered since it's expensive).
*/
static void WCTrexStatue_checkToothOcclusion(Object* self) {
#ifdef DINOMOD_ROM_PATCH
    #define DEPTH_CHECK_INVERVAL 60
#else
    #define DEPTH_CHECK_INVERVAL 0
#endif

    WCTrexStatue_Data* objData;
    Vec3f point;
    Vec3f projected;
    Vec3f uView;
    s32 screenX;
    s32 screenY;
    s32 zDepth;
    s32 zDepthOcclude;
    SRT fxTransform;

    objData = self->data;

#ifdef DEBUG_OCCLUSION
    if (self->modelInstIdx == 0) {
        diPrintf("prevZDepthObj: %d\n", objData->prevZDepthObj);
        diPrintf("depthCheckInterval: %d\n", objData->depthCheckInterval);
    }
#endif

    //Return early if a check isn't needed yet
    if ((objData->depthFlags & DepthFlag_2_New_Check_Needed) == FALSE) {
        return;
    }

    //Stagger depth checks (unless prev obj z-depth is still unknown)
    if (objData->depthCheckInterval > 0) {
        objData->depthCheckInterval -= gUpdateRate;
        if (objData->prevZDepthObj) {
            return;
        }
    } else {
        objData->depthCheckInterval = DEPTH_CHECK_INVERVAL;
        objData->prevZDepthObj = 0;
    }

    point.x = self->srt.transl.x - gWorldX;
    point.y = self->srt.transl.y + 135.0f; //Checking at roughly the height the tooth should be at
    point.z = self->srt.transl.z - gWorldZ;
    camProjectPoint(point.x, point.y, point.z, &projected.x, &projected.y, &projected.z);
    camClipToScreen(projected.x, projected.y, projected.z, &screenX, &screenY, NULL);
    zDepthOcclude = viObjDepth(screenX, screenY, self);

    //Use the last nonzero obj depth value
    if (zDepthOcclude == 0) {
        zDepthOcclude = objData->prevZDepthObj;
    } else {
        objData->prevZDepthObj = zDepthOcclude;
    }

    if (zDepthOcclude == 0) {
        return;
    }

    camGetVec3ToCameraNormalized(self->srt.transl.x, self->srt.transl.y, self->srt.transl.z, &uView.x, &uView.y, &uView.z);
    camProjectPoint(point.x += (uView.x * 20.0f), point.y += (uView.y * 20.0f), point.z += (uView.z * 20.0f), &projected.x, &projected.y, &projected.z);
    camClipToScreen(projected.x, projected.y, projected.z, NULL, NULL, &zDepth);

    if (viContainsPoint(screenX, screenY) && (zDepth > 0) && (zDepth < zDepthOcclude)) {
        objData->depthFlags |= DepthFlag_1_Visible;
    } else {
        objData->depthFlags &= ~DepthFlag_1_Visible;
    }

#ifdef DEBUG_OCCLUSION
    if (self->modelInstIdx == 0) {
        vec3_diPrintf(&point);
        vec3_diPrintf(&projected);
        diPrintf("viContainsPoint: %d\n", viContainsPoint(screenX, screenY));
        diPrintf("screenX: %d\t screenY: %d\n", screenX, screenY);
        diPrintf("zDepth: %d, zDepthOcclude: %d, visible: %s\n", zDepth, zDepthOcclude, (objData->depthFlags & DepthFlag_1_Visible) ? "YES" : "NO");
    }
#endif

    objData->depthFlags &= ~DepthFlag_2_New_Check_Needed;
}

/* 
    The RedEye Statues create sparkle effects when their missing tooth has been placed, 
    but this effect draws at any distance and shows up through walls. This patch adds
    some position/occlusion checks to determine whether to hide the sparkles.
*/
RECOMP_PATCH void WCTrexStatue_obj_Print(Object* self, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    /* RECOMP */
    WCTrexStatue_Data* objData;
    Camera* cam;

    if (visibility) {
        objprintDrawModel(self, gdl, mtxs, vtxs, pols, 1.0f);
    }
    
    /* @recomp: obj_Update's sparkle particle code moved here, to avail of this function's built-in visibility checks */

    WCTrexStatue_checkToothOcclusion(self);

    //Return early if the statue shouldn't sparkle yet
    if ((self->unkDC == FALSE) || (mathRnd(0, 5) != 0)) {
        return;
    }

    //Get the camera
    cam = camGetMain();
    if (cam == NULL) {
        return;
    }

    //Broadphase checks
    {
        // Bail if the camera is behind the statue (outside the lobby room)
        if (self->srt.transl.z + 65.0f <= cam->srt.transl.z) {
            return;
        }
    
        // Bail if the camera is too far ahead of the statue
        if (self->srt.transl.z - 700.0f >= cam->srt.transl.z) {
            return;
        }

        // Bail if the camera is too far above the statue (no check for camera too far below, since it's unlikely)
        if (cam->srt.transl.y > self->srt.transl.y + 240.0f) {
            return;
        }

        // Bail if the camera is too far to the side
        if (self->srt.transl.x - 290.0f > cam->srt.transl.x || cam->srt.transl.x > self->srt.transl.x + 290.0f) {
            return;
        }
    }

    //Occlusion check (expensive!) (Only check occlusion when the camera is potentially in the corridor outside the lobby)
    if (self->srt.transl.z - 323.0f > cam->srt.transl.z) {
        objData = self->data;
        objData->depthFlags |= DepthFlag_2_New_Check_Needed;
        if ((objData->depthFlags & DepthFlag_1_Visible) == FALSE) {
            return;
        }
    }

    //Create a sparkle
    if (self->modelInstIdx == 0) {
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_73F, NULL, 2, -1, NULL);
    } else {
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_740, NULL, 2, -1, NULL);
    }
}

/* @recomp: empty out the obj_Update function and move its code to obj_Print, so the sparkles can only draw while the statue is visible */
RECOMP_PATCH void WCTrexStatue_obj_Update(Object* self) {

}

/* @recomp: Add objData */
RECOMP_PATCH u32 WCTrexStatue_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(WCTrexStatue_Data);
}
