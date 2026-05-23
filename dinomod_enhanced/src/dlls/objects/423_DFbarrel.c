#include "dlls/engine/27.h"
#include "dlls/objects/210_player.h"
#include "game/objects/interaction_arrow.h"
#include "game/objects/object.h"
#include "math_util.h"
#include "modding.h"
#include "player_util.h"
#include "recomputils.h"
#include "common_objsetups.h"

#include "common.h"
#include "string_util.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/memory.h"
#include "sys/objects.h"
#include "sys/objtype.h"
// #include "dlls/objects/357_ExplodeAnimator.h"
// #include "dlls/objects/358_debris.h"
#include "dlls/objects/418_DFriverflow.h"
// #include "dlls/objects/541_DIMexplosion.h"

#include "recomp/dlls/_asm/423_recomp.h"
#include "sys/print.h"

//TODO: remove after decomp update
#define OBJTYPE_27 27
#define OBJTYPE_35 53
#define PARTICLE_355 0x355
#define Damage_Type_Explosion Damage_Type_Barrel_Explosion
#define DFbarrel_setup dll_423_setup
#define DFbarrel_control dll_423_control
#define DFbarrel_update dll_423_update
#define DFbarrel_free dll_423_free
#define DFbarrel_get_data_size dll_423_get_data_size
#define DFbarrel_handle_movement dll_423_func_304
#define DFbarrel_handle_damage dll_423_func_960

typedef struct {
/*00*/    ObjSetup base;
/*18*/    s16 xMin;            //Random position variance for explosion particles
/*1A*/    s16 yMin;            //Random position variance for explosion particles
/*1C*/    s16 zMin;            //Random position variance for explosion particles
/*1E*/    s16 xMax;            //Random position variance for explosion particles
/*20*/    s16 yMax;            //Random position variance for explosion particles
/*22*/    s16 zMax;            //Random position variance for explosion particles
/*24*/    s16 particleID;      //Type of particle to create for the explosion
/*26*/    s16 unused26;
/*28*/    s16 param0Max;       //Random variance for particle param 0
/*2A*/    s16 param1Max;       //Random variance for particle param 1
/*2C*/    u8 particleCount;    //Number of particles to create
/*2D*/    u8 pad2D;
/*2E*/    s16 param0Min;       //Random variance for particle param 0
/*30*/    s16 param1Min;       //Random variance for particle param 1
/*32*/    s16 gamebitExploded; //gamebit set when explosion has played out
/*34*/    s16 gamebitExplodeTrigger;  //gamebit checked to trigger the explosion
} ExplodeAnimator_Setup;

typedef struct {
    ObjSetup base;
    s8 unk18;
    s16 unk1A; //yaw
    s16 unk1C; //pitch
    s16 unk1E; //roll
    s16 unk20;
    s16 unk22;
    s16 unk24;
    s16 unk26;
    s16 unk28;
    s16 unk2A;
    s16 unk2C;
    s16 unk2E;
    s16 unk30;
    s16 unk32;
    s16 unk34;
    s16 unk36;
    u16 unk38;
    s16 unk3A;
    s8 unk3C;
    s16 unk3E; //gamebit?
    s16 unk40; //gamebit?
    s16 unk42; //x
    s16 unk44; //y
    s16 unk46; //z
} Debris_Setup; //0x48

typedef struct {
    ObjSetup base;
    s16 unk18;
    s16 unk1A;
    s16 unk1C;
    s16 unk1E;
    s16 unk20;
    s16 unk22;
} DIMExplosion_Setup; //0x24

typedef struct {
    ObjSetup base;
    s16 _unused18;
    u8 yaw;
    s16 _unused1C;
    s8 wasRespawned;  //@recomp: track barrels recreated by DFbarrelcreator
    s8 hitPoints;     //@recomp: repurposed as health
} DFBarrel_Setup; //0x20

typedef struct {
    u8 _unk0[0xA - 0x0];
    u8 damage;                  //Damage accumulated by the barrel (explodes if it's damaged at all, though!)
    u8 framesSinceDetonation;   //Seems intended to count up to deleting the barrel after it explodes, but it's deleted immediately anyway
    s32 _unusedC;
    f32 accelerationX;          //Acceleration from DFriverflow currents
    f32 accelerationZ;          //Acceleration from DFriverflow currents
    f32 velocityX;              //Copy of barrel's velocity, for flow/bouyancy calcs
    f32 velocityY;              //Copy of barrel's velocity, for flow/bouyancy calcs
    f32 velocityZ;              //Copy of barrel's velocity, for flow/bouyancy calcs
} DFBarrel_Data;

extern void DFbarrel_handle_movement(Object* self);
extern void DFbarrel_handle_damage(Object* self);

