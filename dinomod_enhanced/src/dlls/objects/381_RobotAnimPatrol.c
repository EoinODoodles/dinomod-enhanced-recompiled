#include "modding.h"

#include "dlls/engine/17_partfx.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/338_LFXEmitter.h"
#include "game/objects/object_id.h"
#include "sys/camera.h"
#include "sys/dll.h"
#include "sys/gfx/projgfx.h"
#include "sys/gfx/animseq.h"
#include "sys/gfx/textable.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "sys/objmsg.h"
#include "sys/objlib.h"
#include "sys/print.h"
#include "sys/voxmap.h"
#include "dll.h"
#include "gbi_extra.h"

#include "recomp/dlls/objects/381_RobotAnimPatrol_recomp.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 rotation;
/*19*/ u8 unk19[0x1A - 0x19];
/*1A*/ s16 unk1A;
/*1C*/ u8 unk1C[0x1E - 0x1C];
/*1E*/ s16 unk1E;
/*20*/ s16 unk20;
} RobotAnimPatrol_Setup;

typedef struct {
/*00*/ f32 unk0;
/*04*/ s16 timer;
/*06*/ s16 unk6[12];
/*1E*/ s16 unk1E[12];
/*36*/ s16 unk36;
/*38*/ s16 unk38;
/*3A*/ s16 unk3A;
} RobotAnimPatrol_StunState;

typedef struct {
/*000*/ Object* beam;
/*004*/ Object* target;
/*008*/ Object* lfxEmitter;
/*00C*/ Object* lfxEmitter2;
/*010*/ s8 canSeePlayer;
/*011*/ s8 prevCanSeePlayer;
/*012*/ s8 targetIsPlayer;
/*013*/ u8 _unk13[0x18 - 0x13];
/*018*/ Vec3f spawnPos;
/*024*/ f32 unk24;
/*028*/ f32 unk28;
/*02C*/ f32 unk2C;
/*030*/ f32 unk30;
/*034*/ s16 unk34;
/*036*/ s16 spawnYaw;
/*038*/ s16 unk38;
/*03A*/ s16 beamTexV;
/*03C*/ s16 unk3C;
/*03E*/ s16 unk3E;
/*040*/ s16 chatterSfxTimer;
/*042*/ s16 unk42;
/*044*/ u32 unk44;
/*048*/ u32 soundHandle;
/*04C*/ u32 unk4C;
/*050*/ u8 updateRate;
/*051*/ u8 unk51;
/*052*/ u8 gunDeployState;
/*053*/ u8 unk53;
/*054*/ Vec3f gunBasePos;
/*060*/ Vec3f gunBarrelPos;
/*06C*/ Vec3f gunDir;
/*078*/ u8 _unk78[0x84 - 0x78];
/*084*/ Vec3f beamDir;
/*090*/ Vec3f unk90;
/*09C*/ s32 unk9C;
/*0A0*/ s32 unkA0;
/*0A4*/ s16 unkA4;
/*0A6*/ s16 _unkA6;
/*0A8*/ u8 isGunAimed;
/*0A9*/ u8 shouldShoot;
/*0AA*/ u8 unkAA;
/*0AB*/ u8 crashTick;
/*0AC*/ f32 unkAC;
/*0B0*/ f32 unkB0;
/*0B4*/ f32 unkB4;
/*0B8*/ f32 unkB8;
/*0BC*/ DLL27_Data collider;
/*31C*/ RobotAnimPatrol_StunState stunState;
/*358*/ u8 flags;
/*359*/ u8 unk359;
} RobotAnimPatrol_Data;

// @recomp: fix one side of the laser pointer
/*0x0*/ static DLTri sLaserTris[] = {
    {0x40, 0, 2, 1, {0}}, 
    {0x40, 1, 2, 0, {0}} // @recomp: just reverse the winding order, so the fadeout looks right 
};

/*0x0*/ extern Texture* sRedLaserBeamTexture;

