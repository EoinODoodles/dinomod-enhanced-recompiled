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

s32 func_80025140(Object*, f32, f32, s32);
extern void dll_210_func_14B70(Object* arg0, ObjFSA_Data *arg1);
extern s16 _data_158[];

/** Fix Sabre floating around when he rides SnowHorns, stop projectile spell activating on dismount (originally by MusicalProgrammer) */
RECOMP_PATCH s32 dll_210_func_142C4(Object* arg0, Player_Data* arg1, f32 arg2) {
    Player_Data* temp_s0;
    Object* sp48;
    f32 sp44;
    f32 sp40;
    f32 sp3C;
    s32 sp38;
    s32 sp34;
    s32 sp30;

    sp30 = 0; //@recomp: initialise variable

    gDLL_2_Camera->vtbl->func24.withOneArg(2);
    temp_s0 = arg0->data;
    arg1->unk0.unk4.mode = 0;
    arg1->unk0.animExitAction = dll_210_func_14B70;

    //@recomp: prevent Projectile Spell equip
    arg1->unk834 = 0;
    arg1->flags &= 0xFF00;

    func_800267A4(arg0);
    sp48 = temp_s0->unk858;
    if (sp48 == NULL) {
        arg0->curModAnimIdLayered = -1;
        return 0;
    }
    if (arg1->unk0.enteredAnimState != 0) {
        if (temp_s0->unk76C == NULL) {
            temp_s0->unk76C = (s16 *)_data_158;
        }
        if (temp_s0->unk770 & 2) {
            sp30 = 8;
            func_80024E50(arg0, *temp_s0->unk76C, 0.0f, 0);
            func_80025140(arg0, 0.0f, 0, 0); // arg2 should be 0.0f
            func_80024E50(arg0, temp_s0->unk76C[1], 0.0f, 0xA);
            func_80025140(arg0, 0.0f, 0, 0); // arg2 should be 0.0f
        }
        func_80023D30(arg0, *temp_s0->unk76C, 0.0f, (u8)sp30);
        func_80024108(arg0, 0.0f, 0.0f, NULL);
    }
    if (temp_s0->unk770 & 4) {
        func_800240BC(arg0, sp48->animProgress);
        arg1->unk0.animTickDelta = NULL;
    } else {
        sp3C = ((DLL_IVehicle*)sp48->dll)->vtbl->func16(sp48, &sp44);
        if (sp44 <= 1.0f) {
            arg1->unk0.animTickDelta = sp44;
        } else {
            arg1->unk0.animTickDelta = ((sp3C * 0.05f) + 0.01f);
        }
    }
    if (temp_s0->unk770 & 1) {
        ((DLL_IVehicle*)sp48->dll)->vtbl->func15(sp48, &sp40, &sp34);
        sp38 = (sp40 * 1023.0f);
        if (sp38 < 0) {
            sp38 = -sp38;
        }
        if (sp34 != 0) {
            func_80025540(arg0, temp_s0->unk76C[3], sp38);
            func_8002559C(arg0, temp_s0->unk76C[5], sp38);
        } else {
            func_80025540(arg0, temp_s0->unk76C[2], sp38);
            func_8002559C(arg0, temp_s0->unk76C[4], sp38);
        }
    }
    if (temp_s0->unk770 & 1) {
        func_80024DD0(arg0, 0, 2, 0);
        func_80024DD0(arg0, 1, 2, 0);
    }
    if (temp_s0->unk770 & 2) {
        func_80024DD0(arg0, 1, 0, sp3C * 1023.0f);
        func_80025140(arg0, arg1->unk0.animTickDelta, arg2, 0);
    }
    if (((DLL_IVehicle*)sp48->dll)->vtbl->func10(sp48, arg0) != 0) {
        return 0x27;
    }
    return 0;
}

extern s32 dll_210_func_A018(void);

