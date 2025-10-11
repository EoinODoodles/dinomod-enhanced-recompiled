#include "modding.h"

#include "dlls/objects/214_animobj.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "functions.h"
#include "dll.h"

#include "recomp/dlls/objects/393_CFMainSlideDoor_recomp.h"

typedef struct {
    ObjCreateInfo base;
    s16 unk18;
    s16 unk1A;
    s16 unk1C;
    s8 unk1E;
    u8 unk1F;
    u8 unk20;
    u8 unk21;
    s16 unk22;
} CFMainSlideDoor_CreateInfo;

typedef struct {
    u8 unk0;
} CFMainSlideDoor_State;

extern int CFMainSlideDoor_func_268(Object* a0, Object* a1, AnimObjState* a2, void* a3);

RECOMP_PATCH void CFMainSlideDoor_create(Object* self, CFMainSlideDoor_CreateInfo* createInfo, s32 arg2) {
    CFMainSlideDoor_State* state;

    self->unk0xdc = 0;
    self->srt.yaw = createInfo->unk1F << 8;
    self->unk0xbc = (ObjectCallback)CFMainSlideDoor_func_268;
    self->srt.scale = createInfo->unk21 * 0.015625f;
    self->srt.scale *= self->def->scale;
    state = (CFMainSlideDoor_State*)self->state;

    // @recomp: Don't crash if player isn't found (original patch by MusicalProgrammer)
    Object *player = get_player();
    state->unk0 = player == NULL ? FALSE : vec3_distance_xz(&self->positionMirror, &player->positionMirror) < 130.0f;
}
