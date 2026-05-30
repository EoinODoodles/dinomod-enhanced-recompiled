#include "SHbarrel.h"
#include "custom_objsetups.h"

#include "dlls/engine/54_pickup.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/541_DIMexplosion.h"
#include "game/objects/interaction_arrow.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "player_util.h"
#include "recomputils.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objhits.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "dll.h"

typedef struct {
    Pickup pickup;
    struct {
        Unk80027934 col;
        AABBs32 aabb;
        Vec3f trackPrevPos[1];
        f32 hitsTestRadii[1];
        Vec3f hitsPrevPos[1];
    } collider;
    u8 prevInWater;
    u8 inWater;
    u8 inDeepWater;
    u8 damage;
    s32 ticksSinceDetonation; 
    f32 waterY;
    f32 bumpCooldown;
    f32 waterRippleCooldown;
    f32 prevYVelocity;
} SHbarrel_Data;

void SHbarrel_ctor(void *dll) { }
void SHbarrel_dtor(void *dll) { }

static void SHbarrel_setup(Object* self, SHBarrel_Setup* setup, s32 reset) {
    SHbarrel_Data* objdata = self->data;

    obj_add_object_type(self, OBJTYPE_Barrel);

    self->srt.yaw = setup->yaw << 8;
    self->opacity = 0;

    gDLL_54_pickup->vtbl->setup(self, (Pickup*)self->data, 33);
    gDLL_54_pickup->vtbl->set_dont_save(self->data, TRUE);

    // Collider setup
    objdata->collider.col.unk54[0] = 1; // some radius? not the radius of the barrel
    objdata->collider.col.unk50[0] = -1;
    objdata->collider.col.unk40[0] = self->objhitInfo->unk52; // barrel radius
    objdata->collider.hitsTestRadii[0] = self->objhitInfo->unk52;

    // Collider init
    objdata->collider.trackPrevPos[0] = self->globalPosition;
    objdata->collider.hitsPrevPos[0] = self->globalPosition;
}

static void SHbarrel_drop_if_player_underwater(Object* self) {
    Object* player = get_player();
    if (player == NULL || player->data == NULL) {
        return;
    }

    Player_Data* playerData = player->data;
    if (playerData->unk0.unk4.underwaterDist > 25.0f) {
        playerUtil_stop_carrying(player);
    }
}

static void SHbarrel_player_push(Object* self, SHbarrel_Data* objdata) {
    Object* player = get_player();
    if (player == NULL) {
        return;
    }
    
    if ((self->objhitInfo->unk58 & 8) && self->objhitInfo->unk0 == player) {
        // Touched by player

        // Get vec from player to barrel
        Vec3f vec;
        VECTOR_SUBTRACT(self->globalPosition, player->globalPosition, vec);
        vec.y = 0.0f;
        f32 playerDist = VECTOR_MAGNITUDE(vec);
        VECTOR_DIVIDE_BY_SCALAR(vec, playerDist);

        // Push
        const f32 pushForce = 1.5f;
        f32 playerSpeed = VECTOR_MAGNITUDE(player->velocity);
        VECTOR_MULTIPLY_BY_SCALAR(vec, MIN(playerSpeed, pushForce) * gUpdateRateF);

        self->velocity.x += vec.x;
        self->velocity.z += vec.z;

        // Bump barrel vertically when pushed
        if (self->velocity.y >= 0.0f && playerSpeed > 0.1f && objdata->bumpCooldown <= 0.0f) {
            self->velocity.y += 0.1f;
            objdata->bumpCooldown = 30.0f;
        }
    }
}

