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
    // @recomp: new fields
    f32 yStart;
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
    // @recomp: Track starting Y coord
    objdata->yStart = self->globalPosition.y;

    // @recomp: Don't crash if player isn't found (original patch by MusicalProgrammer)
    Object *player = objGetPlayer();
    objdata->unk0 = player == NULL 
        ? FALSE 
        // @recomp: Also consider Y-coords
        : (vec3DistanceXZ(&self->globalPosition, &player->globalPosition) < 130.0f
            && player->globalPosition.y >= (objdata->yStart - 1.0f));
}

RECOMP_PATCH u32 CFMainSlideDoor_get_data_size(Object *self, u32 a1) {
    return sizeof(CFMainSlideDoor_Data); // @recomp: new size
}

/** 
 * Note: CFMainSlideDoor was moved out of an objgroup so it can be loaded in while directly above the player now.
 *       To compensate, we check the player's Y-coord now.
 */
RECOMP_PATCH int CFMainSlideDoor_func_268(Object* a0, Object* a1, AnimObj_Data* a2, s8 a3) {
    CFMainSlideDoor_Data* objdata;
    CFMainSlideDoor_Setup* setup;
    Object* player;
    Object* sidekick;
    s32 var_t6;
    s32 var_t7;

    player = objGetPlayer();
    sidekick = objGetSidekick();
    objdata = (CFMainSlideDoor_Data*)a0->data; // @recomp: moved up here
    if (player != NULL) {
        // @recomp: Also consider Y-coords
        var_t6 = vec3DistanceXZ(&a0->globalPosition, &player->globalPosition) < 130.0f
            && player->globalPosition.y >= (objdata->yStart - 1.0f);
    } else {
        var_t6 = 0;
    }
    if (sidekick != NULL) {
        // @recomp: Also consider Y-coords
        var_t7 = vec3DistanceXZ(&a0->globalPosition, &sidekick->globalPosition) < 130.0f
            && sidekick->globalPosition.y >= (objdata->yStart - 1.0f);
    } else {
        var_t7 = 0;
    }
    setup = (CFMainSlideDoor_Setup*)a0->setup;
    if (objdata->unk0 == 0) {
        if (mainGetBits(setup->unk18) != 0) {
            if (setup->unk22 == -1 || mainGetBits(setup->unk22) != 0) {
                mainSetBits(setup->unk1A, 1);
                if (var_t6 != 0 || var_t7 != 0) {
                    objdata->unk0 = 2;
                }
            }
        }
    } else if (objdata->unk0 == 1) {
        if (mainGetBits(setup->unk18) != 0 || (setup->unk22 != -1 && mainGetBits(setup->unk22) != 0)) {
            if ((var_t6 == 0) && (var_t7 == 0)) {
                objdata->unk0 = 3;
            }
        }
    }
    if (objdata->unk0 == 2) {
        if (a2->lastMessage == 2) {
            objdata->unk0 = 1;
        }
    } else if (objdata->unk0 == 3) {
        if (a2->lastMessage == 1) {
            objdata->unk0 = 0;
        }
    }
    
    return !(objdata->unk0 == 2) && !(objdata->unk0 == 3);
}
