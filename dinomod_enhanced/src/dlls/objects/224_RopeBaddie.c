#include "configs.h"
#include "modding.h"
#include "object_util.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "dll.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/engine/18_objfsa.h"
#include "dlls/engine/33_BaddieControl.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/221_ChukaChuck.h"
#include "game/objects/interaction_arrow.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/gfx/modgfx.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objhits.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "sys/rand.h"

#include "recomp/dlls/_asm/224_recomp.h"

// #define DEBUG_FSA

#define PLAYER_ATTACK_RANGE 60.0f
#define PLAYER_DISENGAGE_DURATION 240

//TEMPORARY DEFINES
#define RopeBaddie_initFSACallbacks dll_224_func_0
#define RopeBaddie_ctor dll_224_ctor
#define RopeBaddie_obj_Control dll_224_obj_Control
#define RopeBaddie_obj_Print dll_224_obj_Print
#define RopeBaddie_obj_Free dll_224_obj_Free
#define RopeBaddie_obj_GetDataSize dll_224_obj_GetDataSize

#define RopeBaddie_handleMessagesAndDamage dll_224_func_694
#define RopeBaddie_engageTarget dll_224_func_794
#define RopeBaddie_searchForTarget dll_224_func_830
#define RopeBaddie_findRopeNodes dll_224_func_8CC
#define RopeBaddie_getPitchAngle dll_224_func_C10
#define RopeBaddie_clampRopePosition dll_224_func_D48
#define RopeBaddie_handleAnimRopeSound dll_224_func_E08

#define RopeBaddie_animState0Walking dll_224_func_EA0
#define RopeBaddie_animState1Running dll_224_func_10C4
#define RopeBaddie_animState4CombatIdle dll_224_func_15D0
#define RopeBaddie_animState5Bite dll_224_func_168C
#define RopeBaddie_animState6Attack dll_224_func_1768

#define RopeBaddie_logicState0Top dll_224_func_1CBC
#define RopeBaddie_logicState1Respawning dll_224_func_1E10
#define RopeBaddie_logicState2Chase dll_224_func_1E84

#define sAnimStateCallbacks bss_0
#define sLogicStateCallbacks bss_28

#define dHitAnimStateMap data_0
#define dHitDamageMap data_70
#define dAttackSoundIDs data_8C
#define dBiteSounds data_98

#define dll_BaddieControl (gDLL_33_BaddieControl->vtbl)
DLL_INTERFACE(DLL_420_DFRopeNode) {
    /*:*/ DLL_INTERFACE_BASE(DLL_IObject);
    /*07*/ void (*func7)(Object* self, Vec4f* spline); //Get rope spline
    /*08*/ void (*func8)(Object* self, f32 positionValue, f32* ox, f32* oy, f32* oz); //Get point on rope, using a position value (0.0 to 7.0)
    /*09*/ void (*func9)(Object* self, f32* arg1, f32 arg2);
    /*10*/ s16 (*func10)(Object* self, f32 arg1, f32 arg2);
    /*11*/ s32 (*func11)(Object* self, f32 x, f32 y, f32 z, f32* arg4, f32* arg5, s8* arg6); //TODO: verify
    /*12*/ s16 (*func12)(Object* self); //GetRopeYaw (angle between rope's ends, viewed from above)
    /*13*/ void (*func13)(Object* self, u32 arg1); //Set connection state?
    /*14*/ s16 (*func14)(Object* self); //Check if disconnected?
    /*15*/ void (*func15)(Object* self, f32 arg1);
    /*16*/ void (*func16)(Object* self); //clear pointer to other DFropenode object
};

#define dll_DFropenode(obj) (((DLL_420_DFRopeNode*)obj->dll)->vtbl)
#define dll_player(obj) (((DLL_210_Player*)obj->dll)->vtbl)

#define SOUND_510_Ice_Shatter 0x510
#define ObjHitInfo_FLAG_1 1
#define PLAYER_ASTATE_Rope_Climb_Start 44
//END OF TEMPORARY DEFINES

