#include "modding.h"
#include "recomputils.h"

#include "sys/dll.h"
#include "sys/objects.h"
#include "dlls/engine/17.h"

#include "recomp/dlls/objects/264_kamerian_flame_recomp.h"

extern DLL_17 *gDLL_17;

//Allows the Kamerian Dragon's fireballs to be visible (originally by MusicalProgrammer)
RECOMP_PATCH void kamerian_flame_create_flame_billboards(Object* self) {
    Vec3f delta;
    SRT transform;

    transform.transl.x = 0.0f;
    transform.transl.y = 0.0f;
    transform.transl.z = 0.0f;
    transform.roll = 0;
    transform.pitch = 0;
    transform.yaw = 0;
    transform.scale = 0.003f;
    
    //@recomp: arg1 changed to 0x9F
    gDLL_17->vtbl->func1(self, 0x9F, (SRT* ) &transform, 1, -1, NULL);
    
    delta.x = self->srt.transl.x - self->positionMirror2.x;
    delta.y = self->srt.transl.y - self->positionMirror2.y;
    delta.z = self->srt.transl.z - self->positionMirror2.z;
    
    transform.transl.x = delta.x / 3.0f;
    transform.transl.y = delta.y / 3.0f;
    transform.transl.z = delta.z / 3.0f;
    
    gDLL_17->vtbl->func1(self, 0x680, (SRT* ) &transform, 1, -1, NULL);
    
    transform.transl.x *= 4.0f;
    transform.transl.y *= 4.0f;
    transform.transl.z *= 4.0f;
    
    gDLL_17->vtbl->func1(self, 0x680, (SRT* ) &transform, 1, -1, NULL);
}
