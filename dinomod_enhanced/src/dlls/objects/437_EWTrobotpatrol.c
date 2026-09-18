#include "modding.h"
#include "recomputils.h"

#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/437_EWTrobotpatrol.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/camera.h"
#include "sys/dll.h"
#include "sys/gfx/textable.h"
#include "sys/gfx/texture.h"
#include "sys/gfx/projgfx.h"
#include "sys/gfx/animseq.h"
#include "sys/joypad.h"
#include "sys/math.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "sys/lighting.h"
#include "sys/voxmap.h"
#include "dll.h"
#include "gbi_extra.h"

#include "recomp/dlls/objects/437_EWTrobotpatrol_recomp.h"

typedef struct {
/*0*/ f32 stickXF;
/*4*/ f32 stickYInvF;
/*8*/ u16 pressed;
/*A*/ u16 released;
/*C*/ u16 buttons;
/*E*/ s8 stickX;
/*F*/ s8 stickYInv;
} EWTrobotpatrol_ControllerState;

typedef struct {
/*00*/ f32 moveSin;
/*04*/ f32 unk4;
/*08*/ f32 moveCos;
/*0C*/ u8 _unkC[0x18 - 0xC];
/*18*/ f32 unk18;
/*1C*/ f32 unk1C;
/*20*/ f32 unk20;
/*24*/ f32 unk24;
/*28*/ f32 unk28;
/*2C*/ f32 unk2C;
/*30*/ f32 unk30;
/*34*/ f32 unk34;
/*38*/ f32 unk38;
/*3C*/ Vec3f moveVec; // relative to robo orientation
/*48*/ Vec3f unk48;
/*54*/ u8 _unk54[0x6C - 0x54];
/*6C*/ f32 unk6C;
/*70*/ f32 unk70;
/*74*/ f32 deltaTime; // updateRate / 60
/*78*/ f32 unk78;
/*7C*/ f32 unk7C;
/*80*/ f32 unk80;
/*84*/ f32 baseOffsetY;
/*88*/ f32 bobScale;
/*8C*/ s16 bobTheta; // current theta for bobbing up and down while floating
/*8E*/ u8 unk8E;
} EWTrobotpatrol_Body;

// size:0x34 ?
typedef struct {
/*00*/ Vec3f unk0[4];
/*30*/ u8 unk30;
} EWTrobotpatrol_Fx;

typedef struct {
/*00*/ Object* target;
/*04*/ u8 _unk4[0x8 - 0x4];
/*08*/ Vec3f barrelPos;
/*14*/ Vec3f basePos;
/*20*/ Vec3f fireAtPoint;
/*2C*/ Vec3f unk2C;
/*38*/ Vec3f dir;
/*44*/ f32 animDelta;
/*48*/ s16 yaw;
/*4A*/ s16 pitch;
/*4C*/ s16 targetYaw;
/*4E*/ u8 isDeployed;
/*4F*/ u8 mode;
/*50*/ u8 shouldShoot;
/*51*/ u8 updateRate;
} EWTrobotpatrol_Gun;

typedef struct {
/*00*/ f32 colorPulseTValue;
/*04*/ Object* obj; // RobotBeam
/*08*/ Vec3f dir;
/*14*/ s16 beamTexV;
/*16*/ s16 yaw;
/*18*/ s16 yawTarget;
/*1A*/ u8 unk1A;
/*1B*/ u8 unk1B; // unused
/*1C*/ u8 colorPulseDir;
} EWTrobotpatrol_Beam;

// size:0x3C
typedef struct {
/*00*/ f32 unk0;
/*04*/ s16 timer;
/*06*/ s16 unk6[12];
/*1E*/ s16 unk1E[12];
/*36*/ s16 unk36;
/*38*/ s16 unk38;
/*3A*/ s16 unk3A;
} EWTrobotpatrol_StunState;

