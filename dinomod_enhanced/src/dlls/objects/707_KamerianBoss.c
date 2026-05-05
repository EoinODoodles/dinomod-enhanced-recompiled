#include "modding.h"

#include "PR/ultratypes.h"
#include "PR/gbi.h"
#include "dlls/engine/53.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/332_FXEmit.h"
#include "game/objects/object.h"
#include "recomputils.h"
#include "sys/gfx/model.h"
#include "sys/objects.h"
#include "sys/objanim.h"
#include "sys/objlib.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "dll.h"

#include "recomp/dlls/objects/707_KamerianBoss_recomp.h"

typedef struct {
/*00:0*/ u32 pad0_0 : 8;
/*00:8*/ u32 animFinished : 1;
/*00:9*/ u32 rightWingOpened : 1;
/*00:10*/ u32 leftWingOpened : 1;
/*00:11*/ u32 rightPipeDetached: 1;
/*00:12*/ u32 leftPipeDetached: 1;
/*00:13*/ u32 hatchOpened: 1;
/*00:14*/ u32 flameDebounce : 1;
/*00:15*/ u32 health : 8;
/*00:23*/ u32 loadedTempDLL : 1;
/*00:24*/ u32 pad0_24: 8;
/*04*/ Object *player;
/*08*/ Object *unk8[2];
/*10*/ Object *unk10[6];
/*28*/ f32 animTickDelta; // anim progress per tick (60hz)
/*2C*/ s16 rightPipeYOffset;
/*2E*/ s16 leftPipeYOffset;
/*30*/ u16 rightAcidAttackTimer;
/*32*/ u16 leftAcidAttackTimer;
/*34*/ s16 flameAttackTimer;
/*36*/ u16 rightPipeTimer;
/*38*/ u16 leftPipeTimer;
/*3C*/ u32 soundHandle1;
/*40*/ u32 soundHandle2;
/*44*/ u32 soundHandle3;
/*48*/ Vec3f attachmentPositions[15];
/*FC*/ f32 playerStartY;
} KamerianBoss_Data;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 unk18[0x2A-0x18];
/*2A*/ s8 yaw;
} KamerianBoss_Setup;

enum KDModAnims {
    KD_MODANIM_DETATCH_LEFT_PIPE = 0,
    KD_MODANIM_DETATCH_LEFT_PIPE_ALT = 1, // With right wing open
    KD_MODANIM_DETATCH_RIGHT_PIPE = 2,
    KD_MODANIM_DETATCH_RIGHT_PIPE_ALT = 3, // With left wing open
    KD_MODANIM_OPEN_LEFT_WING = 4,
    KD_MODANIM_OPEN_LEFT_WING_ALT = 5, // With right wing open
    KD_MODANIM_OPEN_RIGHT_WING = 6,
    KD_MODANIM_OPEN_RIGHT_WING_ALT = 7, // With left wing open
    KD_MODANIM_OPEN_HATCH = 8,
    KD_MODANIM_ATTACK = 9,
    KD_MODANIM_HURT = 10,
    KD_MODANIM_MELT = 11
};

/*0x0*/ extern u8 _data_0[8];
/*0x8*/ extern Model *sModel;
/*0xC*/ extern s16 sHealthBarTextureIDs[2];

/*0x0*/ extern Texture *sHealthBarTextures[2];
/*0x8*/ extern TextureTile _bss_8[2][2];
/*0x38*/ extern s32 sHealthBarAlpha;
/*0x40*/ extern u8 _bss_40[0x4c0];

extern void KamerianBoss_enable_hit_sphere(s32 hitSphereIdx);
extern void KamerianBoss_disable_hit_sphere(s32 hitSphereIdx);
extern Object* KamerianBoss_create_fx_emit(Object *self, f32 x, f32 y, f32 z, s32 arg4);
extern void KamerianBoss_create_projectile(Object *self, f32 x, f32 y, f32 z, s16 arg4, s16 arg5, f32 arg6, s32 objID);
extern void KamerianBoss_do_flame_attack(Object *self, KamerianBoss_Data *objdata);
extern void KamerianBoss_do_acid_attack(Object *self, KamerianBoss_Data *objdata, s32 side, u16 *timer);
extern void KamerianBoss_do_pipe_texture_anim(Object *self, s32 updateRate);
extern void KamerianBoss_func_E94(Object *self, s32 arg1);