/** Fix snowball-player collision crash in DarkIce Mines (originally by MusicalProgrammer) */
RECOMP_PATCH s32 dll_210_func_17C14(Object* arg0, Player_Data* arg1, f32 arg2) {
    Object* sp34;
    Player_Data* sp30;

    sp30 = arg0->data;
    arg1->unk0.unk341 = 3;
    if (arg1->unk0.enteredAnimState != 0) {

        if (func_80025F40(arg0, &sp34, NULL, NULL)){
            //@recomp: don't try to calculate angle if func_80025F40 returns 0
            arg0->srt.yaw = arctan2_f(-sp34->speed.f[0], -sp34->speed.f[2]);
        }

        func_80023D30(arg0, 0x407, 0.0f, 0U);
        arg1->unk0.animTickDelta = 0.015f;
    }
    switch (arg0->curModAnimId) {
    case 0x407:
        if (arg1->unk0.unk33A != 0) {
            if (sp30->stats->health <= 0) {
                return 0x35;
            }
            func_80023D30(arg0, 0x408, 0.0f, 0U);
            arg1->unk0.animTickDelta = 0.02f;
        }
        break;
    case 0x408:
        if (arg1->unk0.unk33A != 0) {
            return dll_210_func_A018() + 1;
        }
        break;
    }

    gDLL_18_objfsa->vtbl->func7(arg0, &arg1->unk0, arg2, 1);
    return 0;
}

extern void dll_210_func_A024(Object* player, Player_Data* objdata);
extern void dll_210_func_18DB0(Object* obj, ObjFSA_Data* fsa);
extern f32 _bss_1C;