typedef struct {
    f32 initialYDiff;         //The difference between the RopeBaddie's initial Y and its Y after being snapped to the rope
    f32 initialPointY;        //The Baddie's initial Y coord, after being snapped to the rope
    f32 initialY;             //The Baddie's initial Y coord, before finding by the rope
    Vec4f spline;             //Rope spline
    Vec3f initialPoint;       //Coords for the Baddie's initial position along the rope (worldSpace)
    u8 _unk28[0x34 - 0x28];
    Object* initialRopeNode;  //DFropenode Object
    Object* rope;             //DFropenode Object
    f32 nodeDistance;         //For initial rope node search: distance to the DFropenode
    f32 searchedRopePosition; //For initial rope node search: measures the Baddie's position along the searched rope
    s8 _unk44;
    s8 direction;             //Which way the Baddie's facing along the rope (0 or 1)
    u8 unk46;                 //Set to 0, but otherwise unused
    f32 ropePosition;         //Measures the Baddie's current position along the rope (a value from ~0.3 to 6.7)
    f32 initialRopePosition;  //Measures the Baddie's initial position along the rope (a value from ~0.3 to 6.7)
    s32 freezeTimer;          //@recomp: use int. The Baddie's joints glow when this is greater than zero, similar to SharpClaws' frozen effect. Seems to be unused!
    f32 goalPosition;         //The position the Baddie will try to move towards along the rope
    s16 ropeYaw;              //The angle between the rope's ends (viewed from above)
    /* RECOMP */
    s32 disengageTimer;       //Timer for optionally pursuing the player a little while longer after they leave the rope
    s32 prevFreezeTimer;
    u32 soundHandleFrozen;    //Handles twinkle sounds while frozen
    u8 fxFlags;               //Handles particles when thawing/shattering
} RopeBaddie_DataActual;

typedef enum {
    RopeBaddie_ASTATE_0_Walking,     //Traversing the rope idly
    RopeBaddie_ASTATE_1_Running,     //Chasing after the player once they're on the rope
    RopeBaddie_ASTATE_2_Turning,     //Turning 180 on the rope
    RopeBaddie_ASTATE_3_Idle,        //Waiting in place on the rope
    RopeBaddie_ASTATE_4_Combat_Idle, //After attacking, waiting to bite
    RopeBaddie_ASTATE_5_Bite,        //After waiting to attack again
    RopeBaddie_ASTATE_6_Attack,      //Tackles or punches the player
    RopeBaddie_ASTATE_7_Hit,         //Recoils from being hit
    RopeBaddie_ASTATE_8_Dying,       //Curls up, goes limp, and disappears
    RopeBaddie_ASTATE_9_Respawning   //After Gplay timer expires
} RopeBaddie_AnimStates;

typedef enum {
    RopeBaddie_LSTATE_0_Top,
    RopeBaddie_LSTATE_1_Respawning,
    RopeBaddie_LSTATE_2_Chase,
    RopeBaddie_LSTATE_3_Hit,
    RopeBaddie_LSTATE_4_Dying,
    RopeBaddie_LSTATE_5_Dead
} RopeBaddie_LogicStates;

typedef enum {
    RopeBaddie_MODANIM_0_Walk_LOOP,  //Moving hand-over-hand along the rope, leading with left arm. More of a climb really!
    RopeBaddie_MODANIM_1_Tackle,     //Coils back in anticipation, then lunges forward violently. Takes one "step" forward and back with left arm in the process, before settling back into netural.
    RopeBaddie_MODANIM_2_Idle_LOOP,  //Clinging on in neutral pose (right arm leading and left arm behind), with mouth agape. Tail swing around gently.
    RopeBaddie_MODANIM_3_Turn,       //Turns anticlockwise (viewed from above) to face the other way. Root Y counter-rotates throughout with even/linear spacing.
    RopeBaddie_MODANIM_4_Punch,      //Similar to [mAnim1], winds up in anticipation, but then unleashes a mean left hook! Clings on with right arm throughout.
    RopeBaddie_MODANIM_5_Sway_LOOP,  //Snakes body back and forth from the shoulders, as though ready to fight.
    RopeBaddie_MODANIM_6_Bite,       //Winds back slightly, then bites out ahead. Returns to near neutral.
    RopeBaddie_MODANIM_7_Recoil,     //Reels head backwards in pain and curls up slightly, before returning to neutral.
    RopeBaddie_MODANIM_8_Dying,      //Curls body up fully between arms, then goes limp with arms still hooked onto the rope.
    RopeBaddie_MODANIM_9_Dead        //From [mAnim8]'s end pose, sways back and forward slightly (unused?) [Root shifted way up vertically!]
} RopeBaddie_ModAnims;

typedef enum {
    RopeBaddie_FXFLAG_1_Shatter = 1,
    RopeBaddie_FXFLAG_2_Frost = 2
} RopeBaddie_FXFlags;

/*0x0*/ extern ObjFSA_StateCallback sAnimStateCallbacks[10];
/*0x28*/ extern ObjFSA_StateCallback sLogicStateCallbacks[6];

extern void RopeBaddie_initFSACallbacks(void);
extern void RopeBaddie_handleMessagesAndDamage(Object* self, Baddie* baddie, ObjFSA_Data* fsa);
extern void RopeBaddie_engageTarget(Object* self, s32 arg1, Baddie* baddie, ObjFSA_Data* fsa);
extern void RopeBaddie_searchForTarget(Object* self, Baddie* baddie, ObjFSA_Data* fsa);
extern void RopeBaddie_findRopeNodes(Object* self);
extern s16 RopeBaddie_getPitchAngle(RopeBaddie_DataActual* objData);
extern s32 RopeBaddie_clampRopePosition(RopeBaddie_DataActual* objData);
extern void RopeBaddie_handleAnimRopeSound(Object* self, ObjFSA_Data* fsa);

