#include "configs.h"
#include "modding.h"
#include "object_util.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "dll.h"
#include "dlls/engine/18_objfsa.h"
#include "dlls/engine/33_BaddieControl.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/215_SharpClaw.h"
#include "game/objects/object.h"
#include "sys/gfx/modgfx.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/menu.h"
#include "sys/objects.h"
#include "sys/objhits.h"
#include "sys/objtype.h"
#include "sys/print.h"

#include "recomp/dlls/objects/215_SharpClaw_recomp.h"

//TEMPORARY DEFINES
#define SharpClaw_initFSACallbacks SharpClaw_func_0

typedef enum {
    SharpClaw_ASTATE_0_Idle,
    SharpClaw_ASTATE_1_Walk,
    SharpClaw_ASTATE_2, //Approach?
    SharpClaw_ASTATE_3, //Chase?
    SharpClaw_ASTATE_4_Hop_Forward,
    SharpClaw_ASTATE_5_Hop_Backward,
    SharpClaw_ASTATE_6_Hop_Left,
    SharpClaw_ASTATE_7_Hop_Right,
    SharpClaw_ASTATE_8_Turn_90_Right,
    SharpClaw_ASTATE_9_Turn_90_Left,
    SharpClaw_ASTATE_10_Strafe_Left,
    SharpClaw_ASTATE_11_Strafe_Right,
    SharpClaw_ASTATE_12_Taunt1,
    SharpClaw_ASTATE_13_Taunt2,
    SharpClaw_ASTATE_14_Taunt3,
    SharpClaw_ASTATE_15_Battle_Idle,
    SharpClaw_ASTATE_16_Attack_Anticlockwise,
    SharpClaw_ASTATE_17_Attack_Clockwise,
    SharpClaw_ASTATE_18_Attack_Overhead,
    SharpClaw_ASTATE_19_Hit,
    SharpClaw_ASTATE_20, //Entered through case 23 in dHitAnimStateMap?
    SharpClaw_ASTATE_21_Knocked_Down, //via Projectile Spell hit etc.
    SharpClaw_ASTATE_22_Getting_Up,
    SharpClaw_ASTATE_23_Dying,
    SharpClaw_ASTATE_24_Dead
} SharpClaw_AnimStates;

typedef enum {
    SharpClaw_LSTATE_0_Top,
    SharpClaw_LSTATE_1_Respawn,
    SharpClaw_LSTATE_2, //Idle/searching?
    SharpClaw_LSTATE_3, //Taunting?
    SharpClaw_LSTATE_4,
    SharpClaw_LSTATE_5,
    SharpClaw_LSTATE_6,
    SharpClaw_LSTATE_7_Hit,
    SharpClaw_LSTATE_8_Dodge,
    SharpClaw_LSTATE_9_Dying,
    SharpClaw_LSTATE_10_Dead,
    SharpClaw_LSTATE_11, //Engage/approach?
    SharpClaw_LSTATE_12_Attack,
    SharpClaw_LSTATE_13
} SharpClaw_LogicStates;

typedef enum {
    SharpClaw_FLAG_1 = 1,
    SharpClaw_FLAG_2 = 2,
    SharpClaw_FLAG_4 = 4,
    SharpClaw_FLAG_8 = 8,
    SharpClaw_FLAG_10 = 0x10,
    SharpClaw_FLAG_20 = 0x20,
    SharpClaw_FLAG_40 = 0x40,
    SharpClaw_FLAG_80_Vulnerable_During_Attack = 0x80 //Harder SharpClaw, parries attacks when not vulnerable
} SharpClaw_Flags_3B0;

typedef enum {
    SharpClaw_OTHERFLAG_1 = 1,
    SharpClaw_OTHERFLAG_2 = 2,
    SharpClaw_OTHERFLAG_4 = 4,
    SharpClaw_OTHERFLAG_8 = 8,
    SharpClaw_OTHERFLAG_10 = 0x10,
    SharpClaw_OTHERFLAG_20 = 0x20,
    SharpClaw_OTHERFLAG_40 = 0x40,
    SharpClaw_OTHERFLAG_80_Frozen = 0x80,
    SharpClaw_OTHERFLAG_100 = 0x100
} SharpClaw_OtherFlags_3B2;

#define dll_BaddieControl (gDLL_33_BaddieControl->vtbl)

#define SOUND_510_Ice_Shatter 0x510
//END OF TEMPORARY DEFINES