static void SHbarrel_physics(Object* self, SHbarrel_Data* objdata) {
    s32 count;
    s32 i;
    f32 delta;
    f32 diffMagnitude;
    Func_80057F1C_Struct** collisionInfo;
    f32 minDiffMagnitude;
    f32 yDiff;

    if (objdata->bumpCooldown > 0.0f) {
        objdata->bumpCooldown -= gUpdateRateF;
    }

    objdata->inWater = FALSE;
    objdata->inDeepWater = FALSE;
    objdata->waterY = -100000.0f;

    // TODO: this is super framerate dependent...

    //Float on water
    {
        //Find the nearest water plane
        count = func_80057F1C(self, self->srt.transl.x, self->srt.transl.y, self->srt.transl.z, &collisionInfo, 0, 0);
        if (count) {
            delta = 10000.0f;
            s32 floorFound = FALSE;
            f32 floorDist = 100.0f;
            
            for (i = 0; i < count; i++){
                if (collisionInfo[i]->unk14 == 14) { //water only
                    yDiff = self->srt.transl.y - collisionInfo[i]->unk0[0];
                    if (yDiff >= 0) {
                        diffMagnitude = yDiff;
                    } else {
                        diffMagnitude = -yDiff;
                    }
                    
                    if (delta >= 0) {
                        minDiffMagnitude = delta;
                    } else {
                        minDiffMagnitude = -delta;
                    }
                    
                    if (diffMagnitude < minDiffMagnitude) {
                        delta = yDiff;
                        objdata->waterY = collisionInfo[i]->unk0[0];
                    }
                } else {
                    if (!floorFound && collisionInfo[i]->unk0[0] < (self->srt.transl.y + 5.0f) && collisionInfo[i]->unk0[2] > 0.707f) {
                        floorDist = self->globalPosition.y - collisionInfo[i]->unk0[0];
                        floorFound = TRUE;
                    }
                }
            }

            //Correlate velocityY with barrel's depth underwater
            if ((delta != 10000.0f) && (delta < 0)) {
                const f32 offset = 5.0f;
                self->velocity.y += (-(delta + offset) * 0.1f);
                self->velocity.y *= 0.8f;

                // Let player push barrel while in water
                if (floorFound && floorDist > 0.0f) {
                    SHbarrel_player_push(self, objdata);
                    objdata->inWater = TRUE;
                } else if (delta < -2.0f) {
                    // Hit the floor but still reasonably underwater, don't count as beached
                    objdata->inWater = TRUE;
                }

                if (floorDist > 25.0f) {
                    objdata->inDeepWater = TRUE;
                }
            }
        }

        // Damp xz velocity
        if (objdata->inWater) {
            self->velocity.x *= 0.85f;
            self->velocity.z *= 0.85f;
        } else {
            self->velocity.x *= 0.5f;
            self->velocity.z *= 0.5f;
        }
    }

    obj_move(self, self->velocity.x, self->velocity.y, self->velocity.z);
}

static void SHbarrel_collision(Object* self, SHbarrel_Data* objdata, s32 pickupState) {
    // Note: Collision detection assumes a single test point at localspace 0,0,0

    if (pickupState == PICKUP_NotHeld) {
        // Do hit line collision detection
        Vec3f currPos[1] = {self->srt.transl};
        func_80059C40(
            &objdata->collider.hitsPrevPos[0], 
            &currPos[0], 
            objdata->collider.hitsTestRadii[0], 
            0, 
            NULL, 
            self, 
            /*config*/1, -1, 0, 0);
        
        // Do hit line collision resolution
        self->srt.transl.x = currPos[0].x;
        self->srt.transl.z = currPos[0].z;

        objdata->collider.hitsPrevPos[0].x = currPos[0].x;
        objdata->collider.hitsPrevPos[0].z = currPos[0].z;
        objdata->collider.hitsPrevPos[0].y = self->srt.transl.y;
    } else {
        // Being held, don't do resolution
        objdata->collider.hitsPrevPos[0] = self->srt.transl;
    }

    // Recalculate global position
    transform_point_by_object(
        self->srt.transl.x, self->srt.transl.y, self->srt.transl.z, 
        &self->globalPosition.x, &self->globalPosition.y, &self->globalPosition.z, 
        self->parent);

    // Create AABB to test for collisions within
    Vec3f currPos[1] = {self->globalPosition};
    f32 radius[1] = {objdata->collider.col.unk40[0]};
    radius[0] = sqrtf((2 * radius[0]) * radius[0]);

    fit_aabb_around_cubes(&objdata->collider.aabb, currPos, objdata->collider.trackPrevPos, radius, 1);

    // Do track collision detection
    func_80053750(self, &objdata->collider.aabb, 1);

    if (pickupState == PICKUP_NotHeld) {
        // Do track collision resolution
        Vec3f newPos[1] = {self->globalPosition};
        objdata->collider.col.unk50[0] = -1;

        func_8005509C(self, 
            objdata->collider.trackPrevPos[0].f, newPos[0].f, 1, 
            &objdata->collider.col, 0);

        objdata->collider.trackPrevPos[0] = newPos[0];

        // Note: Don't update y coord here, the pickup DLL and float logic will handle that
        self->globalPosition.x = objdata->collider.trackPrevPos[0].x;
        self->globalPosition.z = objdata->collider.trackPrevPos[0].z;
    } else {
        // Being held, don't do resolution
        objdata->collider.trackPrevPos[0] = self->globalPosition;
        objdata->collider.col.unk50[0] = -1;
    }

    // Recalculate local position
    inverse_transform_point_by_object(
        self->globalPosition.x, self->globalPosition.y, self->globalPosition.z, 
        &self->srt.transl.x, &self->srt.transl.y, &self->srt.transl.z, 
        self->parent);
}