/*0x0*/ extern s32 dHitAnimStateMap[];
/*0x70*/ extern s8 dHitDamageMap[];
/*0x8C*/ extern u32 dAttackSoundIDs[3];
/*0x98*/ extern u32 dBiteSounds[1];

/* RECOMP: modGfxDLL for freeze shatter effect */
static DLL_IModgfx* rsModGfxDLL = NULL;

RECOMP_PATCH void RopeBaddie_ctor(void* dll) {
    RopeBaddie_initFSACallbacks();

    //@recomp: change damage map so Ice Blast doesn't do damage by itself 
    //(follow-up attacks while frozen are lethal though, like SFA)
    dHitDamageMap[Damage_Type_Ice_Blast - 2] = 0;
}

/* Custom function for handling the Ice Blast freeze state! Rare had the beginnings of the effect in the RopeBaddie's obj_Print function. */
static s32 RopeBaddie_handleIceBlastFreeze(Object* self, Baddie* baddie, RopeBaddie_DataActual* objData) {
    u8 freezeJustStarted = FALSE;
    s32 damageType;

    if (self == NULL || baddie == NULL || objData == NULL) {
        return FALSE;
    }

    //Do nothing if not frozen
    if (objData->freezeTimer <= 0) {
        return FALSE;
    }

    //End freeze if dead
    if (baddie->fsa.hitpoints <= 0) {
        objData->freezeTimer = 0;
    }

    //Otherwise, decrement freeze timer
    objData->prevFreezeTimer = objData->freezeTimer;
    objData->freezeTimer -= gUpdateRate;

    //Set frozen flag (based on SharpClaw code)
    if ((baddie->unk3B2 & 0x80) == FALSE) {
        baddie->unk3B2 |= 0x80;

        freezeJustStarted = TRUE;
    }

    //Handle playing audio
    if (objData->freezeTimer > 0) {
        #define SPARKLE_VOLUME VOLUME_PERCENT(40)
        #define SPARKLE_FADEOUT_TIME 120

        //Play an initial freeze sound
        if (freezeJustStarted) {
            dll_amSfx->Play(self, SOUND_3E3_Ice_Cracking, MAX_VOLUME, NULL, NULL, 0, NULL);
        }

        //Play twinkle sound when frozen, fading out as the Baddie thaws
        if (objData->soundHandleFrozen == 0) {
            objData->soundHandleFrozen = dll_amSfx->Play(self, SOUND_7F6_Firefly_Twinkle_Loop, SPARKLE_VOLUME, NULL, NULL, 0, NULL);
            dll_amSfx->SetPitch(objData->soundHandleFrozen, 0.5f);
        } else if (objData->freezeTimer < SPARKLE_FADEOUT_TIME) {
            f32 vol = objData->freezeTimer/(f32)SPARKLE_FADEOUT_TIME;
            if (vol < 0.0f) {
                vol = 0.0f;
            }
            vol *= SPARKLE_VOLUME;
            dll_amSfx->SetVol(objData->soundHandleFrozen, vol);
        }

        //Play melting sound when nearly thawed
        if (objData->prevFreezeTimer >= 60 && objData->freezeTimer < 60) {
            dll_amSfx->Play(self, SOUND_80C_Steam_Hissing, VOLUME_PERCENT(30), NULL, NULL, 0, NULL);

            //Create frost effects too
            objData->fxFlags |= RopeBaddie_FXFLAG_2_Frost;
        }
    }

    //Check for non-Ice Blast damage, ending the freeze immediately
    damageType = dll_BaddieControl->check_hit(self, &baddie->fsa, &baddie->unk34C, baddie->unk39E, dHitAnimStateMap, dHitDamageMap, RopeBaddie_LSTATE_3_Hit, &baddie->unk3A8, NULL);
    if (damageType > 0 && damageType != Damage_Type_Ice_Blast) {
        objData->freezeTimer = 0;
        objData->fxFlags |= RopeBaddie_FXFLAG_1_Shatter;
        dll_amSfx->Play(self, SOUND_510_Ice_Shatter, MAX_VOLUME, NULL, NULL, 0, NULL);
    }

    //Check if the freeze has ended
    if (objData->freezeTimer <= 0) {
        self->animProgress = 0.0f;

        //Clear frozen flag
        baddie->unk3B2 &= ~0x80;
        
        //Stop sound loop
        if (objData->soundHandleFrozen) {
            dll_amSfx->Stop(objData->soundHandleFrozen);
            objData->soundHandleFrozen = 0;
        }

        return FALSE;
    } else {
        //Freeze ongoing, result TRUE to end Baddie behaviour early
        return TRUE;
    }
}

