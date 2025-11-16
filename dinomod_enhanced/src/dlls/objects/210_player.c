
#include "modding.h"
#include "recomputils.h"
#include "dll_util.h"
#include "sidekick_util.h"

#include "game/objects/object_id.h"
#include "sys/dll.h"
#include "sys/map_enums.h"
#include "sys/math.h"
#include "sys/menu.h"
#include "sys/objects.h"
#include "sys/objanim.h"
#include "sys/objtype.h"
#include "sys/objmsg.h"
#include "sys/print.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/common/vehicle.h"
#include "dlls/engine/29_gplay.h"
#include "dlls/engine/6_amsfx.h"
#include "functions.h"
#include "dll.h"

#include "recomp/dlls/objects/210_player_recomp.h"

extern void func_8005B5B8(Object*, Object*, s32);

extern ObjFSA_StateCallback _bss_58[81];
extern ObjFSA_StateCallback _bss_19C[1];
extern u8 _data_14[4];
extern s16 _data_18[2];
extern s16 _data_24[2];
extern s16 _data_188[4];
extern s16 _data_170[12];
extern f32 _bss_1AC;

extern void dll_210_func_60A8(Object* arg0, s32 arg1, s32 arg2);
extern f32 dll_210_func_63F0(Player_Data* arg0, f32 updateRate);
extern void dll_210_func_1D4E0(Object* arg0, s32 arg1);
extern void dll_210_func_1DB6C(Object* arg0, f32 arg1);
extern void dll_210_func_1D8B8(Object* player);
extern s32 dll_210_func_1D2A8(Object* arg0, Object* arg1);
extern void dll_210_func_9F1C(Object* arg0, s32 arg1);
extern void dll_210_func_1BC0(Object* arg0, Player_Data* arg1);

// TODO: take from decomp headers
DLL_INTERFACE(DLL_IFoodbag) {
    /*:*/ struct DLL_IObject_Vtbl base;
	/*7*/ UnknownDLLFunc func7;
	/*8*/ UnknownDLLFunc func8;
	/*9*/ UnknownDLLFunc func9;
	/*10*/ UnknownDLLFunc func10;
	/*11*/ UnknownDLLFunc func11;
	/*12*/ void (*func12)(Object *self, s16 a1);
};

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

extern s32 func_80025140(Object*, f32, f32, s32);
extern u8 _data_530;

