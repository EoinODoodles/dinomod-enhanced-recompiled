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
RECOMP_PATCH void DRbullet_recycle(Object* self, SRT* pFired, SRT* pTarget, f32 arg3) {
    DRbullet_Data *objData;
    Object *lfxEmitter;
    f32 lateralSpeed;
    f32 pad;
    Vec3f direction;
    Vec3f displacement;
    Vec3f futurePosition;
    Vec3s16 sCurrentPosition;
    Vec3s16 sFuturePosition;
    Vec3s16 sp44;
    f32 magnitude;
    
    objData = self->data;
    
    //Get normalised direction vector between point of origin and target point
    direction.x = pTarget->transl.x - pFired->transl.x;
    direction.y = pTarget->transl.y - pFired->transl.y;
    direction.z = pTarget->transl.z - pFired->transl.z;
    magnitude = sqrtf(((direction.f[0] * direction.f[0]) + (direction.f[1] * direction.f[1])) + (direction.f[2] * direction.f[2])) / arg3;
    if (magnitude != 0.0f){
        direction.f[0] /= magnitude;
        direction.f[1] /= magnitude;
        direction.f[2] /= magnitude;
    }
    
    //Set bullet's transform
    self->srt.transl.x = pFired->transl.x;
    self->srt.transl.y = pFired->transl.y;
    self->srt.transl.z = pFired->transl.z;
    self->speed.x = direction.f[0];
    self->speed.y = direction.f[1];
    self->speed.z = direction.f[2];    
    lateralSpeed = sqrtf((self->speed.x * self->speed.x) + (self->speed.z * self->speed.z));
    self->srt.yaw = arctan2_f(self->speed.x, self->speed.z);
    self->srt.pitch = -arctan2_f(self->speed.y, lateralSpeed);
    self->srt.roll = 0;
    func_8002674C(self);
    
    //Set bullet to "fired" state and play sound
    objData->state = BULLET_STATE_FIRED;
    gDLL_6_AMSFX->vtbl->play_sound(self, 0x927, 0x7F, &objData->whooshSoundHandle, NULL, 0, NULL);

    //Set bullet's expiry timer based on its distance 10 seconds after being fired (divided by arg3)
    futurePosition.x = self->speed.x * 600.0f;
    futurePosition.y = self->speed.y * 600.0f;
    futurePosition.z = self->speed.z * 600.0f;
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
        objData->timer = sqrtf((displacement.f[0] * displacement.f[0]) + (displacement.f[1] * displacement.f[1]) + (displacement.f[2] * displacement.f[2])) / arg3;
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
        objData->lfxEmitter->speed.x = self->speed.x;
        objData->lfxEmitter->speed.y = self->speed.y;
        objData->lfxEmitter->speed.z = self->speed.z;
    }

    //Set bullet's opacity and scale
    self->opacity = 0xFF;
    self->srt.scale = self->def->scale;
}