typedef struct {
/*000*/ u8 unk0; // unused
/*004*/ s32 destCurveUID;
/*008*/ s32 prevCurveUID;
/*00C*/ u8 _unkC[0x14 - 0xC];
/*014*/ f32 destDist; // current distance to destination position
/*018*/ s16 alertTimer;
/*01A*/ s16 gunCooldown;
/*01C*/ u8 _unk1C[0x1D - 0x1C];
/*01D*/ u8 getNextCurveDebounce;
/*01E*/ u8 alertMode;
/*01F*/ u8 activateState;
/*020*/ u8 combatState;
/*021*/ u8 hasVoxLineOfSightToPlayer;
/*024*/ Unk80008E40 voxRoute;
/*04C*/ Vec3f savedPosition;
/*058*/ Vec3f destPos; // position to move to
/*064*/ u8 _unk64[0x74 - 0x64];
/*074*/ Object* base;
/*078*/ EWTrobotpatrolCallback baseCallback;
/*07C*/ Object* player;
/*080*/ EWTrobotpatrol_ControllerState cont;
/*090*/ EWTrobotpatrol_Body body;
/*120*/ EWTrobotpatrol_Fx fx;
/*154*/ EWTrobotpatrol_Gun gun;
/*1A8*/ EWTrobotpatrol_Beam beam;
/*1C8*/ f32 unk1C8;
/*1CC*/ u8 unk1CC;
/*1D0*/ EWTrobotpatrol_StunState stunState;
} EWTrobotpatrol_Data;

/*0x0*/ extern DLTri sLaserTris[2];

/*0x0*/ extern Texture* sLaserBeamTexture; // red laser beam
/*0x4*/ extern Texture* sNoiseTexture; // noise pattern

void EWTrobotpatrol_setGunMode(EWTrobotpatrol_Gun* gun, s32 mode, Object* target);
s32 EWTrobotpatrol_move(Object* self, EWTrobotpatrol_Body* body, EWTrobotpatrol_Data* objdata, s32 mode);
void EWTrobotpatrol_curveMove(Object* self, EWTrobotpatrol_Data* objdata, EWTrobotpatrol_Body* body);
void EWTrobotpatrol_checkForPlayer(Object* self, EWTrobotpatrol_Data* objdata);
void EWTrobotpatrol_gunPrint(Object* self, ModelInstance* modelInst, Gfx** gdl, Mtx** mtxs, Vtx** vtxs, DLTri** pols);
void EWTrobotpatrol_fireGun(Object* self, EWTrobotpatrol_Gun* gun);
s32 EWTrobotpatrol_aimRaycast(Vec3f* barrelPos, Vec3f* aimPoint, Vec3f* fireAtPoint, Vec3f* a3, Object* target);

// Like EWTrobotpatrol_checkForPlayer, but for the combat searching state
static s32 recomp_isPlayerPerceptible(Object* self, EWTrobotpatrol_Data* objdata, Object* player) {
    Vec3f vec2Player;
    s32 temp_v0_3;
    EWTrobotpatrol_Gun* gun;

    objdata->player = player;
    gun = &objdata->gun;

    // new: Can't see player if they're in a seq
    if (((DLL_210_Player*)player->dll)->vtbl->func66(player, 1) == 0) {
        return FALSE;
    }

    if (objdata->hasVoxLineOfSightToPlayer) {
        // Player in LOS
        // Note: Do a custom dot product check here instead of the usual check this DLL does to avoid
        // a bug where the robot can see behind them.
        vec2Player.x = player->srt.transl.x - gun->basePos.x;
        vec2Player.y = player->srt.transl.y - gun->basePos.y;
        vec2Player.z = player->srt.transl.z - gun->basePos.z;
        vec3Normalize(&vec2Player);

        Vec3f gunDir = gun->dir;

        f32 dot = vec3DotProduct(&vec2Player, &gunDir);
        if (dot >= 0.9f) { // note: this is for the laser, so we're not tuning this exactly to the spotlight beam
            return TRUE;
        }
    }

    // Note: Use smaller non-alerted dist check here as this is just for "hearing"
    vec2Player.f[0] = player->srt.transl.x - self->srt.transl.x;
    vec2Player.f[1] = player->srt.transl.y - self->srt.transl.y;
    vec2Player.f[2] = player->srt.transl.z - self->srt.transl.z;
    if (sqrtf(SQ(vec2Player.f[0]) + SQ(vec2Player.f[1]) + SQ(vec2Player.f[2])) < 150.0f) {
        // Check if player is moving?
        temp_v0_3 = (s32)((DLL_210_Player*)player->dll)->vtbl->func66(player, 2);
        return temp_v0_3 == 3 || temp_v0_3 == 4;
    }

    return FALSE;
}

