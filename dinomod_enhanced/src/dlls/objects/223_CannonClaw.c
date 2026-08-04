#include "math_util.h"
#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/common/sidekick.h"
// #include "dlls/objects/537_DIMCannon.h"
#include "game/objects/interaction_arrow.h"
#include "game/objects/object_id.h"
#include "game/objects/object.h"
#include "macros.h"
#include "sys/dll.h"
#include "sys/gfx/modgfx.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/math.h"
#include "sys/objanim.h"
#include "sys/objects.h"
#include "sys/objexpr.h"
#include "sys/objhits.h"
#include "sys/objmsg.h"
#include "sys/objprint.h"
#include "sys/print.h"
#include "sys/rand.h"

#include "objects/223_CannonClaw.h"

#include "recomp/dlls/objects/223_CannonClaw_recomp.h"

// #define DEBUG_CANNON_CLAW

#define VANISH_TIME_SWITCH 1.8f
#define VANISH_TIME_FX_END 2.0f
#define VANISH_TIME_FINISHED 5.5f

//TEMPORARY DEFINES
#define PARTICLE_52A 0x52A
#define SOUND_237_SharpClaw_Arghhh 0x237
#define SOUND_238_SharpClaw_Snort 0x238
#define SOUND_239_SharpClaw_Rah_Snort 0x239
#define SOUND_23A_SharpClaw_Ugh_Snort 0x23A

typedef struct {
/*00*/  ObjSetup base;
/*18*/  s16 gamebitSiloCoverOpen;
/*1A*/  s16 gamebitCannonClawDead;
/*1C*/  s16 gamebitCannonClawAboard;
/*1E*/  s16 gamebitCannonClawTruce;
/*20*/  s16 gamebitSiloEnter;
/*22*/  s16 gamebitSiloExit;
/*24*/  s16 unk24;
/*26*/  s16 hostileRange;
/*28*/  s8 yaw;
/*29*/  u8 cooldownMin;
/*2A*/  u8 cooldownMax;
/*2B*/  u8 rangeSiloRetreat;
} DIMCannon_Setup;
//END OF TEMPORARY DEFINES

typedef struct {
    f32 prevAnimProgress;
    f32 vanishTimer;
    u32 soundHandleVoice;
    u8 state;
    u8 prevState;
    u8 flags;
    HeadAnimation headAnim;
} CannonClaw_Data; //@recomp addition

typedef enum {
    CannonClaw_STATE_0_Idle,
    CannonClaw_STATE_1_Firing,
    CannonClaw_STATE_2_Dying,
    CannonClaw_STATE_3_Dead
} CannonClaw_States;

typedef enum {
    //Bank 0
    CannonClaw_MODANIM_0_10_Die_Spirited_Away = 0xA,
    CannonClaw_MODANIM_0_11_Die,                        

    //Bank 2
    CannonClaw_MODANIM_2_8_Cannon_Idle_LOOP = 0x208,
    CannonClaw_MODANIM_2_9_Cannon_Fire_Recoil      
} CannonClaw_ModAnims;

typedef struct {
    u16 modAnimID;
    f32 animSpeed;
    f32 initialAnimProgress;
    s32 blendDuration;
} CannonClaw_StateAnimData;

static CannonClaw_StateAnimData rsStateAnims[] = {
    {CannonClaw_MODANIM_2_8_Cannon_Idle_LOOP,   0.008f, 0.0f, 30},
    {CannonClaw_MODANIM_2_9_Cannon_Fire_Recoil, 0.016f, 0.3f, 2},
    {CannonClaw_MODANIM_0_11_Die,               0.009f, 0.0f, 2}
};

static u32 rsWeaponHitSounds[] = {
    SOUND_374_Whack, 
    SOUND_375_Smack1, 
    SOUND_376_Smack2
};

#ifdef DEBUG_CANNON_CLAW
RECOMP_HOOK_DLL(CannonClaw_obj_Control) void debugCannonClaw(Object* self) {
    CannonClaw_Data* objData = self->data;

    diPrintf("%s state: %d\n", self->def->name, objData->state);
    diPrintf("objData->vanishTimer: %f\n", &objData->vanishTimer);

    recomp_printf("%s state: %d\n", self->def->name, objData->state);
    recomp_printf("objData->vanishTimer: %f\n", &objData->vanishTimer);
    recomp_printf("%f, %f, %f\n", &self->globalPosition.x, &self->globalPosition.y, &self->globalPosition.z);

} 
#endif