s32 RobotAnimPatrol_aimRaycast(Vec3f*, Vec3f*, Vec3f*, Object*);
void RobotAnimPatrol_fireGun(Object* self, RobotAnimPatrol_Data* objdata);

RECOMP_PATCH void RobotAnimPatrol_gunPrint(Object* self, ModelInstance* modelInst, Gfx** gdl, Mtx** mtxs, Vtx** vtxs, DLTri** tris) {
    Vec3f aimPoint;
    SRT srt;
    MtxF* boneMtx;
    f32 magnitude;
    Vtx* vtx;
    RobotAnimPatrol_Data* objdata;
    s32 bone;
    f32 laserY1;
    f32 laserZ1;
    f32 laserY2;
    f32 laserX1;
    f32 laserZ2;
    f32 laserX2;

    objdata = self->data;
    if ((objdata->gunDeployState == 1) || (objdata->gunDeployState == 4)) {
        vtx = *vtxs;
        bone = self->def->pAttachPoints[2].bones[self->modelInstIdx]; // gun base
        boneMtx = (MtxF*) &((f32*)modelInst->matrices[modelInst->unk34 & 1])[bone << 4];
        objdata->gunBasePos.x = boneMtx->m[3][0] + gWorldX;
        objdata->gunBasePos.y = boneMtx->m[3][1];
        objdata->gunBasePos.z = boneMtx->m[3][2] + gWorldZ;
        bone = self->def->pAttachPoints[1].bones[self->modelInstIdx]; // gun barrel
        boneMtx = (MtxF*) &((f32*)modelInst->matrices[modelInst->unk34 & 1])[bone << 4];
        objdata->gunBarrelPos.x = boneMtx->m[3][0] + gWorldX;
        objdata->gunBarrelPos.y = boneMtx->m[3][1];
        objdata->gunBarrelPos.z = boneMtx->m[3][2] + gWorldZ;
        objdata->gunDir.x = objdata->gunBarrelPos.x - objdata->gunBasePos.x;
        objdata->gunDir.y = objdata->gunBarrelPos.y - objdata->gunBasePos.y;
        objdata->gunDir.z = objdata->gunBarrelPos.z - objdata->gunBasePos.z;
        magnitude = 1.0f / sqrtf(SQ(objdata->gunDir.f[0]) + SQ(objdata->gunDir.f[1]) + SQ(objdata->gunDir.f[2]));
        objdata->gunDir.x *= magnitude;
        objdata->gunDir.y *= magnitude;
        objdata->gunDir.z *= magnitude;
        aimPoint.x = objdata->gunDir.x * 200.0f;
        aimPoint.y = objdata->gunDir.y * 200.0f;
        aimPoint.z = objdata->gunDir.z * 200.0f;
        aimPoint.x += objdata->gunBarrelPos.x;
        aimPoint.y += objdata->gunBarrelPos.y;
        aimPoint.z += objdata->gunBarrelPos.z;
        RobotAnimPatrol_aimRaycast(&objdata->gunBarrelPos, &aimPoint, &objdata->unk90, objdata->target);
        // @recomp: Remove laser pointer. The EWTrobotpatrol patch will cause this to start showing due to
        //          the use of uninitialized data and the laser doesn't look as good on these versions.
        /*
        texDPTextures(gdl, sRedLaserBeamTexture, NULL, RENDER_UNK10 | RENDER_Z_COMPARE, 0, FALSE, TRUE);
        dlSetPrimColor(gdl, 255, 255, 255, 255);
        magnitude = sqrtf(SQ(objdata->gunDir.x) + SQ(objdata->gunDir.z));
        srt.yaw = mathAtan2f(objdata->gunDir.x, objdata->gunDir.z);
        srt.pitch = -mathAtan2f(objdata->gunDir.y, magnitude);
        srt.roll = 0;
        srt.transl.x = objdata->gunBarrelPos.x;
        srt.transl.y = objdata->gunBarrelPos.y;
        srt.transl.z = objdata->gunBarrelPos.z;
        srt.scale = 0.1f;
        camSetupObjectSRTMatrix(gdl, mtxs, &srt, 1.0f, 0.0f, NULL);
        bcopy(sLaserTris, *tris, sizeof(sLaserTris));
        gSPVertex((*gdl)++, OS_PHYSICAL_TO_K0(*vtxs), 4, 0);
        dlTriangles(gdl, *tris, 2);

        laserZ2 = objdata->unk90.x - objdata->gunBarrelPos.x;
        laserX2 = objdata->unk90.z - objdata->gunBarrelPos.z;
        magnitude = sqrtf(SQ(laserZ2) + SQ(laserX2));
        laserX1 = 0.0f;
        laserY1 = 0.0f;
        laserZ1 = 0.0f;

        laserX2 = 0.0f;
        laserY2 = 0.0f;
        laserZ2 = magnitude / mathCosfInterp(srt.pitch);
        laserZ2 += 4.0f;
        
        laserX2 *= 10.0f;
        laserZ2 *= 10.0f;
        
        // @bug: The laser pointer tris set up here don't show up because the tex coords are uninitialized
        // @recomp: Set texture coords so laser renders correctly (note: the coords are an educated guess)
        vtx->v.ob[0] = (s32)laserX1;\
        vtx->v.ob[1] = (s32)laserY1 + 7;\
        vtx->v.ob[2] = (s32)laserZ1;
        vtx->v.cn[0] = 255;\
        vtx->v.cn[1] = 0;\
        vtx->v.cn[2] = 0;\
        vtx->v.cn[3] = 40;
        vtx->v.tc[0] = qu105(15); // @recomp
        vtx->v.tc[1] = qu105(0); // @recomp
        vtx++;

        vtx->v.ob[0] = (s32)laserX1;\
        vtx->v.ob[1] = (s32)laserY1 - 7;\
        vtx->v.ob[2] = (s32)laserZ1;
        vtx->v.cn[0] = 255;\
        vtx->v.cn[1] = 0;\
        vtx->v.cn[2] = 0;\
        vtx->v.cn[3] = 40;
        vtx->v.tc[0] = qu105(0); // @recomp
        vtx->v.tc[1] = qu105(0); // @recomp
        vtx++;

        vtx->v.ob[0] = (s32)laserX2;\
        vtx->v.ob[1] = (s32)laserY2 + 7;\
        vtx->v.ob[2] = (s32)laserZ2;
        vtx->v.cn[0] = 255;\
        vtx->v.cn[1] = 0;\
        vtx->v.cn[2] = 0;\
        vtx->v.cn[3] = 40;
        vtx->v.tc[0] = qu105(15); // @recomp
        vtx->v.tc[1] = qu105(31); // @recomp
        vtx++;

        vtx->v.ob[0] = (s32)laserX2;\
        vtx->v.ob[1] = (s32)laserY2 - 7;\
        vtx->v.ob[2] = (s32)laserZ2;
        vtx->v.cn[0] = 255;\
        vtx->v.cn[1] = 0;\
        vtx->v.cn[2] = 0;\
        vtx->v.cn[3] = 40;
        vtx->v.tc[0] = qu105(0); // @recomp
        vtx->v.tc[1] = qu105(31); // @recomp
        vtx++;
        
        srt.roll = 0x4000;
        camSetupObjectSRTMatrix(gdl, mtxs, &srt, 1.0f, 0.0f, NULL);
        gSPVertex((*gdl)++, OS_PHYSICAL_TO_K0(*vtxs), 4, 0);
        dlTriangles(gdl, *tris, 2);
        *vtxs = vtx;
        *tris += 2;
        */
        if (objdata->isGunAimed && objdata->shouldShoot) {
            RobotAnimPatrol_fireGun(self, objdata);
            objdata->shouldShoot = FALSE;
        }
        objdata->isGunAimed = FALSE;
    }
}