RECOMP_PATCH void EWTrobotpatrol_moveAndShoot(Object* self, EWTrobotpatrol_Data* objdata, EWTrobotpatrol_Body* body, EWTrobotpatrol_ControllerState* cont) {
    Object* player;
    EWTrobotpatrol_Data* objdata2;
    f32 playerDist;
    f32 vec[3];
    Vec3s16 playerVoxPos;
    Vec3s16 selfVoxPos;

    objdata2 = self->data;
    player = objGetPlayer();
    vec[0] = self->srt.transl.x - player->srt.transl.x;
    vec[1] = self->srt.transl.y - player->srt.transl.y;
    vec[2] = self->srt.transl.z - player->srt.transl.z;
    playerDist = sqrtf(SQ(vec[0]) + SQ(vec[1]) + SQ(vec[2]));
    vox_func_80007EE0(&player->srt.transl, &playerVoxPos);
    playerVoxPos.s[1] += 2;
    vox_func_80007EE0(&self->srt.transl, &selfVoxPos);
    objdata->hasVoxLineOfSightToPlayer = vox_func_80008048(&playerVoxPos, &selfVoxPos, NULL, NULL, 0);
    if ((objdata->activateState != 0) && (objdata->activateState == 2)) {
        // Activated, do combat
        objdata->baseCallback(objdata->base, self, /*aggro*/TRUE, 0);
        if (objdata->combatState){} // @fake
        switch (objdata->combatState) {
        case 0:
            // @recomp: Aim beam at player
            objdata2->beam.yawTarget = mathAtan2f(vec[0], vec[2]);
            EWTrobotpatrol_setGunMode(&objdata2->gun, 2, player);
            if (EWTrobotpatrol_move(self, body, objdata, 0) != 0) {
                bcopy(&objdata->savedPosition, &objdata->destPos, sizeof(objdata->destPos));
                objdata->combatState = 1;
                objdata->alertTimer = 300;
            }
            break;
        case 1:
            EWTrobotpatrol_move(self, body, objdata, 1);
            // @recomp: Custom perception check that works better with the unused "searching" gun state
            if ((playerDist < 300.0f) && recomp_isPlayerPerceptible(self, objdata, player)) {
                objdata->combatState = 2;
                objdata->alertTimer = 300;
                objdata->gunCooldown = 15; // @recomp: Increase initial cooldown to give gun time to line up a shot
            } else {
                // @recomp: Aim beam with gun
                if (objdata2->gun.isDeployed) {
                    objdata2->beam.yawTarget = objdata2->gun.yaw;
                }
                objdata->alertTimer -= gUpdateRate;
                // @recomp: Use unused gun mode after the player has been hidden for a bit
                if (objdata->alertTimer <= 200) {
                    EWTrobotpatrol_setGunMode(&objdata2->gun, 1, NULL);
                }
                if (objdata->alertTimer < 0) {
                    objdata->alertMode = 0;
                    objdata->activateState = 0;
                    objdata->combatState = 0;
                }
            }
            break;
        case 2:
            // @recomp: Aim beam at player
            objdata2->beam.yawTarget = mathAtan2f(vec[0], vec[2]);
            // @recomp: Restore gun mode since we set the unused mode in state 1
            EWTrobotpatrol_setGunMode(&objdata2->gun, 2, player);
            EWTrobotpatrol_move(self, body, objdata, 1);
            // @recomp: Leave combat state 2 if player is in seq
            if ((playerDist > 300.0f) || !objdata->hasVoxLineOfSightToPlayer || ((DLL_210_Player*)player->dll)->vtbl->func66(player, 1) == 0) {
                objdata->combatState = 1;
            }
            objdata->gunCooldown -= gUpdateRate;
            if (objdata->gunCooldown < 0) {
                objdata->gunCooldown = 0;
            }
            if (objdata->gunCooldown == 0) {
                if (objdata->hasVoxLineOfSightToPlayer 
                        && (((DLL_210_Player*)player->dll)->vtbl->func66(player, 9) == 0) 
                        && (((DLL_210_Player*)player->dll)->vtbl->func66(player, 1) != 0)) {
                    objdata2->gun.shouldShoot = 3;
                }
                objdata->gunCooldown = 150;
            }
            break;
        }
    } else {
        // Not alerted
        objdata2->gun.animDelta = -0.02f; // hide weapon
        if (objdata->destCurveUID != -1) {
            EWTrobotpatrol_curveMove(self, objdata, body);
            if (objdata->getNextCurveDebounce > 0) {
                objdata->getNextCurveDebounce -= 1;
            }
            if ((objdata->getNextCurveDebounce == 0) && (objdata->destDist < 3.0f)) {
                objdata->baseCallback(objdata->base, self, 0, objdata->destCurveUID);
                objdata->getNextCurveDebounce = 120;
            }
        }
        // @recomp: Ignore player if they're in a seq
        if (((DLL_210_Player*)player->dll)->vtbl->func66(player, 1) != 0) {
            EWTrobotpatrol_checkForPlayer(self, objdata);
        }
        if (objdata->alertMode == 2) {
            dll_amSfx->Play(self, SOUND_112_RobotPatrol_Activate, MAX_VOLUME, NULL, NULL, 0, NULL);
            //if (objdata2){} // @fake
            objdata2->gun.animDelta = 0.02f; // show weapon
            objdata2->beam.colorPulseDir = 0;
            objdata2->beam.colorPulseTValue = 0.0f;
            bcopy(&self->srt.transl, &objdata->savedPosition, sizeof(objdata->savedPosition));
            objdata->activateState = 2;
            objdata->combatState = 0;
        }
    }
}

