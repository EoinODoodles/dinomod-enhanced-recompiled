#include "configs.h"
#include "modding.h"
#include "player_util.h"
#include "recompconfig.h"

#include "dll.h"
#include "dlls/engine/33_BaddieControl.h"
#include "game/objects/interaction_arrow.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/rand.h"

#include "recomp/dlls/objects/232_VampireBat_recomp.h"

//TEMPORARY DEFINES
#define dll_baddieControl (gDLL_33_BaddieControl->vtbl)

#define VampireBat_obj_Setup dll_232_setup
#define VampireBat_obj_Control dll_232_control
#define VampireBat_handleMotion dll_232_func_AB8
#define VampireBat_logicState2FlyRandom dll_232_func_CF0

#define dHitAnimStateMap _data_0
#define dHitDamageMap _data_70
#define sAnimStateCallbacks _bss_0
#define sLogicStateCallbacks _bss_8
//END OF TEMPORARY DEFINES

typedef struct {
    Vec3f home;
    Vec3f goal;
    f32 prevVelocityY;
    f32 heightAboveFloor;
    u8 prevConfig; //@recomp: repurposing unused field
    u8 _unk21;
    s16 dyingPitchSpeed;
    s16 dyingYawSpeed;
    s16 dyingRollSpeed;
    s32 _unk28;
} VampireBat_Data;

typedef enum {
    VampireBat_ASTATE_0_Flying,
    VampireBat_ASTATE_1_Hit
} VampireBat_AnimStates;

typedef enum {
    VampireBat_LSTATE_0_Top,
    VampireBat_LSTATE_1_Fly_Around_Randomly,
    VampireBat_LSTATE_2_Fly_To_Target,
    VampireBat_LSTATE_3_Dying,
    VampireBat_LSTATE_4_Dead
} VampireBat_LogicStates;

/*0x0*/ extern s32 dHitAnimStateMap[];
/*0x70*/ extern s8 dHitDamageMap[];

/*0x0*/ extern ObjFSA_StateCallback sAnimStateCallbacks[2];
/*0x8*/ extern ObjFSA_StateCallback sLogicStateCallbacks[5];

extern void VampireBat_handleMotion(Object* self, VampireBat_Data* objData);

static void VampireBat_handleConfigChange(Object* self) {
    VampireBat_BattleMode config = configs_GetVampireBatMode();
    Baddie* baddie;
    VampireBat_Data* objData;

    baddie = self->data;
    if (baddie == NULL) {
        return;
    }

    objData = baddie->objdata;
    if (objData == NULL) {
        return;
    }

    //Check if config changed (and the relevant changes were applied to this bat)
    if (objData->prevConfig == config) {
        return;
    }

    objData->prevConfig = config;

    //Toggle collision and targetting
    if ((baddie->fsa.logicState != VampireBat_LSTATE_4_Dead) && (config == VAMPIREBAT_BATTLE_ON)) {
        if (self->objhitInfo) {
            self->objhitInfo->unk58 |= 1;
        }

        self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
    } else {
        if (self->objhitInfo) {
            self->objhitInfo->unk58 &= ~1;
        }

        self->unkAF |= ARROW_FLAG_8_No_Targetting;
    }

    //Optionally ignore the player
    if (baddie->fsa.logicState > VampireBat_LSTATE_1_Fly_Around_Randomly) {
        baddie->fsa.logicState = VampireBat_LSTATE_0_Top; 
    }
}

