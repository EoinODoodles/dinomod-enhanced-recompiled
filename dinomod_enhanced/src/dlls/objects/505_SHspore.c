#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "configs.h"

#include "common.h"
#include "sys/main.h"
#include "sys/objanim.h"

#include "recomp/dlls/objects/505_SHspore_recomp.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 unk18;                       //unused
/*1A*/ s16 angularRange;                //maximum angular distance from desired travel angle
/*1C*/ s16 windAngle;                   //desired direction the spore should travel in
} SHSpore_Setup;

typedef struct {
/*000*/ DLL27_Data terrainCollider;
/*260*/ f32 lifetime;
/*264*/ f32 lateralSpeed;
/*268*/ f32 lateralAcceleration;
/*26C*/ f32 lateralSpeedGoal;
/*270*/ f32 angularJitterTimer;         //waiting time between slightly randomising flight angle
/*274*/ f32 velocityX;
/*278*/ f32 velocityZ;
/*27C*/ f32 coefficientX;               //multiplier for lateral acceleration
/*280*/ f32 coefficientZ;               //multiplier for lateral acceleration
/*284*/ f32 lateralDecelerationTimer;   //waiting time until lateral acceleration dwindles
/*288*/ f32 lateralAccelerationGoal;
/*28C*/ f32 angleChangeTimer;           //waiting time between large randomisation of flight angle
/*290*/ f32 deletionTimer;      
/*294*/ s16 angle;                      //lateral direction of travel
/*296*/ s16 angleJitter;                //lateral direction of travel (plus randomised angular noise)
/*298*/ s16 angleGoal;
} SHSpore_Data;

extern void SHspore_change_flight_direction(Object* self, SHSpore_Data* objData);
extern void SHspore_jitter_flight_direction(Object* self, SHSpore_Data* objData);

RECOMP_PATCH void SHspore_control(Object* self) {
    SHSpore_Data* objData;
    s8 pad[10];
    s32 particleCount;
    f32 temp;
    f32 lateralJolt;
    s32 index;
    Object* collidedObject;
    s32 i; //@recomp

    objData = (SHSpore_Data*)self->data;

    if (objData->deletionTimer != 0.0f) {
        self->srt.yaw += gUpdateRate << 6;
        objData->deletionTimer -= gUpdateRateF;
        if (objData->deletionTimer <= 0.0f) {
            obj_destroy_object(self);
        }
    } else {
        //Update motion
        objData->angularJitterTimer -= gUpdateRateF;
        if (objData->angularJitterTimer < 0.0f) {
            objData->angularJitterTimer = 0.0f;
        }
        objData->angleChangeTimer -= gUpdateRateF;
        if (objData->angleChangeTimer < 0.0f) {
            objData->angleChangeTimer = 0.0f;
        }

        self->velocity.y += -0.009f * gUpdateRateF;
        if (self->velocity.y < -0.2f) {
            self->velocity.y = -0.2f;
        }
        if (self->velocity.y > 0) {
            /* @recomp: framerate-independent ascent 
              (fixes bug where Spores launch up to higher altitude at lower FPS, 
              which made them harder to catch on N64 than in emulators/recomp) */
            for (i = 0; i < gUpdateRate; i++) {
                self->velocity.y *= 0.97f;
            }
        }
        if (self->velocity.y < 0.0f) {
            func_8002674C(self);
        }

        SHspore_change_flight_direction(self, objData);
        if ((rand_next(0, 100) < 5) && (objData->angularJitterTimer <= 0.0f)) {
            SHspore_jitter_flight_direction(self, objData);
        }

        objData->lateralDecelerationTimer -= gUpdateRateF;
        if (objData->lateralDecelerationTimer <= 0.0f) {
            objData->coefficientX *= 0.97f;
            objData->coefficientZ *= 0.97f;
            objData->lateralDecelerationTimer = 0.0f;
        } else {
            lateralJolt = objData->lateralAccelerationGoal - objData->lateralAcceleration;
            objData->lateralAcceleration += lateralJolt * 0.01f * gUpdateRateF;
        }

        self->velocity.x = objData->velocityX + (objData->coefficientX * objData->lateralAcceleration);
        self->velocity.z = objData->velocityZ + (objData->coefficientZ * objData->lateralAcceleration);
        obj_move(self, self->velocity.x * gUpdateRateF, self->velocity.y * gUpdateRateF, self->velocity.z * gUpdateRateF);
        gDLL_27->vtbl->func_1E8(self, &objData->terrainCollider, gUpdateRateF);
        gDLL_27->vtbl->func_5A8(self, &objData->terrainCollider);
        gDLL_27->vtbl->func_624(self, &objData->terrainCollider, gUpdateRateF);
        func_80026128(self, 0xA, 0, 0);

        //Handle object collisions
        collidedObject = self->objhitInfo->unk48;
        if (collidedObject) {
            particleCount = 20;

            if (get_player() == collidedObject) {
                //Player collecting purple mushroom
                i = main_increment_bits(BIT_Inventory_Purple_Mushrooms);
                particleCount = 0;

                //@recomp: optionally show an item collection pop-up
                if (recomp_get_config_u32("cmdmenu_info_popup_expand")) {
                    gDLL_1_cmdmenu->vtbl->info_show(
                        BIT_Inventory_Purple_Mushrooms, 
                        INFO_POPUP_DURATION,
                        i
                    );
                }
            }
            
            if (collidedObject->id != OBJ_SHrocketmushroo) {
                //Other objects (ignoring SHrocketmushroom since the spores emerge out of it)
                gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_B31_Item_Collection_Chime, MAX_VOLUME, NULL, 0, 0, 0);
                gDLL_13_Expgfx->vtbl->func4(self);

                //Create collision particles
                for (index = 0; index < particleCount; index++){
                    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_3F3, NULL, PARTFXFLAG_4, -1, NULL);
                }

                objData->deletionTimer = 200.0f;
                self->srt.flags |= OBJFLAG_INVISIBLE;
                func_800267A4(self);
            }
        } else {
            objData->lifetime -= gUpdateRateF;
            //Destroy the spore if its lifetime runs out or it collides with terrain
            if (objData->lifetime <= 0.0f || objData->terrainCollider.unk25C & 0x11) {
                gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_8A2_Spore_Disintegrate, MAX_VOLUME, NULL, 0, 0, 0);
                gDLL_13_Expgfx->vtbl->func4(self);

                //Create collision particles
                for (index = 0; index < 20; index++){
                    gDLL_17_partfx->vtbl->spawn(self, PARTICLE_3F3, NULL, PARTFXFLAG_4, -1, NULL);
                }

                objData->deletionTimer = 200.0f;
                self->srt.flags |= OBJFLAG_INVISIBLE;
                func_800267A4(self);
            }
        }
    }
}