/** Fixed crash if you warp while spamming attack (originally by MusicalProgrammer) */
RECOMP_PATCH s32 dll_210_func_18630(Object* player, Player_Data* objdata, f32 arg2) {
    u8 sp47;
    Object* sp40;
    Player_Data* sp3C;

    //@recomp: return early
    if (player->linkedObject == NULL){
        return 0;
    }

    sp3C = player->data;
    sp40 = player->linkedObject;
    objdata->unk0.unk341 = 1;
    if (objdata->unk0.enteredAnimState == 0) {
        sp47 = 0;
        if (objdata->unk0.animTickDelta > 0.0f) {
            if (!(objdata->unk0.unk34A & 1)) {
                if (sp3C->unk3B4[sp3C->unk8A1].unk24 < player->animProgress) {
                    gDLL_6_AMSFX->vtbl->play_sound(player, sp3C->unk3B4[sp3C->unk8A1].unk2C, 0x7FU, NULL, NULL, 0, NULL);
                    objdata->unk0.unk34A |= 1;
                }
            }
            if (!(objdata->unk0.unk34A & 2) && ((&sp3C->unk3B4[sp3C->unk8A1])->unk28 < player->animProgress)) {
                gDLL_6_AMSFX->vtbl->play_sound(player, sp3C->unk3B8[rand_next(0, 2)], 0x7FU, NULL, NULL, 0, NULL);
                objdata->unk0.unk34A |= 2;
            }
        }
        if (sp3C->unk3B4[sp3C->unk8A1].unk9 >= 0) {
            if ((sp3C->unk3B4[sp3C->unk8A1].unk18 < player->animProgress) || ((objdata->unk0.animTickDelta < 0.0f) && (player->animProgress < _bss_1C))) {
                objdata->unk0._unk33E |= 2;
            }
            if (sp3C->unk3B4[sp3C->unk8A1].unk10 < player->animProgress) {
                objdata->unk0._unk33E |= 1;
            }
            if (sp3C->unk3B4[sp3C->unk8A1].unk14 < player->animProgress) {
                objdata->unk0._unk33E &= ~1;
            }
            if (objdata->unk0.unk310 & 0x8000) {
                if (objdata->unk0._unk33E & 1) {
                    objdata->unk0._unk33E |= 4;
                }
            }
            if ((objdata->unk0._unk33E & 4) && (objdata->unk0._unk33E & 2)) {
                sp3C->unk8A1 = (u8) (&sp3C->unk3B4[sp3C->unk8A1])->unk9;
                sp47 = 1;
            }
        }
    } else {
        if (sp40->group == GROUP_UNK48) {
            ((DLL_Unknown *)sp40->dll)->vtbl->func[11].withOneArg((s32)sp40);
        }
        sp47 = 1;
        sp3C->flags &= ~0x40;
        player->objhitInfo->unk61 = 0;
        dll_210_func_A024(player, objdata);
        objdata->unk0.animExitAction = dll_210_func_18DB0;
    }
    if (sp47 != 0) {
        player->unk5C = &sp3C->unk3B4[sp3C->unk8A1].unk34;
        if (sp3C->unk3B4[sp3C->unk8A1].unk0 != player->curModAnimId && sp3C->unk3B4[sp3C->unk8A1].unk2 != player->curModAnimId ) {
            func_80023D30(player, objdata->unk0.target != NULL ? sp3C->unk3B4[sp3C->unk8A1].unk0 : sp3C->unk3B4[sp3C->unk8A1].unk2, 0.0f, 0U);
        }
        objdata->unk0._unk33E &= ~0xEF;
        objdata->unk0.animTickDelta = (&sp3C->unk3B4[sp3C->unk8A1])->unkC;
        objdata->unk0.unk34A = 0;
        objdata->unk0.unk27C = 0.0f;
        if (objdata->unk0.target == NULL) {
            if (objdata->unk0.unk290 > 0.3f) {
                player->srt.yaw += objdata->unk0.unk32A * 0xB6;
                objdata->unk0.unk328 = 0;
                objdata->unk0.unk32A = 0;
            }
        } else {
            gDLL_18_objfsa->vtbl->func11(player, &objdata->unk0, arg2, 2);
        }
        if (player->objhitInfo != NULL) {
            player->objhitInfo->unk61 = 0;
        }
        if (sp40->group == GROUP_UNK48) {
            ((DLL_Unknown *)sp40->dll)->vtbl->func[12].withTwoArgs((s32)sp40, 1);
            ((DLL_Unknown *)sp40->dll)->vtbl->func[13].withTwoArgs((s32)sp40, (&sp3C->unk3B4[sp3C->unk8A1])->unk30);
            ((DLL_Unknown *)sp40->dll)->vtbl->func[18].withThreeArgsCustom2(sp40, (&sp3C->unk3B4[sp3C->unk8A1])->unk1C, (&sp3C->unk3B4[sp3C->unk8A1])->unk20);
        }
    }
    player->objhitInfo->unk5F = (&sp3C->unk3B4[sp3C->unk8A1])->unk4;
    player->objhitInfo->unk60 = (&sp3C->unk3B4[sp3C->unk8A1])->unk8;
    gDLL_18_objfsa->vtbl->func7(player, &objdata->unk0, arg2, 1);
    if (player->animProgress > 0.99f) {
        player->objhitInfo->unk61 = 0;
        if (objdata->unk0.target != NULL) {
            gDLL_18_objfsa->vtbl->func17(player, &objdata->unk0);
            return 0x36;
        }
        return 2;
    }
    if ((player->animProgress > 0.7f) && ((&sp3C->unk3B4[sp3C->unk8A1])->unk9 < 0) && (objdata->unk0.unk310 & 0x8000)) {
        player->objhitInfo->unk61 = 0;
        if (objdata->unk0.target != NULL) {
            gDLL_18_objfsa->vtbl->func11(player, &objdata->unk0, arg2, 2);
            return 0x3C;
        }
        if (objdata->unk0.unk290 > 0.3f) {
            player->srt.yaw += objdata->unk0.unk32A * 0xB6;
            objdata->unk0.unk328 = 0;
            objdata->unk0.unk32A = 0;
        }
        return 0x3D;
    }
    return 0;
}

/** Fix glitch where player rapidly cycles between different animations (originally by Banjeoin) 
  *
  * This is a temporary recomp hook version of the fix, since dll_210_func_AE34 isn't fully decomped yet
  *
  * TODO: replace with a more robust direct patch in dll_210_func_AE34
*/
RECOMP_HOOK_DLL(dll_210_control) void playerModAnimOffsetUnderflowFix(Object* self) {
    Player_Data* objData;
    char message[] = "Oh dear, player modAnimOffset underflowed!\nAttempting to fix...\n";

    if (!self){
        return;
    }

    objData = self->data;
    if (!objData){
        return;
    }

    if (objData->unk8C0 < 0){
        diPrintf(message);
        recomp_eprintf(message);
        objData->unk8C0 = 3;
    }
}