static void SHbarrel_handle_damage(Object* self, SHbarrel_Data* objdata) {
    //Check for damage, return early if nothing damaged the barrel
    s32 hitDamage;
    s32 damageType = func_80025F40(self, NULL, NULL, &hitDamage);
    if (damageType == 0) {
        return;
    }

    // Don't blow up instantly if the player accidentally hits it with their sword
    if ((damageType == Damage_Type_Explosion) || (damageType == Damage_Type_Projectile)) {
        objdata->damage += 3;
    } else {
        objdata->damage += 1;
    }

    //When damaged, play an impact sound
    gDLL_6_AMSFX->vtbl->play(self, SOUND_372_Crate_Struck, MAX_VOLUME, NULL, NULL, 0, NULL);

    if (objdata->damage < 3) {
        return;
    }

    //Explode!
    {
        // e x p a n d
        func_8002683C(self, 
            self->def->hitbox_flagsB6 + 35, 
            self->def->unk94 - 35, 
            self->def->unk96 + 35
        );

        func_80026940(self, 40);
        func_80026128(self, Damage_Type_Explosion, 4, 0);
        
        DIMExplosion_Setup* explosion = obj_alloc_setup(sizeof(DIMExplosion_Setup), OBJ_DIMExplosion);
        explosion->base.x = self->srt.transl.x;
        explosion->base.y = self->srt.transl.y;
        explosion->base.z = self->srt.transl.z;
        obj_create((ObjSetup*)explosion, OBJINIT_STANDALONE | OBJINIT_FLAG4, self->mapID, -1, self->parent);
        
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_355, NULL, 0, -1, NULL);
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_352, NULL, 0, -1, NULL);

        if (objdata->inWater) {
            gDLL_24_Waterfx->vtbl->set_circular_ripple_scale(FALSE, 0.06f);
            gDLL_24_Waterfx->vtbl->spawn_circular_ripple(self->globalPosition.x, objdata->waterY, self->globalPosition.z, 0, 0.0f, 2);
            gDLL_24_Waterfx->vtbl->set_circular_ripple_scale(TRUE, 0.0f);
        }
    }

    objdata->ticksSinceDetonation = 1;
}