extern void SharpClaw_initFSACallbacks(void);
extern void SharpClaw_handleFootsteps(Object* self, Baddie* baddie, ObjFSA_Data* fsa);
extern void SharpClaw_func_2044(Object* self, SRT* fxTransform, s32 useModGfx);
extern s32 SharpClaw_areAnyAlliesAttacking(u8 message, Object* self);
extern void SharpClaw_func_E88(Object* self, Baddie* baddie, ObjFSA_Data* fsa);
extern void SharpClaw_func_14C0(Object* self, AnimObj_Data* animData, Baddie* baddie, ObjFSA_Data* fsa);
extern void SharpClaw_func_18EC(Object* self, Baddie* baddie, ObjFSA_Data* fsa);

/*0x0*/ extern s32 dHitAnimStateMap[];
/*0x94*/ extern s8 dHitDamageMap[];

/*0x0*/ extern SRT sFXTransform;

/* RECOMP: modGfxDLL for freeze shatter effect */
static DLL_IModgfx* rsModGfxDLL = NULL;

typedef struct {
    s8 unk0; //Index/count related to locking on?
    u8 maxTurnAcceleration;
    u8 turnAcceleration;
    f32 hitComboTimer;
    s16 hitCombo;
    u8 messageReceived;
    u16 turnAmount;
    s16 targetYawDiff;
    u16 targetDistance;
    s32 freezeTimer; //@recomp: use int
    /* RECOMP */      
    s32 prevFreezeTimer;      
    u32 soundHandleFrozen;    //Handles twinkle sounds while frozen
    u8 fxFlags;               //Handles particles when thawing/shattering
} SharpClaw_DataActual;

/* Custom */
typedef enum {
    SharpClaw_FXFLAG_1_Shatter = 1,
    SharpClaw_FXFLAG_2_Frost = 2
} SharpClaw_FXFlags;

RECOMP_PATCH void SharpClaw_ctor(void* dll) {
    SharpClaw_initFSACallbacks();

    //@recomp: change damage map so Ice Blast doesn't do damage by itself 
    //(follow-up attacks while frozen are lethal though, like SFA)
    dHitDamageMap[Damage_Type_Ice_Blast - 2] = 0;
}