RECOMP_PATCH void DFbarrel_setup(Object* self, DFBarrel_Setup* objSetup, s32 reset) {
    obj_add_object_type(self, OBJTYPE_27);
    self->srt.yaw = objSetup->yaw << 8;
    self->stateFlags |= OBJSTATE_UPDATE_DISABLED;
    ((DLL_Unknown*)gDLL_54)->vtbl->func[0].withThreeArgs((s32)self, (s32)self->data, 33); //TODO: remove cast once function signature understood

    //@recomp: fade in when respawned
    if (objSetup->wasRespawned) {
        self->opacity = 0;
    }
}

// // @recomp: Check if player is holding the barrel while underwater, and let go of it
// // TODO: revisit this/add a more general patch into the player DLL to fix underwater carrying?
// RECOMP_PATCH void DFbarrel_update(Object* self) {
//     Player_Data* playerData;
//     Object* player = get_player();
//     if (!player) {
//         return;
//     }
//     playerData = player->data;
//     if (!playerData) {
//         return;
//     }
    
//     if (playerData->unk868 != self) {
//         return;
//     }

//     if (playerData->unk0.unk4.underwaterDist > 26.0f) {
//         playerUtil_stop_carrying(self); //nooo, crashes... 
//     }
// }

#define FADE_SPEED 5

RECOMP_PATCH void DFbarrel_control(Object* self) {
    DFBarrel_Data* objData = self->data;
    
    //@recomp: fade in
    if ((self->opacity < OBJECT_OPACITY_MAX) && (objData->framesSinceDetonation == 0)) {
        if ((gUpdateRate * FADE_SPEED) < (OBJECT_OPACITY_MAX - self->opacity)) {
            self->opacity += gUpdateRate * FADE_SPEED;
        } else {
            self->opacity = OBJECT_OPACITY_MAX;
        }
    }

    //Do nothing if not on a map?
    if (map_world_coords_to_block_index(self->srt.transl.x, self->srt.transl.y, self->srt.transl.z) == -1) {
        return;
    }
    
    switch (objData->framesSinceDetonation) {
    case 0:
        if (gDLL_54->vtbl->func1.withOneArgS32((s32)self) == 0) {
            DFbarrel_handle_movement(self);
            DFbarrel_handle_damage(self);
        }
        break;
    case 1:
        func_800267A4(self);
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
        // objData->framesSinceDetonation = 20;        //@recomp: don't skip to being destroyed
        // return;                                     //@recomp: fallthrough
    default:
        objData->framesSinceDetonation += gUpdateRate; //@recomp: framerate-independent
        break; //@recomp: don't fallthrough
    // case 20:
    //     obj_destroy_object(self);
    //     break;
    }

    diPrintf("framesSinceDetonation: %d\n", objData->framesSinceDetonation);

    //@recomp: move out of switch to account for gUpdateRate pushing past switch case 20
    if (objData->framesSinceDetonation >= 100) { //@recomp: let the ember particles hang around a little longer
        obj_destroy_object(self);
    }
}

/* Lets the player push the barrel around while it's in water */
static void DFbarrel_handle_player_water_behaviour(Object* self) {
    Object* player;
    Player_Data* playerData;
    _Bool playerIsSwimming;
    f32 depth;
    f32 playerDistance;     //Distance between player and barrel
    Vec3f vPlayerToBarrel;  //Direction from player to barrel
    Vec3f vPlayerPush;      //Direction player is moving in
    s32 playerAngle;
    f32 dotProduct_vPush_vPlayerToBarrel;

    //Check if player swimming
    player = get_player();
    if (!player) {
        return;
    }
    playerData = player->data;
    if (playerData->unk0.animState == 33 ||
        playerData->unk0.animState == 32 ||
        playerData->unk0.animState == 31){
        playerIsSwimming = TRUE;
    } else {
        playerIsSwimming = FALSE;
    }

    //Don't allow arrow interaction when barrel and player are both in the water
    if (playerIsSwimming){
        if (!(self->unkAF & ARROW_FLAG_8_No_Targetting)){
            self->unkAF |= ARROW_FLAG_8_No_Targetting;
        }
    } else if (self->unkAF & ARROW_FLAG_8_No_Targetting) {
        self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
    }

    //Get player-to-barrel vector
    VECTOR_SUBTRACT(self->globalPosition, player->globalPosition, vPlayerToBarrel)
    playerDistance = VECTOR_MAGNITUDE(vPlayerToBarrel);

    //Allow the player to push the barrel
    if (playerIsSwimming){

        //Let player push the barrel when it's nearby in the water
        if (playerDistance > 35){
            return;
        }

        //Get player push unit vector, based on their real yaw (vec is multiplied by player's speed later)
        playerAngle = player->parent ? player->parent->srt.yaw + player->srt.yaw : player->srt.yaw;
        rotate_point_by_angle_2D(
            0.0f, -1.0f, 
            &vPlayerPush.x, &vPlayerPush.z,
            playerAngle);

        //Normalise player-to-barrel vector
        if (playerDistance > 0.0f){
            playerDistance = 1.0f/playerDistance; //convert to divisor
        }
        vPlayerToBarrel.x *= playerDistance;
        vPlayerToBarrel.y *= playerDistance;
        vPlayerToBarrel.z *= playerDistance;

        //Get dot product between player push direction and player-to-barrel direction
        dotProduct_vPush_vPlayerToBarrel = DOT_PRODUCT(vPlayerPush, vPlayerToBarrel);
        if (dotProduct_vPush_vPlayerToBarrel < 0.0f){
            dotProduct_vPush_vPlayerToBarrel = 0.0f;
        }

        //Push the barrel (factor in dot product to increase push strength as player pushes towards barrel)
        dotProduct_vPush_vPlayerToBarrel *= 0.03f*playerData->unk0.speed*gUpdateRateF;
        self->velocity.x += vPlayerPush.x*dotProduct_vPush_vPlayerToBarrel;
        self->velocity.z += vPlayerPush.z*dotProduct_vPush_vPlayerToBarrel;
    }
}