static void SHbarrel_control(Object* self) {
    SHbarrel_Data* objdata = self->data;

    if (objdata->waterRippleCooldown > 0.0f) {
        objdata->waterRippleCooldown -= gUpdateRateF;
    }

    // Fade in
    if ((self->opacity < OBJECT_OPACITY_MAX) && (objdata->ticksSinceDetonation == 0)) {
        const s32 fadeSpeed = 5;
        if ((gUpdateRate * fadeSpeed) < (OBJECT_OPACITY_MAX - self->opacity)) {
            self->opacity += gUpdateRate * fadeSpeed;
        } else {
            self->opacity = OBJECT_OPACITY_MAX;
        }
    }

    // Handle post detonation
    if (objdata->ticksSinceDetonation > 0) {
        if (objdata->ticksSinceDetonation == 1) {
            func_800267A4(self);
            self->unkAF |= ARROW_FLAG_8_No_Targetting;
        }

        objdata->ticksSinceDetonation += gUpdateRate;

        if (objdata->ticksSinceDetonation >= (60 * 4)) {
            obj_destroy_object(self);
        }

        return;
    }

    s32 pickupState = gDLL_54_pickup->vtbl->get_state(self->data);
    if (pickupState == PICKUP_Held) {
        // Hacky, better to do this in the player DLL some day
        SHbarrel_drop_if_player_underwater(self);
    }

    pickupState = gDLL_54_pickup->vtbl->control(self, &objdata->pickup);
    switch (pickupState) {
        case PICKUP_NotHeld:
            SHbarrel_physics(self, objdata);
            SHbarrel_handle_damage(self, objdata);
            // Don't allow pickup while in deep water
            self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
            if (objdata->inDeepWater) {
                self->unkAF |= ARROW_FLAG_10_Greyed_Out;
            } else {
                self->unkAF &= ~ARROW_FLAG_10_Greyed_Out;
            }
            // Water ripples
            if (objdata->inWater) {
                // Spawn a movement ripple when being pushed fast enough
                Vec3f xzVelocity = { self->velocity.x, 0.0f, self->velocity.z };
                f32 speed = VECTOR_MAGNITUDE(xzVelocity);
                if (speed > 0.5f && objdata->waterRippleCooldown <= 0.0f) {
                    objdata->waterRippleCooldown = 3.0f;

                    s32 angle = arctan2_f(-self->velocity.x, -self->velocity.z);
                    gDLL_24_Waterfx->vtbl->spawn_movement_ripple(self->globalPosition.x, objdata->waterY, self->globalPosition.z, angle, 0.0f);
                }

                // Do a splash
                if ((self->velocity.y < 0.0f && objdata->prevYVelocity >= 0.0f) || !objdata->prevInWater) {
                    gDLL_24_Waterfx->vtbl->set_circular_ripple_scale(FALSE, 0.03f);
                    gDLL_24_Waterfx->vtbl->spawn_circular_ripple(self->globalPosition.x, objdata->waterY, self->globalPosition.z, 0, 0.0f, 3);
                    gDLL_24_Waterfx->vtbl->set_circular_ripple_scale(TRUE, 0.0f);
                }
            }
            break;
        case PICKUP_Held:
        default:
            // Reset velocity while held
            self->velocity.x = 0.0f;
            self->velocity.y = 0.0f;
            self->velocity.z = 0.0f;

            objdata->waterRippleCooldown = 0.0f;
            break;
    }

    objdata->prevYVelocity = self->velocity.y;
    objdata->prevInWater = objdata->inWater;

    SHbarrel_collision(self, objdata, pickupState);
}

static void SHbarrel_update(Object* self) {}

static void SHbarrel_print(Object* self, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    SHbarrel_Data* objdata = self->data;

    if (objdata->ticksSinceDetonation == 0 && gDLL_54_pickup->vtbl->should_print(self, visibility)) {
        draw_object(self, gdl, mtxs, vtxs, pols, 1.0f);
    }
}

static void SHbarrel_free(Object* self, s32 onlySelf) {
    obj_free_object_type(self, OBJTYPE_Barrel);
    gDLL_54_pickup->vtbl->free(self);
}

static u32 SHbarrel_get_model_flags(Object *self) {
    return MODFLAGS_NONE;
}

static u32 SHbarrel_get_data_size(Object *self, u32 offsetAddr) {
    return sizeof(SHbarrel_Data);
}

DLL_IObject_Vtbl DLL_SHbarrel_vtbl = {
    .setup = (void*)SHbarrel_setup,
    .control = SHbarrel_control,
    .update = SHbarrel_update,
    .print = SHbarrel_print,
    .free = SHbarrel_free,
    .get_model_flags = SHbarrel_get_model_flags,
    .get_data_size = SHbarrel_get_data_size
};