/* Handles optional behaviour where the RopeBaddie briefly continues pursuing the player after they leave the rope. */
static void RopeBaddie_handleDisengageTimer(Object* self, Baddie* baddie) {
    Object* target;
    Object* playerNode = NULL;
    RopeBaddie_DataActual* objData = baddie->objdata;

    if (baddie == NULL || objData == NULL) {
        return;
    }

    target = baddie->fsa.target;

    if (target) {
        playerNode = dll_player(target)->func45(target);
    }

    if (playerNode == NULL) {
        if (configs_GetRopeBaddieCombatEdits() == FALSE) {
            baddie->fsa.target = NULL;
            return;
        } else {
            objData->disengageTimer -= gUpdateRate;
        }
    } else if (objData->rope == playerNode) {
        objData->disengageTimer = PLAYER_DISENGAGE_DURATION;
    }

    //Disengage when the timer's up, or the player's fallen below
    if (objData->disengageTimer < 0 || (target && (self->srt.transl.y - target->srt.transl.y > 200.0f))) {
        objData->disengageTimer = 0;
        baddie->fsa.target = NULL;
    }
}

/**
  * - Fix a bug where the RopeBaddie left its collision behind after dying.
  * - Fix how their LockIcon sometimes flickered on/off rapidly after dying. 
  */
RECOMP_PATCH void RopeBaddie_obj_Control(Object* self) {
    Baddie* baddie;
    RopeBaddie_DataActual* objData;
    Baddie_Setup* objSetup;

    baddie = self->data;
    objSetup = (Baddie_Setup*)self->setup;
    objData = baddie->objdata;

    if (self->unkDC) {
        if (gDLL_29_Gplay->vtbl->did_time_expire(objSetup->base.uID)) {
            dll_BaddieControl->setup(self, objSetup, baddie, 10, 6, 0x10E, 0x36, 20.0f);
            baddie->fsa.logicState = RopeBaddie_LSTATE_1_Respawning;
            baddie->fsa.enteredLogicState = TRUE;
            self->opacity = 0;
        }
        return;
    }

    if (objData->initialRopeNode) {
        //@recomp: handle frozen state
        if (RopeBaddie_handleIceBlastFreeze(self, baddie, objData)) {
            dll_DFropenode(objData->rope)->func8(objData->rope, objData->ropePosition, &self->srt.transl.x, &self->srt.transl.y, &self->srt.transl.z);
            return;
        }

        //@recomp: (Optionally) don't disengage immediately when the player falls from the rope, instead use a brief countdown
        RopeBaddie_handleDisengageTimer(self, baddie);

        gDLL_18_objfsa->vtbl->tick(self, &baddie->fsa, 1.0f, 1.0f, sAnimStateCallbacks, sLogicStateCallbacks);

        dll_DFropenode(objData->rope)->func8(objData->rope, objData->ropePosition, &self->srt.transl.x, &self->srt.transl.y, &self->srt.transl.z);

        RopeBaddie_handleMessagesAndDamage(self, baddie, &baddie->fsa);
        if ((baddie->fsa.target != NULL) || (baddie->fsa.hitpoints == 0)) {
            RopeBaddie_engageTarget(self, 0, baddie, &baddie->fsa);
        } else {
            RopeBaddie_searchForTarget(self, baddie, &baddie->fsa);
        }
    } else {
        RopeBaddie_findRopeNodes(self);
    }

    //@recomp: remove collision and LockIcon when dead
    if (baddie->fsa.logicState == RopeBaddie_LSTATE_4_Dying || 
        baddie->fsa.logicState == RopeBaddie_LSTATE_5_Dead ||
        baddie->fsa.logicState == RopeBaddie_LSTATE_1_Respawning
    ) {
        self->objhitInfo->unk58 &= ~1;
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
    } else {
        self->objhitInfo->unk58 |= 1;
        self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
    }
}

