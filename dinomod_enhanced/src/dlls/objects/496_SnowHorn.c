#include "dlls/engine/6_amsfx.h"
#include "game/objects/object_id.h"
#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "dll.h"
#include "sys/main.h"
#include "sys/dll.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/objanim.h"
#include "sys/print.h"
#include "game/objects/object.h"
#include "dlls/objects/214_animobj.h"
#include "dlls/objects/227_tumbleweed.h"
#include "functions.h"
#include "types.h"

#include "recomp/dlls/objects/496_snowhorn_recomp.h"

extern s32 func_80032538(Object* self);

typedef struct {
/*000*/ s32 *unk0;
/*004*/ s16 unkRadius;
/*006*/ s16 unk6;
/*008*/ s16 unk8;
/*00A*/ s16 sleepTimer; //randomly-assigned value?
/*00c*/ u16 flags;
/*00e*/ u16 unkE; //yaw?
/*010*/ s32 unk10;
/*014*/ Vec3f playerPositionCopy;
/*020*/ f32 distanceFromPlayer;
/*024*/ s8 unk24;
/*025*/ s8 unk25;
/*026*/ s8 unk26;
/*027*/ s8 unk27;
/*028*/ Object* frostWeed;
/*02c*/ s16 unk2C;
/*02e*/ s16 unk2E;
/*030*/ s32 unk30;
/*034*/ s32 unk34;
/*038*/ s32 unk38;
/*03c*/ s32 unk3C;
/*040*/ s32 unk40;
/*044*/ s16* someAnimIDList;
/*048*/ f32* unk48;
/*04c*/ s32* chatSequenceList;
/*050*/ f32 unk50;
/*054*/ f32 unk54;
/*058*/ f32 walkSpeed; //has something to do with the struct at 0x60?
/*05C*/ s32 unk5C;
/*060*/ UnkCurvesStruct60 unk60;
/*0E0*/ s32 unkE0;
/*0E4*/ s32 unkE4;
/*0E8*/ s32 unkE8;
/*0EC*/ s32 unkEC;
/*0F0*/ s32 unkF0;
/*0F4*/ s32 unkF4;
/*0F8*/ s32 unkF8;
/*0FC*/ s32 unkFC;
/*100*/ s32 unk100;
/*104*/ s32 unk104;
/*108*/ s32 unk108;
/*10C*/ s32 unk10C;
/*110*/ s32 unk110;
/*114*/ s32 unk114;
/*118*/ s32 unk118;
/*11C*/ s32 unk11C;
/*110*/ s32 unk120;
/*114*/ s32 unk124;
/*118*/ s32 unk128;
/*11C*/ s32 unk12C;
/*110*/ s32 unk130;
/*114*/ s32 unk134;
/*118*/ s32 unk138;
/*11C*/ s32 unk13C;
/*110*/ s32 unk140;
/*114*/ s32 unk144;
/*118*/ s32 unk148;
/*11C*/ s32 unk14C;
/*110*/ s32 unk150;
/*114*/ s32 unk154;
/*118*/ s32 unk158;
/*11C*/ s32 unk15C;
/*110*/ s32 unk160;
/*114*/ s32 unk164;
/*118*/ s32 unk168;
/*11C*/ s32 unk16C;
/*110*/ Vec3f* unk170;
/*114*/ s32 unk174;
/*118*/ Vec3f unk178[8]; //position samples/deltas - maybe for walk-related calculus?
/*1d8*/ s8 unk1d8[0x208];
/*3e0*/ u32 unk3e0;
/*3e4*/ u32 unk3e4;
/*3e8*/ u32 unk3e8;
/*3ec*/ u32 unk3ec;
/*3f0*/ u32 unk3f0;
/*3f4*/ u32 unk3f4;
/*3f8*/ u32 unk3f8;
/*3fc*/ u32 unk3fc;
/*400*/ s8 lookAtUnk;
/*401*/ s8 unk401;
/*402*/ s8 unk402;
/*403*/ s8 unk403;
/*404*/ f32 copyPlayerX;
/*408*/ f32 copyPlayerY;
/*40c*/ f32 copyPlayerZ;
/*410*/ f32 unk410;
/*414*/ f32 unk414;
/*418*/ f32 unk418;
/*41C*/ f32 unk41C;
/*420*/ f32 unk420;
/*424*/ u8 unk424;
/*425*/ u8 unk425;
/*426*/ u8 unk426;
/*427*/ u8 unk427;
/*428*/ s8 garundaTe_weedsEaten;
/*429*/ s8 unk429;
/*42A*/ s8 unk42A;
/*42B*/ s8 unk42B;
} SnowHorn_Data;

typedef struct{
/*0x10*/ ObjSetup base;
/*0x18*/ s16 unkRadius;
/*0x1A*/ s16 unk1A;
/*0x1C*/ s8 rotation;
/*0x1D*/ s8 unk1D;
} SnowHorn_Setup;