/* Custom function for handling the Ice Blast freeze state */
static s32 SharpClaw_handleIceBlastFreeze(Object* self, Baddie* baddie, SharpClaw_DataActual* objData) {
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
            objData->fxFlags |= SharpClaw_FXFLAG_2_Frost;
        }
    }

    //Check for non-Ice blast damage, ending the freeze immediately
    damageType = dll_BaddieControl->check_hit(self, &baddie->fsa, &baddie->unk34C, baddie->unk39E, dHitAnimStateMap, dHitDamageMap, SharpClaw_LSTATE_7_Hit, &baddie->unk3A8, &sFXTransform);
    if (damageType > 0 && damageType != Damage_Type_Ice_Blast) {
        objData->freezeTimer = 0;
        objData->fxFlags |= SharpClaw_FXFLAG_1_Shatter;
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

/*
    Fix bugs related to the Ice Blast freeze state:
    - The SharpClaw used sometimes still move depending on when they were frozen.
    - They used die nearly instantly due to the rapid damage from the spell.
    - They used to leave behind their collision after dying to Ice Blast.
*/
RECOMP_PATCH void SharpClaw_obj_Control(Object* self) {
    Baddie* baddie;
    Baddie_Setup* objSetup;

    baddie = self->data;
    objSetup = (Baddie_Setup*)self->setup;

    if (menuGetCurrent() == MENU_TITLE_SCREEN) {
        return;
    }

    if (self->unkDC != 0) {
        //Wait to respawn
        if (gDLL_29_Gplay->vtbl->did_time_expire(objSetup->base.uID)) {
            dll_BaddieControl->setup(self, objSetup, baddie, 0x19, 0xE, 0x10E, 0x36, 20.0f);
            baddie->fsa.logicState = SharpClaw_LSTATE_1_Respawn;
            baddie->fsa.enteredLogicState = TRUE;
            self->opacity = 0;
            baddie->unk3B6 = 150;
        }
        return;
    }

    //@recomp: rework Ice Blast behaviour
    if (SharpClaw_handleIceBlastFreeze(self, baddie, baddie->objdata)) {
        return;
    }

    if (!(baddie->unk3B0 & SharpClaw_FLAG_4) && (self->unkE0 == 0)) {
        if (objSetup && !(baddie->unk3B0 & SharpClaw_FLAG_8)) {
            self->srt.transl.x = objSetup->base.x;
            self->srt.transl.y = objSetup->base.y;
            self->srt.transl.z = objSetup->base.z;
        }

        gDLL_3_Animation->vtbl->start_obj_sequence(objSetup->unk2E, self, -1);
        self->unkE0 = 1;
        return;
    }

    if (baddie->unk3B2 & SharpClaw_OTHERFLAG_2) {
        if (!(baddie->unk3B0 & SharpClaw_FLAG_4)) {
            dll_BaddieControl->func9(self, &baddie->fsa, &baddie->unk34C, baddie->unk39E, (s8*)&baddie->unk3B4, 0, 0, 0, 1);
            if (baddie->unk3B2 & SharpClaw_OTHERFLAG_4) {
                baddie->fsa.logicState = SharpClaw_LSTATE_13;
            }
        }

        baddie->unk3B0 &= ~SharpClaw_FLAG_4;
        baddie->unk3B2 &= ~SharpClaw_OTHERFLAG_2;

        if (baddie->fsa.hitpoints > 0) {
            func_8002674C(self);
        }
    }

    if (dll_BaddieControl->func11(self, baddie, 1)) {
        SharpClaw_func_E88(self, baddie, &baddie->fsa);
        if (baddie->unk3B4 == 2) {
            SharpClaw_func_14C0(self, 0, baddie, &baddie->fsa);
        } else {
            SharpClaw_func_18EC(self, baddie, &baddie->fsa);
        }
    }
}

/* Build on the freeze effect */
RECOMP_PATCH void SharpClaw_obj_Print(Object* self, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    Baddie* baddie;
    SharpClaw_DataActual* objData;
    /* RECOMP */
    static SRT fxTransform;
    static u8 colour[] = { 0xFC, 0xFF, 0xFF };
    u8 opacity;

    baddie = self->data;
    objData = baddie->objdata;

    if (!visibility || self->unkDC) {
        return;
    }

    //Tint blue/red while frozen or hurt
    if (objData->freezeTimer > 0.0f) {
        //@recomp: fade out icy tint during thaw
        if (objData->freezeTimer > 0) {
            if (objData->freezeTimer < 60) {
                opacity = 0x9B * ((f32)objData->freezeTimer / 60.0f);
            } else {
                opacity = 0x9B;
            }
            objprintSetBlendColor(0x64, 0xFF, 0xFF, opacity);
        }
    } else {
        if (baddie->unk3E8 != 0.0f) {
            objprintSetBlendColor(0xC8, 0, 0, baddie->unk3E8);
        }
    }

    objprintDrawModel(self, gdl, mtxs, vtxs, pols, 1.0f);

    //Thaw/shatter effects
    if (objData->fxFlags & SharpClaw_FXFLAG_1_Shatter) {
        objData->fxFlags &= ~SharpClaw_FXFLAG_1_Shatter;
        
        fxTransform.transl.x = self->srt.transl.x;
        fxTransform.transl.y = self->srt.transl.y + 15.0f;
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
    } else if (objData->fxFlags & SharpClaw_FXFLAG_2_Frost) {
        objData->fxFlags &= ~SharpClaw_FXFLAG_2_Frost;

        fxTransform.transl.x = self->srt.transl.x;
        fxTransform.transl.y = self->srt.transl.y + 15.0f;
        fxTransform.transl.z = self->srt.transl.z;
        fxTransform.scale = 1.0f;

        for (u8 i = 0; i < 10; i++) {
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_56, &fxTransform, 0x200001, -1, colour);
        }
    } else if (objData->freezeTimer >= 60) { //@recomp: change timing, so the effect fades out during the thaw
        //Create particles while frozen
        gDLL_32_modelfx->vtbl->func2(self, PARTICLE_52A, NULL);
    }

    if (baddie->unk3B2 & (SharpClaw_OTHERFLAG_40 | SharpClaw_OTHERFLAG_20)) {
        if (baddie->unk3B2 & SharpClaw_OTHERFLAG_20) {
            gDLL_32_modelfx->vtbl->func2(self, PARTICLE_330, &baddie->unk3E8);
            gDLL_32_modelfx->vtbl->func2(self, PARTICLE_330, &baddie->unk3E8);
        }
        gDLL_32_modelfx->vtbl->func2(self, PARTICLE_32F, &baddie->unk3E8);
    }

    if (baddie->unk3B2 & SharpClaw_OTHERFLAG_100) {
        gDLL_32_modelfx->vtbl->func2(self, PARTICLE_333, &baddie->unk3E8);
        gDLL_32_modelfx->vtbl->func2(self, PARTICLE_334, &baddie->unk3E8);
        baddie->unk3B2 &= ~SharpClaw_OTHERFLAG_100;
    }
}

