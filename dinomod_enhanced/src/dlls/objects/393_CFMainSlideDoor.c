#include "modding.h"

#include "sys/gfx/animseq.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "dll.h"

#include "recomp/dlls/objects/393_CFMainSlideDoor_recomp.h"

typedef struct {
    ObjSetup base;
    s16 unk18;
    s16 unk1A;
    s16 unk1C;
    s8 unk1E;
    u8 unk1F;
    u8 unk20;
    u8 unk21;
    s16 unk22;
} CFMainSlideDoor_Setup;

typedef struct {
    u8 unk0;
} CFMainSlideDoor_Data;

extern int CFMainSlideDoor_func_268(Object* a0, Object* a1, AnimObj_Data* a2, s8 a3);

RECOMP_PATCH void CFMainSlideDoor_setup(Object* self, CFMainSlideDoor_Setup* setup, s32 arg2) {
    CFMainSlideDoor_Data* objdata;

    self->unkDC = 0;
    self->srt.yaw = setup->unk1F << 8;
    self->animCallback = CFMainSlideDoor_func_268;
    self->srt.scale = setup->unk21 * 0.015625f;
    self->srt.scale *= self->def->scale;
    objdata = (CFMainSlideDoor_Data*)self->data;

    // @recomp: Don't crash if player isn't found (original patch by MusicalProgrammer)
    Object *player = get_player();
    objdata->unk0 = player == NULL ? FALSE : vec3_distance_xz(&self->globalPosition, &player->globalPosition) < 130.0f;
}
