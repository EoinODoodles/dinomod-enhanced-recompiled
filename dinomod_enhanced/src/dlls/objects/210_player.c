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
RECOMP_PATCH void dll_210_add_magic(Object* self, s32 magicDifference) {
    Player_Data* objdata = self->data;
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
            mapID = map_get_map_id_from_xz_ws(self->srt.transl.x, self->srt.transl.z);
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
RECOMP_PATCH s32 dll_210_func_125BC(Object *self, ObjFSA_Data *fsa, f32 updateRate) {
    f32 effectX;
    f32 effectZ;
    f32 f2;
    f32 f0;
    s32 i;
    DLL27_Data *temp_s3;

    if (fsa->enteredAnimState != 0) {
        fsa->unk270 = 0x1F;
    }
    fsa->flags |= 0x200000;
    temp_s3 = &fsa->unk4;
    if (fsa->enteredAnimState != 0) {
        gDLL_6_AMSFX->vtbl->play_sound(self, 0x3D8U, 0x7FU, NULL, NULL, 0, NULL);
        for (i = 0; i < 3; i++) {
            effectX = ((f32) rand_next(-50, 50) / 10.0f) + self->srt.transl.x;
            effectZ = ((f32) rand_next(-50, 50) / 10.0f) + self->srt.transl.z;
            gDLL_24_Waterfx->vtbl->func_174C(effectX, temp_s3->waterY, effectZ, 4.0f);
            gDLL_24_Waterfx->vtbl->func_1CC8(effectX, temp_s3->waterY, effectZ, 0, 0.0f, 3);
        }
    }

    if (
        temp_s3->underwaterDist > 25.0f
        // && temp_s3->floorDist < 100.0f //@recomp: remove check
    ) {
        return 33 + 1;
    }

    if (temp_s3->unk25C & 0x10) {
        return 1 + 1;
    }
    f0 = temp_s3->waterY - 6.0f;
    f2 = f0 - self->srt.transl.y;
    if (f2 > 25.0f) {
        f2 = 25.0f;
    }
    self->speed.y += (f2 / 25.0f) * 0.13f * gUpdateRateF;
    self->speed.y -= 0.1f * gUpdateRateF;
    self->speed.y *= 0.96f;
    if (self->speed.y > 1.4f) {
        self->speed.y = 1.4f;
    }
    self->speed.x *= 0.98f;
    self->speed.z *= 0.98f;
    for (i = 0; i < 4; i++) {
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_202, NULL, PARTFXFLAG_NONE, -1, NULL);
    }
    return 0;
}

s32 func_80025140(Object*, f32, f32, s32);
extern void dll_210_func_14B70(Object* arg0, ObjFSA_Data *arg1);
extern s16 _data_158[];

/** Fix Sabre floating around when he rides SnowHorns, stop projectile spell activating on dismount (originally by MusicalProgrammer) */
RECOMP_PATCH s32 dll_210_func_142C4(Object* self, Player_Data* objData, f32 arg2) {
    Player_Data* objData2;
    Object* steed;
    f32 sp44;
    f32 sp40;
    f32 sp3C;
    s32 sp38;
    s32 sp34;
    s32 sp30;

    sp30 = 0; //@recomp: initialise variable

    gDLL_2_Camera->vtbl->func24.withOneArg(2);
    objData2 = self->data;
    objData->unk0.unk4.mode = 0;
    objData->unk0.animExitAction = dll_210_func_14B70;

    //@recomp: prevent Projectile Spell equip
    objData->unk834 = 0;
    objData->flags &= 0xFF00;

    func_800267A4(self);
    steed = objData2->unk858;
    if (steed == NULL) {
        self->curModAnimIdLayered = -1;
        return 0;
    }
    if (objData->unk0.enteredAnimState != 0) {
        if (objData2->unk76C == NULL) {
            objData2->unk76C = (s16 *)_data_158;
        }
        if (objData2->unk770 & 2) {
            sp30 = 8;
            func_80024E50(self, *objData2->unk76C, 0.0f, 0);
            func_80025140(self, 0.0f, 0, 0); // arg2 should be 0.0f
            func_80024E50(self, objData2->unk76C[1], 0.0f, 0xA);
            func_80025140(self, 0.0f, 0, 0); // arg2 should be 0.0f
        }
        func_80023D30(self, *objData2->unk76C, 0.0f, (u8)sp30);
        func_80024108(self, 0.0f, 0.0f, NULL);
    }
    if (objData2->unk770 & 4) {
        func_800240BC(self, steed->animProgress);
        objData->unk0.animTickDelta = NULL;
    } else {
        sp3C = ((DLL_IVehicle*)steed->dll)->vtbl->func16(steed, &sp44);
        if (sp44 <= 1.0f) {
            objData->unk0.animTickDelta = sp44;
        } else {
            objData->unk0.animTickDelta = ((sp3C * 0.05f) + 0.01f);
        }
    }
    if (objData2->unk770 & 1) {
        ((DLL_IVehicle*)steed->dll)->vtbl->func15(steed, &sp40, &sp34);
        sp38 = (sp40 * 1023.0f);
        if (sp38 < 0) {
            sp38 = -sp38;
        }
        if (sp34 != 0) {
            func_80025540(self, objData2->unk76C[3], sp38);
            func_8002559C(self, objData2->unk76C[5], sp38);
        } else {
            func_80025540(self, objData2->unk76C[2], sp38);
            func_8002559C(self, objData2->unk76C[4], sp38);
        }
    }
    if (objData2->unk770 & 1) {
        func_80024DD0(self, 0, 2, 0);
        func_80024DD0(self, 1, 2, 0);
    }
    if (objData2->unk770 & 2) {
        func_80024DD0(self, 1, 0, sp3C * 1023.0f);
        func_80025140(self, objData->unk0.animTickDelta, arg2, 0);
    }
    if (((DLL_IVehicle*)steed->dll)->vtbl->func10(steed, self) != 0) {
        return 0x27;
    }
    return 0;
}