RECOMP_PATCH void SharpClaw_obj_Free(Object* self, s32 onlySelf) {
    Baddie* baddie = self->data;

    objFreeObjectType(self, OBJTYPE_Baddie);
    objFreeObjectType(self, OBJTYPE_63);

    if (self->linkedObject != NULL) {
        objFreeObject(self->linkedObject);
        self->linkedObject = NULL;
    }

    dll_BaddieControl->free(self, baddie, 0x20);

    /* RECOMP */
    if (rsModGfxDLL) {
        dllFree(rsModGfxDLL);
        rsModGfxDLL = NULL;
    }

    //Stop sounds
    SharpClaw_DataActual* objData = baddie->objdata;
    if (objData->soundHandleFrozen) {
        dll_amSfx->Stop(objData->soundHandleFrozen);
        objData->soundHandleFrozen = 0;
    }
}

/* Flinch after being hit by Ice Blast (once thawed out) */
RECOMP_PATCH void SharpClaw_func_E88(Object* self, Baddie* baddie, ObjFSA_Data* fsa) {
    SharpClaw_DataActual* objData;
    s32 hit;
    s32 result;

    objData = baddie->objdata;

    if (self->linkedObject != NULL) {
        self->linkedObject->parent = self->parent;
    }

    dll_BaddieControl->func4(self, objGetPlayer(), 0x10, &objData->turnAmount, &objData->targetYawDiff, &objData->targetDistance);
    fsa->targetDist = objData->targetDistance;
    if (!(baddie->unk3B0 & SharpClaw_FLAG_20)) {
        dll_BaddieControl->func14(self, (Baddie*)fsa, &baddie->unk3B2, 9, 10, baddie->unk3A6, baddie->unk3A4);
    }
    dll_BaddieControl->change_weapon(self, baddie);

    SharpClaw_handleFootsteps(self, baddie, fsa);
    objExprEyeIdle(self, &baddie->unk3BC);

    result = dll_BaddieControl->func20(self, fsa, &baddie->unk34C, baddie->unk39E, &baddie->unk3B4, 0, 0, 0);
    if (result == 1) {
        baddie->unk3B2 |= SharpClaw_OTHERFLAG_4;
    } else if (result == 2) {
        baddie->unk3B2 &= ~SharpClaw_OTHERFLAG_4;
    }

    if ((fsa->animState == SharpClaw_ASTATE_16_Attack_Anticlockwise) ||
        (fsa->animState == SharpClaw_ASTATE_17_Attack_Clockwise) ||
        (fsa->animState == SharpClaw_ASTATE_18_Attack_Overhead)
    ) {
        func_80028D2C(self);
    } else {
        func_80026160(self);
    }

    objData->hitComboTimer += gUpdateRateF;

    if (baddie->unk3B0 & SharpClaw_FLAG_80_Vulnerable_During_Attack) {
        if ((fsa->animState == SharpClaw_ASTATE_16_Attack_Anticlockwise) ||
            (fsa->animState == SharpClaw_ASTATE_17_Attack_Clockwise) ||
            (fsa->animState == SharpClaw_ASTATE_18_Attack_Overhead) ||
            (baddie->unk3B2 & SharpClaw_OTHERFLAG_10)
        ) {
            hit = dll_BaddieControl->check_hit(self, fsa, &baddie->unk34C, baddie->unk39E, dHitAnimStateMap, dHitDamageMap, SharpClaw_LSTATE_7_Hit, &baddie->unk3A8, &sFXTransform);
            if (hit) {
                SharpClaw_func_2044(self, &sFXTransform, FALSE);
            }
        } else {
            hit = dll_BaddieControl->check_hit(self, fsa, &baddie->unk34C, baddie->unk39E, NULL, NULL, SharpClaw_LSTATE_7_Hit, &baddie->unk3A8, &sFXTransform);
            if (hit) {
                SharpClaw_func_2044(self, &sFXTransform, TRUE);
            }
        }
    } else {
        if (((fsa->animState == SharpClaw_ASTATE_15_Battle_Idle) || (fsa->animState == SharpClaw_ASTATE_5_Hop_Backward))
            && !(baddie->unk3B2 & SharpClaw_OTHERFLAG_10)
        ) {
            hit = dll_BaddieControl->check_hit(self, fsa, &baddie->unk34C, baddie->unk39E, NULL, NULL, SharpClaw_LSTATE_7_Hit, &baddie->unk3A8, &sFXTransform);
            if (hit) {
                SharpClaw_func_2044(self, &sFXTransform, TRUE);
            }
        } else {
            hit = dll_BaddieControl->check_hit(self, fsa, &baddie->unk34C, baddie->unk39E, dHitAnimStateMap, dHitDamageMap, SharpClaw_LSTATE_7_Hit, &baddie->unk3A8, &sFXTransform);
            if (hit) {
                SharpClaw_func_2044(self, &sFXTransform, FALSE);
            }
        }
    }

    //React to explosions/projectiles
    if ((hit == Damage_Type_Explosion) ||
        (hit == Damage_Type_E) ||
        (hit == Damage_Type_Projectile)
    ) {
        baddie->unk3B4 = 2;
        fsa->target = objGetPlayer();
        return;
    }

    //Become frozen by the Ice Blast Spell
    if (hit == Damage_Type_Ice_Blast) {
        baddie->unk3B4 = 2;
        objData->freezeTimer = 400;
        // baddie->unk3B2 |= SharpClaw_OTHERFLAG_80_Frozen; //@recomp: set this flag in the freezeTimer handling function instead

        //@recomp: set hit FSA states to cause a flinch after being frozen
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, SharpClaw_ASTATE_19_Hit);
        fsa->logicState = SharpClaw_LSTATE_7_Hit;
        return;
    }

    //Handle other kinds of damage
    if (hit) {
        baddie->unk3B4 = 2;

        //Count successive hits
        if (objData->hitComboTimer < 240.0f) {
            objData->hitCombo++;
        } else {
            objData->hitCombo = 0;
        }

        if (fsa->hitpoints > 0) {
            objData->hitComboTimer = 0.0f;

            //Dodge after being struck three times in succession
            if (objData->hitCombo >= 2) {
                objData->hitCombo = 0;
                fsa->logicState = SharpClaw_LSTATE_8_Dodge;
                fsa->enteredLogicState = TRUE;
                if (SharpClaw_areAnyAlliesAttacking(SharpClaw_MESSAGE_2_Had_Priority_Over_Allies, self)) {
                    objData->messageReceived = SharpClaw_MESSAGE_1_Others_Attacking;
                }
            } else if (SharpClaw_areAnyAlliesAttacking(SharpClaw_MESSAGE_2_Had_Priority_Over_Allies, self)) {
                objData->messageReceived = SharpClaw_MESSAGE_1_Others_Attacking;
            }
        }
    }
}