RECOMP_PATCH void VampireBat_obj_Setup(Object* self, Baddie_Setup* setup, s32 reset) {
    VampireBat_Data* objData;
    Baddie* baddie;
    u8 flags;

    baddie = self->data;

    flags = 2 | 4;
    if (reset) {
        flags = 1 | 2 | 4;
    }
    if ((setup->unk2B & 0x20) == FALSE) {
        flags |= 8;
    }
    dll_baddieControl->setup(self, setup, baddie, 2, 5, 0x108, flags, 20.0f);

    self->animCallback = NULL;

    objData = baddie->objdata;
    bzero(objData, sizeof(VampireBat_Data));

    objData->home.x = setup->base.x;
    objData->home.y = setup->base.y;
    objData->home.z = setup->base.z;
    objData->prevVelocityY = 0;
    objData->heightAboveFloor = 0;

    objAnimSet(self, 0, 0, 0);

    baddie->fsa.animState = VampireBat_ASTATE_0_Flying;
    baddie->fsa.logicState = VampireBat_LSTATE_0_Top;
    baddie->fsa.flags |= OBJFSA_FLAG_1000000;
    baddie->fsa.hitpoints = 1;
    baddie->unk3B6 = 0;
    baddie->unk3B4 = 0;

    if (self->shadow != NULL) {
        self->shadow->flags |= OBJ_SHADOW_FLAG_8000 | OBJ_SHADOW_FLAG_TOP_DOWN | OBJ_SHADOW_FLAG_USE_OBJ_YAW | OBJ_SHADOW_FLAG_CUSTOM_OBJ_POS | OBJ_SHADOW_FLAG_CUSTOM_DIR;
    }

    baddie->fsa.unk4.mode = 0;

    func_800267A4(self);

    //@recomp: handle bat battle configs
    objData->prevConfig = -1;
    VampireBat_handleConfigChange(self);
}

