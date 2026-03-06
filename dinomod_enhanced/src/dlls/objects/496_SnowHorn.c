#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "common.h"
#include "sys/objtype.h"
#include "dlls/objects/214_animobj.h"
#include "dlls/objects/227_tumbleweed.h"

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
/*060*/ UnkCurvesStruct unk60;
/*168*/ s32 unk168;
/*16C*/ s32 unk16C;
/*170*/ DLL27_Data unk170;
/*3d0*/ s8 _unk3D0[0x3E0-0x3D0];
/*3e0*/ u32 unk3e0;
/*3e4*/ u32 unk3e4;
/*3e8*/ u32 unk3e8;
/*3ec*/ u32 unk3ec;
/*3f0*/ u32 unk3f0;
/*3f4*/ u32 unk3f4;
/*3f8*/ u32 unk3f8;
/*3fc*/ u32 unk3fc;
/*400*/ HeadAnimation lookAtUnk;
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
RECOMP_PATCH void dll_496_func_1D68(Object* self, SnowHorn_Data* objdata, SnowHorn_Setup* objsetup) {
    Object* frostWeed;
    s32 weeds;
    s8 FROSTWEED_MAX_OVERRIDE = getFrostWeedMaxOverride(); //@recomp
    s8 FROSTWEED_TWIGS_ACCEPTED = getFrostWeedTwigsConfigs(); //@recomp
    
    self->unkAF &= 0xFFF7;
    switch (objdata->flags) {
        case 0:
            //Calling out to the player periodically
            objdata->unk8 += gUpdateRate;
            if (objdata->unk8 >= 0x3E9) {
                gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_1E2_Garunda_Te_Will_somebody_get_me_out_of_here, MAX_VOLUME, 0, 0, 0, 0);
                gDLL_22_Subtitles->vtbl->func_368(0xA);
                objdata->unk8 = 0;
            }
            if (self->unkAF & 4) {
                objdata->flags = 1;
            }
            break;
        case 1:
            if (func_80032538(self)) {
                gDLL_3_Animation->vtbl->func17(0, self, -1);
                objdata->flags = 2;
                main_set_bits(0x115, objdata->flags);
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
                if (!((DLL_227_Tumbleweed*)frostWeed->dll)->vtbl->is_gravitating(frostWeed)) {
                    ((DLL_227_Tumbleweed*)(frostWeed->dll))->vtbl->gravitate_towards_point(frostWeed, &objdata->playerPositionCopy);
                    objdata->frostWeed = frostWeed;
                    if (0){
                        objdata->garundaTe_weedsEaten = FROSTWEED_MAX_OVERRIDE;
                    }
                    objdata->garundaTe_weedsEaten++;
                    if (objdata->garundaTe_weedsEaten > FROSTWEED_MAX_OVERRIDE) {
                        objdata->garundaTe_weedsEaten = FROSTWEED_MAX_OVERRIDE;
                    }
                    main_set_bits(0x48B, objdata->garundaTe_weedsEaten);
                    objdata->flags = 3;
                }
            }
            break;
        case 3:
            if (vec3_distance_xz_squared(&objdata->playerPositionCopy, &objdata->frostWeed->positionMirror) < 6.25f) {
                objdata->flags = 4;
            }
            break;
        case 4:
            if (objdata->unk424 & 8) {
                weeds = objdata->garundaTe_weedsEaten;
                if (weeds >= FROSTWEED_MAX_OVERRIDE) {
                    main_set_bits(0x102, 1);
                    objdata->flags = 5;
                    main_set_bits(0x115, objdata->flags);
                    break;
                }
                if (weeds % 3 == 0) {
                    gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_74B_Garunda_Te_That_tastes_great_Hurry_up_boy, MAX_VOLUME, 0, 0, 0, 0);
                    gDLL_22_Subtitles->vtbl->func_368(2);
                }
                objdata->flags = 2;
            }
            break;
        case 5:
            if (func_80032538(self)) {
                if (objdata->unk425 % 2) {
                    gDLL_3_Animation->vtbl->func17(3, self, -1);
                } else {
                    gDLL_3_Animation->vtbl->func17(2, self, -1);
                }
                objdata->unk425 += 1;
            }
            break;
        case 6:
            //SpellStone activation
            if (func_80032538(self)) {
                gDLL_3_Animation->vtbl->func17(4, self, -1);
            } else if (gDLL_1_UI->vtbl->func_DF4(0x123)) {
                main_set_bits(0x22B, 1);
                objdata->flags = 7;
                main_set_bits(0x115, objdata->flags);
            }
            break;
        case 7:
            self->unkAF |= 8;
            break;
    }
    
    if (objdata->flags >= 2 && objdata->flags < 5) {
        diPrintf("noweeds=%d/%d\n", objdata->garundaTe_weedsEaten, FROSTWEED_MAX_OVERRIDE);
    }
}