RECOMP_PATCH s32 SharpClaw_logicState7Hit(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    // SharpClaw_DataActual* objData;
    Baddie* baddie;

    baddie = self->data;
    // objData = baddie->objdata;

    // @recomp: move Ice Blast freeze logic to a function call in obj_Control, 
    // since the timer doesn't work properly if it's only decremented here.

    // if (baddie->unk3B2 & SharpClaw_OTHERFLAG_80_Frozen) {
    //     objData->freezeTimer -= gUpdateRate;
    //     if (objData->freezeTimer <= 0.0f) {
    //         objData->freezeTimer = 0.0f;
    //         baddie->unk3B2 &= ~SharpClaw_OTHERFLAG_80_Frozen;
    //         if (fsa->hitpoints > 0) {
    //             return FSA_NEXTSTATE_SYNC(SharpClaw_LSTATE_12_Attack);
    //         } else {
    //             return FSA_NEXTSTATE_SYNC(SharpClaw_LSTATE_9_Dying);
    //         }
    //     }
    // } else 
    if (fsa->hitpoints <= 0) {
        //Die when out of health
        return FSA_NEXTSTATE_SYNC(SharpClaw_LSTATE_9_Dying);
    } else if (fsa->unk33A) {
        if (fsa->animState == SharpClaw_ASTATE_21_Knocked_Down) {
            //Get up after being knocked down
            gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, SharpClaw_ASTATE_22_Getting_Up);
        } else if (fsa->hitpoints < mathRnd(2, 4)) {
            //Dodge back when at low health
            return FSA_NEXTSTATE_SYNC(SharpClaw_LSTATE_8_Dodge);
        } else {
            //Otherwise counterattack
            baddie->unk3B6 = 300;
            return FSA_NEXTSTATE_SYNC(SharpClaw_LSTATE_12_Attack);
        }
    }

    return 0;
}

/* Extend objData */
RECOMP_PATCH u32 SharpClaw_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(Baddie) + sizeof(SharpClaw_DataActual);
}
