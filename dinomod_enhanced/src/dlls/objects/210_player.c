#include "dll.h"
#include "dlls/engine/29_gplay.h"
#include "dlls/engine/6_amsfx.h"
#include "functions.h"
#include "modding.h"
#include "recomputils.h"
#include "dll_util.h"
#include "sidekick_util.h"

#include "game/objects/object_id.h"
#include "sys/dll.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/objanim.h"
#include "sys/objtype.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/common/vehicle.h"

#include "recomp/dlls/objects/210_player_recomp.h"
#include "sys/print.h"

typedef void (*func_1D04C)(Object *obj, s32);
static func_1D04C player_func_1D04C; 
static void func_1D04C_hijack(Object *self, s32);

RECOMP_HOOK_DLL(dll_210_ctor) void player_ctor_hook(DLLFile *dll) {
    player_func_1D04C = dinomod_hijack_dll_export(dll, 61, func_1D04C_hijack);
}

RECOMP_HOOK_RETURN_DLL(dll_210_dtor) void player_dtor_hook() {
    player_func_1D04C = NULL;
}

static void func_1D04C_hijack(Object *self, s32 a1) {
    // @recomp: Ignore func_1D04C calls if in Galadon fight (removes forced z-targeting)
    Object **objectList;
    s32 count;

    objectList = obj_get_all_of_type(4, &count);

    for (s32 i = 0; i < count; i++) {
        if (objectList[i]->id == OBJ_DIM_Boss) {
            return;
        }
    }

    player_func_1D04C(self, a1);
}

static u32 soundCooldown;

//@recomp: debounce magic refill sound
RECOMP_PATCH void dll_210_add_magic(Object* player, s32 magicDifference) {
    Player_Data* objdata = player->data;
    PlayerStats* stats;
    s32 magic;
    s8 mapID;

    if (objdata->unk8BB != 0) {
        stats = objdata->stats;
        magic = stats->magic;
        magic += magicDifference;

        if (magic < 0) {
            magic = 0;
        } else {
            if (stats->magicMax < magic) {
                magic = stats->magicMax;
            }
        }
        stats->magic = magic;

        //@recomp: debounce sound
        if (magicDifference > 0 
            && !soundCooldown
        ) {
            mapID = map_get_map_id_from_xz_ws(player->srt.transl.x, player->srt.transl.z);
            if (mapID == MAP_BOSS_KAMERIAN_DRAGON){
                return;
            } else if (mapID == MAP_DRAGON_ROCK_BOTTOM){
                soundCooldown = 180;
            } else {
                soundCooldown = 30;
            }

            gDLL_6_AMSFX->vtbl->play_sound(NULL, SOUND_5EB_Magic_Refill_Chime, MAX_VOLUME, 0, 0, 0, 0);
        }
    }
}

RECOMP_HOOK_RETURN_DLL(dll_210_control) void playerSoundDebouncing(Object* self) {
    if (soundCooldown > 0){
        soundCooldown -= gUpdateRate;
        if (soundCooldown < 0){
            soundCooldown = 0;
        }
    }
}

/** Fix swimming softlock (originally by MusicalProgrammer) */
RECOMP_PATCH s32 dll_210_func_125BC(Object* arg0, Player_Data* arg1, u32 arg2) {
    f32 temp_fs0;
    f32 temp_fs1;
    f32 f2;
    f32 f0;
    s32 i;
    Player_Data* temp_s3;

    if (arg1->unk0.enteredAnimState != 0) {
        ((s16*)arg1)[0x138] = 0x1F;
    }
    arg1->unk0.flags |= 0x200000;
    temp_s3 = (Player_Data *) &arg1->unk0.unk4;
    if (arg1->unk0.enteredAnimState != 0) {
        gDLL_6_AMSFX->vtbl->play_sound(arg0, 0x3D8U, 0x7FU, NULL, NULL, 0, NULL);
        for (i = 0; i < 3; i++) {
            temp_fs0 = ((f32) rand_next(-50, 50) / 10.0f) + arg0->srt.transl.x;
            temp_fs1 = ((f32) rand_next(-50, 50) / 10.0f) + arg0->srt.transl.z;
            gDLL_24_Waterfx->vtbl->func_174C(temp_fs0, temp_s3->unk0.unk4.floorY, temp_fs1, 4.0f);
            gDLL_24_Waterfx->vtbl->func_1CC8(temp_fs0, temp_s3->unk0.unk4.floorY, temp_fs1, 0, 0.0f, 3);
        }
    }

    if (
        temp_s3->unk0.unk4.unk1AC > 25.0f 
        // && temp_s3->unk0.unk4.floorNormalZ < 100.0f //@recomp: remove check
    ) {
        return 0x21;
    }

    if ((s8)temp_s3->unk0.unk4.numTestPoints & 0x10) {
        return 2;
    }
    f0 = temp_s3->unk0.unk4.floorY - 6.0f;
    f2 = f0 - arg0->srt.transl.y;
    if (f2 > 25.0f) {
        f2 = 25.0f;
    }
    arg0->speed.y += (f2 / 25.0f) * 0.13f * gUpdateRateF;
    arg0->speed.y -= 0.1f * gUpdateRateF;
    arg0->speed.y *= 0.96f;
    if (arg0->speed.y > 1.4f) {
        arg0->speed.y = 1.4f;
    }
    arg0->speed.x *= 0.98f;
    arg0->speed.z *= 0.98f;
    for (i = 0; i < 4; i++) {
        gDLL_17_partfx->vtbl->spawn(arg0, PARTICLE_202, NULL, PARTFXFLAG_NONE, -1, NULL);
    }
    return 0;
}
