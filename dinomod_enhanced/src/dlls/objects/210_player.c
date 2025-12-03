#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "dll_util.h"
#include "sidekick_util.h"

#include "common.h"
#include "sys/menu.h"
#include "sys/objanim.h"
#include "sys/objtype.h"
#include "sys/objmsg.h"

#include "dlls/objects/common/vehicle.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/214_animobj.h"
#include "dlls/objects/277_iceblast.h"

#include "recomp/dlls/objects/210_player_recomp.h"

extern s32 func_80025140(Object*, f32, f32, s32);
extern void func_8005B5B8(Object*, Object*, s32);

extern u8 _data_14[4];
extern s16 _data_18[2];
extern f32 _bss_1C;
extern s16 _data_24[2];
extern s16 _data_158[];
extern s16 _data_170[12];
extern s16 _data_188[4];
extern f32 _data_528;
extern s8 _data_52C;
extern u8 _data_530;
extern s16 _data_7C4[2];

extern void dll_210_func_1BC0(Object* arg0, Player_Data* arg1);
extern int dll_210_func_24FC(Object *player, ObjFSA_Data *fsa);
extern void dll_210_func_60A8(Object* arg0, s32 arg1, s32 arg2);
extern f32 dll_210_func_63F0(Player_Data* arg0, f32 updateRate);
extern void dll_210_func_6DD8(Object* player, Player_Data* data, s32 arg2);
extern void dll_210_func_9F1C(Object* arg0, s32 arg1);
extern s32 dll_210_func_A018(void);
extern void dll_210_func_A024(Object* player, ObjFSA_Data* objdata);
extern void dll_210_func_14B70(Object* arg0, ObjFSA_Data *arg1);
extern void dll_210_func_18DB0(Object* obj, ObjFSA_Data* fsa);
extern s32 dll_210_func_1A9D4(Object* player, s32* arg1, s32* arg2, s32* arg3, f32 arg4, f32 arg5);
extern void dll_210_func_1AAD8(Object* player, ObjFSA_Data *fsa);
void dll_210_add_health(Object* player, s32 amount);
extern s32 dll_210_func_1D2A8(Object* arg0, Object* arg1);
extern void dll_210_func_1D4E0(Object* arg0, s32 arg1);
extern void dll_210_func_1D8B8(Object* player);
extern void dll_210_func_1DB6C(Object* arg0, f32 arg1);
extern void dll_210_func_1DC48(Object* player);
extern Object *dll_210_func_1DD94(Object* player, s32 arg1);


extern s8 _bss_0;
extern s16 _bss_2;
extern ObjFSA_StateCallback _bss_58[81];
extern ObjFSA_StateCallback _bss_19C[1];
extern u8 _bss_1AA[0x2];
extern f32 _bss_1AC;
extern Object *_bss_210[4];
extern s16 _bss_220[2];
extern ObjFSA_StateCallback _bss_224[1];

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

