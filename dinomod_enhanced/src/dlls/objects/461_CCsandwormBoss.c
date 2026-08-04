#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "dlls/objects/common/sidekick.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/objanim.h"
#include "sys/rand.h"

extern void func_80026160(Object* obj);

#include "recomp/dlls/objects/461_CCsandwormBoss_recomp.h"

typedef struct {
    u8 state;                       //State Machine value
    u8 particleTickCount;           //Extra particles should be created for this many ticks after emerging from the sand
    u8 isUnderHome;                 //SandWorm Boss is under the sand, and directly under its home position
    u8 facePlayerDuringSequence;    //Boss rotates to face the player while the animCallback function is active
    Object* player;     // Krystal
    Object* sidekick;   // Kyte
    Object* seqObj;     // CCnewseqobj
    Object* barrel;
    f32 animSpeed;
    f32 timer;
    f32 flashRedTimer;
} CCsandwormBoss_Data;

typedef enum {
    /* PRE-BATTLE SHARPCLAW FIGHT */
    CCsandwormBoss_STATE_0_Leadup_Fighting_One_SharpClaw,
    CCsandwormBoss_STATE_1_Leadup_Fighting_Two_SharpClaw,
    CCsandwormBoss_STATE_2_Leadup_Already_Completed, //Used during setup to skip directly to the battle when revisiting
    CCsandwormBoss_STATE_3_Revisit_Restore_Fire_Crystal,

    /* MAIN BATTLE */
    CCsandwormBoss_STATE_4_Idle, //Emerged from sand, stationary, attacks player when nearby.
    CCsandwormBoss_STATE_5_Idle_Attacking_Krystal,
    CCsandwormBoss_STATE_6_Distracted_Attacking_Krystal,
    CCsandwormBoss_STATE_7_Distracted_Attacking_Kyte,
    CCsandwormBoss_STATE_8_Hurt_Attacking_Krystal,
    CCsandwormBoss_STATE_9_Distracted_by_Kyte,
    CCsandwormBoss_STATE_10_Diving_Under_Sand,
    CCsandwormBoss_STATE_11_Burrowing_Under_Sand,
    CCsandwormBoss_STATE_12_Underground_Attacking_Krystal, //Re-emerging from sand
    CCsandwormBoss_STATE_13_Hurt_by_Barrel, //Vulnerable
    CCsandwormBoss_STATE_14_Dying,
    CCsandwormBoss_STATE_15_Defeated
} CCsandwormBoss_States;

typedef enum {
    //Bank 0
    CCsandwormBoss_MODANIM_A_0_Clutching_at_Throat = 0,     //Raising claws to neck in pain (intended for after swallowing explosive barrel?)
    CCsandwormBoss_MODANIM_A_1_Fall_Over,
    CCsandwormBoss_MODANIM_A_2_Diving,                      //[BURROWING INTRO]
    CCsandwormBoss_MODANIM_A_3_Snap_Attack_Sweep,           //Clockwise sweep
    CCsandwormBoss_MODANIM_A_4_Snap_Attack_Sweep_and_Dive,  //Clockwise sweep
    CCsandwormBoss_MODANIM_A_5_Idle_LOOP,
    CCsandwormBoss_MODANIM_A_6_Snap_Attack_Forward,
    CCsandwormBoss_MODANIM_A_7_Double_Snap_Attack,
    CCsandwormBoss_MODANIM_A_8_Emerge_and_Snap_Attack,      //[BURROWING EXIT]
    CCsandwormBoss_MODANIM_A_9_Burrowing_Head_Peep_LOOP,    //[BURROWING] Head briefly breaches surface while burrowing in a sinusoidal cycle
    CCsandwormBoss_MODANIM_A_10_Headbutt_Attack,
    CCsandwormBoss_MODANIM_A_11_Emerge,                     //[BURROWING EXIT] 
    CCsandwormBoss_MODANIM_A_12_Burrowing_Peek,             //[BURROWING] Pop up partially, then dive back underground
    CCsandwormBoss_MODANIM_A_13_Dive_and_Peek,              //[BURROWING INTRO] Dive underground, then briefly peek back up
    CCsandwormBoss_MODANIM_A_14_Retreat_Underground,        //[BURROWING INTRO] Goes back to burrowing, without a dive
    CCsandwormBoss_MODANIM_A_15_Idle_Fidget_LOOP,           //Arches back slightly, and raises claws

    //Bank 1
    CCsandwormBoss_MODANIM_B_0_Claw_Swipe_Attack = 0x100,
    CCsandwormBoss_MODANIM_B_1_Dying,
    CCsandwormBoss_MODANIM_B_2_Boss_Intro_Emerging_from_Sand
} CCsandwormBoss_ModAnim;