RECOMP_PATCH void EWTrobotpatrol_obj_Print(Object* self, Gfx** gdl, Mtx** mtxs, Vtx** vtxs, DLTri** pols, s8 visibility) {
    EWTrobotpatrol_Data* objdata;
    ModelInstance* modelInst;
    Object* beam;
    MtxF* attachPointMtx;
    s32 bone;
    s32 _pad;
    f32 temp_fv1_2;
    MtxF sp64;
    u8 beamShadowR;
    u8 beamShadowG;
    u8 beamShadowB;
    u8 alertR;
    u8 alertG;
    u8 alertB;
    u8 savedAmbientR;
    u8 savedAmbientG;
    u8 savedAmbientB;
    ObjectShadow* beamShadow;
    Vec3f sp48;
    s32 _pad2;
    u8 ambientR;
    u8 ambientG;
    u8 ambientB;
    Vec3f* beamDir;

    lightGetAmbient(&ambientR, &ambientG, &ambientB);
    objdata = self->data;
    modelInst = self->modelInsts[self->modelInstIdx];
    if (visibility != 0) {
        objprintDrawModel(self, gdl, mtxs, (Vertex**)vtxs, (Triangle**)pols, 1.0f);
    } else if (!(modelInst->unk34 & 8)) {
        // Not drawing self but we still need the model matrices to be live
        mod_func_8001943C(self, &sp64, 1.0f, 0.0f);
        mod_func_80019730(modelInst, modelInst->model, self, &sp64);
    }
    beam = objdata->beam.obj;
    beamDir = &objdata->beam.dir;
    // Attach beam to attach point 0
    bone = self->def->pAttachPoints[0].bones[self->modelInstIdx]; // beam projector
    attachPointMtx = (MtxF*) &((f32*)modelInst->matrices[modelInst->unk34 & 1])[bone << 4];
    beam->srt.transl.x = attachPointMtx->m[3][0] + gWorldX;
    beam->srt.transl.y = attachPointMtx->m[3][1];
    beam->srt.transl.z = attachPointMtx->m[3][2] + gWorldZ;
    camGetObjectChildPosition(beam, &beam->globalPosition.x, &beam->globalPosition.y, &beam->globalPosition.z);
    if (trackObjVisCheck(beam) != 0) {
        beamShadow = beam->shadow;
        // Pulse beam shadow color between white and red when alerted
        switch (objdata->alertMode) {
        case 1:
            alertR = 208;
            alertG = 0;
            alertB = 0;
            break;
        case 2:
            alertR = 208;
            alertG = 0;
            alertB = 0;
            break;
        case 0:
        default:
            alertR = 186;
            alertG = 255;
            alertB = 255;  
            break;
        }
        beamShadowR = 186;
        beamShadowG = 255;
        beamShadowB = 255;
        beamShadowR += (objdata->beam.colorPulseTValue * (f32) (alertR - beamShadowR));
        beamShadowG += (objdata->beam.colorPulseTValue * (f32) (alertG - beamShadowG));
        beamShadowB += (objdata->beam.colorPulseTValue * (f32) (alertB - beamShadowB));
        // Calculate shadow direction
        bone = self->def->pAttachPoints[3].bones[self->modelInstIdx]; // body center
        attachPointMtx = (MtxF*) &((f32*)modelInst->matrices[modelInst->unk34 & 1])[bone << 4];
        sp48.f[0] = attachPointMtx->m[3][0] + gWorldX;
        sp48.f[1] = attachPointMtx->m[3][1];
        sp48.f[2] = attachPointMtx->m[3][2] + gWorldZ;
        sp48.f[0] = beam->srt.transl.x - sp48.f[0];
        sp48.f[1] = beam->srt.transl.y - sp48.f[1];
        sp48.f[2] = beam->srt.transl.z - sp48.f[2];
        temp_fv1_2 = 1.0f / sqrtf(SQ(sp48.f[0]) + SQ(sp48.f[1]) + SQ(sp48.f[2]));
        beamDir->x = (f32) (sp48.f[0] * temp_fv1_2);
        beamDir->y = (f32) (sp48.f[1] * temp_fv1_2);
        beamDir->z = (f32) (sp48.f[2] * temp_fv1_2);
        // Update shadow
        beamShadow->dir.x = -beamDir->x;
        beamShadow->dir.y = -beamDir->y;
        beamShadow->dir.z = -beamDir->z;
        beamShadow->tr.x = beam->srt.transl.x;
        beamShadow->tr.y = beam->srt.transl.y;
        beamShadow->tr.z = beam->srt.transl.z;
        beamShadow->flags |= OBJ_SHADOW_FLAG_FADE_OUT; // this results in the shadow never appearing
        beamShadow->r = beamShadowR;
        beamShadow->g = beamShadowG;
        beamShadow->b = beamShadowB;
        // Update beam
        beam->prevLocalPosition.x = beam->srt.transl.x;
        beam->prevLocalPosition.y = beam->srt.transl.y;
        beam->prevLocalPosition.z = beam->srt.transl.z;
        beam->srt.yaw = objdata->beam.yaw;
        beam->srt.pitch = 0;
        beam->srt.roll = 0;
        beam->srt.scale = 0.2f;
        beam->opacityWithFade = self->opacityWithFade;
        if (beam->opacityWithFade > 160) {
            beam->opacityWithFade = 160;
        }
        beam->opacityWithFade = (u8) ((beam->opacityWithFade * (beam->opacity + 1)) >> 8);
        // ? Was this originally trying to apply the shadow color to the beam model itself?
        savedAmbientR = ambientR;
        savedAmbientG = ambientG;
        savedAmbientB = ambientB;
        ambientR = beamShadowR;
        ambientG = beamShadowG;
        ambientB = beamShadowB;
        // @recomp: Apply shadow color to beam model
        objprintSetBlendColor(beamShadowR, beamShadowG, beamShadowB, 255);
        // Draw beam
        objprintDrawModel(beam, gdl, mtxs, (Vertex**)vtxs, (Triangle**)pols, 1.0f);
        ambientR = savedAmbientR;
        ambientG = savedAmbientG;
        ambientB = savedAmbientB;
        // Mark beam model matrices as no longer live (why?)
        beam->modelInsts[beam->modelInstIdx]->unk34 &= ~0x8;
    }
    EWTrobotpatrol_gunPrint(self, modelInst, gdl, mtxs, vtxs, pols);
}