/* Build on the incomplete freeze effect */
RECOMP_PATCH void RopeBaddie_obj_Print(Object* self, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    Baddie* baddie;
    RopeBaddie_DataActual* objData;
    /* RECOMP */
    static SRT fxTransform;
    static u8 colour[] = { 0xFC, 0xFF, 0xFF };
    u8 opacity;

    baddie = self->data;
    objData = baddie->objdata;

    if (!visibility || self->unkDC) {
        return;
    }

    //@recomp: freeze effects
    if (objData->freezeTimer > 0 || objData->fxFlags & (RopeBaddie_FXFLAG_1_Shatter | RopeBaddie_FXFLAG_2_Frost)) {
        //@recomp: apply an icy tint too, like the SharpClaws
        if (objData->freezeTimer > 0) {
            if (objData->freezeTimer < 60) {
                opacity = 0x9B * ((f32)objData->freezeTimer / 60.0f);
            } else {
                opacity = 0x9B;
            }
            objprintSetBlendColor(0x64, 0xFF, 0xFF, opacity);
        }

        objprintDrawModel(self, gdl, mtxs, vtxs, pols, 1.0f);

        //Create glow effect
        if (objData->freezeTimer >= 60) {
            gDLL_32_modelfx->vtbl->func2(self, PARTICLE_52A, NULL);
        }

        //Thaw/shatter effects
        if (objData->fxFlags & RopeBaddie_FXFLAG_1_Shatter) {
            objData->fxFlags &= ~RopeBaddie_FXFLAG_1_Shatter;
            
            fxTransform.transl.x = self->srt.transl.x;
            fxTransform.transl.y = self->srt.transl.y - 20.0f;
            fxTransform.transl.z = self->srt.transl.z;
            fxTransform.scale = 1.0f;

            // if (baddie->fsa.hitpoints <= 0) {
            //     self->opacity = 1;
            // }

            if (rsModGfxDLL == NULL) {
                rsModGfxDLL = dllLoad(DLL_ID_107, 1);
            }
            
            if (rsModGfxDLL) {
                //TODO: tint the polygons an icy colour! It looks way too brutal when it's the same texture as the character
                rsModGfxDLL->vtbl->func0(self, 0xE, &fxTransform, 0, 0, 0);
            }
        } else if (objData->fxFlags & RopeBaddie_FXFLAG_2_Frost) {
            objData->fxFlags &= ~RopeBaddie_FXFLAG_2_Frost;

            fxTransform.transl.x = self->srt.transl.x;
            fxTransform.transl.y = self->srt.transl.y - 20.0f;
            fxTransform.transl.z = self->srt.transl.z;
            fxTransform.scale = 2.0f;

            for (u8 i = 0; i < 10; i++) {
                gDLL_17_partfx->vtbl->spawn(self, PARTICLE_56, &fxTransform, 0x200001, -1, colour);
            }
        }

        return;
    }

    //Regular draw
    {
        objprintDrawModel(self, gdl, mtxs, vtxs, pols, 1.0f);

        if (baddie->unk3B2 & (0x40 | 0x20)) {
            if (baddie->unk3B2 & 0x20) {
                gDLL_32_modelfx->vtbl->func2(self, PARTICLE_330, &baddie->unk3E8);
                gDLL_32_modelfx->vtbl->func2(self, PARTICLE_330, &baddie->unk3E8);
            }
            gDLL_32_modelfx->vtbl->func2(self, PARTICLE_32F, &baddie->unk3E8);
        }
        if (baddie->unk3B2 & 0x100) {
            gDLL_32_modelfx->vtbl->func2(self, PARTICLE_333, &baddie->unk3E8);
            gDLL_32_modelfx->vtbl->func2(self, PARTICLE_334, &baddie->unk3E8);
            baddie->unk3B2 &= ~0x100;
        }
    }
}

RECOMP_PATCH void RopeBaddie_obj_Free(Object* self, s32 onlySelf) {
    Baddie* baddie = self->data;
    objFreeObjectType(self, OBJTYPE_Baddie);
    dll_BaddieControl->free(self, baddie, 0);

    /* RECOMP */
    if (rsModGfxDLL) {
        dllFree(rsModGfxDLL);
        rsModGfxDLL = NULL;
    }

    //Stop sounds
    RopeBaddie_DataActual* objData = baddie->objdata;
    if (objData->soundHandleFrozen) {
        dll_amSfx->Stop(objData->soundHandleFrozen);
        objData->soundHandleFrozen = 0;
    }
}

RECOMP_PATCH void RopeBaddie_searchForTarget(Object* self, Baddie* baddie, ObjFSA_Data* fsa) {
    Object* target;
    /* RECOMP */
    Object* player = objGetPlayer();

    self->objhitInfo->unk58 &= ~ObjHitInfo_FLAG_1;

    //@recomp: optionally don't target the player until they're finished climbing onto the rope
    //(since it can be annoying to be knocked off before you even have a chance to evade)
    if (configs_GetRopeBaddieCombatEdits() && player) {
        Player_Data* playerData = player->data;
        if (playerData && playerData->unk0.animState != PLAYER_ASTATE_Rope_Climb) {
            return;
        }
    }

    target = dll_BaddieControl->func17(self, fsa, baddie->unk3E2, M_180_DEGREES);
    if (target != NULL) {
        fsa->target = target;
        fsa->unk33D = 0;
    }
}