extern s32 dll_210_func_A018(void);

/** Fix snowball-player collision crash in DarkIce Mines (originally by MusicalProgrammer) */
RECOMP_PATCH s32 dll_210_func_17C14(Object* self, Player_Data* objData, f32 arg2) {
    Object* sp34;
    Player_Data* objData2;

    objData2 = self->data;
    objData->unk0.unk341 = 3;
    if (objData->unk0.enteredAnimState != 0) {

        if (func_80025F40(self, &sp34, NULL, NULL)){
            //@recomp: don't try to calculate angle if func_80025F40 returns 0
            self->srt.yaw = arctan2_f(-sp34->speed.f[0], -sp34->speed.f[2]);
        }

        func_80023D30(self, 0x407, 0.0f, 0U);
        objData->unk0.animTickDelta = 0.015f;
    }
    switch (self->curModAnimId) {
    case 0x407:
        if (objData->unk0.unk33A != 0) {
            if (objData2->stats->health <= 0) {
                return 0x35;
            }
            func_80023D30(self, 0x408, 0.0f, 0U);
            objData->unk0.animTickDelta = 0.02f;
        }
        break;
    case 0x408:
        if (objData->unk0.unk33A != 0) {
            return dll_210_func_A018() + 1;
        }
        break;
    }

    gDLL_18_objfsa->vtbl->func7(self, &objData->unk0, arg2, 1);
    return 0;
}

extern void dll_210_func_A024(Object* player, Player_Data* objdata);
extern void dll_210_func_18DB0(Object* obj, ObjFSA_Data* fsa);
extern f32 _bss_1C;