/** Stop spells from unequipping whenever a cutscene starts (originally by MusicalProgrammer) */
RECOMP_PATCH void dll_210_func_692C(Object* self, Player_Data* objData, f32 arg2) {
    s32 var_s2;
    s32 var_v0;
    ModelInstance *modelInstance;
    AnimState *animState;
    Object *linkedObject = self->linkedObject;
    u8 new_var;
    u8 new_var2;
    u8 *temp;

    if (linkedObject == NULL) {
        return;
    }

    var_v0 = 0;
    do {
        temp = &_data_530;
        new_var = 1;
        var_s2 = 0;
        switch (objData->unk878) {
        case 2:
            if (var_v0 != 0) {
                func_80024E50(self, self->curModAnimId, self->animProgress, 0);
                func_80024E50(self, 0x8A, 0.0f, 0);
            }
            if ((self->animProgressLayered > 0.2f) && !*temp) {
                gDLL_6_AMSFX->vtbl->play_sound(self, objData->unk3B8[4], 0x7FU, NULL, NULL, 0, NULL);
                _data_530 = new_var;
            }
            if (self->animProgressLayered > 0.4f) {
                objData->unk8A8 = 2;
            }
            if (func_80025140(self, 0.015f, arg2, 0) != 0) {
                objData->unk878 = 3;
                var_s2 = new_var;
            }
            break;
        case 13:
            if (!*temp) {
                gDLL_6_AMSFX->vtbl->play_sound(self, objData->unk3B8[4], 0x7FU, NULL, NULL, 0, NULL);
                _data_530 = new_var;
            }
            objData->unk8A8 = 2;
            objData->unk878 = 0;
            break;
        case 1:
            if (var_v0 != 0) {
                func_80024E50(self, self->curModAnimId, self->animProgress, 0);
                func_80024E50(self, 0x8A, 0.99f, 0);
            }
            if ((self->animProgressLayered < 0.8f) && !*temp) {
                gDLL_6_AMSFX->vtbl->play_sound(self, objData->unk3B8[3], 0x7FU, NULL, NULL, 0, NULL);
                _data_530 = new_var;
            }
            if (self->animProgressLayered < 0.4f) {
                objData->unk8A8 = 0;
            }
            if (func_80025140(self, -0.015f, arg2, 0) != 0) {
                objData->unk87C = -1;
                objData->unk878 = 3;
                var_s2 = new_var;
            }
            break;
        case 14:
            if (!*temp) {
                gDLL_6_AMSFX->vtbl->play_sound(self, objData->unk3B8[3], 0x7FU, NULL, NULL, 0, NULL);
                _data_530 = new_var;
            }
            // objData->unk87C = -1; //@recomp: don't set value
            objData->unk8A8 = 0;
            objData->unk878 = 0;
            break;
        case 3:
            if (var_v0 != 0) {
                func_80024E50(self, self->curModAnimId, self->animProgress, 0);
            } else {
                func_80025140(self, self->animProgress - self->animProgressLayered, 1.0f, 0);
            }
            modelInstance = self->modelInsts[self->modelInstIdx];
            animState = modelInstance->animState1;
            if (animState->unk58[0] == 0) {
                self->curModAnimIdLayered = -1;
                objData->unk878 = 0;
            }
            break;
        default:
            if (self->linkedObject != NULL) {
                new_var2 = 0;
                if (objData->unk8A8 != 0) {
                    if (objData->unk8A9 == 0) {
                        objData->unk878 = new_var;
                        var_s2 = new_var;
                        _data_530 = new_var2;
                        break;
                    }
                    if (objData->unk8A9 == 1) {
                        objData->unk878 = 0xE;
                        var_s2 = new_var;
                        _data_530 = new_var2;
                    }
                    break;
                }
                if (objData->unk8A9 == 2) {
                    objData->unk878 = 2;
                    var_s2 = new_var;
                    _data_530 = new_var2;
                    break;
                }
                if (objData->unk8A9 == 4) {
                    objData->unk878 = 0xD;
                    var_s2 = new_var;
                    _data_530 = new_var2;
                }
            }
            break;
        }
        var_v0 = var_s2;
    } while (var_s2 != 0);
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

extern s32 func_80025140(Object*, f32, f32, s32);
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

extern s8 _bss_0;
extern s16 _bss_2;
extern f32 _data_528;
extern s8 _data_52C;
/* Fix mount logic when multiple vehicles are loaded. (Original patch by MusicalProgrammer) */
// TODO: replace with a cleaner match from the decomp, when we have one
RECOMP_PATCH int dll_210_func_4910(Object* arg0, Object* arg1, AnimObj_Data* arg2, s8 arg3) {
    AnimObjSetup* animSetup;
    Object* temp_a0_4;
    s32 var_v0;
    s32 spC8;
    Object** objects;
    s32 spC0;
    f32 temp_fa1;
    f32 temp_fv0_2;
    f32 spB4;
    f32 spB0;
    f32 spAC;
    f32 spA8;
    f32 spA4;
    f32 temp_fv0_4;
    f32 sp9C;
    s16* temp_s0_2;
    s32 temp_t1;
    s32 temp_v1_8;
    Object* var_s0; // sp8C
    s32 var_s1;
    Player_Data* objdata;
    Vec3f* temp_s0;
    Object* temp_v0_6;
    void *pad;
    //f32 *temp_data_528 = &_data_528;
    f32 sp6C[3];
    s16 sp6A;
    //s8 *temp_data_52C = &_data_52C;
    Object *tempObj;
    f32 sp60;

    objdata = arg0->data;
    animSetup = (AnimObjSetup*)arg1->setup;
    spC8 = 0;
    arg2->unkF4 = (AnimObj_DataF4Callback)dll_210_func_60A8;
    objdata->unk818 = 0.0f;
    _bss_1AC = dll_210_func_63F0(objdata, gUpdateRateF);
    if (_bss_1AC > 4.0f) {
        _bss_1AC = 4.0f;
    }
    if (objdata->flags & 0x8000) {
        if (objdata->stats->health > 0) {
            gDLL_18_objfsa->vtbl->set_anim_state(arg0, &objdata->unk0, 1);
            objdata->flags &= ~0x8000;
            return 0;
        }
        return 1;
    }
    if (arg0->linkedObject == NULL) {
        arg0->linkedObject = obj_create(obj_alloc_create_info(0x18, _data_24[objdata->unk8B4]), OBJ_INIT_FLAG4, -1, -1, arg0->parent);
    } else {
        arg0->linkedObject->parent = arg0->parent;
    }
    func_800267A4(arg0);
    objdata->flags &= ~2;
    if (arg2->unk62 != 0) {
        objdata->unk8A9 = 1;
        objdata->flags &= ~0x400;
        if ((animSetup->unk20 == 0) || arg2->unk62 == 3 || arg2->unk62 == 2) {
            arg2->unk7A = arg2->unk7C;
            if (arg2->unk62 != 2) {
                arg2->unk58 = 1.0f;
                arg2->unk4C.x = arg0->srt.transl.x - arg1->srt.transl.x;
                arg2->unk4C.y = arg0->srt.transl.y - arg1->srt.transl.y;
                arg2->unk4C.z = arg0->srt.transl.z - arg1->srt.transl.z;
                arg2->yawDiff = arg0->srt.yaw - (arg1->srt.yaw & 0xFFFF);
                // @recomp: CIRCLE_WRAP with s32 instead of s16
                {
                    s32 yawDiff = arg2->yawDiff;
                    CIRCLE_WRAP(yawDiff);
                    arg2->yawDiff = yawDiff;
                }
                arg2->pitchDiff = arg0->srt.pitch - (arg1->srt.pitch & 0xFFFF);
                // @recomp: CIRCLE_WRAP with s32 instead of s16
                {
                    s32 pitchDiff = arg2->pitchDiff;
                    CIRCLE_WRAP(pitchDiff);
                    arg2->pitchDiff = pitchDiff;
                }
                arg2->rollDiff = (arg1->srt.roll  & 0xFFFF) - (arg0->srt.roll & 0xFFFF);
                // @recomp: CIRCLE_WRAP with s32 instead of s16
                {
                    s32 rollDiff = arg2->rollDiff;
                    CIRCLE_WRAP(rollDiff);
                    arg2->rollDiff = rollDiff;
                }
                arg2->unk62 = 2;
            }
            arg2->unk58 -= arg2->unk24 * gUpdateRateF;
            if (arg2->unk58 <= 0.0f) {
                arg2->unk62 = 0;
            }
            arg0->curModAnimIdLayered = -1;
        } else if (arg2->unk62 == 4) {
            arg2->unk7A &= ~0xC;
            arg2->unk7C &= ~0x8;
            temp_v0_6 = (Object *)gDLL_2_Camera->vtbl->func15.asVoidS32();
            if (temp_v0_6 == NULL || temp_v0_6->unk74 == NULL) {
                return 0;
            }
            temp_s0 = temp_v0_6->unk74;
            spB4 = arg0->srt.transl.x - temp_s0->x;
            spB0 = arg0->srt.transl.z - temp_s0->z;
            spA4 = (arg0->srt.transl.y + 30.0f) - temp_s0->y;
            _bss_2 = arctan2_f(spB4, spB0);
            temp_fv0_4 = sqrtf(SQ(spB4) + SQ(spB0));
            arg2->yawDiff = _bss_2 - (arg0->srt.yaw & 0xFFFF);
            // @recomp: CIRCLE_WRAP with s32 instead of s16
            {
                s32 yawDiff = arg2->yawDiff;
                CIRCLE_WRAP(yawDiff);
                arg2->yawDiff = yawDiff;
            }
            arg2->pitchDiff = -arctan2_f(spA4, temp_fv0_4);
            arg2->rollDiff = 0;
            arg2->unk58 = 0.0f;
            arg2->unk24 = 0.083333336f;
            arg2->unk62 = 5;
            objdata->unk354.unk16 = func_80034804(arg0, 0)[1];
            objdata->unk354.unk14 = arg2->yawDiff;
            objdata->unk378.unk16 = 0;
            objdata->unk378.unk14 = arg2->pitchDiff;
            _bss_0 = 0;
            sp6C[0] = temp_s0->x;
            sp6C[1] = temp_s0->y;
            sp6C[2] = temp_s0->z;
            if (arg0->curModAnimId != 0) {
                func_80023D30(arg0, 0, 0.0f, 0U);
            }
            return 1;
        } else if (arg2->unk62 == 5) {
            arg2->unk7A &= ~0x4;
            func_8002674C(arg0);
            temp_s0_2 = func_80034804(arg0, 0);
            if (_bss_0 == 3) {
                if ((arg2->unk58 >= 1.0f) && (gDLL_2_Camera->vtbl->func18.asVoidS32() == 0)) {
                    if (arg3 == 0) {
                        arg2->unk62 = 0;
                    } else {
                        arg2->unk62 = 6;
                    }
                    if (objdata->unk858 != NULL) {
                        gDLL_18_objfsa->vtbl->set_anim_state(arg0, &objdata->unk0, 0x24);
                        return 1;
                    }
                    gDLL_18_objfsa->vtbl->set_anim_state(arg0, &objdata->unk0, 1);
                    return 1;
                }
                temp_fv0_2 = arg2->unk58;
                arg2->unk58 += (arg2->unk24 * gUpdateRateF);
                if (arg2->unk58 > 1.0f) {
                    arg2->unk58 = 1.0f;
                }
                temp_fv0_2 = arg2->unk58 - temp_fv0_2;
                arg0->srt.yaw += (s16) (temp_fv0_2 * arg2->yawDiff);
                temp_s0_2[1] = _bss_2 - (arg0->srt.yaw & 0xFFFF);
                // @recomp: CIRCLE_WRAP with s32 instead of s16
                {
                    s32 temp_s0_2_1 = temp_s0_2[1];
                    CIRCLE_WRAP(temp_s0_2_1);
                    temp_s0_2[1] = temp_s0_2_1;
                }
            } else {
                _bss_0 |= func_800343B8(&objdata->unk354, temp_s0_2, 100.0f, 2000.0f);
                _bss_0 |= func_80034518(&objdata->unk378, temp_s0_2, 100.0f, 2000.0f) * 2;
            }
            return 1;
        } else if (arg2->unk62 == 6) {
            if (arg3 == 0) {
                arg2->unk62 = 0;
                return 0;
            }
            func_8002674C(arg0);
            return 0;
        } else {
            if (arg2->unk62 != 1) {
                arg2->unk4C.x = arg0->srt.transl.x;
                arg2->unk4C.y = arg0->srt.transl.y;
                arg2->unk4C.z = arg0->srt.transl.z;
                _data_528 = 10000.0f;
                _data_52C = 0;
            }
            spC8 = 1;
            arg2->unk7A = 0;
            arg2->unk62 = 1;
            sp9C = sqrtf(SQ(arg2->unk4C.x - arg0->srt.transl.x) + SQ(arg2->unk4C.z - arg0->srt.transl.z));
            spAC = arg1->srt.transl.x - arg2->unk4C.x;
            spA8 = arg1->srt.transl.z - arg2->unk4C.z;
            temp_fv0_4 = sqrtf(SQ(spAC) + SQ(spA8));
            if (sp9C <= _data_528) {
                _data_52C += 1;
            }
            if ((temp_fv0_4 <= sp9C) || (_data_52C >= 6)) {
                var_v0 = arg0->srt.yaw - (arg1->srt.yaw & 0xFFFF);
                CIRCLE_WRAP(var_v0)
                // CLAMP
                if (var_v0 > 0x4000) { var_v0 = 0x4000; }
                if (var_v0 < -0x4000) { var_v0 = -0x4000; }

                arg0->srt.yaw -= (var_v0 * gUpdateRate) >> 3;
                if (_data_52C >= 7) {
                    var_v0 = 0;
                }
                if ((var_v0 < 0x100) && (var_v0 >= -0xFF)) {
                    arg2->unk7A = arg2->unk7C;
                    arg2->unk62 = 0;
                    arg2->animCurvesCurrentFrameB = arg2->animCurvesCurrentFrameA - 1;
                    arg0->curModAnimIdLayered = -1;
                    spC8 = 0;
                } else {
                    objdata->unk0.unk288 = 0.0f;
                    objdata->unk0.unk284 = 0.0f;
                    gDLL_18_objfsa->vtbl->func3(&arg1->srt);
                    objdata->unk0.unk310 = 0;
                    objdata->unk0.unk30C = 0;
                    arg0->unkDC = 0;
                    objdata->unk0.unk324 = 0;
                    objdata->unk0.unk4.mode = 1;
                    objdata->unk8B8 = 0;
                    gDLL_18_objfsa->vtbl->tick(arg0, &objdata->unk0, gUpdateRateF, gUpdateRateF, _bss_58, _bss_19C);
                }
            } else {
                spAC /= temp_fv0_4;
                spA8 /= temp_fv0_4;
                objdata->unk0.unk288 = -spAC * 40.0f;
                objdata->unk0.unk284 = spA8 * 40.0f;
                arg0->srt.transl.x = arg2->unk4C.x + (sp9C * spAC);
                arg0->srt.transl.z = arg2->unk4C.z + (sp9C * spA8);
                gDLL_18_objfsa->vtbl->func3(&arg1->srt);
                objdata->unk0.unk310 = 0;
                objdata->unk0.unk30C = 0;
                arg0->unkDC = 0;
                objdata->unk0.unk324 = 0;
                objdata->unk0.unk4.mode = 1;
                objdata->unk8B8 = 0;
                gDLL_18_objfsa->vtbl->tick(arg0, &objdata->unk0, gUpdateRateF, gUpdateRateF, _bss_58, _bss_19C);
            }
            _data_528 = sp9C;
        }
        if (arg2->unk62 == 0) {
            gDLL_18_objfsa->vtbl->set_anim_state(arg0, &objdata->unk0, 1);
        }
    } else {
        arg2->unk7A |= arg2->unk7C & ~0x400;
        objdata->unk0.unk340 = 0;
        objdata->unk0.unk324 = 0;
        objdata->unk0.unk310 = 0;
        objdata->unk0.unk30C = 0;
        objdata->unk0.unk4.mode = 0;
        objdata->unk0.unk288 = 0.0f;
        objdata->unk0.unk284 = 0.0f;
        for (var_s1 = 0; var_s1 < arg2->unk98; var_s1++) {
            switch (arg2->unk8E[var_s1]) {
            case 3:
                objects = obj_get_all_of_type(0xB, &spC0);
                // @recomp: Choose closest vehicle when multiple are in the scene
                f32 closestDist = SQ(1000000.0f);
                for (var_s1 = 0; var_s1 < spC0; var_s1++) {
                    var_s0 = objects[var_s1];
                    if (
                        var_s0->id == OBJ_IMSnowBike
                        || var_s0->id == OBJ_CRSnowBike
                        || var_s0->id == OBJ_BWLog
                        || var_s0->id == OBJ_DIMSnowHorn1
                        || var_s0->id == OBJ_DR_EarthWarrior
                        || var_s0->id == OBJ_DR_CloudRunner
                    ) {
                        f32 dist = vec3_distance_squared(&arg0->positionMirror, &var_s0->positionMirror);
                        if (dist < closestDist) {
                            closestDist = dist;
                            objdata->unk858 = var_s0;
                        }
                    }
                }
                if (objdata->unk858 != NULL) {
                    var_s0 = objdata->unk858;
                    objdata->unk728 = 1.0f;
                    objdata->unk72C.x = objdata->unk7EC.x;
                    objdata->unk72C.y = objdata->unk7EC.y;
                    objdata->unk72C.z = objdata->unk7EC.z;
                    ((DLL_IVehicle*)var_s0->dll)->vtbl->func14(var_s0, 2);
                    arg0->srt.flags |= 8;
                    arg0->unk64->flags |= 0x1000;
                    arg2->unk7A &= ~0x4;
                    //temp_v1_8 = var_s0->id;
                    switch (var_s0->id) {
                        case OBJ_IMSnowBike:
                        case OBJ_CRSnowBike:
                            objdata->unk76C = _data_158;
                            objdata->unk770 = 3;
                            func_80023D30(arg0, 0x17, 0.0f, 1);
                            break;
                        case OBJ_BWLog:
                            objdata->unk76C = _data_188;
                            objdata->unk770 = 3;
                            break;
                        default:
                            objdata->unk76C = _data_170;
                            objdata->unk770 = 4;
                            func_80023D30(arg0, 0xF5, 0.0f, 1);
                            break;
                    }
                    gDLL_18_objfsa->vtbl->set_anim_state(arg0, &objdata->unk0, 0x24);
                }
                break;
            case 2:
                gDLL_2_Camera->vtbl->func8(0, 1);
                gDLL_3_Animation->vtbl->func19(0x54, 4, 0, 0);
                var_s0 = objdata->unk858;
                if (var_s0 != NULL) {
                    ((DLL_IVehicle*)var_s0->dll)->vtbl->func14(var_s0, 0);
                    arg0->srt.flags &= ~0x8;
                    arg0->unk64->flags &= ~0x1000;
                    var_s0 = NULL;
                    arg2->unk7A |= 4;
                    objdata->unk858 = NULL;
                    arg0->curModAnimIdLayered = -1;
                    gDLL_18_objfsa->vtbl->set_anim_state(arg0, &objdata->unk0, 1);
                }
                break;
            case 4:
                var_s0 = objdata->unk858;
                gDLL_3_Animation->vtbl->func19(0x57, 0, 0, 0);
                objdata->unk76C = NULL;
                if ((var_s0 != NULL) && (var_s0->id == 0x22)) {
                    gDLL_18_objfsa->vtbl->set_anim_state(arg0, &objdata->unk0, 0x22);
                } else {
                    gDLL_18_objfsa->vtbl->set_anim_state(arg0, &objdata->unk0, 0x24);
                }
                break;
            case 11:
                var_s0 = objdata->unk858;
                if ((var_s0 != NULL) && (var_s0->id == 0x416)) {
                    gDLL_2_Camera->vtbl->func8(0, 0x69);
                    gDLL_3_Animation->vtbl->func19(0x54, 4, 0, 0);
                } else if ((var_s0 != NULL) && (var_s0->id == 0x419)) {
                    gDLL_3_Animation->vtbl->func19(0x65, 0, 0, 0);
                } else {
                    gDLL_2_Camera->vtbl->func8(0, 0x1D);
                    gDLL_3_Animation->vtbl->func19(0x54, 4, 0, 0);
                }
                break;
            case 6:
                gDLL_3_Animation->vtbl->func19(0x56, 0, 0, 0);
                gDLL_18_objfsa->vtbl->set_anim_state(arg0, &objdata->unk0, 0x23);
                break;
            case 7:
                arg2->unk7A &= ~0x3;
                dll_210_func_1D4E0(arg0, 1);
                arg0->srt.flags |= 8;
                break;
            case 8:
                arg2->unk7A = arg2->unk7C;
                dll_210_func_1D4E0(arg0, 0);
                arg0->srt.flags &= ~8;
                break;
            case 10:
                objdata->unk878 = 2;
                objdata->unk8A9 = 2;
                objdata->unk8A8 = 1;
                break;
            case 12:
                sp6A = gDLL_22_Subtitles->vtbl->func_214C((s32) arg0->unkC4->id);
                gDLL_22_Subtitles->vtbl->func_2248(1U);
                gDLL_22_Subtitles->vtbl->func_368((u16) sp6A);
                arg0->unkC4 = NULL;
                break;
            case 13:
                gDLL_3_Animation->vtbl->func30(arg0->unkC4->id, arg0->unkC4, 0);
                dll_210_func_1DB6C(arg0->unkC4, 29.0f);
                gDLL_3_Animation->vtbl->func17(arg0->unkDC, arg0, -1);
                break;
            case 14:
                if (*_data_14 == 1) {
                    *_data_18 = -1;
                    ((DLL_IFoodbag*)objdata->unk85C->dll)->vtbl->func12(objdata->unk85C, arg0->unkE0);
                    arg2->unk9D |= 4;
                    arg0->unkC4 = NULL;
                    return 4;
                }
                if ((*_data_14 != 0) || (arg0->unkDC < 0)) {
                    arg2->unk9D |= 4;
                    arg0->unkC4 = NULL;
                } else {
                    *_data_18 = arg0->unkE0;
                }
                arg0->unkDC -= 1;
                break;
            case 17:
                dll_210_func_1D8B8(arg0);
                break;
            case 15:
                func_8005B5B8(arg0, NULL, 1);
                break;
            case 16:
                sp60 = 400.0f;
                tempObj = obj_get_nearest_type_to(OBJTYPE_7, arg0, &sp60);
                if (tempObj != NULL) {
                    func_8005B5B8(arg0, tempObj, 1);
                }
                break;
            case 23:
                dll_210_func_1D2A8(arg0, NULL);
                break;
            case 20:
                objdata->flags |= 0x40000;
                break;
            case 21:
                objdata->flags &= ~0x40000;
                break;
            case 22:
                objdata->flags |= 0x20000;
                break;
            case 18:
                objdata->flags |= 0x8000;
                break;
            case 19:
                menu_set(1);
                break;
            case 25:
                dll_210_func_9F1C(arg0, 1);
                break;
            }
        }

        if (objdata->unk708 != NULL) {
            if (objdata->unk708->def->unkAA >= 0) {
                if (arg2->unk8D == 0x1A) {
                    gDLL_1_UI->vtbl->func_1338(objdata->unk708->def->unkAA, 0xA0, 0x8C);
                }
            } else {
                gDLL_1_UI->vtbl->func_130C(objdata->unk708->def->unkA2, 0xA0, 0x8C);
            }
            if (arg2->unk8D == 1) {
                gDLL_3_Animation->vtbl->func19(0x54, 3, 0, 0);
                obj_send_mesg(objdata->unk708, 0x7000B, arg0, NULL);
                objdata->unk708 = NULL;
            }
        }
    }
    if ((objdata->unk858 != NULL) && 
        (((DLL_IVehicle*)objdata->unk858->dll)->vtbl->func13(objdata->unk858) == 2)) {
        arg2->unk7A &= ~0x3;
    }
    ((void (*)(Object*, Player_Data*, f32)) objdata->unk3BC)(arg0, objdata, gUpdateRateF);
    dll_210_func_1BC0(arg0, objdata);
    return spC8;
}