/* Reset disengage timer when attacked, and finish Rare's freeze effect (I'm guessing that's what it was for, anyway!) */
RECOMP_PATCH void RopeBaddie_handleMessagesAndDamage(Object* self, Baddie* baddie, ObjFSA_Data* fsa) {
    s32 damageType;
    /* RECOMP */
    RopeBaddie_DataActual* objData = baddie->objdata;

    //Handle messages
    dll_BaddieControl->func20(self, fsa, &baddie->unk34C, baddie->unk39E, &baddie->unk3B4, 0, 0, 0);

    //Check for damage
    damageType = dll_BaddieControl->check_hit(self, fsa, &baddie->unk34C, baddie->unk39E, dHitAnimStateMap, dHitDamageMap, RopeBaddie_LSTATE_3_Hit, &baddie->unk3A8, NULL);
    if (damageType == Damage_Type_Projectile) {
        baddie->unk3B4 = 2;
        fsa->target = objGetPlayer();
    } else if (damageType == Damage_Type_Ice_Blast) {
        //@recomp: handle becoming frozen
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, RopeBaddie_ASTATE_7_Hit);

        if (objData) {
            objData->freezeTimer = 400;
            objData->fxFlags |= RopeBaddie_FXFLAG_2_Frost;
        }
    }

    //@recomp: reset disengage timer when attacked
    if (damageType && objData) {        
        objData->disengageTimer = PLAYER_DISENGAGE_DURATION;
    }
}

/* Don't try to attack if the player isn't nearby. */
RECOMP_PATCH s32 RopeBaddie_animState1Running(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    RopeBaddie_DataActual* objData;
    Baddie* baddie;

    baddie = self->data;
    objData = baddie->objdata;

    if (fsa->enteredAnimState) {
        objAnimSet(self, RopeBaddie_MODANIM_0_Walk_LOOP, 0.0f, 0);
        fsa->unk33A = FALSE;
    }

    gDLL_18_objfsa->vtbl->func7(self, fsa, updateRate, 0);

    RopeBaddie_handleAnimRopeSound(self, fsa);

    dll_DFropenode(objData->rope)->func9(objData->rope, &objData->ropePosition, (1 - (objData->direction * 2)) * fsa->animTickDelta * 50.4f);

    if (RopeBaddie_clampRopePosition(objData)) {
        //@recomp: don't attack when the player's far away
        if (configs_GetRopeBaddieCombatEdits() == FALSE || fsa->targetDist < PLAYER_ATTACK_RANGE) {
            return FSA_NEXTSTATE_SYNC(RopeBaddie_ASTATE_6_Attack);
        } else {
            return FSA_NEXTSTATE_SYNC(RopeBaddie_ASTATE_4_Combat_Idle);
        }
    }

    self->srt.pitch = RopeBaddie_getPitchAngle(objData);

    return 0;
}

/* At the end of the combat idle, have a reduced chance of playing the bite animation next if the player isn't nearby. */
RECOMP_PATCH s32 RopeBaddie_animState4CombatIdle(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie;
    RopeBaddie_DataActual* objData;

    baddie = self->data;
    objData = baddie->objdata;

    if (fsa->enteredAnimState) {
        objAnimSet(self, RopeBaddie_MODANIM_5_Sway_LOOP, 0.0f, 0);
        fsa->unk33A = FALSE;
    }
    fsa->animTickDelta = 0.03f;

    self->srt.pitch = RopeBaddie_getPitchAngle(objData);

    if (fsa->unk33A) {
        if (configs_GetRopeBaddieCombatEdits() == FALSE) {
            return FSA_NEXTSTATE_SYNC(RopeBaddie_ASTATE_5_Bite);
        } else {
            if (fsa->targetDist < PLAYER_ATTACK_RANGE) {
                return FSA_NEXTSTATE_SYNC(RopeBaddie_ASTATE_5_Bite);
            } else if (fsa->target && fsa->target->controlNo == OBJCONTROL_Player && dll_player(fsa->target)->func45(fsa->target) == objData->rope) {
                //@recomp: If the player's far away and on the same rope, pursue them
                return FSA_NEXTSTATE_SYNC(RopeBaddie_ASTATE_1_Running);
            } else {
                //@recomp: if the player isn't nearby, have a 1/4 chance of biting and a 3/4 chance
                //of continuing combat idle (so you have a better chance to see the normally rarely-used combat idle animation!)
                if (!mathRnd(0, 3)) {
                    return FSA_NEXTSTATE_SYNC(RopeBaddie_ASTATE_5_Bite);
                } else {
                    self->animProgress = 0.0f;
                }
            }
        }
    }

    return 0;
}

