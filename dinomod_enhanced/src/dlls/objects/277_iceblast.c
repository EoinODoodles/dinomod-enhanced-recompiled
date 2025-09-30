#include "modding.h"
#include "recomputils.h"

#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "dlls/objects/210_player.h"

#include "recomp/dlls/_asm/277_recomp.h"

typedef struct {
s16 timer;
} IceBlastState;

//Fixes the direction of the Ice Blast when used by Sabre (originally by MusicalProgrammer)
RECOMP_PATCH void dll_277_func_80(Object* self) {
    Object* player;
    Object* weapon;
    IceBlastState* state;
    SRT transform;

    player = get_player();
    state = self->state;
    if (!player) {
        return;
    }
    
    weapon = player->linkedObject;
    if (!weapon) {
        return;
    }
    
    self->srt.roll = weapon->srt.roll;
    self->srt.pitch = weapon->srt.pitch;
    self->srt.yaw = weapon->srt.yaw;
    state->timer -= (s16)delayFloat;
    if (state->timer <= 0) {
        state->timer = 0x1E;
        self->speed.x = 0.0f; 
        self->speed.y = -5.0f;
        self->speed.z = 0.0f; 

        
        transform.transl.x = 0.0f;
        transform.transl.y = 0.0f;
        transform.transl.z = 0.0f;
        transform.scale = 1.0f;
        transform.roll = weapon->srt.roll;
        transform.pitch = weapon->srt.pitch;
        transform.yaw = weapon->srt.yaw;

        //@recomp: fix ice blast direction when using Sabre's sword
        if (weapon->id == OBJ_sword)
            transform.pitch -= 0x8000;
        
        rotate_vec3((SRT* ) &transform, &self->speed);
        self->srt.transl.x = weapon->positionMirror.x;
        self->srt.transl.y = weapon->positionMirror.y;
        self->srt.transl.z = weapon->positionMirror.z;
        self->srt.transl.x += self->speed.x * delayFloat;
        self->srt.transl.y += self->speed.y * delayFloat;
        self->srt.transl.z += self->speed.z * delayFloat;
        ((DLL_210_Player*)player->dll)->vtbl->func14(player, -1);
    }

    self->positionMirror2.x = self->srt.transl.x;
    self->positionMirror2.y = self->srt.transl.y;
    self->positionMirror2.z = self->srt.transl.z;
    self->srt.transl.x += (self->speed.x * delayFloat);
    self->srt.transl.y += (self->speed.y * delayFloat);
    self->srt.transl.z += (self->speed.z * delayFloat);

}