typedef enum {
    CCnewseqobj_ObjSeq_4_Boss_Intro_Emerge = 4, //Emerging from the ground after defeating 3 SharpClaw
    CCnewseqobj_ObjSeq_5_Eating_Explosive_Barrel = 5, //Popping up, eating barrel, and being hurt by the explosion
    CCnewseqobj_ObjSeq_8_Eating_Player = 8 //Player eaten by boss (Game Over)
} CCsandwormBoss_CCSeqObjIndices;

typedef enum {
    CCsandwormBoss_ObjSeq_0_Dying = 0 //Dying, coughing up Fire Crystal
} CCsandwormBoss_BossSeqIndices;

extern u32 dAttackSoundIDs[];

extern void CCsandwormBoss_enter_idle_state(Object* self, CCsandwormBoss_Data* objData);
extern void CCsandwormBoss_attack(Object* self, Object* obj, CCsandwormBoss_Data* objData, s32 newState);
extern void CCsandwormBoss_enter_distracted_state(Object* self, CCsandwormBoss_Data* objData);
extern void CCsandwormBoss_turn_towards_object(Object* self, Object* obj);
extern void CCsandwormBoss_move_towards_point(Object* self, Vec3f* point, f32 speed);
extern void CCsandwormBoss_check_for_projectile_spell(Object* self, CCsandwormBoss_Data* objData);
extern void CCsandwormBoss_create_particles(Object* self, CCsandwormBoss_Data* objData);

