#include "modding.h"
#include "recomputils.h"

#include "common.h"

#include "recomp/dlls/objects/735_DRbullet_recomp.h"

typedef struct {
    Object* lfxEmitter;
    u32 whooshSoundHandle;  //Controls whooshing sound as bullet whistles past player
    u8 state;
    u8 flags;
    s32 timer;
} DRbullet_Data;

typedef enum {
    BULLET_STATE_INACTIVE = 0,
    BULLET_STATE_IMPACT = 1,
    BULLET_STATE_FIRED = 2
} DRbullet_States;

extern Object* DRbullet_create_lfxEmitter(Object* self, s32 arg1);

// Stop DRbullet LFXEmitters from being created, as they fill the LFX buffer beyond capacity. Especially when you're flying. (Originally by MusicalProgrammer)
RECOMP_PATCH void DRbullet_recycle(Object* self, SRT* pFired, SRT* pTarget, f32 speed) {
    DRbullet_Data *objData;
    Object *lfxEmitter;
    f32 lateralSpeed;
    f32 pad;
    Vec3f velocity;
    Vec3f displacement;
    Vec3f futurePosition;
    Vec3s16 sCurrentPosition;
    Vec3s16 sFuturePosition;
    Vec3s16 sp44;
    f32 magnitudeOverSpeed;
    
    objData = self->data;
    
    //Get unit vector from point of origin to target point (and then multiply it by speed)
    velocity.x = pTarget->transl.x - pFired->transl.x;
    velocity.y = pTarget->transl.y - pFired->transl.y;
    velocity.z = pTarget->transl.z - pFired->transl.z;
    magnitudeOverSpeed = sqrtf(((velocity.f[0] * velocity.f[0]) + (velocity.f[1] * velocity.f[1])) + (velocity.f[2] * velocity.f[2])) / speed;
    if (magnitudeOverSpeed != 0.0f){
        velocity.f[0] /= magnitudeOverSpeed;
        velocity.f[1] /= magnitudeOverSpeed;
        velocity.f[2] /= magnitudeOverSpeed;
    }
    
    //Set bullet's transform
    self->srt.transl.x = pFired->transl.x;
    self->srt.transl.y = pFired->transl.y;
    self->srt.transl.z = pFired->transl.z;
    self->velocity.x = velocity.f[0];
    self->velocity.y = velocity.f[1];
    self->velocity.z = velocity.f[2];    
    lateralSpeed = sqrtf((self->velocity.x * self->velocity.x) + (self->velocity.z * self->velocity.z));
    self->srt.yaw = arctan2_f(self->velocity.x, self->velocity.z);
    self->srt.pitch = -arctan2_f(self->velocity.y, lateralSpeed);
    self->srt.roll = 0;
    func_8002674C(self);
    
    //Set bullet to "fired" state and play sound
    objData->state = BULLET_STATE_FIRED;
    gDLL_6_AMSFX->vtbl->play_sound(self, 0x927, 0x7F, &objData->whooshSoundHandle, NULL, 0, NULL);

    //Set bullet's expiry timer based on 10 second trajectory after being fired
    futurePosition.x = self->velocity.x * 600.0f;
    futurePosition.y = self->velocity.y * 600.0f;
    futurePosition.z = self->velocity.z * 600.0f;
    futurePosition.x += self->srt.transl.x;
    futurePosition.y += self->srt.transl.y;
    futurePosition.z += self->srt.transl.z;
    func_80007EE0(&self->srt.transl, &sCurrentPosition);
    func_80007EE0(&futurePosition, &sFuturePosition);
    
    if (func_80008048(&sCurrentPosition, &sFuturePosition, &sp44, NULL, 0) == 0){
        func_80007E2C(&futurePosition, &sp44);
        displacement.f[0] = futurePosition.f[0] - self->srt.transl.f[0];
        displacement.f[1] = futurePosition.f[1] - self->srt.transl.f[1];
        displacement.f[2] = futurePosition.f[2] - self->srt.transl.f[2];
        objData->timer = sqrtf((displacement.f[0] * displacement.f[0]) + (displacement.f[1] * displacement.f[1]) + (displacement.f[2] * displacement.f[2])) / speed;
    } else {
        objData->timer = 600;
    }
    
    //Create new lfxEmitter
    lfxEmitter = objData->lfxEmitter;
    if (lfxEmitter != NULL){
        obj_destroy_object(lfxEmitter);
    }    
    // lfxEmitter = DRbullet_create_lfxEmitter(self, 0x24B); //@recomp: don't create new lfxEmitters
    objData->lfxEmitter = lfxEmitter;
    if (lfxEmitter != NULL){
        objData->lfxEmitter->velocity.x = self->velocity.x;
        objData->lfxEmitter->velocity.y = self->velocity.y;
        objData->lfxEmitter->velocity.z = self->velocity.z;
    }

    //Set bullet's opacity and scale
    self->opacity = 0xFF;
    self->srt.scale = self->def->scale;
}