RECOMP_PATCH void KamerianBoss_enable_hit_sphere(s32 hitSphereIdx) {
    s32 sp4;

    switch (hitSphereIdx) {
    case 0:
    case 1:
        sp4 = 24;
        break;
    // @recomp: 2 -> 14 (head)
    case 14:
        sp4 = 44;
        break;
    }
    sModel->hitSpheres[hitSphereIdx].unk2 = (s16) sp4;
}

RECOMP_PATCH void KamerianBoss_setup(Object *self, KamerianBoss_Setup *setup, s32 arg2) {
    s32 i;
    Texture *texture;
    s32 var_s0_2;
    u32 temp_v0;
    KamerianBoss_Data *objdata;

    self->animCallback = NULL;
    objdata = self->data;
    self->srt.yaw = setup->yaw << 8;
    func_80023D30(self, KD_MODANIM_DETATCH_RIGHT_PIPE, 0.0f, 0);
    bzero(objdata, sizeof(KamerianBoss_Data));
    objdata->health = 10;
    objdata->animTickDelta = 0.0f;
    func_8002674C(self);
    sModel = self->modelInsts[0]->model;
    sHealthBarAlpha = 0;

    // init hitspheres
    // @recomp: Fix hit sphere indices (disables just the wing hit spheres)
    KamerianBoss_disable_hit_sphere(0);
    KamerianBoss_disable_hit_sphere(1);

    for (i = 0; i < 2; i++) {
        texture = tex_load_deferred(sHealthBarTextureIDs[i]);
        sHealthBarTextures[i] = texture;
        _bss_8[i][0].tex = texture;
        _bss_8[i][0].animProgress = 0;
        _bss_8[i][0].x = 0;
        _bss_8[i][0].y = 0;
        _bss_8[i][1].tex = NULL;
    }

    // load fxemit objects
    i = 1;
    do {
        var_s0_2 = i != 0 ? 163 : -163;        
        objdata->unk8[i] = KamerianBoss_create_fx_emit(
            self,
            var_s0_2 + self->globalPosition.x,
            self->globalPosition.y + 175.0f,
            self->globalPosition.z + 145.0f,
            0x691
        );
        var_s0_2 = i--;
    } while (var_s0_2 != 0);
}