/** Fix Ice Blast / Grenade Spell selection (originally by MusicalProgrammer) */
RECOMP_PATCH void dll_210_func_1DDC(Object* player, Player_Data* arg1, ObjFSA_Data* fsa) {
    Object* sp8C;
    s32 messageArgument;
    f32 temp_fv0;
    u32 message;
    f32 var_fs0;
    f32 var_fs1;
    s32 temp_v0;

    messageArgument = NULL;
    while (obj_recv_mesg(player, &message, &sp8C, (void **)&messageArgument)) {
        switch (message) {
        case 0x80002:
            if (messageArgument == BIT_Spell_Projectile || 
                messageArgument == BIT_Spell_Ice_Blast ||   //@recomp: allow Ice Blast to be picked
                messageArgument == BIT_Spell_Grenade        //@recomp: allow Grenade to be picked
            ) {
                if (dll_210_func_24FC(player, fsa) != 0) {
                    temp_v0 = gDLL_2_Camera->vtbl->func3();
                    if ((temp_v0 != 0x64) && (temp_v0 != 0x5E)) {
                        gDLL_2_Camera->vtbl->func6(0x64, 1, 0, 0, NULL, 0x3C, 0xFF);
                        gDLL_18_objfsa->vtbl->set_anim_state(player, fsa, 0x3A);
                        arg1->flags |= 0x400000;
                    }
                } else {
                    gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_912_Object_Refused, MAX_VOLUME, NULL, NULL, 0, NULL);
                    break; //@recomp;
                }
            }

            //@recomp: change conditions for reaching here
            dll_210_func_6DD8(player, arg1, messageArgument);
            break;
        case 0xE0000:
            if (sp8C == fsa->target) {
                fsa->target = 0;
                fsa->unk33D = 0;
                gDLL_2_Camera->vtbl->func17.withOneArg(0);
            }
            break;
        case 0x60003:
            var_fs0 = sp8C->srt.transl.x - player->srt.transl.x;
            var_fs1 = sp8C->srt.transl.z - player->srt.transl.z;
            temp_fv0 = sqrtf(SQ(var_fs0) + SQ(var_fs1));
            if (temp_fv0 > 1.0f) {
                var_fs0 /= temp_fv0;
                var_fs1 /= temp_fv0;
            }
            player->speed.y = 2.5f;
            player->speed.x = var_fs0 * 2.5f;
            player->speed.z = var_fs1 * 2.5f;
            gDLL_18_objfsa->vtbl->set_anim_state(player, fsa, 0x4D);
            dll_210_add_health(player, -messageArgument);
            if (arg1->unk868 != NULL) {
                arg1->unk868->unkE0 = 0;
                arg1->unk868 = NULL;
            }
            break;
        case 0x60004:
            var_fs0 = sp8C->srt.transl.x - player->srt.transl.x;
            var_fs1 = sp8C->srt.transl.z - player->srt.transl.z;
            temp_fv0 = sqrtf((var_fs0 * var_fs0) + (var_fs1 * var_fs1));
            if (temp_fv0 > 1.0f) {
                var_fs0 /= temp_fv0;
                var_fs1 /= temp_fv0;
            }
            player->speed.y = 2.5f;
            player->speed.x = -var_fs0 * 2.5f;
            player->speed.z = -var_fs1 * 2.5f;
            gDLL_18_objfsa->vtbl->set_anim_state(player, fsa, 0x4D);
            dll_210_add_health(player, -messageArgument);
            if (arg1->unk868 != NULL) {
                arg1->unk868->unkE0 = 0;
                arg1->unk868 = NULL;
            }
            break;
        case 0x60005:
            var_fs0 = sp8C->srt.transl.x - player->srt.transl.x;
            var_fs1 = sp8C->srt.transl.z - player->srt.transl.z;
            temp_fv0 = sqrtf((var_fs0 * var_fs0) + (var_fs1 * var_fs1));
            if (temp_fv0 > 1.0f) {
                var_fs0 /= temp_fv0;
                var_fs1 /= temp_fv0;
            }
            player->speed.y = 2.5f;
            player->speed.x = -var_fs0 * 2.5f;
            player->speed.z = -var_fs1 * 2.5f;
            gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_Krystal_Hurt_Ough, MAX_VOLUME, NULL, NULL, 0, NULL);
            gDLL_18_objfsa->vtbl->set_anim_state(player, fsa, 0x4D);
            func_80023D30(player, 0x450, 0.0f, 0);
            dll_210_add_health(player, -messageArgument);
            if (arg1->unk868 != NULL) {
                arg1->unk868->unkE0 = 0;
                arg1->unk868 = NULL;
            }
            break;
        case 0x7000A:
            if (messageArgument > 0) {
                if (main_get_bits(messageArgument) != 0) {
                    obj_send_mesg(sp8C, 0x7000BU, player, NULL);
                    if (fsa->animState != 0x2B) {
                        gDLL_18_objfsa->vtbl->set_anim_state(player, fsa, 0x2B);
                    }
                } else {
                    main_set_bits(messageArgument, 1U);
                    if (fsa->animState != 0x2A) {
                        gDLL_18_objfsa->vtbl->set_anim_state(player, fsa, 0x2A);
                    }
                }
            } else if (fsa->animState != 0x2A) {
                gDLL_18_objfsa->vtbl->set_anim_state(player, fsa, 0x2A);
            }
            arg1->unk708 = sp8C;
            arg1->unk70C = messageArgument & 0xFFFF;
            if (arg1->unk708->unk64 != NULL) {
                arg1->unk708->unk64->flags = 0x20000;
            }
            arg1->unk8A9 = 1;
            break;
        case 0x100008:
            arg1->unk870 = 1;
            if (arg1->unk868 == NULL) {
                arg1->unk868 = sp8C;
                arg1->unk86C = (messageArgument >> 0x10) / 10.0f;
                gDLL_18_objfsa->vtbl->set_anim_state(player, fsa, 5);
                arg1->unk8A9 = 1;
            }
            break;
        case 0x100010:
            arg1->unk870 = 1;
            if (arg1->unk868 == NULL) {
                arg1->unk868 = sp8C;
                arg1->unk86C = messageArgument >> 0x10;
                gDLL_18_objfsa->vtbl->set_anim_state(player, fsa, 5);
                arg1->unk8A9 = 1;
            }
            break;
        }
    }
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

