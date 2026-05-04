#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "configs.h"

#include "common.h"
#include "sys/main.h"

#include "recomp/dlls/objects/537_DIMCannon_recomp.h"

typedef struct {
    ObjSetup base;
    s8 unk18;
    s8 unk19;
    s16 unk1A;
    s16 unk1C;
    s16 unk1E;
    s16 unk20;
    s16 unk22;
} DIMCannonBall_Setup;

typedef struct {
    u8 unk0;
    s8 unk1;
    s8 unk2;
    s8 unk3;
    u8 unk4;
    u8 unk5;
    u8 unk6;
    u8 unk7;
} Unk_Data;

typedef struct {
    ObjSetup base;
    s16 unk18;
    s16 unk1A;
    s16 unk1C;
    s16 unk1E;
    s16 unk20;
    s16 unk22;
} DIMExplosion_Setup;

// Fix cannon explosions being invisible (TODO: maybe fix this differently, using `objData->unk4/5/6/7`)
RECOMP_PATCH void dll_537_func_16AC(Object* self) {
    // Unk_Data* objData;
    DIMCannonBall_Setup* shotSetup;

    // objData = self->data;

    shotSetup = (DIMCannonBall_Setup*)obj_alloc_setup(sizeof(DIMExplosion_Setup), OBJ_DIMExplosion);
    // shotSetup->base.loadFlags = objData->unk4;
    // shotSetup->base.byte6 = objData->unk6;
    // shotSetup->base.byte5 = objData->unk5;
    // shotSetup->base.fadeDistance = objData->unk7;
    shotSetup->base.x = self->srt.transl.x;
    shotSetup->base.y = self->srt.transl.y;
    shotSetup->base.z = self->srt.transl.z;
    obj_create((ObjSetup*)shotSetup, 5, self->mapID, -1, self->parent);
}