extern void CannonClaw_die(Object* self);

RECOMP_PATCH void CannonClaw_obj_Setup(Object* self, ObjSetup* setup, s32 reset) {
    /* RECOMP */
    DIMCannon_Setup* cannonSetup;
    CannonClaw_Data* objData = self->data; 

    self->srt.yaw = -M_180_DEGREES;
    self->srt.transl.y = setup->y + 2.0f;  

    //@recomp: Check if cannon's gamebit is set
    if (self->parent) {
        cannonSetup = (DIMCannon_Setup*)self->parent->setup;
        if (mainGetBits(cannonSetup->gamebitCannonClawDead)) {
            objData->state = CannonClaw_STATE_3_Dead;
            objData->vanishTimer = VANISH_TIME_FINISHED;
            self->unkDC = 1; //@recomp: Don't draw
            return;          //@recomp: No further setup
        }
    }

    //@recomp: receive messages
    objInitMesgQueue(self, 2);
}

/* Receive messages from DIMCannon about various events, and store them as flags */
static void CannonClaw_receiveMessages(Object* self) {
    CannonClaw_Data* objData = self->data;
    Object* outSender;
    u32 outMesgID;

    while (objRecvMesg(self, &outMesgID, &outSender, 0)){
        //Ignore messages if they're not from the CannonClaw's own cannon
        if (outSender != self->parent) {
            continue;
        }

        switch (outMesgID) {
        case CannonClaw_FLAG_1_Cannon_Fired:
            objData->flags |= CannonClaw_FLAG_1_Cannon_Fired;
            break;
        case CannonClaw_FLAG_2_Hit_Player_or_Sidekick:
            objData->flags &= ~(CannonClaw_FLAG_4_Entered_Silo | CannonClaw_FLAG_8_Exited_Silo);
            objData->flags |= CannonClaw_FLAG_2_Hit_Player_or_Sidekick;
            break;
        case CannonClaw_FLAG_4_Entered_Silo:
            objData->flags |= CannonClaw_FLAG_4_Entered_Silo;
            break;
        case CannonClaw_FLAG_8_Exited_Silo:
            objData->flags |= CannonClaw_FLAG_8_Exited_Silo;
            break;
        case CannonClaw_FLAG_10_Distracted:
            objData->flags |= CannonClaw_FLAG_10_Distracted;
            break;
        }
    }
}