RECOMP_PATCH void KamerianBoss_control(Object *self) {
    ObjectHitInfo *temp_t6_3;
    s32 var_s0;
    s32 i;
    s32 j;
    s32 hitSphereIdx;
    s32 collisionType;
    //s32 var_a0;
    //s32 var_v1;
    KamerianBoss_Data *objdata;

    objdata = self->data;
    if (objdata->player == NULL) {
        objdata->player = get_player();
        // @recomp: The player's initial y coord is not reliable if they noclip into the room or somehow load
        //          the area without being on the floor. Use our position instead for a reference point of the floor. 
        //objdata->playerStartY = objdata->player->srt.transl.y;
        objdata->playerStartY = self->srt.transl.y + 52;
    }
    // @recomp: Disable when dead
    if (objdata->hatchOpened != 0 && objdata->health <= 0) {
        return;
    }
    if (objdata->player != NULL) {
        ((DLL_210_Player*)objdata->player->dll)->vtbl->add_magic(objdata->player, 10);
        if (objdata->rightPipeTimer) {
            if (gUpdateRate < objdata->rightPipeTimer) {
                objdata->rightPipeTimer -= gUpdateRate;
                for (i = 0; i < 6; i += 2) {
                    if (objdata->rightPipeTimer > 120.0f) {
                        KamerianBoss_func_E94(self, i);
                    } else {
                        objdata->unk10[i]->srt.transl.x = 0.0f;
                        objdata->unk10[i]->srt.transl.y = 0.0f;
                        objdata->unk10[i]->srt.transl.z = 0.0f;
                    }
                }
            } else {
                objdata->rightPipeTimer = 0;
                for (i = 0; i < 6; i += 2) {
                    obj_destroy_object(objdata->unk10[i]);
                    objdata->unk10[i] = NULL;
                }
            }
        }
        if (objdata->leftPipeTimer) {
            if (gUpdateRate < objdata->leftPipeTimer) {
                objdata->leftPipeTimer -= gUpdateRate;
                for (i = 1; i < 7; i += 2) {
                    if (objdata->leftPipeTimer > 120.0f) {
                        KamerianBoss_func_E94(self, i);
                    } else {
                        objdata->unk10[i]->srt.transl.x = 0.0f;
                        objdata->unk10[i]->srt.transl.y = 0.0f;
                        objdata->unk10[i]->srt.transl.z = 0.0f;
                    }
                };
            } else {
                objdata->leftPipeTimer = 0;
                for (i = 1; i < 7; i += 2) {
                    obj_destroy_object(objdata->unk10[i]);
                    objdata->unk10[i] = NULL;
                }
            }
        }
        KamerianBoss_do_pipe_texture_anim(self, gUpdateRate);
        objdata->animFinished = func_80024108(self, objdata->animTickDelta, gUpdateRateF, NULL);
        if (objdata->rightPipeYOffset != 0) {
            if (objdata->rightPipeYOffset < 15000) {
                objdata->rightPipeYOffset += gUpdateRate * 50;
            }
            func_80034804(self, 4)[7] = objdata->rightPipeYOffset;
        }
        if (objdata->leftPipeYOffset != 0) {
            if (objdata->leftPipeYOffset < 15000) {
                objdata->leftPipeYOffset += gUpdateRate * 50;
            }
            func_80034804(self, 3)[7] = objdata->leftPipeYOffset;
        }
        // Useless assignment of v1? required to match
        // var_v1 = 0;
        // if (objdata->rightWingOpened != 0) {
        //     var_v1 |= 1;
        // }
        // if (objdata->leftWingOpened != 0) {
        //     var_v1 |= 2;
        // }
        // if (objdata->hatchOpened != 0) {
        //     var_v1 |= 4;
        // }
        if ((objdata->animFinished != 0) && (objdata->animTickDelta != 0.0f)) {
            switch (self->curModAnimId) {
            case KD_MODANIM_DETATCH_RIGHT_PIPE:
            case KD_MODANIM_DETATCH_RIGHT_PIPE_ALT:
                KamerianBoss_enable_hit_sphere(0);
                break;
            case KD_MODANIM_DETATCH_LEFT_PIPE:
            case KD_MODANIM_DETATCH_LEFT_PIPE_ALT:
                KamerianBoss_enable_hit_sphere(1);
                break;
            case KD_MODANIM_OPEN_RIGHT_WING:
            case KD_MODANIM_OPEN_RIGHT_WING_ALT:
                objdata->rightWingOpened = TRUE;
                //var_a0 = 1;
                gDLL_6_AMSFX->vtbl->play(self, SOUND_9AE, MAX_VOLUME, NULL, NULL, 0, NULL);
                if (objdata->soundHandle1 != 0) {
                    gDLL_6_AMSFX->vtbl->stop(objdata->soundHandle1);
                    objdata->soundHandle1 = 0;
                }
                break;
            case KD_MODANIM_OPEN_LEFT_WING:
            case KD_MODANIM_OPEN_LEFT_WING_ALT:
                objdata->leftWingOpened = TRUE;
                gDLL_6_AMSFX->vtbl->play(self, SOUND_9AE, MAX_VOLUME, NULL, NULL, 0, NULL);
                if (objdata->soundHandle2 != 0) {
                    gDLL_6_AMSFX->vtbl->stop(objdata->soundHandle2);
                    objdata->soundHandle2 = 0;
                }
                break;
            case KD_MODANIM_OPEN_HATCH:
                // @recomp: Re-enable head hit sphere. Technically, this shrinks the sphere. The initial size
                //          is quite big to cover more of the closed hatch, but needs to be shrank once open
                //          to better represent the size of the head.
                KamerianBoss_enable_hit_sphere(14);
                objdata->hatchOpened = TRUE;
                gDLL_6_AMSFX->vtbl->play(self, SOUND_9AE, MAX_VOLUME, NULL, NULL, 0, NULL);
                if (objdata->soundHandle3 != 0) {
                    gDLL_6_AMSFX->vtbl->stop(objdata->soundHandle3);
                    objdata->soundHandle3 = 0;
                }
                break;
            }
            objdata->animTickDelta = 0.0f;
        }
        var_s0 = 0;
        if (objdata->rightWingOpened != 0) {
            var_s0 |= 1;
        }
        if (objdata->leftWingOpened != 0) {
            var_s0 |= 2;
        }
        if (objdata->hatchOpened != 0) {
            var_s0 |= 4;
        }
        if ((var_s0 == 3) && (self->curModAnimId < KD_MODANIM_OPEN_HATCH)) {
            func_80023D30(self, KD_MODANIM_OPEN_HATCH, 0.0f, 0);
            // @recomp: Start boss music
            //          (original patch by MusicalProgrammer)
            gDLL_5_AMSEQ->vtbl->set(NULL, 0xFE, 0, 0, 0);
            objdata->animTickDelta = 0.01f;
            sHealthBarAlpha = gUpdateRate;
            gDLL_6_AMSFX->vtbl->play(self, SOUND_9AD, MAX_VOLUME, &objdata->soundHandle3, NULL, 0, NULL);
        }
        func_800269CC(self, self->objhitInfo, _data_0[var_s0]);
        i = self->objhitInfo->unk62;
        while (i--) {
            hitSphereIdx = self->objhitInfo->unk63[i];
            collisionType = self->objhitInfo->hitTypeList[i];
            if (objdata->animTickDelta == 0.0f) {
                // @recomp: Fix hit sphere indices
                switch (hitSphereIdx) {
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                    if (objdata->rightPipeTimer == 0) {
                        objdata->rightPipeTimer = 600;
                        for (j = 0; j < 6; j += 2) {
                            objdata->unk10[j] = KamerianBoss_create_fx_emit(self, self->globalPosition.x - 163.0f, self->globalPosition.y + 175.0f, self->globalPosition.z + 145.0f, 0x693);
                        }
                        gDLL_6_AMSFX->vtbl->play(self, SOUND_9AA, MAX_VOLUME, NULL, NULL, 0, NULL);
                    } else if ((collisionType == Damage_Type_Projectile) && (objdata->rightPipeTimer > 50)) {
                        // @recomp: Fix hit sphere indices
                        KamerianBoss_disable_hit_sphere(7);
                        KamerianBoss_disable_hit_sphere(8);
                        KamerianBoss_disable_hit_sphere(9);
                        KamerianBoss_disable_hit_sphere(10);
                        KamerianBoss_disable_hit_sphere(11);
                        func_80023D30(self, 
                            objdata->leftWingOpened ? KD_MODANIM_DETATCH_RIGHT_PIPE_ALT : KD_MODANIM_DETATCH_RIGHT_PIPE, 
                            0.0f, 0);
                        objdata->animTickDelta = 0.005f;
                        objdata->rightPipeYOffset = 1;
                        objdata->rightPipeDetached = TRUE;
                        obj_destroy_object(objdata->unk8[0]);
                        gDLL_6_AMSFX->vtbl->play(self, SOUND_9AB, MAX_VOLUME, NULL, NULL, 0, NULL);
                    }
                    break;
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                    if (objdata->leftPipeTimer == 0) {
                        objdata->leftPipeTimer = 600;
                        for (j = 1; j < 7; j += 2) {
                            objdata->unk10[j] = KamerianBoss_create_fx_emit(self, self->globalPosition.x + 163.0f, self->globalPosition.y + 175.0f, self->globalPosition.z + 145.0f, 0x693);
                        }
                        gDLL_6_AMSFX->vtbl->play(self, SOUND_9AA, MAX_VOLUME, NULL, NULL, 0, NULL);
                    } else if ((collisionType == Damage_Type_Projectile) && (objdata->leftPipeTimer > 50)) {
                        // @recomp: Fix hit sphere indices
                        KamerianBoss_disable_hit_sphere(2);
                        KamerianBoss_disable_hit_sphere(3);
                        KamerianBoss_disable_hit_sphere(4);
                        KamerianBoss_disable_hit_sphere(5);
                        KamerianBoss_disable_hit_sphere(6);
                        func_80023D30(self, 
                            objdata->rightWingOpened ? KD_MODANIM_DETATCH_LEFT_PIPE_ALT : KD_MODANIM_DETATCH_LEFT_PIPE, 
                            0.0f, 0);
                        objdata->animTickDelta = 0.005f;
                        objdata->leftPipeYOffset = 1;
                        objdata->leftPipeDetached = TRUE;
                        obj_destroy_object(objdata->unk8[1]);
                        gDLL_6_AMSFX->vtbl->play(self, SOUND_9AB, MAX_VOLUME, NULL, NULL, 0, NULL);
                    }
                    break;
                case 0:
                    if (collisionType == Damage_Type_Projectile) {
                        if ((objdata->leftPipeDetached) && (objdata->rightPipeDetached)) {
                            KamerianBoss_disable_hit_sphere(0);
                            func_80023D30(self, 
                                objdata->leftWingOpened ? KD_MODANIM_OPEN_RIGHT_WING_ALT : KD_MODANIM_OPEN_RIGHT_WING, 
                                0.0f, 0);
                            objdata->animTickDelta = 0.005f;
                            gDLL_6_AMSFX->vtbl->play(self, SOUND_9AC, MAX_VOLUME, NULL, NULL, 0, NULL);
                            gDLL_6_AMSFX->vtbl->play(self, SOUND_9AD, MAX_VOLUME, &objdata->soundHandle1, NULL, 0, NULL);
                        }
                    }
                    break;
                case 1:
                    if (collisionType == Damage_Type_Projectile) {
                        if ((objdata->leftPipeDetached) && (objdata->rightPipeDetached)) {
                            KamerianBoss_disable_hit_sphere(1);
                            func_80023D30(self, 
                                objdata->rightWingOpened ? KD_MODANIM_OPEN_LEFT_WING_ALT : KD_MODANIM_OPEN_LEFT_WING, 
                                0.0f, 0);
                            objdata->animTickDelta = 0.005f;
                            gDLL_6_AMSFX->vtbl->play(self, SOUND_9AC, MAX_VOLUME, NULL, NULL, 0, NULL);
                            gDLL_6_AMSFX->vtbl->play(self, SOUND_9AD, MAX_VOLUME, &objdata->soundHandle2, NULL, 0, NULL);
                        }
                    }
                    // @recomp: Don't fallthrough
                    break;
                // @recomp: Correctly handle being damaged
                case 13:
                case 14:
                    if (objdata->hatchOpened != 0 && objdata->health > 0) {
                        objdata->health--;

                        gDLL_6_AMSFX->vtbl->play(self, 0x965, 0x7F, NULL, NULL, 0, NULL);
                        func_80023D30(self, 10, 0.0f, 0);
                        objdata->animTickDelta = 0.005f;
                        objdata->flameAttackTimer = 144;

                        if (objdata->health <= 0) {
                            main_set_bits(0x18A, 1);
                        }
                    }
                    break;
                }
            }
        }
        if (objdata->hatchOpened != 0) {
            KamerianBoss_do_flame_attack(self, objdata);
        }
        if ((objdata->rightPipeDetached) && !(objdata->rightWingOpened)) {
            KamerianBoss_do_acid_attack(self, objdata, 0, &objdata->rightAcidAttackTimer);
        }
        if ((objdata->leftPipeDetached) && !(objdata->leftWingOpened)) {
            KamerianBoss_do_acid_attack(self, objdata, 1, &objdata->leftAcidAttackTimer);
        }
        if ((sHealthBarAlpha != 0) && (sHealthBarAlpha < 0xFF)) {
            sHealthBarAlpha += gUpdateRate;
            if (sHealthBarAlpha > 0xFF) {
                sHealthBarAlpha = 0xFF;
            }
        }
        if ((objdata->hatchOpened) && (objdata->player != NULL)) {
            if (!objdata->loadedTempDLL) {
                u16 sp8C[3] = {0x0002, 0x0000, 0x0000}; // _data_10
                u16 sp84[3] = {0x0018, 0x0014, 0x0008}; // _data_18
                create_temp_dll(53);
                ((DLL_53*)gTempDLLInsts[1])->vtbl->func2(self, _bss_40, -18000, 9800, 3);
                ((DLL_53*)gTempDLLInsts[1])->vtbl->func6(_bss_40, &sp84, &sp84, 3);
                _bss_40[0x4A9] |= 8;
                objdata->loadedTempDLL = TRUE;
            }
            ((DLL_53*)gTempDLLInsts[1])->vtbl->func1(_bss_40, objdata->player);
            ((DLL_53*)gTempDLLInsts[1])->vtbl->func0(self, _bss_40);
        }
    }
}