/** Fixed crash if you warp while spamming attack (originally by MusicalProgrammer) */
RECOMP_PATCH s32 dll_210_func_18630(Object* self, Player_Data* objData, f32 arg2) {
    u8 sp47;
    Object* weapon;
    Player_Data* objData2;

    //@recomp: return early
    if (self->linkedObject == NULL){
        return 0;
    }

    objData2 = self->data;
    weapon = self->linkedObject;
    objData->unk0.unk341 = 1;
    if (objData->unk0.enteredAnimState == 0) {
        sp47 = 0;
        if (objData->unk0.animTickDelta > 0.0f) {
            if (!(objData->unk0.unk34A & 1)) {
                if (objData2->unk3B4[objData2->unk8A1].unk24 < self->animProgress) {
                    gDLL_6_AMSFX->vtbl->play_sound(self, objData2->unk3B4[objData2->unk8A1].unk2C, 0x7FU, NULL, NULL, 0, NULL);
                    objData->unk0.unk34A |= 1;
                }
            }
            if (!(objData->unk0.unk34A & 2) && ((&objData2->unk3B4[objData2->unk8A1])->unk28 < self->animProgress)) {
                gDLL_6_AMSFX->vtbl->play_sound(self, objData2->unk3B8[rand_next(0, 2)], 0x7FU, NULL, NULL, 0, NULL);
                objData->unk0.unk34A |= 2;
            }
        }
        if (objData2->unk3B4[objData2->unk8A1].unk9 >= 0) {
            if ((objData2->unk3B4[objData2->unk8A1].unk18 < self->animProgress) || ((objData->unk0.animTickDelta < 0.0f) && (self->animProgress < _bss_1C))) {
                objData->unk0._unk33E |= 2;
            }
            if (objData2->unk3B4[objData2->unk8A1].unk10 < self->animProgress) {
                objData->unk0._unk33E |= 1;
            }
            if (objData2->unk3B4[objData2->unk8A1].unk14 < self->animProgress) {
                objData->unk0._unk33E &= ~1;
            }
            if (objData->unk0.unk310 & 0x8000) {
                if (objData->unk0._unk33E & 1) {
                    objData->unk0._unk33E |= 4;
                }
            }
            if ((objData->unk0._unk33E & 4) && (objData->unk0._unk33E & 2)) {
                objData2->unk8A1 = (u8) (&objData2->unk3B4[objData2->unk8A1])->unk9;
                sp47 = 1;
            }
        }
    } else {
        if (weapon->group == GROUP_UNK48) {
            ((DLL_Unknown *)weapon->dll)->vtbl->func[11].withOneArg((s32)weapon);
        }
        sp47 = 1;
        objData2->flags &= ~0x40;
        self->objhitInfo->unk61 = 0;
        dll_210_func_A024(self, objData);
        objData->unk0.animExitAction = dll_210_func_18DB0;
    }
    if (sp47 != 0) {
        self->unk5C = &objData2->unk3B4[objData2->unk8A1].unk34;
        if (objData2->unk3B4[objData2->unk8A1].unk0 != self->curModAnimId && objData2->unk3B4[objData2->unk8A1].unk2 != self->curModAnimId ) {
            func_80023D30(self, objData->unk0.target != NULL ? objData2->unk3B4[objData2->unk8A1].unk0 : objData2->unk3B4[objData2->unk8A1].unk2, 0.0f, 0U);
        }
        objData->unk0._unk33E &= ~0xEF;
        objData->unk0.animTickDelta = (&objData2->unk3B4[objData2->unk8A1])->unkC;
        objData->unk0.unk34A = 0;
        objData->unk0.unk27C = 0.0f;
        if (objData->unk0.target == NULL) {
            if (objData->unk0.unk290 > 0.3f) {
                self->srt.yaw += objData->unk0.unk32A * 0xB6;
                objData->unk0.unk328 = 0;
                objData->unk0.unk32A = 0;
            }
        } else {
            gDLL_18_objfsa->vtbl->func11(self, &objData->unk0, arg2, 2);
        }
        if (self->objhitInfo != NULL) {
            self->objhitInfo->unk61 = 0;
        }
        if (weapon->group == GROUP_UNK48) {
            ((DLL_Unknown *)weapon->dll)->vtbl->func[12].withTwoArgs((s32)weapon, 1);
            ((DLL_Unknown *)weapon->dll)->vtbl->func[13].withTwoArgs((s32)weapon, (&objData2->unk3B4[objData2->unk8A1])->unk30);
            ((DLL_Unknown *)weapon->dll)->vtbl->func[18].withThreeArgsCustom2(weapon, (&objData2->unk3B4[objData2->unk8A1])->unk1C, (&objData2->unk3B4[objData2->unk8A1])->unk20);
        }
    }
    self->objhitInfo->unk5F = (&objData2->unk3B4[objData2->unk8A1])->unk4;
    self->objhitInfo->unk60 = (&objData2->unk3B4[objData2->unk8A1])->unk8;
    gDLL_18_objfsa->vtbl->func7(self, &objData->unk0, arg2, 1);
    if (self->animProgress > 0.99f) {
        self->objhitInfo->unk61 = 0;
        if (objData->unk0.target != NULL) {
            gDLL_18_objfsa->vtbl->func17(self, &objData->unk0);
            return 0x36;
        }
        return 2;
    }
    if ((self->animProgress > 0.7f) && ((&objData2->unk3B4[objData2->unk8A1])->unk9 < 0) && (objData->unk0.unk310 & 0x8000)) {
        self->objhitInfo->unk61 = 0;
        if (objData->unk0.target != NULL) {
            gDLL_18_objfsa->vtbl->func11(self, &objData->unk0, arg2, 2);
            return 0x3C;
        }
        if (objData->unk0.unk290 > 0.3f) {
            self->srt.yaw += objData->unk0.unk32A * 0xB6;
            objData->unk0.unk328 = 0;
            objData->unk0.unk32A = 0;
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