/** Stop spells from unequipping themselves whenever you do an action (originally by MusicalProgrammer) */
void dll_210_func_64B4(Object* player, Player_Data* arg1, f32 arg2) {
    Object* temp_s2;
    s32 temp_s3;
    s32 var_s4;
    s32 var_v0;
    f32 f20;
    ModelInstance *modelInstance;
    AnimState *animState;

    temp_s2 = player->linkedObject;
    var_v0 = 0;
    f20 = 0.01f;
    do {
        var_s4 = 0;
        switch (arg1->unk878) {
        case 2:
            if (var_v0 != 0) {
                func_80024E50(player, player->curModAnimId, player->animProgress, 0);
                func_80024E50(player, 0x8A, 0.0f, 0);
            }
            temp_s3 = func_80025140(player, f20, arg2, 0);
            if (player->animProgressLayered > 0.24f) {
                arg1->unk8A8 = 1;
            }
            if (player->animProgressLayered > 0.59f) {
                arg1->unk8A8 = 2;
            }
            if ((temp_s2 != NULL) && (player->animProgressLayered > 0.7f) && (temp_s2->group == GROUP_UNK48)) {
                ((DLL_Unknown*)temp_s2->dll)->vtbl->func[7].withOneS32OneF32((s32)temp_s2, 0.15f);
            }
            if (temp_s3 != 0) {
                arg1->unk878 = 3;
                var_s4 = 1;
            }
            break;
        case 13:
            if ((temp_s2 != NULL) && (temp_s2->group == GROUP_UNK48)) {
                ((DLL_Unknown*)temp_s2->dll)->vtbl->func[7].withOneS32OneF32((s32)temp_s2, 1.0f);
            }
            arg1->unk8A8 = 2;
            arg1->unk878 = 0;
            break;
        case 1:
            if (var_v0 != 0) {
                func_80024E50(player, player->curModAnimId, player->animProgress, 0);
                func_80024E50(player, 0x8A, 0.99f, 0);
            }
            temp_s3 = func_80025140(player, -f20, arg2, 0);
            if (player->animProgressLayered < 0.59f) {
                arg1->unk8A8 = 1;
            }
            if (player->animProgressLayered < 0.24f) {
                arg1->unk8A8 = 0;
            }
            if ((temp_s2 != NULL) && (player->animProgressLayered < 0.7f) && (temp_s2->group == GROUP_UNK48)) {
                ((DLL_Unknown*)temp_s2->dll)->vtbl->func[8].withOneArg((s32)temp_s2);
            }
            if (temp_s3 != 0) {
                arg1->unk87C = -1;
                arg1->unk878 = 3;
                var_s4 = 1;
            }
            break;
        case 14:
            if (temp_s2->group == GROUP_UNK48) {
                ((DLL_Unknown*)temp_s2->dll)->vtbl->func[8].withOneArg((s32)temp_s2);
            }
            // arg1->unk87C = -1; //@recomp: don't unequip spells
            arg1->unk8A8 = 0;
            arg1->unk878 = 0;
            break;
        case 3:
            if (var_v0 != 0) {
                func_80024E50(player, player->curModAnimId, player->animProgress, 0);
            } else {
                func_80025140(player, player->animProgress - player->animProgressLayered, 1.0f, 0);
            }
            modelInstance = player->modelInsts[player->modelInstIdx];
            animState = modelInstance->animState1;
            if (animState->unk58[0] == 0) {
                player->curModAnimIdLayered = -1;
                arg1->unk878 = 0;
            }
            break;
        default:
            if (player->linkedObject != NULL) {
                var_v0 = arg1->unk8A9;
                if (arg1->unk8A8 != 0) {
                    if (var_v0 == 0) {
                        arg1->unk878 = 1;
                        var_s4 = 1;
                        break;
                    }
                    if (var_v0 == 1) {
                        arg1->unk878 = 0xE;
                        var_s4 = 1;
                    }
                    break;
                }

                if (var_v0 == 2) {
                    arg1->unk878 = 2;
                    var_s4 = 1;
                    break;
                }
                if (var_v0 == 4) {
                    arg1->unk878 = 0xD;
                    var_s4 = 1;
                }
            }
            break;
        }
        var_v0 = var_s4;
    } while (var_s4 != 0);
}

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