/* Plays sound clips of the CannonClaw reacting to different things. Uses a soundHandle so they don't stack up! */
static void CannonClaw_handleVoiceLines(Object* self) {
    typedef struct {
        u16 id;
        u8 vol;
    } CannonClawSounds;

    static CannonClawSounds rsCannonClawVoiceLinesLaugh[] = {
        {0x411, MAX_VOLUME},
        {0x5BC, MAX_VOLUME/2},
        {0x8D2, MAX_VOLUME/2}
    };
    static CannonClawSounds rsCannonClawVoiceLinesAngry[] = {
        {0x4D0, MAX_VOLUME/2},
        {0x4D1, MAX_VOLUME/2},
        {0xB26, MAX_VOLUME}
    };
    static CannonClawSounds rsCannonClawVoiceLinesHurt[] = {
        {SOUND_236_SharpClaw_Argh,      MAX_VOLUME},
        {SOUND_237_SharpClaw_Arghhh,    MAX_VOLUME},
        {SOUND_238_SharpClaw_Snort,     MAX_VOLUME},
        {SOUND_239_SharpClaw_Rah_Snort, MAX_VOLUME},
        {SOUND_23A_SharpClaw_Ugh_Snort, MAX_VOLUME}
    };

    CannonClaw_Data* objData = self->data;
    Object* sender;
    u32 outMesgID;
    CannonClawSounds* sound;

    //Check if the CannonClaw's voice line has finished
    if (objData->soundHandleVoice && (dll_amSfx->IsPlaying(objData->soundHandleVoice) == FALSE)) {
        objData->soundHandleVoice = 0;
    }

    //Play a voice line when hurt
    if ((objData->prevState < CannonClaw_STATE_2_Dying) && (objData->state >= CannonClaw_STATE_2_Dying)) {
        //Interrupt current voice line (if playing)
        if (objData->soundHandleVoice) {
            dll_amSfx->Stop(objData->soundHandleVoice);
        }

        sound = &rsCannonClawVoiceLinesHurt[mathRnd(0, ARRAYCOUNT(rsCannonClawVoiceLinesHurt) - 1)];
        objData->soundHandleVoice = dll_amSfx->Play(self, sound->id, sound->vol, NULL, NULL, 0, NULL);
    }

    //Do nothing when already speaking
    if (objData->soundHandleVoice) {
        return;
    }

    //Play voice lines when various events are flagged through messages
    {
        //Laugh when hitting player/sidekick, or when entering silo
        if ((objData->flags & CannonClaw_FLAG_2_Hit_Player_or_Sidekick) ||
            (objData->flags & CannonClaw_FLAG_4_Entered_Silo)
        ) {
            objData->flags &= ~(CannonClaw_FLAG_2_Hit_Player_or_Sidekick | CannonClaw_FLAG_4_Entered_Silo);
            sound = &rsCannonClawVoiceLinesLaugh[mathRnd(0, ARRAYCOUNT(rsCannonClawVoiceLinesLaugh) - 1)];
            objData->soundHandleVoice = dll_amSfx->Play(self, sound->id, sound->vol, NULL, NULL, 0, NULL);
            return;
        }

        //Grunt when exiting silo, or when distracted by Tricky
        if ((objData->flags & CannonClaw_FLAG_8_Exited_Silo) ||
            (objData->flags & CannonClaw_FLAG_10_Distracted)
        ) {
            objData->flags &= ~(CannonClaw_FLAG_8_Exited_Silo | CannonClaw_FLAG_10_Distracted);
            sound = &rsCannonClawVoiceLinesAngry[mathRnd(0, ARRAYCOUNT(rsCannonClawVoiceLinesAngry) - 1)];
            objData->soundHandleVoice = dll_amSfx->Play(self, sound->id, sound->vol, NULL, NULL, 0, NULL);
            return;
        }
    }
}

/* Creates hit particle effects (the same as those used by the regular SharpClaw) */
static void CannonClaw_createHitEffects(Object* self, SRT* fxTransform, s32 useModGfx) {
    DLL_IModgfx* modGfxDLL;
    s32 i;
    s32 dModGfxParams[] = { 0x00000006, 0x00000069, 0x00000069, 0x000000ff };
    f32 fxParam;
    Object* player;

    //Don't create particles if the player is far away
    player = objGetPlayer();
    if (player && (vec3DistanceXZSquared(&self->globalPosition, &player->globalPosition) > SQ(BLOCKS_GRID_UNIT))) {
        return;
    }

    if (useModGfx == FALSE) {
        fxParam = 0.014f;
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_325, fxTransform, 0x200001, -1, &fxParam);

        fxTransform->scale = 92.0f;
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_323, fxTransform, 0x200001, -1, NULL);

        fxParam = 0.015f;
        fxTransform->scale = 231.0f;
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_323, fxTransform, 0x200001, -1, &fxParam);

        fxTransform->transl.x -= self->globalPosition.x;
        fxTransform->transl.y -= self->globalPosition.y;
        fxTransform->transl.z -= self->globalPosition.z;
        fxTransform->scale = 123.0f;

        for (i = 0; i < 15; i++) {
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_324, fxTransform, 2, -1, NULL);
        }
    } else {
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_328, fxTransform, 0x200001, -1, NULL);

        fxTransform->transl.x -= self->globalPosition.x;
        fxTransform->transl.y -= self->globalPosition.y;
        fxTransform->transl.z -= self->globalPosition.z;

        modGfxDLL = dllLoad(DLL_ID_106, 1);

        dModGfxParams[1] += mathRnd(0, 155);
        dModGfxParams[2] += mathRnd(0, 155);

        fxTransform->yaw = 0;
        fxTransform->pitch = 0;
        fxTransform->roll = 0;
        fxTransform->scale = 1.0f;

        modGfxDLL->vtbl->func0(self, 0, fxTransform, 1, -1, dModGfxParams);
        if (modGfxDLL != NULL) {
            dllFree(modGfxDLL);
        }
    }
}