RECOMP_PATCH void KamerianBoss_print(Object *self, Gfx **gdl, Mtx **mtxs, Vertex **vtxs, Triangle **pols, s8 visibility) {
    KamerianBoss_Data *objdata;
    s32 hpBarWidth;
    s32 i;

    objdata = self->data;
    // @recomp: Change to floating point division to prevent any damage rounding immedately to zero.
    // @recomp: Scale to the actual texture width.
    hpBarWidth = ((f32)objdata->health / 10.0f) * (sHealthBarTextures[0]->width - 18);
    if ((visibility != 0) && (self->unkDC == 0)) {
        // Draw self
        draw_object(self, gdl, mtxs, vtxs, pols, 1.0f);
        // Draw health bar
        if (sHealthBarAlpha != 0 && objdata->health > 0) {
            // @recomp: New, but still kinda jank, health bar
            rcp_tile_write_x(gdl, _bss_8[0], 
                /*x*/96.0f, 
                /*y*/24.0f, 
                /*width*/(f32) hpBarWidth, 
                /*height*/sHealthBarTextures[0]->height - 8, 
                /*s*/24 << 4, 
                /*t*/8 << 4, 
                /*xScale*/(128.0f / (sHealthBarTextures[0]->width - 18)), 
                /*yScale*/1.0f, 
                /*color*/0xFF000000 | (sHealthBarAlpha & 0xFF), 
                /*flags*/TILE_WRITE_TRANSLUCENT | TILE_WRITE_POINT_FILT);

            rcp_tile_write_x(gdl, _bss_8[0], 
                /*x*/96.0f + ((hpBarWidth) * (128.0f / (sHealthBarTextures[0]->width - 18))), 
                /*y*/24.0f, 
                /*width*/(f32) ((sHealthBarTextures[0]->width - 18) - hpBarWidth), 
                /*height*/sHealthBarTextures[0]->height - 8, 
                /*s*/24 << 4, 
                /*t*/8 << 4, 
                /*xScale*/(128.0f / (sHealthBarTextures[0]->width - 18)), 
                /*yScale*/1.0f, 
                /*color*/0x330000FF, 
                /*flags*/TILE_WRITE_TRANSLUCENT | TILE_WRITE_POINT_FILT);
        }
        // Get attachment positions
        i = 15;
        while(i--) {
            func_80031F6C(self, 
                i, 
                &objdata->attachmentPositions[i].x, 
                &objdata->attachmentPositions[i].y, 
                &objdata->attachmentPositions[i].z, 
                0);
        }
        // Head lookat
        if (objdata->loadedTempDLL) {
            // @recomp: Fix look-at origin joint ID (original patch by MusicalProgrammer)
            ((DLL_53*)gTempDLLInsts[1])->vtbl->func3(self, _bss_40, 12);
        }
    }
}