/* Fix a bug where the animStates could sometimes temporarily get stuck at the end of this state. */
s32 RopeBaddie_animState5Bite(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie;
    RopeBaddie_DataActual* objData;

    baddie = self->data;
    objData = baddie->objdata;

    if (fsa->enteredAnimState) {
        objAnimSet(self, RopeBaddie_MODANIM_6_Bite, 0.0f, 0);
        fsa->unk33A = FALSE;
    }
    fsa->animTickDelta = 0.03f;

    gDLL_18_objfsa->vtbl->func12(self, fsa, 0, 0, dBiteSounds);
    self->srt.pitch = RopeBaddie_getPitchAngle(objData);

    //@recomp: go to another animState, so the Baddie doesn't get stuck here
    if (fsa->unk33A) {
        return FSA_NEXTSTATE_SYNC(RopeBaddie_ASTATE_4_Combat_Idle);
    }

    return 0;
}

/* Wait a little bit before applying damaging collision, so the Baddie doesn't cause damage during the attack windup */
RECOMP_PATCH s32 RopeBaddie_animState6Attack(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    RopeBaddie_DataActual* objData;
    Baddie* baddie;
    /* RECOMP */
    u8 useCombatEdits = configs_GetRopeBaddieCombatEdits();

    baddie = self->data;
    objData = baddie->objdata;

    //@recomp: optionally don't activate collision until the actual attacking part of the animation, to give more of a chance to evade
    if (useCombatEdits == FALSE || self->animProgress > 0.7f) {
        self->objhitInfo->unk5F = 9;
        self->objhitInfo->unk60 = 1;
        func_80028D2C(self);
    }

    if (mathRnd(0, 100) < 50) {
        if (fsa->enteredAnimState) {
            objAnimSet(self, RopeBaddie_MODANIM_1_Tackle, 0.0f, 0);
            fsa->unk33A = FALSE;
        }
    } else {
        if (fsa->enteredAnimState) {
            objAnimSet(self, RopeBaddie_MODANIM_4_Punch, 0.0f, 0);
            fsa->unk33A = FALSE;
        }
    }

    //@recomp: optionally wind up up the attack a little more slowly, so the player has a fairer chance of getting away
    if (useCombatEdits && self->animProgress < 0.5f) {
        fsa->animTickDelta = 0.02f;
    } else {
        fsa->animTickDelta = 0.03f;
    }

    gDLL_18_objfsa->vtbl->func7(self, fsa, updateRate, 1);
    gDLL_18_objfsa->vtbl->func12(self, fsa, 0, mathRnd(0, ARRAYCOUNT(dAttackSoundIDs) - 1), dAttackSoundIDs);

    dll_DFropenode(objData->rope)->func9(objData->rope, &objData->ropePosition, (1 - (objData->direction * 2)) * fsa->unk278);

    RopeBaddie_clampRopePosition(objData);
    self->srt.pitch = RopeBaddie_getPitchAngle(objData);

    if (fsa->unk33A) {
        return FSA_NEXTSTATE_SYNC(RopeBaddie_ASTATE_4_Combat_Idle);
    } else {
        return 0;
    }
}

/* Fix a situation where the RopeBaddie could temporarily get stuck at the end of the bite animState */
RECOMP_PATCH s32 RopeBaddie_logicState0Top(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Object* target;
    RopeBaddie_DataActual* objData;
    u16 turnAmount;
    s16 yawDiff;
    u16 distance;
    Baddie* baddie;

    target = fsa->target;
    baddie = self->data;
    objData = baddie->objdata;

    if (target != NULL) {
        if ((dll_player(target)->func45(target) == objData->rope) && (fsa->animState != RopeBaddie_ASTATE_2_Turning) && ((4.0f * gUpdateRateF) < fsa->logicStateTime)) {
            dll_BaddieControl->func4(self, fsa->target, 0x10, &turnAmount, &yawDiff, &distance);
            if (turnAmount < 4 || turnAmount >= 12) {
                return FSA_NEXTSTATE_SYNC(RopeBaddie_LSTATE_2_Chase);
            }

            gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, RopeBaddie_ASTATE_2_Turning);
            fsa->animTickDelta = 0.028f;
            fsa->unk33A = FALSE;
        }
    } else {
        //@recomp: return to non-combat state if the target's lost 
        //(so the Baddie doesn't get stuck at the end of `RopeBaddie_ASTATE_5_Bite`)
        if (fsa->animState == RopeBaddie_ASTATE_5_Bite && fsa->unk33A) {
            gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, RopeBaddie_ASTATE_3_Idle);
        }
    }

    return 0;
}

/**
  * (Optional) Don't disengage the player immediately when they drop from the rope - instead use a short countdown.
  * This gives the player a chance to see some of the Baddie's combat animations that are otherwise difficult to encounter!
  */