RECOMP_PATCH void DFbarrel_handle_movement(Object* self) {
    DFBarrel_Data* objData;
    Object* riverFlow;
    Object** objects;
    s32 count;
    s32 i;
    f32 delta;
    f32 dy;
    f32 dz;
    f32 range;
    f32 diffMagnitude;
    Func_80057F1C_Struct** collisionInfo;
    Vec3f position;
    f32 minDiffMagnitude;
    f32 yDiff;
    /* RECOMP */
    _Bool isUnderwater = FALSE;

    objData = self->data;

    //Get swept away by DFriverflow objects
    {
        objData->accelerationX = objData->accelerationZ = 0.0f;

        for (objects = obj_get_all_of_type(OBJTYPE_Riverflow, &count), i = 0; i < count; i++) {
            riverFlow = objects[i];
            dy = riverFlow->srt.transl.y - self->srt.transl.y;
            if ((dy <= 200.0f) && (dy >= -200.0f)) {
                delta = riverFlow->srt.transl.x - self->srt.transl.x;
                dz = riverFlow->srt.transl.z - self->srt.transl.z;
                delta = sqrtf(SQ(delta) + SQ(dz));
                
                range = ((DFriverflow_Setup*)riverFlow->setup)->range * 1.5f;
                if (delta < range) {
                    delta = (range - delta) / range;
                    delta *= riverFlow->srt.scale * 10.0f;
                    
                    objData->accelerationX += fsin16_precise(riverFlow->srt.yaw) * delta;
                    objData->accelerationZ += fcos16_precise(riverFlow->srt.yaw) * delta;
                }
            }
        }
        
        if (count != 0) {
            objData->accelerationX /= count;
            objData->accelerationZ /= count;
            objData->velocityX -= (objData->accelerationX * 0.05f);
            objData->velocityZ -= (objData->accelerationZ * 0.05f);
            objData->velocityX *= 0.99f;
            objData->velocityZ *= 0.99f;
            
            delta = sqrtf(SQ(objData->velocityX) + SQ(objData->velocityZ));
            if (delta > 0.85f) {
                objData->velocityX *= 0.85f / delta;
                objData->velocityZ *= 0.85f / delta;
            }
            self->velocity.x = objData->velocityX;
            self->velocity.z = objData->velocityZ;
        }
    }

    //Float on water
    {
        objData->velocityY = self->velocity.y;
        
        //Find the nearest water plane
        count = func_80057F1C(self, self->srt.transl.x, self->srt.transl.y, self->srt.transl.z, &collisionInfo, 0, 0);
        if (count) {
            delta = 10000.0f;
            
            for (i = 0; i < count; i++){
                if (collisionInfo[i]->unk14 == 14) { //water only?
                    //@recomp: float a little below surface level 
                    yDiff = (self->srt.transl.y + 6) - collisionInfo[i]->unk0[0];
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
                    }
                }
            }

            //Correlate velocityY with barrel's depth underwater
            if ((delta != 10000.0f) && (delta < 0)) {
                objData->velocityY += (-delta * 0.1f);
                objData->velocityY *= 0.8f;

                isUnderwater = TRUE;
                DFbarrel_handle_player_water_behaviour(self);
            }
        }

        self->velocity.y = objData->velocityY;

        //@recomp: limit velocityY, so barrel doesn't rocket into the next calendar year when released underwater
        if (self->velocity.y > 1.5f) {
            self->velocity.y = 1.5f;
        }
    }

    //@recomp: lose lateral momentum
    if (isUnderwater) {
        self->velocity.x *= 0.95f;
        self->velocity.z *= 0.95f;

        //Spin slightly when moving through water
        s32 newYaw = self->srt.yaw + (self->velocity.z - self->velocity.x) * 1000;
        CIRCLE_WRAP(newYaw);
        self->srt.yaw = newYaw;
    } else {
        self->velocity.x *= 0.9f;
        self->velocity.z *= 0.9f;
    }

    position.x = self->srt.transl.x;
    position.y = self->srt.transl.y;
    position.z = self->srt.transl.z;

    obj_move(self, self->velocity.x * gUpdateRateF, self->velocity.y * gUpdateRateF, self->velocity.z * gUpdateRateF);

    func_80059C40(&position, &self->srt.transl, 10.0f, 0, NULL, self, 8, -1, 0xFF, 0);
}