/* Unparents the CannonClaw from the cannon while dying, so the particle effects can work with their expected worldSpace coordinates */
static void CannonClaw_unparent(Object* self, f32 dy) {
    s32 yaw;

    if (self->parent == NULL) {
        return;
    }

    self->srt.transl.x = self->globalPosition.x;
    self->srt.transl.y = self->globalPosition.y + dy;
    self->srt.transl.z = self->globalPosition.z;
    yaw = self->parent->srt.yaw - M_180_DEGREES;
    CIRCLE_WRAP(yaw);
    self->srt.yaw = yaw;
    self->parent = NULL;
}

RECOMP_PATCH void CannonClaw_obj_Control(Object* self) {
    DIMCannon_Setup* cannonSetup;
    Object* sidekick;
    /* RECOMP */
    CannonClaw_Data* objData = self->data;
    s32 animFinished = FALSE;
    s32 yaw;
    Object* cannon;
    s32 damageType;
    static SRT fxTransform;

    //@recomp: continue to State Machine even when not being drawn
    // //Do nothing when not being drawn
    // if (self->unkDC) {
    //     return;
    // }

    //@recomp: store last tick's state
    if (objData->state != objData->prevState) {
        objData->prevState = objData->state;
    }

    //@recomp: Handle dying effects
    if (objData->vanishTimer > 0.0f) {
        objData->vanishTimer += gUpdateRateF * 0.005f;
    }

    //@recomp: handle LockIcon
    cannon = self->parent;
    if ((objData->state >= CannonClaw_STATE_2_Dying) ||                            //No LockIcon while dying
        (cannon && cannon->setup && (cannon->globalPosition.y < cannon->setup->y)) //No LockIcon when the CannonClaw retreats into an underground silo
    ) {
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
    } else {
        self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
    }

    //@recomp: Handle animation (per state)
    if (objData->state != CannonClaw_STATE_3_Dead) {
        objData->prevAnimProgress = self->animProgress;

        if (objData->state < ARRAYCOUNT(rsStateAnims)) {
            //Change animation when needed
            if (self->curModAnimId != rsStateAnims[objData->state].modAnimID) {
                objAnimSet(self, 
                    rsStateAnims[objData->state].modAnimID, 
                    rsStateAnims[objData->state].initialAnimProgress, 
                    0
                );
                objAnim_func_80024D74(self, rsStateAnims[objData->state].blendDuration);
            }

            //Advance animation
            animFinished = objAnimAdvance(self, rsStateAnims[objData->state].animSpeed, gUpdateRateF, NULL);
        }

        //Handle eye darts/blinks
        if (objData->state < CannonClaw_STATE_2_Dying) {
            objExprEyeIdle(self, &objData->headAnim);
        } else if (objData->prevState < CannonClaw_STATE_2_Dying) {
            //TODO: SnowClaw model has no blink texture frames, and no tagged eyelid faces
            TextureAnimator* eyelidL;
            TextureAnimator* eyelidR;

            eyelidL = objExprGetTexAnimator(self, HEAD_ANIMATION_TAG_Eyelid_L, 0);
            eyelidR = objExprGetTexAnimator(self, HEAD_ANIMATION_TAG_Eyelid_R, 0);

            if (eyelidL && eyelidR) {
                eyelidL->frame = 0x200;
                eyelidR->frame = 0x200;
            }
        }
    }

    //Handle damage
    if (objData->state < CannonClaw_STATE_2_Dying) {
        damageType = func_8002601C(self, NULL, NULL, NULL, &fxTransform.transl.x, &fxTransform.transl.y, &fxTransform.transl.z);
        if (damageType) {
            //Set cannon gamebit
            if (self->parent && self->parent->setup) {
                cannonSetup = (DIMCannon_Setup*)self->parent->setup;
                mainSetBits(cannonSetup->gamebitCannonClawDead, TRUE);
            }
            
            //@recomp: fix missing null check for the sidekick
            sidekick = objGetSidekick();
            if (sidekick) {
                ((DLL_ISidekick*)sidekick->dll)->vtbl->func21(sidekick, 0, NULL);
            }

            //Start dying
            objData->state = CannonClaw_STATE_2_Dying;
            self->animProgress = 0.0f;
            objData->vanishTimer = 1.0f;

            //@recomp: Unparent self from cannon (so particle effects don't get confused by local coordinates)
            CannonClaw_unparent(self, 0.5f);

            //@recomp: Play weapon impact sound
            dll_amSfx->Play(self, rsWeaponHitSounds[mathRnd(0, ARRAYCOUNT(rsWeaponHitSounds) - 1)], MAX_VOLUME, NULL, NULL, 0, NULL);

            //@recomp: Create hurt effects (same as those used by the regular SharpClaws)
            fxTransform.transl.x += gWorldX;
            fxTransform.transl.z += gWorldZ;
            CannonClaw_createHitEffects(self, &fxTransform, FALSE);
        }
    }

    //Receive messages from DIMCannon
    if (objData->state < CannonClaw_STATE_2_Dying) {
        CannonClaw_receiveMessages(self);
    }

    //Handle voice lines
    CannonClaw_handleVoiceLines(self);

    //@Recomp: State Machine
    switch (objData->state) {
    case CannonClaw_STATE_0_Idle:
        //Recoil when the cannon fires
        if (objData->flags & CannonClaw_FLAG_1_Cannon_Fired) {
            objData->flags &= ~CannonClaw_FLAG_1_Cannon_Fired;
            objData->state = CannonClaw_STATE_1_Firing;
        }
        break;
    case CannonClaw_STATE_1_Firing:
        if (animFinished) {
            objData->state = CannonClaw_STATE_0_Idle;
        }
        break;
    case CannonClaw_STATE_2_Dying:
        if (animFinished) {
            CannonClaw_die(self);
            objData->state = CannonClaw_STATE_3_Dead;
            dll_amSfx->Play(self, SOUND_B1F_Slow_Magic_Chimes, MAX_VOLUME, NULL, NULL, 0, NULL);
        }

        if (self->animProgress >= 0.5) {
            f32 tValueFade;

            //Fade out during second half of animation
            tValueFade = 1.0f - ((self->animProgress - 0.5f)*2.0f);
            if (tValueFade < 0) {
                tValueFade = 0;
            } else if (tValueFade > 1) {
                tValueFade = 1;
            }
            self->opacity = (u8)(OBJECT_OPACITY_MAX * ease_in_quart(tValueFade));

#ifdef DEBUG_CANNON_CLAW
            diPrintf("OPACITY: %d, animProgress: %f\n", self->opacity, &self->animProgress);
            recomp_printf("OPACITY: %d, animProgress: %f\n", self->opacity, self->animProgress);
#endif

            //Play dying sound halfway through animation
            if (objData->prevAnimProgress < 0.5f) {
                dll_amSfx->Play(self, SOUND_B21_Dissipating_Hiss, MAX_VOLUME, NULL, NULL, 0, NULL);
            }
        }

        break;
    case CannonClaw_STATE_3_Dead:
        if (objData->vanishTimer >= VANISH_TIME_FINISHED) {
            objFreeObject(self);
        }
        break;
    }
}

RECOMP_PATCH void CannonClaw_obj_Print(Object* self, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    /* RECOMP */
    CannonClaw_Data* objData = self->data;

    if (visibility && self->unkDC == 0) {
		objprintDrawModel(self, gdl, mtxs, vtxs, pols, 1.0f);
	}

    //@Recomp: Draw dying effects
    if ((objData->vanishTimer > 0.0f) && (objData->state != CannonClaw_STATE_3_Dead)) {
        if (objData->vanishTimer >= VANISH_TIME_SWITCH) {
            gDLL_32_modelfx->vtbl->func2(self, PARTICLE_330, NULL);
        } else if (objData->vanishTimer < VANISH_TIME_FX_END) {
            gDLL_32_modelfx->vtbl->func2(self, PARTICLE_32F, &objData->vanishTimer);
        }
    }
}

RECOMP_PATCH u32 cannon_claw_get_data_size(Object* self, u32 offsetAddr) {
    return sizeof(CannonClaw_Data);
}