RECOMP_PATCH void VampireBat_obj_Control(Object* self) {
    s16 dFXScales[4] = {0x0206, 0x0167, 0x0165, 0x0206};
    Baddie_Setup* objSetup;
    Baddie* baddie;
    ObjectShadow* objShadow;
    f32 dVelocityY;
    VampireBat_Data* objData;
    Object* player;
    Vec3f delta;
    SRT fxTransform;
    s32 count;
    f32 playerDistBase;
    s32 idx;

    baddie = self->data;
    objSetup = (Baddie_Setup*)self->setup;
    objShadow = self->shadow;
    objData = baddie->objdata;
    player = objGetPlayer();

    if (self->unkDC) {
        return;
    }

    //@recomp: handle bat battle configs
    VampireBat_handleConfigChange(self);

    if (self->unkE0 == 0) {
        self->srt.transl.x = objSetup->base.x;
        self->srt.transl.y = objSetup->base.y;
        self->srt.transl.z = objSetup->base.z;
        gDLL_3_Animation->vtbl->start_obj_sequence(objSetup->unk2E, self, -1);
        self->unkE0 = 1;
        return;
    }

    //Do nothing when dead
    if (baddie->fsa.logicState == VampireBat_LSTATE_4_Dead) {
        return;
    }

    //@recomp: ignore the player while they're in important cutscenes
    if (baddie->fsa.logicState == VampireBat_LSTATE_2_Fly_To_Target && 
        player && baddie->fsa.target == player && 
        playerUtil_isImportantSequencePlaying()
    ) {
        baddie->fsa.logicState = VampireBat_LSTATE_0_Top; 
    }

    //Advance animation based on change in velocityY
    dVelocityY = self->velocity.y - objData->prevVelocityY;
    objData->prevVelocityY = self->velocity.y;
    if (dVelocityY < 0.0f) {
        dVelocityY *= -0.04f;
    } else {
        dVelocityY *= 0.08f;
    }
    if (dVelocityY > 0.1f) {
        dVelocityY = 0.1f;
    }
    objAnimAdvance(self, dVelocityY, gUpdateRateF, NULL);

    //Get the distance between the player and the bat's base position
    delta.x = objData->home.x - player->srt.transl.x;
    delta.y = objData->home.y - player->srt.transl.y;
    delta.z = objData->home.z - player->srt.transl.z;
    playerDistBase = sqrtf(SQ(delta.f[0]) + SQ(delta.f[1]) + SQ(delta.f[2]));

    //Acquire the player as a target if they're in range
    if (baddie->unk3B6 == 0) {
        if (playerDistBase < baddie->unk3E2) {
            baddie->fsa.target = player;
            baddie->unk3B6 = 1;
        }
    } else {
        if (playerDistBase > baddie->unk3E2) {
            baddie->fsa.target = NULL;
            baddie->unk3B6 = 0;
        }
    }

    //Set shadow position
    objShadow->tr.x = self->srt.transl.x;
    objShadow->tr.z = self->srt.transl.z;
    if (trackGetHeightFloor(self, self->srt.transl.x, self->srt.transl.y, self->srt.transl.z, &objData->heightAboveFloor, 0)) {
        objShadow->tr.y = self->srt.transl.y - objData->heightAboveFloor;
    }

    //Get distance from bat to target
    if (baddie->fsa.target != NULL) {
        delta.x = baddie->fsa.target->globalPosition.x - self->globalPosition.x;
        delta.y = baddie->fsa.target->globalPosition.y - self->globalPosition.y;
        delta.z = baddie->fsa.target->globalPosition.z - self->globalPosition.z;
        baddie->fsa.targetDist = sqrtf(SQ(delta.f[0]) + SQ(delta.f[1]) + SQ(delta.f[2]));
    }

    if ((baddie->unk3B0 & 0x20) == FALSE) {
        dll_baddieControl->func14(self, baddie, &baddie->unk3B2, -1, -1, baddie->unk3A6, baddie->unk3A4);
    }
    dll_baddieControl->func20(self, &baddie->fsa, &baddie->unk34C, baddie->unk39E, NULL, 0, 0, 0);

    //Create particles when hit
    if (baddie->fsa.hitpoints > 0) {
        s32 damageType;
        s32 hit;

        hit = dll_baddieControl->check_hit(self, &baddie->fsa, &baddie->unk34C, baddie->unk39E, dHitAnimStateMap, dHitDamageMap, 1, &baddie->unk3A8, &fxTransform);
        damageType = func_80025F40(self, NULL, NULL, NULL);

        if (hit || damageType) {
            //@recomp: set hit state
            if (baddie->fsa.animState != VampireBat_ASTATE_1_Hit) {
                gDLL_18_objfsa->vtbl->set_anim_state(self, &baddie->fsa, VampireBat_ASTATE_1_Hit);
            }

            idx = 0;
            idx = ((DLL_Unknown*)player->linkedObject->dll)->vtbl->func[19].withOneVoidArgS32(player->linkedObject);
            if (idx > 3) {
                idx = 3;
            } else if (idx < 0) {
                idx = 0;
            }

            fxTransform.scale = dFXScales[idx];
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_323, &fxTransform, PARTFXFLAG_200000 | PARTFXFLAG_1, -1, NULL);

            fxTransform.transl.x -= self->srt.transl.x;
            fxTransform.transl.y -= self->srt.transl.y;
            fxTransform.transl.z -= self->srt.transl.z;
            fxTransform.scale = dFXScales[idx];

            count = 4;
            while (count--) {
                gDLL_17_partfx->vtbl->spawn(self, PARTICLE_324, &fxTransform, PARTFXFLAG_2, -1, NULL);
            }
        }
    }

    dll_baddieControl->func10(self, &baddie->fsa, 0.0f, -1);
    baddie->unk3AC = self->animObj;
    self->animObj = NULL;
    gDLL_18_objfsa->vtbl->tick(self, &baddie->fsa, gUpdateRateF, gUpdateRateF, sAnimStateCallbacks, sLogicStateCallbacks);
    self->animObj = baddie->unk3AC;
}

RECOMP_PATCH s32 VampireBat_logicState2FlyRandom(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie;
    VampireBat_Data* objData;
    s16 randomAngle;
    f32 distance;

    baddie = self->data;
    objData = baddie->objdata;

    //Randomise the goal point: a fixed distance from home laterally in a random direction, at a random height
    randomAngle = mathRnd(-M_180_DEGREES, M_180_DEGREES - 1);
    distance = baddie->unk3E2 * 0.75f;
    objData->goal.x = objData->home.x + Sinf(randomAngle) * distance;
    objData->goal.y = objData->home.y + mathRnd(30, 100);
    objData->goal.z = objData->home.z + Cosf(randomAngle) * distance;

    VampireBat_handleMotion(self, objData);

    //Advance state when a target is acquired
    if ((baddie->unk3B6 == 1) 
        && (configs_GetVampireBatMode() != VAMPIREBAT_BATTLE_OFF_IGNORE) //@recomp: optionally ignore the player
        && playerUtil_isImportantSequencePlaying() == FALSE         //@recomp: ignore the player if an important sequence is playing
    ) {
        return FSA_NEXTSTATE_SYNC(VampireBat_LSTATE_2_Fly_To_Target);
    }

    return 0;
}