/** Fixed crash if you warp while spamming attack (originally by MusicalProgrammer) */
RECOMP_PATCH s32 dll_210_func_18630(Object* self, ObjFSA_Data* fsa, f32 arg2) {
    u8 sp47;
    Object* weapon;
    Player_Data* objData;

    //@recomp: return early
    if (self->linkedObject == NULL){
        return 0;
    }

    objData = self->data;
    weapon = self->linkedObject;
    fsa->unk341 = 1;
    if (fsa->enteredAnimState == 0) {
        sp47 = 0;
        if (fsa->animTickDelta > 0.0f) {
            if (!(fsa->unk34A & 1)) {
                if (objData->unk3B4[objData->unk8A1].unk24 < self->animProgress) {
                    gDLL_6_AMSFX->vtbl->play_sound(self, objData->unk3B4[objData->unk8A1].unk2C, MAX_VOLUME, NULL, NULL, 0, NULL);
                    fsa->unk34A |= 1;
                }
            }
            if (!(fsa->unk34A & 2) && ((&objData->unk3B4[objData->unk8A1])->unk28 < self->animProgress)) {
                gDLL_6_AMSFX->vtbl->play_sound(self, objData->unk3B8[rand_next(0, 2)], MAX_VOLUME, NULL, NULL, 0, NULL);
                fsa->unk34A |= 2;
            }
        }
        if (objData->unk3B4[objData->unk8A1].unk9 >= 0) {
            if ((objData->unk3B4[objData->unk8A1].unk18 < self->animProgress) || ((fsa->animTickDelta < 0.0f) && (self->animProgress < _bss_1C))) {
                fsa->unk33E |= 2;
            }
            if (objData->unk3B4[objData->unk8A1].unk10 < self->animProgress) {
                fsa->unk33E |= 1;
            }
            if (objData->unk3B4[objData->unk8A1].unk14 < self->animProgress) {
                fsa->unk33E &= ~1;
            }
            if (fsa->unk310 & 0x8000) {
                if (fsa->unk33E & 1) {
                    fsa->unk33E |= 4;
                }
            }
            if ((fsa->unk33E & 4) && (fsa->unk33E & 2)) {
                objData->unk8A1 = (u8) (&objData->unk3B4[objData->unk8A1])->unk9;
                sp47 = 1;
            }
        }
    } else {
        if (weapon->group == GROUP_UNK48) {
            ((DLL_Unknown *)weapon->dll)->vtbl->func[11].withOneArg((s32)weapon);
        }
        sp47 = 1;
        objData->flags &= ~0x40;
        self->objhitInfo->unk61 = 0;
        dll_210_func_A024(self, fsa);
        fsa->animExitAction = dll_210_func_18DB0;
    }
    if (sp47 != 0) {
        self->unk5C = &objData->unk3B4[objData->unk8A1].unk34;
        if (objData->unk3B4[objData->unk8A1].unk0 != self->curModAnimId && objData->unk3B4[objData->unk8A1].unk2 != self->curModAnimId ) {
            func_80023D30(self, fsa->target != NULL ? objData->unk3B4[objData->unk8A1].unk0 : objData->unk3B4[objData->unk8A1].unk2, 0.0f, 0U);
        }
        fsa->unk33E &= ~0xEF;
        fsa->animTickDelta = (&objData->unk3B4[objData->unk8A1])->unkC;
        fsa->unk34A = 0;
        fsa->unk27C = 0.0f;
        if (fsa->target == NULL) {
            if (fsa->unk290 > 0.3f) {
                self->srt.yaw += fsa->unk32A * 0xB6;
                fsa->unk328 = 0;
                fsa->unk32A = 0;
            }
        } else {
            gDLL_18_objfsa->vtbl->func11(self, fsa, arg2, 2);
        }
        if (self->objhitInfo != NULL) {
            self->objhitInfo->unk61 = 0;
        }
        if (weapon->group == GROUP_UNK48) {
            ((DLL_Unknown *)weapon->dll)->vtbl->func[12].withTwoArgs((s32)weapon, 1);
            ((DLL_Unknown *)weapon->dll)->vtbl->func[13].withTwoArgs((s32)weapon, (&objData->unk3B4[objData->unk8A1])->unk30);
            ((DLL_Unknown *)weapon->dll)->vtbl->func[18].withThreeArgsCustom2(weapon, (&objData->unk3B4[objData->unk8A1])->unk1C, (&objData->unk3B4[objData->unk8A1])->unk20);
        }
    }
    self->objhitInfo->unk5F = (&objData->unk3B4[objData->unk8A1])->unk4;
    self->objhitInfo->unk60 = (&objData->unk3B4[objData->unk8A1])->unk8;
    gDLL_18_objfsa->vtbl->func7(self, fsa, arg2, 1);
    if (self->animProgress > 0.99f) {
        self->objhitInfo->unk61 = 0;
        if (fsa->target != NULL) {
            gDLL_18_objfsa->vtbl->func17(self, fsa);
            return 0x36;
        }
        return 2;
    }
    if ((self->animProgress > 0.7f) && ((&objData->unk3B4[objData->unk8A1])->unk9 < 0) && (fsa->unk310 & 0x8000)) {
        self->objhitInfo->unk61 = 0;
        if (fsa->target != NULL) {
            gDLL_18_objfsa->vtbl->func11(self, fsa, arg2, 2);
            return 0x3C;
        }
        if (fsa->unk290 > 0.3f) {
            self->srt.yaw += fsa->unk32A * 0xB6;
            fsa->unk328 = 0;
            fsa->unk32A = 0;
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

/* Fix mount logic when multiple vehicles are loaded. (Original patch by MusicalProgrammer) */
// TODO: replace with a cleaner match from the decomp, when we have one
RECOMP_PATCH int dll_210_func_4910(Object* arg0, Object* arg1, AnimObj_Data* arg2, s8 arg3) {
    AnimObj_Setup* animSetup;
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
    animSetup = (AnimObj_Setup*)arg1->setup;
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
            temp_v0_6 = (Object *)gDLL_2_Camera->vtbl->func15();
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
            objdata->unk354.headStartAngle = func_80034804(arg0, 0)[1];
            objdata->unk354.headGoalAngle = arg2->yawDiff;
            objdata->unk378.headStartAngle = 0;
            objdata->unk378.headGoalAngle = arg2->pitchDiff;
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

static s16 iceblast_timer = 0;

/** - Don't default to Projectile Spell (originally by MusicalProgrammer) 
  * - Optionally reduce the magic depletion rate of the Ice Blast Spell 
  */
RECOMP_PATCH s32 dll_210_func_18EAC(Object* player, ObjFSA_Data* fsa, f32 arg2) {
    static f32 _bss_20;
    static f32 _bss_24;
    static f32 _bss_28;
    static f32 _bss_2C;
    static f32 _bss_30;
    static s8 _bss_34;
    static DLL_Unknown *_data_7C0 = 0;
    s32 sp9C;
    s32 magic;
    Object* weapon;
    f32 temp_fa1;
    f32 sp8C;
    f32 var_fa0;
    f32 var_ft4;
    f32 sp80;
    SRT fxTransform;
    f32 var_fv0;
    f32 var_fv1;
    s32 temp_v0;
    s8 temp_v0_4;
    s16 temp_ft5;
    u32 temp_a0_4;
    s32 i;
    Player_Data* temp_s1;
    //@recomp: Ice Blast config
    int reduceIceBlastCost = recomp_get_config_u32("iceblast_cost");

    temp_s1 = player->data;
    if (fsa->enteredAnimState != 0) {
        if (fsa->target != NULL) {
            func_80023D30(player, 0x43E, 0.0f, 0);
            dll_210_func_6DD8(player, temp_s1, *_bss_220);
            fsa->animTickDelta = 0.015f;
        } else if (temp_s1->unk8A8 != 0) {
            func_80023D30(player, 0x43D, 0.0f, 0);
            fsa->animTickDelta = 0.04f;
        } else {
            func_80023D30(player, 0x448, 0.0f, 0);
            if (player->id == 0x1F) {
                fsa->animTickDelta = 0.035f;
            } else {
                fsa->animTickDelta = 0.024f;
            }
        }
        fsa->animExitAction = dll_210_func_1AAD8;
        dll_210_func_A024(player, fsa);
        temp_s1->unk830 = 0.0f;
        temp_s1->unk82C = 0.0f;
        weapon = player->linkedObject;
        _bss_34 = 0;
        _bss_28 = _bss_30 = _bss_20 = _bss_2C = 0.0f;
        ((DLL_Unknown*)weapon->dll)->vtbl->func[14].withTwoArgs((s32)weapon, 0);
    }
    switch (player->curModAnimId) {
    case 0x43D:
        if (fsa->unk33A != 0) {
            func_80023D30(player, 0x43E, 0.0f, 0);
            // dll_210_func_6DD8(player, temp_s1, 0x2D); //@recomp: don't default to Projectile Spell
            gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_BA4_Spell_Aim_Hum_Loop, MAX_VOLUME, &temp_s1->unk848, NULL, 0, NULL);
            fsa->animTickDelta = 0.015f;
        }
        break;
    case 0x449:
        if (fsa->unk33A != 0) {
            if (fsa->target != NULL) {
                return 0x36;
            }
            return -1;
        }
        break;
    case 0x448:
        if ((player->animProgress > 0.4f) && (temp_s1->unk8A8 == 0)) {
            gDLL_6_AMSFX->vtbl->play_sound(player, temp_s1->unk3B8[4], MAX_VOLUME, NULL, NULL, 0, NULL);
            temp_s1->unk8A8 = 2U;
            temp_s1->unk8A9 = 2;
            weapon = player->linkedObject;
            ((DLL_Unknown*)weapon->dll)->vtbl->func[7].withOneS32OneF32((s32)weapon, 0.15f);
        }
        if (fsa->unk33A != 0) {
            func_80023D30(player, 0x43E, 0.0f, 0);
            // dll_210_func_6DD8(player, temp_s1, 0x2D); //@recomp: don't default to Projectile Spell
            gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_BA4_Spell_Aim_Hum_Loop, MAX_VOLUME, &temp_s1->unk848, NULL, 0, NULL);
            fsa->animTickDelta = 0.015f;
        }
        break;
    case 0x43E:
        if (fsa->target != NULL) {
            if (fsa->target->unk74 != NULL) {
                var_fv0 = ((f32 *)fsa->target->unk74)[3] - player->srt.transl.x;
                sp8C = ((f32 *)fsa->target->unk74)[4] - player->srt.transl.y;
                var_fv1 = ((f32 *)fsa->target->unk74)[5] - player->srt.transl.z;
            } else {
                var_fv0 = fsa->target->srt.transl.x - player->srt.transl.x;
                sp8C = fsa->target->srt.transl.y - player->srt.transl.y;
                var_fv1 = fsa->target->srt.transl.z - player->srt.transl.z;
            }
            var_fv0 = (s16) (arctan2_f(sp8C, sqrtf(SQ(var_fv0) + SQ(var_fv1))) - 0x800) / 5461.0f;
            // var_fv0 = var_fv0; // required to match?
            if (var_fv0 < -1.0f) {
                var_fv0 = -1.0f;
            } else if (var_fv0 > 1.0f) {
                var_fv0 = 1.0f;
            }
            var_fv1 = _bss_28;
            var_fv0 -= var_fv1;

            var_fv1 += (var_fv0) * 0.01f * arg2;
            _bss_28 = var_fv1;
        } else {
            if (*_bss_220 == 0x777) {
                var_fa0 = fsa->unk284 / 50.0f;
                if (var_fa0 < -1.45f) {
                    var_fa0 = -1.45f;
                } else if (var_fa0 > 1.45f) {
                    var_fa0 = 1.45f;
                }
            } else {
                var_fa0 = fsa->unk284 / 60.0f;
                if (var_fa0 < -1.0f) {
                    var_fa0 = -1.0f;
                } else if (var_fa0 > 1.0f) {
                    var_fa0 = 1.0f;
                }
            }
            var_fa0 -= temp_s1->unk830;
            temp_s1->unk830 += var_fa0 * 0.1f * arg2;
            var_fv1 = fsa->unk288 / 60.0f;
            if (var_fv1 < -1.0f) {
                var_fv1 = -1.0f;
            } else if (var_fv1 > 1.0f) {
                var_fv1 = 1.0f;
            }
            var_fv1 -= temp_s1->unk82C;
            temp_s1->unk82C += var_fv1 * 0.1f * arg2;
            if (temp_s1->unk82C > 0.0f) {
                var_fv1 = temp_s1->unk82C - 0.75f;
                if (var_fv1 < 0.0f) {
                    var_fv1 = 0.0f;
                }
            } else {
                var_fv1 = temp_s1->unk82C + 0.75f;
                if (var_fv1 > 0.0f) {
                    var_fv1 = 0.0f;
                }
            }
            player->srt.yaw = (player->srt.yaw + (var_fv1 * -1000.0f));
        }
        magic = (s32) temp_s1->stats->magic;
        weapon = player->linkedObject;
        fxTransform.scale = ((DLL_Unknown*)weapon->dll)->vtbl->func[16].withOneArgS32((s32)weapon);
        if ((temp_s1->unk766 & 0x8000) && (magic == 0)) {
            gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_912_Object_Refused, MAX_VOLUME, NULL, NULL, 0, NULL);
        }
        if ((temp_s1->unk764 & 0x8000) && (magic != 0)) {
            // @fake
            if (_bss_34) {}
            if (_bss_34 != 0) {
                _bss_2C -= arg2;
                if (_bss_2C < 400.0f) {
                    _bss_2C = 400.0f;
                    _bss_34 ^= 1;
                }
            } else {
                _bss_2C += arg2;
                if (_bss_2C > 420.0f) {
                    _bss_2C = 420.0f;
                    _bss_34 ^= 1;
                }
            }
            if (*_bss_220 != 0x5CE) {
                if ((_bss_2C >= 10.0f) && (temp_s1->unk848 == 0)) {
                    gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_6AB_Electric_Arcing_Loop, 1U, &temp_s1->unk848, NULL, 0, NULL);
                    gDLL_6_AMSFX->vtbl->func_954(temp_s1->unk848, 0.5f);
                } else if (_bss_2C < 10.0f) {
                    if (temp_s1->unk848 != 0) {
                        gDLL_6_AMSFX->vtbl->func_A1C(temp_s1->unk848);
                        temp_s1->unk848 = 0U;
                    }
                }
                if (_bss_2C >= 420.0f) {
                    _bss_30 -= arg2;
                    if (_bss_30 <= 0.0f) {
                        gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_6AD_Electric_Crackle, rand_next(0x20, 0x60), NULL, NULL, 0, NULL);
                        _bss_30 = rand_next(0x4B0, 0x708);
                    }
                } else if (_bss_2C < 0.0f) {
                    _bss_2C = 0.0f;
                }
                if (fxTransform.scale == 0.0f) {
                    if (_bss_2C > 120.0f) {
                        if (magic >= 3) {
                            weapon = player->linkedObject;
                            ((DLL_Unknown*)weapon->dll)->vtbl->func[14].withTwoArgs((s32)weapon, 1);
                            gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_6AC_Electric_Zap, 0x60U, NULL, NULL, 0, NULL);
                        } else {
                            _bss_2C = 120.0f;
                        }
                    }
                } else if (fxTransform.scale == 1.0f) {
                    fsa->animTickDelta = 0.02f;
                    if (_bss_2C > 300.0f) {
                        if (magic >= 9) {
                            weapon = player->linkedObject;
                            ((DLL_Unknown*)weapon->dll)->vtbl->func[14].withTwoArgs((s32)weapon, 2);
                            gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_6AC_Electric_Zap, MAX_VOLUME, NULL, NULL, 0, NULL);
                        } else {
                            _bss_2C = 300.0f;
                        }
                    }
                } else {
                    fsa->animTickDelta = 0.027f;
                }
                if (temp_s1->unk848 != 0) {
                    sp80 = ((_bss_2C / 420.0f) * 0.5f) + 0.5f;
                    if (sp80 > 1.0f) {
                        sp80 = 1.0f;
                    }
                    if (*_bss_220 == 0x777) {
                        temp_ft5 = ((sp80 - 0.5f) * 127.0f);
                        diPrintf("throwdist %d\n\0error\n\0 Light Created \0 WARNING: Screen Overlay already used \n\0 WARNING: Screen Overlay already Killed \n", temp_ft5);
                        weapon = player->linkedObject;
                        ((DLL_Unknown*)weapon->dll)->vtbl->func[14].withTwoArgs((s32)weapon, temp_ft5);
                    }
                    gDLL_6_AMSFX->vtbl->func_954(temp_s1->unk848, sp80);
                    gDLL_6_AMSFX->vtbl->func_860(temp_s1->unk848, 127.0f * sp80);
                }
                _bss_20 -= arg2;
                if (_bss_20 <= 0.0f) {
                    gDLL_17_partfx->vtbl->spawn(player->linkedObject, PARTICLE_3EC, &fxTransform, PARTFXFLAG_2, -1, NULL);
                    if (fxTransform.scale == 0.0f) {
                        _bss_20 = 8.0f;
                    } else if (fxTransform.scale == 1.0f) {
                        _bss_20 = 5.0f;
                    } else {
                        _bss_20 = 3.0f;
                    }
                }
            } else {
                if (*_bss_220 == 0x5CE) {
                    //Using Ice Blast Spell
                    if (temp_s1->unk848 == 0) {
                        gDLL_6_AMSFX->vtbl->play_sound(player->linkedObject, SOUND_95A_Frigid_Air_Loop, 1, &temp_s1->unk848, NULL, 0, NULL);
                    }
                    if (*_bss_210 == 0) {
                        dll_210_func_1DC48(player);
                    }
                    if (_bss_210[3] == 0) {
                        _bss_210[3] = dll_210_func_1DD94(player, 0x249);
                    }

                    //@recomp: handle magic drain here instead of in the Iceblast control function
                    if (reduceIceBlastCost){
                        //Check if any Ice Blast Objects exist
                        if (_bss_210[0] || _bss_210[1] || _bss_210[2]){
                            iceblast_timer -= gUpdateRate;
                            if (iceblast_timer < 0){
                                iceblast_timer = 60; //this seems to run twice per tick, so doubled (for -1 per half-second drain)
                                ((DLL_210_Player*)player->dll)->vtbl->add_magic(player, -1);
                            }
                        }
                    }

                    _bss_24 -= arg2;
                    if (_bss_24 <= 0.0f) {
                        if (_data_7C0 == 0) {
                            _data_7C0 = dll_load_deferred(0x1048U, 1U);
                        }
                        if (_data_7C0 != 0) {
                            _data_7C0->vtbl->func[0].withSixArgs((s32)player->linkedObject, player->id == 0, 0, 0x10404, -1, 0);
                        }
                        _bss_24 = 35.0f;
                    }
                    if (*_bss_1AA == 0) {
                        var_fv0 = (*_data_7C4 - player->linkedObject->srt.pitch);
                        var_fv0 /= 3000.0f;
                        sp80 = 0.7f + var_fv0;
                        gDLL_6_AMSFX->vtbl->func_860(temp_s1->unk848, (127.0f * sp80));
                        if (sp80 > 0.775f) {
                            sp80 = 0.775f;
                        } else if (sp80 < 0.625f) {
                            sp80 = 0.625f;
                        }
                        gDLL_6_AMSFX->vtbl->func_954(temp_s1->unk848, sp80);
                        *_data_7C4 = player->linkedObject->srt.pitch;
                    }
                    if (player->id == 0) {
                        fxTransform.pitch = -0x8000;
                    }
                    if (rand_next(0, 2) == 0) {
                        gDLL_17_partfx->vtbl->spawn(player->linkedObject, PARTICLE_525, &fxTransform, PARTFXFLAG_1, -1, NULL);
                    }
                    if (rand_next(0, 2) == 0) {
                        gDLL_17_partfx->vtbl->spawn(player->linkedObject, PARTICLE_526, &fxTransform, PARTFXFLAG_1, -1, NULL);
                    }
                    if (rand_next(0, 2) == 0) {
                        gDLL_17_partfx->vtbl->spawn(player->linkedObject, PARTICLE_529, &fxTransform, PARTFXFLAG_1, -1, NULL);
                    }
                }
            }
        }
        if (temp_s1->unk830 > 0.0f) {
            func_80025540(player, 0x441, temp_s1->unk830 * 1023.0f);
        } else {
            func_80025540(player, 0x440, -temp_s1->unk830 * 1023.0f);
        }
        func_80034804(player, 9)[1] = temp_s1->unk82C * -10240.0f;
        temp_s1->flags &= ~0x400;
        if ((fsa->target == NULL) && (dll_210_func_1A9D4(player, &temp_s1->aimX, &temp_s1->aimY, &temp_s1->aimZ, temp_s1->unk82C, temp_s1->unk830) != 0)) {
            temp_s1->flags |= 0x400;
        }
        if ((((fsa->target != NULL) && !(temp_s1->unk764 & 0x8000)) || ((fsa->target == NULL) && (temp_s1->unk768 & 0x8000))) && (magic != 0) && (*_bss_220 != 0x5CE)) {
            gDLL_13_Expgfx->vtbl->func4(player->linkedObject);
            fxTransform.transl.x = player->linkedObject->srt.transl.x;
            fxTransform.transl.y = player->linkedObject->srt.transl.y;
            fxTransform.transl.z = player->linkedObject->srt.transl.z;
            for (sp9C = 0; sp9C < 0x14; sp9C++) {
                gDLL_17_partfx->vtbl->spawn(player->linkedObject, PARTICLE_3ED, &fxTransform, PARTFXFLAG_200000 | PARTFXFLAG_1, -1, NULL);
            }
            if (fxTransform.scale == 0.0f) {
                magic = 1;
            } else if (fxTransform.scale == 1.0f) {
                magic = 3;
            } else {
                magic = 9;
            }
            dll_210_add_magic(player, -magic);
            _bss_224[0](player, fsa, temp_s1->unk830);
            fsa->animTickDelta = 0.02f;
            func_80023D30(player, 0x43F, 0.0f, 0);
            if (temp_s1->unk830 > 0.0f) {
                func_80025540(player, 0x44B, temp_s1->unk830 * 1023.0f);
            } else {
                func_80025540(player, 0x44A, -temp_s1->unk830 * 1023.0f);
            }
            if (fxTransform.scale < 2.0f) {
                _bss_34 = 0;
            } else {
                _bss_34 = 2;
            }
            if (temp_s1->unk848 != 0) {
                gDLL_6_AMSFX->vtbl->func_A1C(temp_s1->unk848);
                temp_s1->unk848 = 0U;
            }
            weapon = player->linkedObject;
            _bss_30 = 0.0f;
            _bss_20 = 0.0f;
            _bss_2C = 0.0f;
            ((DLL_Unknown*)weapon->dll)->vtbl->func[14].withTwoArgs((s32)weapon, 0);
            temp_s1->flags &= ~0x400;
        } else if ((temp_s1->unk768 & 0x8000) && (magic != 0) && (*_bss_220 == 0x5CE)) {
            if (_data_7C0 != 0) {
                dll_unload(_data_7C0);
            }
            _data_7C0 = 0;
            if (temp_s1->unk848 != 0) {
                gDLL_6_AMSFX->vtbl->func_A1C(temp_s1->unk848);
                temp_s1->unk848 = 0U;
            }
            for (i = 0; i < 4; i++) {
                weapon = _bss_210[i];
                if (weapon != NULL) {
                    obj_destroy_object(weapon);
                    _bss_210[i] = 0;
                }
            }
        }
        break;
    case 0x43F:
        if (temp_s1->unk830 > 0.0f) {
            func_80025540(player, 0x44B, temp_s1->unk830 * 1023.0f);
        } else {
            func_80025540(player, 0x44A, -temp_s1->unk830 * 1023.0f);
        }
        temp_s1->flags &= ~0x400;
        if ((fsa->target == NULL) && (dll_210_func_1A9D4(player, &temp_s1->aimX, &temp_s1->aimY, &temp_s1->aimZ, temp_s1->unk82C, temp_s1->unk830) != 0)) {
            temp_s1->flags |= 0x400;
        }
        if (fsa->unk33A != 0) {
            _bss_34--;
            if (_bss_34 < 0 || *_bss_220 == 0x777) {
                _bss_34 = 0;
                if (fsa->target != NULL) {
                    return 0x36;
                }
                func_80023D30(player, 0x43E, 0.0f, 0);
                if (temp_s1->unk830 > 0.0f) {
                    func_80025540(player, 0x441, temp_s1->unk830 * 1023.0f);
                } else {
                    func_80025540(player, 0x440, -temp_s1->unk830 * 1023.0f);
                }
                fsa->animTickDelta = 0.015f;
                gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_BA4_Spell_Aim_Hum_Loop, MAX_VOLUME, &temp_s1->unk848, NULL, 0, NULL);
            } else {
                fxTransform.transl.x = player->linkedObject->srt.transl.x;
                fxTransform.transl.y = player->linkedObject->srt.transl.y;
                fxTransform.transl.z = player->linkedObject->srt.transl.z;
                for (sp9C = 0; sp9C < 0x14; sp9C++) {
                    gDLL_17_partfx->vtbl->spawn(player->linkedObject, PARTICLE_3ED, &fxTransform, PARTFXFLAG_200000 | PARTFXFLAG_1, -1, NULL);
                }
                _bss_224[0](player, fsa, temp_s1->unk830);
                fsa->animTickDelta = 0.02f;
                func_80023D30(player, 0x43F, 0.0f, 0);
            }
        }
        break;
    default:
        break;
    }

    if (fsa->target == NULL && ((temp_s1->flags & 0x400000 && temp_s1->unk766 & 0x4000) || !(temp_s1->flags & 0x401000)) && player->curModAnimId != 0x449) {
        func_80023D30(player, 0x449, 0.0f, 0);
        fsa->animTickDelta = 0.04f;
        temp_v0 = gDLL_2_Camera->vtbl->func3();
        if ((temp_v0 != 0x54) && (temp_v0 != 0x5E)) {
            gDLL_2_Camera->vtbl->func6(0x54, 0, 1, 0, NULL, 0x3C, 0xFE);
        }
    }
    if (fsa->target != NULL) {
        gDLL_18_objfsa->vtbl->func11(player, fsa, arg2, 4);
    }
    return 0;
}