RECOMP_PATCH void EWTrobotpatrol_gunPrint(Object* self, ModelInstance* modelInst, Gfx** gdl, Mtx** mtxs, Vtx** vtxs, DLTri** pols) {
    static s16 data_50 = 0;
    Vec3f aimPoint;
    SRT barrelSRT;
    Object* player;
    MtxF* jointMtx;
    f32 magnitude;
    Vtx* vtx;
    f32 laserY1;
    f32 laserZ1;
    f32 laserY2;
    f32 laserX1;
    EWTrobotpatrol_Gun* gun;
    EWTrobotpatrol_Data* objdata;
    s32 bone;
    f32 laserZ2;
    f32 laserX2;

    data_50 += 3;
    if (data_50 > 30) {
        data_50 = 0;
    }
    objdata = self->data;
    gun = &objdata->gun;
    if (!gun->isDeployed) {
        return;
    }
    vtx = *vtxs;
    player = objGetPlayer();
    bone = self->def->pAttachPoints[2].bones[self->modelInstIdx]; // gun base
    jointMtx = (MtxF*) &((f32*)modelInst->matrices[modelInst->unk34 & 1])[bone << 4];
    gun->basePos.x = jointMtx->m[3][0] + gWorldX;
    gun->basePos.y = jointMtx->m[3][1];
    gun->basePos.z = jointMtx->m[3][2] + gWorldZ;
    bone = self->def->pAttachPoints[1].bones[self->modelInstIdx]; // gun barrel
    jointMtx = (MtxF*) &((f32*)modelInst->matrices[modelInst->unk34 & 1])[bone << 4];
    gun->barrelPos.x = jointMtx->m[3][0] + gWorldX;
    gun->barrelPos.y = jointMtx->m[3][1];
    gun->barrelPos.z = jointMtx->m[3][2] + gWorldZ;
    gun->dir.x = gun->barrelPos.x - gun->basePos.x;
    gun->dir.y = gun->barrelPos.y - gun->basePos.y;
    gun->dir.z = gun->barrelPos.z - gun->basePos.z;
    magnitude = sqrtf(SQ(gun->dir.f[0]) + SQ(gun->dir.f[1]) + SQ(gun->dir.f[2]));
    if (magnitude != 0.0f) {
        magnitude = 1.0f / magnitude;
    } else {
        magnitude = 0.0f;
    }
    gun->dir.x *= magnitude;
    gun->dir.y *= magnitude;
    gun->dir.z *= magnitude;
    aimPoint.x = gun->dir.x * 300.0f;
    aimPoint.y = gun->dir.y * 300.0f;
    aimPoint.z = gun->dir.z * 300.0f;
    aimPoint.x += gun->barrelPos.x;
    aimPoint.y += gun->barrelPos.y;
    aimPoint.z += gun->barrelPos.z;
    EWTrobotpatrol_aimRaycast(&gun->barrelPos, &aimPoint, &gun->fireAtPoint, &gun->unk2C, player);
    gDPLoadTextureBlockS((*gdl)++,
        /*timg*/sNoiseTexture + 1,
        /*fmt*/G_IM_FMT_IA,
        /*siz*/G_IM_SIZ_8b,
        /*width*/sNoiseTexture->width,
        /*height*/sNoiseTexture->height,
        /*pal*/0,
        /*cms*/G_TX_NOMIRROR | G_TX_WRAP,
        /*cmt*/G_TX_NOMIRROR | G_TX_WRAP,
        /*masks*/5,
        /*maskt*/5,
        /*shifts*/G_TX_NOLOD,
        /*shiftt*/G_TX_NOLOD            
    );
    gDPTileSync((*gdl)++);
    gDPLoadMultiBlockS((*gdl)++,
        /*timg*/sLaserBeamTexture + 1,
        /*tmem*/sNoiseTexture->sizeBytes >> 3,
        /*rtile*/1,
        /*fmt*/G_IM_FMT_RGBA,
        /*siz*/G_IM_SIZ_32b,
        /*width*/sLaserBeamTexture->width,
        /*height*/sLaserBeamTexture->height,
        /*pal*/0,
        /*cms*/G_TX_NOMIRROR | G_TX_WRAP,
        /*cmt*/G_TX_NOMIRROR | G_TX_WRAP,
        /*masks*/5,
        /*maskt*/5,
        /*shifts*/G_TX_NOLOD,
        /*shiftt*/G_TX_NOLOD    
    );
    dlSetPrimColor(gdl, 255, 255, 255, 85);
    gDPSetCombineMode(*gdl, G_CC_DINO_PRIM_RGB_INTERFERENCE_A, G_CC_DINO_MODULATERGB_PRIMA2);
    dlApplyCombine(gdl);
    gDPSetOtherMode(*gdl, 
        G_AD_PATTERN | G_CD_NOISE | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_PERSP | G_CYC_2CYCLE | G_PM_NPRIMITIVE, 
        G_AC_NONE | G_ZS_PIXEL | G_RM_NOOP | G_RM_ZB_CLD_SURF2);
    dlApplyOtherMode(gdl);
    dlClearGeometryMode(gdl, G_CULL_BOTH); // @recomp: Disable backface culling so the whole laser is visible
    magnitude = sqrtf(SQ(gun->dir.x) + SQ(gun->dir.z));
    barrelSRT.yaw = mathAtan2f(gun->dir.x, gun->dir.z);
    barrelSRT.pitch = -mathAtan2f(gun->dir.y, magnitude);
    barrelSRT.roll = 0;
    barrelSRT.transl.x = gun->barrelPos.x;
    barrelSRT.transl.y = gun->barrelPos.y;
    barrelSRT.transl.z = gun->barrelPos.z;
    barrelSRT.scale = 0.1f;
    camSetupObjectSRTMatrix(gdl, mtxs, &barrelSRT, 1.0f, 0.0f, NULL);
    bcopy(sLaserTris, *pols, sizeof(sLaserTris));
    gSPVertex((*gdl)++, OS_PHYSICAL_TO_K0(*vtxs), 4, 0);
    dlTriangles(gdl, *pols, 2);
    laserX1 = 0.0f;
    laserY1 = 0.0f;
    laserZ1 = 0.0f;
    magnitude = sqrtf(SQ(gun->fireAtPoint.x - gun->barrelPos.x) + SQ( gun->fireAtPoint.z - gun->barrelPos.z));
    laserX2 = 0.0f;
    laserY2 = 0.0f;
    laserZ2 = mathCosfInterp(barrelSRT.pitch);
    if (laserZ2 != 0.0f) {
        laserZ2 = magnitude / laserZ2;
    } else {
        laserZ2 = 0.0f;
    }
    laserX2 *= 10.0f;
    laserZ2 *= 10.0f;
    // @bug: The laser pointer tris set up here don't show up because the tex coords are uninitialized
    // @recomp: Set texture coords so laser renders correctly (note: the coords are an educated guess)
    vtx->v.ob[0] = (s32)laserX1;
    vtx->v.ob[1] = (s32)laserY1 + 14;
    vtx->v.ob[2] = (s32)laserZ1;
    vtx->v.cn[0] = 255;
    vtx->v.cn[1] = 0;
    vtx->v.cn[2] = 0;
    vtx->v.cn[3] = 205;
    vtx->v.tc[0] = qu105(16); // @recomp
    vtx->v.tc[1] = qu105(0); // @recomp
    vtx++;

    vtx->v.ob[0] = (s32)laserX1;
    vtx->v.ob[1] = (s32)laserY1 - 14;
    vtx->v.ob[2] = (s32)laserZ1;
    vtx->v.cn[0] = 255;
    vtx->v.cn[1] = 0;
    vtx->v.cn[2] = 0;
    vtx->v.cn[3] = 205;
    vtx->v.tc[0] = qu105(0); // @recomp
    vtx->v.tc[1] = qu105(0); // @recomp
    vtx++;

    vtx->v.ob[0] = (s32)laserX2;\
    vtx->v.ob[1] = (s32)laserY2 + 14;\
    vtx->v.ob[2] = (s32)laserZ2;
    vtx->v.cn[0] = 255;\
    vtx->v.cn[1] = 0;\
    vtx->v.cn[2] = 0;\
    vtx->v.cn[3] = 105;
    vtx->v.tc[0] = qu105(16); // @recomp
    vtx->v.tc[1] = qu105(31); // @recomp
    vtx++;

    vtx->v.ob[0] = (s32)laserX2;\
    vtx->v.ob[1] = (s32)laserY2 - 14;\
    vtx->v.ob[2] = (s32)laserZ2;
    vtx->v.cn[0] = 255;\
    vtx->v.cn[1] = 0;\
    vtx->v.cn[2] = 0;\
    vtx->v.cn[3] = 105;
    vtx->v.tc[0] = qu105(0); // @recomp
    vtx->v.tc[1] = qu105(31); // @recomp
    vtx++;
    barrelSRT.roll = 0x4000;
    camSetupObjectSRTMatrix(gdl, mtxs, &barrelSRT, 1.0f, 0.0f, NULL);
    gSPVertex((*gdl)++, OS_PHYSICAL_TO_K0(*vtxs), 4, 0);
    dlTriangles(gdl, *pols, 2);
    *vtxs = vtx;
    *pols += 2;
    if (gun->shouldShoot > 0) {
        EWTrobotpatrol_fireGun(self, gun);
        gun->shouldShoot = 0;
    }
    dlSetGeometryMode(gdl, G_CULL_BACK); // @recomp: Restore backface culling (we disabled it above)
    texRenderReset();
    lightAmbientDL(gdl);
}