RECOMP_PATCH s32 RopeBaddie_logicState2Chase(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    f32 distance;
    Object* target;
    f32 dx;
    f32 dz;
    s32 yawDiff;
    /* RECOMP */
    Baddie* baddie;
    RopeBaddie_DataActual* objData;
    u8 atEndOfRope;

    baddie = self->data;
    objData = baddie->objdata;

    target = fsa->target;

    //Return to non-combat state if the target's lost
    if (target == NULL) { //@recomp: don't change states as soon as the player leaves the rope
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, RopeBaddie_ASTATE_0_Walking);
        return FSA_NEXTSTATE_SYNC(RopeBaddie_LSTATE_0_Top);
    }

    if (fsa->animState != RopeBaddie_ASTATE_6_Attack) {
        //Get the angle to the player
        dx = self->srt.transl.x - target->srt.transl.x;
        dz = self->srt.transl.z - target->srt.transl.z;
        yawDiff = (mathAtan2f(dx, dz) - self->srt.yaw) & 0xFFFF;
        
        //Check if the player's behind the Baddie, or get their distance when they're in front
        if ((yawDiff > M_90_DEGREES) && (yawDiff < M_90_DEGREES * 3)) {
            dx = -100.0f;
        } else {
            dx = sqrtf(SQ(dx) + SQ(dz)) - 45.0f;
        }

        if (dx < 0.0f) {
            distance = -dx;
        } else {
            distance = dx;
        }

        //@recomp: check if the Baddie's at the end of the rope, and if so don't try to use the walking/running animState
        atEndOfRope = RopeBaddie_clampRopePosition(objData);

        //Attack when the player's close
        if ((fsa->targetDist < PLAYER_ATTACK_RANGE) &&  //@recomp
            distance < 1.0f && (fsa->animState == RopeBaddie_ASTATE_1_Running || (fsa->animState == RopeBaddie_ASTATE_5_Bite && fsa->unk33A))
        ) {
            gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, RopeBaddie_ASTATE_6_Attack);
        } else if (fsa->animState != RopeBaddie_ASTATE_1_Running) {
            //Chase forward
            if (!atEndOfRope && //@recomp
                (dx > 2.5f) && (fsa->animState != RopeBaddie_ASTATE_4_Combat_Idle) && (fsa->animState != RopeBaddie_ASTATE_5_Bite || fsa->unk33A)
            ) {
                gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, RopeBaddie_ASTATE_1_Running);
            }

            //Backpedal if the player is too close
            if (!atEndOfRope && //@recomp
                dx < -2.5f
            ) {
                gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, RopeBaddie_ASTATE_1_Running);
            }
        }

        if (fsa->animState == RopeBaddie_ASTATE_1_Running) {
            if (dx > 0.0f) {
                fsa->animTickDelta = 0.04f;
            } else {
                //Backpedal if the player is too close
                fsa->animTickDelta = -0.07f;
            }
        }
    }

    return 0;
}

/** Fix a bug where the RopeBaddie sometimes moonwalked (or moonclimbed I suppose?) just after respawning, because their yaw had reset. */
RECOMP_PATCH s32 RopeBaddie_logicState1Respawning(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    /* RECOMP */
    Baddie* baddie = self->data;
    RopeBaddie_DataActual* objData = baddie->objdata;

    if (fsa->enteredLogicState) {
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, RopeBaddie_ASTATE_9_Respawning);

        //@recomp: reconfigure yaw after respawn
        self->srt.yaw = objData->ropeYaw + ((objData->direction == 0) << 0xF);
    }

    if (fsa->unk33A) {
        //@recomp: turn if needed
        if (objData->direction != (objData->goalPosition <= objData->ropePosition)) {
            gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, RopeBaddie_ASTATE_2_Turning);
        }

        return FSA_NEXTSTATE_SYNC(RopeBaddie_LSTATE_0_Top);
    } else {
        return 0;
    }
}

/* Extend objData */
RECOMP_PATCH u32 RopeBaddie_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(Baddie) + sizeof(RopeBaddie_DataActual);
}

#ifdef DEBUG_FSA
RECOMP_HOOK_DLL(dll_224_obj_Control) void logRopeBaddieStates(Object* self) {
    static s32 prevAnimState = -1;
    static s32 prevLogicState = -1;

    static char animStateNames[][16] = {
        "A0_Walking",     
        "A1_Running",     
        "A2_Turning",     
        "A3_Idle",        
        "A4_Combat_Idle", 
        "A5_Bite",        
        "A6_Attack",      
        "A7_Hit",         
        "A8_Dying",       
        "A9_Respawning"   
    };

    static char logicStateNames[][16] = {
        "L0_Top",
        "L1_Respawning",
        "L2_Chase",
        "L3_Hit",
        "L4_Dying",
        "L5_Dead"
    };

    Baddie* baddie = self->data;
    RopeBaddie_DataActual* objData = baddie->objdata;
    Baddie_Setup* objSetup = (Baddie_Setup*)self->setup;

    //Immediate respawn
    objSetup->unk2C = 0;

    diPrintf("RopeBaddie %x:\n%s\n%s\n", self->setup->uID, animStateNames[baddie->fsa.animState], logicStateNames[baddie->fsa.logicState]);
}
#endif