RECOMP_PATCH void CCsandwormBoss_tick_battle(Object *self, CCsandwormBoss_Data *objData) {
    ObjSetup* setup;
    f32 dist;
  
    setup = self->setup;
    objData->timer += gUpdateRateF;
    dist = M_INFINITY_F;
    objGetNearestTypeTo(OBJTYPE_Pickup, self, &dist);

    // @recomp: Bail if player pointer isn't setup
    //          (original patch by MusicalProgrammer)
    if (objData->player == NULL) {
        return;
    }

    diPrintf("worm %d, barrel %d\n", (s32) vec3DistanceXZ(&self->globalPosition, &objData->player->globalPosition), (s32) dist);
    
    switch (objData->state) {
    case CCsandwormBoss_STATE_4_Idle:
        CCsandwormBoss_turn_towards_object(self, objData->player);

        //Attack the player when nearby
        if (vec3DistanceXZSquared(&self->globalPosition, &objData->player->globalPosition) < SQ(180)) {
            CCsandwormBoss_attack(self, objData->player, objData, CCsandwormBoss_STATE_5_Idle_Attacking_Krystal);
        
        //Become distracted when Kyte uses her Distract Command
        } else {
            if (((DLL_ISidekick*)objData->sidekick->dll)->vtbl->func24(objData->sidekick)) {
                diPrintf("kyte dist %d interest range 50.0F\n", (s32) vec3DistanceXZ(&self->globalPosition, &objData->sidekick->globalPosition));
                if (vec3DistanceXZSquared(&self->globalPosition, &objData->sidekick->globalPosition) < SQ(60)) {
                    CCsandwormBoss_enter_distracted_state(self, objData);
                    objData->timer = 0.0f;
                }
            }
        }
        break;
    case CCsandwormBoss_STATE_5_Idle_Attacking_Krystal:
        CCsandwormBoss_turn_towards_object(self, objData->player);
        if (self->animProgress > 0.95f) {
            CCsandwormBoss_enter_idle_state(self, objData);
        }
        break;
    case CCsandwormBoss_STATE_6_Distracted_Attacking_Krystal:
        CCsandwormBoss_turn_towards_object(self, objData->player);
        if (self->animProgress > 0.95f) {
            CCsandwormBoss_enter_distracted_state(self, objData);
        }
        break;
    case CCsandwormBoss_STATE_7_Distracted_Attacking_Kyte:
        CCsandwormBoss_turn_towards_object(self, objData->sidekick);
        if (self->animProgress > 0.95f) {
            CCsandwormBoss_enter_distracted_state(self, objData);
        }
        break;
    case CCsandwormBoss_STATE_8_Hurt_Attacking_Krystal:
        CCsandwormBoss_turn_towards_object(self, objData->player);
        if (self->animProgress > 0.95f) {
            objData->state = CCsandwormBoss_STATE_13_Hurt_by_Barrel;
            objData->animSpeed = 0.005f;
            objAnimSet(self, CCsandwormBoss_MODANIM_A_5_Idle_LOOP, 0, 0);
        }
        CCsandwormBoss_check_for_projectile_spell(self, objData);
        break;
    case CCsandwormBoss_STATE_9_Distracted_by_Kyte:
        CCsandwormBoss_turn_towards_object(self, objData->sidekick);

        if (objData->timer > 300.0f) {
            STUBBED_PRINTF("setting flight group to %d\n", 0x65);
            mainSetBits(BIT_Kyte_Flight_Curve, 0x65);
        } else {
            STUBBED_PRINTF("setting flight group to %d\n", 0xC3);
            mainSetBits(BIT_Kyte_Flight_Curve, 0xC3);
        }

        //Attack the player when they're in range
        if (vec3DistanceXZSquared(&self->globalPosition, &objData->player->globalPosition) < SQ(180)) {
            CCsandwormBoss_attack(self, objData->player, objData, CCsandwormBoss_STATE_6_Distracted_Attacking_Krystal);
        
        //Attack Kyte when she's in range
        } else if (vec3DistanceXZSquared(&self->globalPosition, &objData->sidekick->globalPosition) < SQ(180)) {
            CCsandwormBoss_attack(self, objData->sidekick, objData, CCsandwormBoss_STATE_7_Distracted_Attacking_Kyte);
        
        //Follow Kyte if she's still nearby or using the Distract Command
        } else if (
            (((DLL_ISidekick*)objData->sidekick->dll)->vtbl->func24(objData->sidekick)) || 
            (vec3DistanceXZSquared(&self->globalPosition, &objData->sidekick->globalPosition) < SQ(300))
        ) {
            objGetAnimChange(self, 1.5f, &objData->animSpeed);
            CCsandwormBoss_move_towards_point(self, &objData->sidekick->srt.transl, 1.5f);
        
        //Otherwise, return to idle state if the worm's base position is nearby
        } else if (vec3DistanceXZSquared(&self->globalPosition, (Vec3f* ) &setup->x) < SQ(100)) {
            CCsandwormBoss_enter_idle_state(self, objData);

        //Otherwise, stop being distracted and dive under the sand
        } else {
            objData->state = CCsandwormBoss_STATE_10_Diving_Under_Sand;
            objData->animSpeed = 0.01f;
            objAnimSet(self, CCsandwormBoss_MODANIM_A_2_Diving, 0, 0);
        }
        break;
    case CCsandwormBoss_STATE_10_Diving_Under_Sand:
        if (self->animProgress > 0.95f) {
            objData->state = CCsandwormBoss_STATE_11_Burrowing_Under_Sand;
        }
        break;
    case CCsandwormBoss_STATE_11_Burrowing_Under_Sand:
        //Check if the worm is exactly under its home position (in X/Z)
        if (((s32) setup->x == (s32) self->srt.transl.f[0]) && ((s32) setup->z == (s32) self->srt.transl.f[2])) {
            
            //Get distance to player
            dist = vec3DistanceXZSquared(&self->globalPosition, &objData->player->globalPosition);

            //Eat the player if they're directly above the home position (Game Over)
            if (dist < SQ(50)) {
                STUBBED_PRINTF("eat player sequence\n");

                objData->facePlayerDuringSequence = FALSE;
                gDLL_3_Animation->vtbl->start_obj_sequence(CCnewseqobj_ObjSeq_8_Eating_Player, objData->seqObj, -1);
            
            //Attack the player if they're close to the home position (but not fatally close)
            } else if (dist < SQ(180)) {
                STUBBED_PRINTF("attack player from under ground\n");

                objData->state = CCsandwormBoss_STATE_12_Underground_Attacking_Krystal;
                objData->animSpeed = 0.005f;
                objAnimSet(self, CCsandwormBoss_MODANIM_A_8_Emerge_and_Snap_Attack, 0, 0);
                dll_amSfx->Play(self, dAttackSoundIDs[mathRnd(0, 3)], MAX_VOLUME, NULL, NULL, 0, NULL);
                objData->isUnderHome = FALSE;
                objData->particleTickCount = 3;
            
            } else {
                //Otherwise, check if there's a barrel directly above the worm's home position
                dist = 50.0f;
                objData->barrel = objGetNearestTypeTo(OBJTYPE_Pickup, self, &dist);
                
                //If the barrel's not being held, eat it and become vulnerable
                if (objData->barrel && (gDLL_54_pickup->vtbl->get_state(objData->barrel->data) == PICKUP_NotHeld)) {
                    STUBBED_PRINTF("eat barrel\n");
                    
                    objData->state = CCsandwormBoss_STATE_13_Hurt_by_Barrel;
                    objData->facePlayerDuringSequence = FALSE;
                    objData->timer = 0.0f;
                    gDLL_3_Animation->vtbl->start_obj_sequence(CCnewseqobj_ObjSeq_5_Eating_Explosive_Barrel, objData->seqObj, -1);
                
                //If there's no edible barrel, just emerge from the sand
                } else {
                    STUBBED_PRINTF("get up without attack\n");

                    objData->state = CCsandwormBoss_STATE_12_Underground_Attacking_Krystal;
                    objData->animSpeed = 0.01f;
                    objAnimSet(self, CCsandwormBoss_MODANIM_A_11_Emerge, 0, 0);
                    objData->isUnderHome = FALSE;
                    objData->particleTickCount = 3;
                }
            }
        } else {
            //Return home
            CCsandwormBoss_move_towards_point(self, (Vec3f* ) &setup->x, 3.0f);
            if (vec3DistanceXZSquared(&self->globalPosition, (Vec3f* ) &setup->x) < SQ(100)) {
                objData->isUnderHome = TRUE;
            }
        }
        break;
    case CCsandwormBoss_STATE_12_Underground_Attacking_Krystal:
        CCsandwormBoss_turn_towards_object(self, objData->player);
        if (self->animProgress > 0.95f) {
            CCsandwormBoss_enter_idle_state(self, objData);
        }
        break;
    case CCsandwormBoss_STATE_13_Hurt_by_Barrel:
        //Move the barrel back to its base position
        if (objData->barrel != NULL) {
            setup = objData->barrel->setup;
            objData->barrel->srt.transl.f[0] = setup->x;
            objData->barrel->srt.transl.f[1] = setup->y;
            objData->barrel->srt.transl.f[2] = setup->z;

            STUBBED_PRINTF("barrel %x put to %f %f %f\n", setup->uID, &setup->x, &setup->y, &setup->z);
            objData->barrel = NULL;
        }

        //Return to idle after 50 seconds
        if (objData->timer > 3000.0f) {
            objData->state = CCsandwormBoss_STATE_4_Idle;
        
        //Attack the player when nearby
        } else if (vec3DistanceXZSquared(&self->globalPosition, &objData->player->globalPosition) < SQ(180)) {
            CCsandwormBoss_attack(self, objData->player, objData, CCsandwormBoss_STATE_8_Hurt_Attacking_Krystal);
        }

        //Check for finishing blow
        CCsandwormBoss_check_for_projectile_spell(self, objData);
        break;
    case CCsandwormBoss_STATE_14_Dying:
        objData->state = CCsandwormBoss_STATE_15_Defeated;
        self->srt.flags |= OBJFLAG_INVISIBLE;
        func_800267A4(self);
        func_80026160(self);
        mainSetBits(BIT_CC_SandWormBoss_Defeated, TRUE);
        break;
    case CCsandwormBoss_STATE_15_Defeated:
        return;
    }

    objAnimAdvance(self, objData->animSpeed, gUpdateRateF, 0);

    CCsandwormBoss_create_particles(self, objData);
}