RECOMP_PATCH void DFbarrel_handle_damage(Object* self) {
    DFBarrel_Data* objData;
    DIMExplosion_Setup* explosion;
    s32 hitDamage;
    Object* obj;
    f32 distance;
    ExplodeAnimator_Setup* explodeAnimSetup;
    Debris_Setup* debrisSetup;
    /* RECOMP */
    DFBarrel_Setup* objSetup = (DFBarrel_Setup*)self->setup;

    objData = self->data;

    //Check for damage, return early if nothing damaged the barrel
    if (func_80025F40(self, NULL, NULL, &hitDamage) == 0) {
        return;
    }

    //When damaged, play an impact sound and increase damage counter
    if (objData->damage < objSetup->hitPoints - 1) { //@recomp: only play on hits leading up to explosion
        gDLL_6_AMSFX->vtbl->play(self, SOUND_372_Crate_Struck, MAX_VOLUME, NULL, NULL, 0, NULL);
    }
    objData->damage += hitDamage;

    //Return early if the barrel hasn't reached the damage threshold
    if (objData->damage < objSetup->hitPoints) { //@recomp: optional health for barrel
        return;
    }

    //Explode!
    {
        // @recomp: scale up the barrel's damage range
        // (Only custom barrels for now, but could maybe apply to all or add an objSetup param?)
        if (objSetup->hitPoints > 0) {
            func_8002683C(self, 
                self->def->hitbox_flagsB6 + 50, 
                self->def->unk94 - 50, 
                self->def->unk96 + 50
            );
        }

        func_80026940(self, 40);
        func_80026128(self, Damage_Type_Explosion, 4, 0);
        
        explosion = obj_alloc_setup(sizeof(DIMExplosion_Setup), OBJ_DIMExplosion);
        explosion->base.x = self->srt.transl.x;
        explosion->base.y = self->srt.transl.y;
        explosion->base.z = self->srt.transl.z;
        obj_create((ObjSetup*)explosion, 5, self->mapID, -1, self->parent);
        
        //@recomp: Leave behind some embers like the CFbarrels do
        SRT fxTransform;
        fxTransform.transl.x = self->srt.transl.x;
        fxTransform.transl.y = self->srt.transl.y;
        fxTransform.transl.z = self->srt.transl.z;
        for (int i = 0; i < 4; i++) {
            gDLL_17_partfx->vtbl->spawn(self, 0x3A, &fxTransform, 0, -1, NULL);
        }
    }

    //Check for any nearby debris/explodeAnimator objects, and set their gamebits
    {
        distance = 110.0f;

        //Debris
        if ((obj = obj_get_nearest_type_to(OBJTYPE_ExplodeObj, self, &distance))) {
            debrisSetup = (Debris_Setup*)obj->setup;
            if (debrisSetup->unk40 != NO_GAMEBIT) {
                main_set_bits(debrisSetup->unk40, 1);
            }
        }

        //ExplodeAnimator
        if ((obj = obj_get_nearest_type_to(OBJTYPE_ExplodeAnimator, self, &distance))) {
            explodeAnimSetup = (ExplodeAnimator_Setup*)obj->setup;
            if (explodeAnimSetup->gamebitExplodeTrigger != NO_GAMEBIT) {
                main_set_bits(explodeAnimSetup->gamebitExplodeTrigger, 1);
            }
        }
    }
    
    objData->framesSinceDetonation = 1;
    self->opacity = 0;
}

RECOMP_PATCH void DFbarrel_free(Object* self, s32 onlySelf) {
    DFBarrel_Data* objData = self->data;

    obj_free_object_type(self, OBJTYPE_27);
    ((DLL_Unknown*)gDLL_54)->vtbl->func[3].withOneArg((s32)self);
}

u32 RECOMP_PATCH DFbarrel_get_data_size(Object* self) {
    return sizeof(DFBarrel_Data);
}