RECOMP_PATCH s32 EWTrobotpatrol_aimRaycast(Vec3f* barrelPos, Vec3f* aimPoint, Vec3f* fireAtPoint, Vec3f* a3, Object* target) {
    Vec3s16 voxBarrelPos;
    Vec3s16 voxAimPoint;
    Vec3s16 voxHitPos;
    f32 var_fv1;
    s32 _pad;
    s32 _pad2;
    Vec3f hitPos;
    Vec3f sp5C;
    Vec3f sp50;
    Vec3f sp44;
    s8 _pad_sp43;
    s8 sp42;
    s32 _pad3;
    f32 sp38;
    s32 _pad4;

    sp44.f[0] = aimPoint->f[0] - barrelPos->f[0];
    sp44.f[1] = aimPoint->f[1] - barrelPos->f[1];
    sp44.f[2] = aimPoint->f[2] - barrelPos->f[2];
    var_fv1 = sqrtf(SQ(sp44.f[0]) + SQ(sp44.f[1]) + SQ(sp44.f[2]));
    if (var_fv1 != 0.0f) {
        var_fv1 = 1.0f / var_fv1;
    }
    sp44.f[0] *= var_fv1;
    sp44.f[1] *= var_fv1;
    sp44.f[2] *= var_fv1;
    //if (a3){} // @fake
    vox_func_80007EE0(barrelPos, &voxBarrelPos);
    vox_func_80007EE0(aimPoint, &voxAimPoint);
    sp5C.f[0] = aimPoint->f[0];
    sp5C.f[1] = aimPoint->f[1];
    sp5C.f[2] = aimPoint->f[2];
    sp38 = 1.0f;
    if (target != NULL) {
        // Hit sphere raycast(?)
        sp42 = func_8002AD3C(target, barrelPos, aimPoint, &sp5C, &sp38);
    } else {
        sp42 = 0;
    }
    // @recomp: Rewrite logic to use either sphere hit or vox hit pos (which is closer) and a 
    //          fallback if neither is hit. This routine is normally quite broken and uses
    //          uninitialized stack memory causing the aim to be messed up.
    a3->f[0] = aimPoint->f[0];
    a3->f[1] = aimPoint->f[1];
    a3->f[2] = aimPoint->f[2];
    _Bool hasVoxHit = FALSE;
    if (vox_func_80008048(&voxBarrelPos, &voxAimPoint, &voxHitPos, NULL, 0) == 0) {
        vox_func_80007E2C(&hitPos, &voxHitPos);
        hasVoxHit = TRUE;
    }
    if (sp42 && sp38 < 1.0f && hasVoxHit) {
        f32 sphereHitDist = vec3DistanceSquared(barrelPos, &sp5C);
        f32 voxHitDist = vec3DistanceSquared(barrelPos, &hitPos);

        if (sphereHitDist < voxHitDist) {
            fireAtPoint->f[0] = sp5C.f[0] + (sp44.f[0] * 5);
            fireAtPoint->f[1] = sp5C.f[1] + (sp44.f[1] * 5);
            fireAtPoint->f[2] = sp5C.f[2] + (sp44.f[2] * 5);
            return 1;
        } else {
            fireAtPoint->f[0] = hitPos.f[0] + (sp44.f[0] * 15);
            fireAtPoint->f[1] = hitPos.f[1] + (sp44.f[1] * 15);
            fireAtPoint->f[2] = hitPos.f[2] + (sp44.f[2] * 15);
            return 2;
        }
    } else if (sp42 && sp38 < 1.0f) {
        fireAtPoint->f[0] = sp5C.f[0] + (sp44.f[0] * 5);
        fireAtPoint->f[1] = sp5C.f[1] + (sp44.f[1] * 5);
        fireAtPoint->f[2] = sp5C.f[2] + (sp44.f[2] * 5);
        return 1;
    } else if (hasVoxHit) {
        fireAtPoint->f[0] = hitPos.f[0] + (sp44.f[0] * 15);
        fireAtPoint->f[1] = hitPos.f[1] + (sp44.f[1] * 15);
        fireAtPoint->f[2] = hitPos.f[2] + (sp44.f[2] * 15);
        return 2;
    } else {
        fireAtPoint->f[0] = aimPoint->f[0];
        fireAtPoint->f[1] = aimPoint->f[1];
        fireAtPoint->f[2] = aimPoint->f[2];
        return 0;
    }
}

RECOMP_HOOK_DLL(EWTrobotpatrol_stun) void EWTrobotpatrol_stun_hook(Object* self) {
    EWTrobotpatrol_Data* objdata = self->data;
    // Lower combat state when stunned so the robot forgets where the player is
    if (objdata->combatState == 2) {
        objdata->combatState = 1;
        // Immediately enter "looking around" gun mode
        if (objdata->gun.mode == 2) {
            EWTrobotpatrol_setGunMode(&objdata->gun, 1, NULL);
        }
    }
}