static s32 getFrostWeedMaxOverride() {
    return recomp_get_config_u32("garunda_te_frostweeds_override");
}

static _Bool getFrostWeedTwigsConfigs() {
    return recomp_get_config_u32("garunda_te_frostweeds_accept_twigs");
}

// Adds config options for Garunda Te's FrostWeed quest
RECOMP_PATCH void dll_496_func_1D68(Object* self, SnowHorn_Data* state, SnowHorn_Setup* objsetup) {
    Object* frostWeed;
    s32 weeds;
    s8 FROSTWEED_MAX_OVERRIDE = getFrostWeedMaxOverride(); //@recomp
    s8 FROSTWEED_TWIGS_ACCEPTED = getFrostWeedTwigsConfigs(); //@recomp
    
    self->unk0xaf &= 0xFFF7;
    switch (state->flags) {
        case 0:
            //Calling out to the player periodically
            state->unk8 += delayByte;
            if (state->unk8 >= 0x3E9) {
                gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_1E2_Garunda_Te_Will_somebody_get_me_out_of_here, MAX_VOLUME, 0, 0, 0, 0);
                gDLL_22_Subtitles->vtbl->func_368(0xA);
                state->unk8 = 0;
            }
            if (self->unk0xaf & 4) {
                state->flags = 1;
            }
            break;
        case 1:
            if (func_80032538(self)) {
                gDLL_3_Animation->vtbl->func17(0, self, -1);
                state->flags = 2;
                main_set_bits(0x115, state->flags);
            }
            break;
        case 2:
            //Eating FrostWeeds
            if (func_80032538(self)) {
                gDLL_3_Animation->vtbl->func17(1, self, -1);
            }
            
            frostWeed = obj_get_nearest_type_to(4, self, 0);
            objsetup = (SnowHorn_Setup*)self->setup;
            if (frostWeed && (frostWeed->id == OBJ_Tumbleweed2 || 
                (FROSTWEED_TWIGS_ACCEPTED && frostWeed->id == OBJ_Tumbleweed2twig)) &&  //@recomp: option of accepting FrostWeed twigs as well
                vec3_distance_xz_squared(&self->positionMirror, &frostWeed->positionMirror) < objsetup->unkRadius * objsetup->unkRadius) {
                if (!((DLL_227_Tumbleweed*)frostWeed->dll)->vtbl->func4(frostWeed)) {
                    ((DLL_227_Tumbleweed*)(frostWeed->dll))->vtbl->func3(frostWeed, &state->playerPositionCopy);
                    state->frostWeed = frostWeed;
                    if (0){
                        state->garundaTe_weedsEaten = FROSTWEED_MAX_OVERRIDE;
                    }
                    state->garundaTe_weedsEaten++;
                    if (state->garundaTe_weedsEaten > FROSTWEED_MAX_OVERRIDE) {
                        state->garundaTe_weedsEaten = FROSTWEED_MAX_OVERRIDE;
                    }
                    main_set_bits(0x48B, state->garundaTe_weedsEaten);
                    state->flags = 3;
                }
            }
            break;
        case 3:
            if (vec3_distance_xz_squared(&state->playerPositionCopy, &state->frostWeed->positionMirror) < 6.25f) {
                state->flags = 4;
            }
            break;
        case 4:
            if (state->unk424 & 8) {
                weeds = state->garundaTe_weedsEaten;
                if (weeds >= FROSTWEED_MAX_OVERRIDE) {
                    main_set_bits(0x102, 1);
                    state->flags = 5;
                    main_set_bits(0x115, state->flags);
                    break;
                }
                if (weeds % 3 == 0) {
                    gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_74B_Garunda_Te_That_tastes_great_Hurry_up_boy, MAX_VOLUME, 0, 0, 0, 0);
                    gDLL_22_Subtitles->vtbl->func_368(2);
                }
                state->flags = 2;
            }
            break;
        case 5:
            if (func_80032538(self)) {
                if (state->unk425 % 2) {
                    gDLL_3_Animation->vtbl->func17(3, self, -1);
                } else {
                    gDLL_3_Animation->vtbl->func17(2, self, -1);
                }
                state->unk425 += 1;
            }
            break;
        case 6:
            //SpellStone activation
            if (func_80032538(self)) {
                gDLL_3_Animation->vtbl->func17(4, self, -1);
            } else if (gDLL_1_UI->vtbl->func7(0x123)) {
                main_set_bits(0x22B, 1);
                state->flags = 7;
                main_set_bits(0x115, state->flags);
            }
            break;
        case 7:
            self->unk0xaf |= 8;
            break;
    }
    
    if (state->flags >= 2 && state->flags < 5) {
        diPrintf("noweeds=%d/%d\n", state->garundaTe_weedsEaten, FROSTWEED_MAX_OVERRIDE);
    }
}
