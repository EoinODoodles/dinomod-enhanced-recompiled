#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "common.h"
#include "dlls/objects/210_player.h"

#include "recomp/dlls/objects/277_iceblast_recomp.h"

typedef struct {
    s16 timer;
    s16 unused2;
} Iceblast_Data;

// Fixes the direction of the Ice Blast when used by Sabre (originally by MusicalProgrammer)
// Also allows the cost of the Ice Blast spell to optionally be lowered, by handling it in the player DLL instead
RECOMP_PATCH void iceblast_control(Object* self) {
    Object* player;
    Object* weapon;
    Iceblast_Data* objdata;
    SRT transform;
    //@recomp
    Player_Data* playerData;
    int reduceIceBlastCost = recomp_get_config_u32("iceblast_cost");

    player = get_player();
    objdata = self->data;
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
    objdata->timer -= gUpdateRate;
    if (objdata->timer <= 0) {
        //Don't recycle self when player's magic runs out
        if (player){
            playerData = player->data;
            if (playerData && playerData->stats && playerData->stats->magic == 0){
                objdata->timer = 0;
                return;
            }
        }

        objdata->timer = 30;
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
        
        rotate_vec3((SRT* ) &transform, (f32*)&self->speed);
        self->srt.transl.x = weapon->positionMirror.x;
        self->srt.transl.y = weapon->positionMirror.y;
        self->srt.transl.z = weapon->positionMirror.z;
        self->srt.transl.x += self->speed.x * gUpdateRateF;
        self->srt.transl.y += self->speed.y * gUpdateRateF;
        self->srt.transl.z += self->speed.z * gUpdateRateF;

        //@recomp: switch for magic cost handling
        if (!reduceIceBlastCost){
            ((DLL_210_Player*)player->dll)->vtbl->add_magic(player, -1);
        }
    }

    self->positionMirror2.x = self->srt.transl.x;
    self->positionMirror2.y = self->srt.transl.y;
    self->positionMirror2.z = self->srt.transl.z;
    self->srt.transl.x += self->speed.x * gUpdateRateF;
    self->srt.transl.y += self->speed.y * gUpdateRateF;
    self->srt.transl.z += self->speed.z * gUpdateRateF;
}

// Hide debug cubes
RECOMP_PATCH void iceblast_print(Object* self, Gfx** gfx, Mtx** mtx, Vertex** vtx, Triangle** pols, s32 visibility) {
    // draw_object(self, gfx, mtx, vtx, pols, 1.0f);
}
