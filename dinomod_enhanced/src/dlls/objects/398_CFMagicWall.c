#include "modding.h"
#include "recomputils.h"

#include "sys/camera.h"
#include "sys/map.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "constants.h"

#include "recomp/dlls/objects/398_CFMagicWall_recomp.h"

typedef struct {
    ObjSetup base;
    u8 unk18;
    u8 unk19;
    u8 unk1A;
} CFMagicWall_Setup;

extern void CFMagicWall_func_384(Object* self, u8 opacity, s32 animatorID);

RECOMP_PATCH void CFMagicWall_control(Object* self) {
    CFMagicWall_Setup* setup;
    Object* player;
    u8 var_a1;
    f32 var_fv0;
    f32 var_ft1;

    setup = (CFMagicWall_Setup*)self->setup;
    player = get_player();
    var_ft1 = (f32) setup->unk1A;

    // @recomp: Don't crash if player or sidekick isn't found (original by MusicalProgrammer)
    Object *sidekick = get_sidekick();

    f32 playerDist = player == NULL ? F32_MAX : vec3_distance(&self->globalPosition, &player->globalPosition);
    f32 sidekickDist = sidekick == NULL ? F32_MAX : vec3_distance(&self->globalPosition, &sidekick->globalPosition);

    if (playerDist < sidekickDist) {
        var_fv0 = playerDist;
    } else {
        var_fv0 = sidekickDist;
    }
    
    f32 camDist = camera_get_distance_to_point(self->srt.transl.x, self->srt.transl.y, self->srt.transl.z);

    if (camDist < var_fv0) {
        var_fv0 = camDist;
    }

    if (var_fv0 < var_ft1) {
        var_a1 = ((var_fv0 / var_ft1) * 255.0f);
        if (setup->unk19 == 0) {
            var_a1 = (255 - var_a1);
        }
    } else {
        var_a1 = (setup->unk19 * 255);
    }
    CFMagicWall_func_384(self, var_a1, setup->unk18);
}
