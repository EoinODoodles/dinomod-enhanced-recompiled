#include "math_util.h"
#include "modding.h"
#include "player_util.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "PR/gbi.h"
#include "macros.h"
#include "common.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "game/objects/interaction_arrow.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objanim.h"
#include "sys/objects.h"
#include "sys/objmsg.h"
#include "sys/print.h"
#include "sys/rand.h"
#include "dll.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/common/sidekick.h"
#include "dlls/objects/common/foodbag.h"
#include "dlls/objects/315_sidefoodbag.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/503_SHkillermushroom.h"

#include "recomp/dlls/objects/503_SHkillermushroom_recomp.h"

#define ANGRY_SLOW_SPORE_DELAY 70.0f
#define ANGRY_FAST_SPORE_DELAY 40.0f
#define MUSHROOM_DEFAULT_HEALTH 3
#define SHkillermushroom_FLAG_Delayed_Attack_Done 0x8

typedef struct {
    ObjSetup base;
	u8 health;				//@recomp: non-zero values allow mushroom to be defeated
    u8 attackCooldown;      //How long (in frames) until the mushroom leaves its attack outro state
    u16 regrowWaitDuration; //How long (in frames) until the mushroom starts regrowing after being collected/killed
    s16 gamebitAttacked;    //Optional gamebitID to set when the mushroom is damaged
    u8 aggroRadius;         //While in idle state, attacks the player with spores if they enter this radius
    u8 modelIdx;            //Mushroom design (only has 1 model, though)
} SHkillermushroom_Setup_Modified;

typedef struct {
    f32 timer;              //Cooldown timer for various behaviour (growing, attacking, etc.)
    f32 scaleMax;           //Maximum scale once grown (randomly varied by ±10%)
    f32 growDuration;       //How long it'll take for the mushroom to finish regrowing (after starting to scale up)
    f32 baseScale;          //Baseline scale, stored during setup
    f32 scaleSpeed;         //Rate of change of scale when growing
    s32 health;				//@recomp: repurposing unused struct member
    s32 goalAngle;			//@recomp: repurposing unused struct member
    s32 angle;				//@recomp: repurposing unused struct member 
    f32 attachX;            //Attach point coords for spore particles
    f32 attachY;            //Attach point coords for spore particles
    f32 attachZ;            //Attach point coords for spore particles
    f32 sporeInhaleRange;   //Player can be harmed by spores when inside this radius (grows during attack)
    f32 stunFxTimer;        //Countdown for periodically emitting particles while stunned
    u32 soundHandle;        //Controls the spore sound loop, and the stun sound loop
    s16 regrowWaitDuration; //How long (in frames) until the mushroom starts regrowing after being collected/killed
    u8 state;               //State Machine value
    u8 flags;               //Tracks various conditions for the State Machine (animation finished, vulnerable, can't hurt player)
} SHkillermushroom_Data_Modified;

extern s16 dStateModAnimIDs[];
extern f32 dStateAnimSpeeds[];

extern void SHkillermushroom_reset(Object* self, SHkillermushroom_Data_Modified* objData, int startAtZeroScale);

/** Handles switching the mushroom's hidden state config mid-gameplay */
static void SHkillermushroom_handle_config_change(Object* self, SHkillermushroom_Setup_Modified* objSetup, SHkillermushroom_Data_Modified* objData) {
    int showHiddenStates = recomp_get_config_u32("shkillermushroom_show_hidden_states");
    
    if (!self || !objSetup || !objData) {
        return;
    }

    //Don't affect custom mushroom instances that have a nonzero objSetup health value
    //(i.e. someone added a mushroom into a map and specifically want it to use the hidden states)
    if (objSetup->health > 0) {
        return;
    }

    //Handle option change
	if (
        ((objData->health > 0)  && (showHiddenStates == FALSE)) ||  //Hiding hidden states
        ((objData->health <= 0) && (showHiddenStates == TRUE))      //Showing hidden states
	) {
        objData->health = showHiddenStates ? MUSHROOM_DEFAULT_HEALTH : 0;
        if (objData->scaleMax == 0) {
            objData->scaleMax = (rand_next(-100, 100) * 0.001f) + objData->baseScale;
        }
        if (self->srt.scale < objData->scaleMax/2) {
		    self->srt.scale = objData->scaleMax;
        }
        self->opacity = OBJECT_OPACITY_MAX;
        self->srt.flags &= ~OBJFLAG_INVISIBLE;
		objData->state = SHkillermushroom_STATE_0_Idle;
		objData->flags = SHkillermushroom_FLAG_Vulnerable;
		func_8002674C(self);

        //Stop sound loop
        if (objData->soundHandle != 0) {
            gDLL_6_AMSFX->vtbl->stop(objData->soundHandle);
            objData->soundHandle = 0;
        }
	}
}

RECOMP_PATCH void SHkillermushroom_control(Object* self) {
    SHkillermushroom_Data_Modified* objData;
    Object* player;
    Object* hitBy;
    s32 hitSphereID;
    s32 hitDamage;
    f32 dz;
    f32 dx;
    f32 dy;
    s32 opacity;
    s32 pad;
    u16 playerDistance;
    u8 i;
    SHkillermushroom_Setup_Modified* objSetup;
    SRT fxTransform;
	/* RECOMP */
	s32 attackType;
    s32 yaw;

    objData = self->data;
    player = get_player();
    objSetup = (SHkillermushroom_Setup_Modified*)self->setup;

    func_80026160(self);
    self->unkAF |= ARROW_FLAG_8_No_Targetting;
    objData->flags |= SHkillermushroom_FLAG_Vulnerable;

	//@recomp: toggling hidden behaviour on mushrooms
    SHkillermushroom_handle_config_change(self, objSetup, objData);

    switch (objData->state) {
    case SHkillermushroom_STATE_6_Dying_Intro:
        //Inaccessible state: spraying spores, then toppling over and fading out!

        //Start spore sound loop
        if (objData->soundHandle == 0) {
            gDLL_6_AMSFX->vtbl->play(self, SOUND_53B_Spore_Spray_Loop, MAX_VOLUME, &objData->soundHandle, NULL, 0, NULL);
        }

        //Become invulnerable
        objData->flags &= ~SHkillermushroom_FLAG_Vulnerable;

        //Increase spore damage radius
        objData->sporeInhaleRange += INHALE_RADIUS_SPEED * gUpdateRateF;
        if (objData->sporeInhaleRange > INHALE_RADIUS_MAX) {
            objData->sporeInhaleRange = INHALE_RADIUS_MAX;
        }

        //Harm the player if they're in range
        if (!(objData->flags & SHkillermushroom_FLAG_Disable_Spore_Damage) &&
            (vec3_distance(&self->globalPosition, &player->globalPosition) <= objData->sporeInhaleRange) &&
            (((DLL_210_Player*)player->dll)->vtbl->func42(player) == 0) &&
            (((DLL_210_Player*)player->dll)->vtbl->func43(player) == 0)
        ) {
            func_8002635C(player, self, 0x15, 1, 0);
            objData->flags |= SHkillermushroom_FLAG_Disable_Spore_Damage;
        }

        //Go to topple outro when animation finished
        if (objData->flags & SHkillermushroom_FLAG_Animation_Finished) {
            objData->state = SHkillermushroom_STATE_2_Dying_Outro;
            objData->timer = 0.0f;

            //Stop sound loop
            if (objData->soundHandle != 0) {
                gDLL_6_AMSFX->vtbl->stop(objData->soundHandle);
                objData->soundHandle = 0;
            }
        }

        //Create spore particles
        fxTransform.transl.x = objData->attachX;
        fxTransform.transl.y = objData->attachY;
        fxTransform.transl.z = objData->attachZ;
        for (i = 5; i != 0; i--) {
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_3EB, &fxTransform, PARTFXFLAG_200000 | PARTFXFLAG_1, -1, NULL);
        }
        break;
    case SHkillermushroom_STATE_2_Dying_Outro:
        //Stay invulnerable
        objData->flags &= ~SHkillermushroom_FLAG_Vulnerable;

        //Fade out when animation finished
        if (objData->flags & SHkillermushroom_FLAG_Animation_Finished) {
            opacity = self->opacity - (gUpdateRate * 4);
            if (opacity < 0) {
                opacity = 0;
				objData->state = SHkillermushroom_STATE_10_Hidden; //@recomp: hidden state before regrow state, to handle collision
            }
            self->opacity = opacity;

            //Regrow
            // objData->timer += gUpdateRateF;
            // if (objData->timer > objData->regrowWaitDuration) {
            //     SHkillermushroom_reset(self, objData, TRUE);
            //     objData->state = SHkillermushroom_STATE_1_Regrow;
            // }
        }
        break;

    case SHkillermushroom_STATE_3_Spore_Attack_Intro:
        //Wait for attack intro animation to finish
        self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
        if (objData->flags & SHkillermushroom_FLAG_Animation_Finished) {
            objData->state = SHkillermushroom_STATE_4_Spore_Attack;
        }
        break;
    case SHkillermushroom_STATE_4_Spore_Attack:
        //Start spore sound loop
        if (objData->soundHandle == 0) {
            gDLL_6_AMSFX->vtbl->play(self, SOUND_53B_Spore_Spray_Loop, MAX_VOLUME, &objData->soundHandle, NULL, 0, NULL);
        }

        self->unkAF &= ~ARROW_FLAG_8_No_Targetting;

        //Increase spore damage radius
        objData->sporeInhaleRange += INHALE_RADIUS_SPEED * gUpdateRateF;

        //Harm the player if they're in range
        if (!(objData->flags & SHkillermushroom_FLAG_Disable_Spore_Damage) &&
            (vec3_distance(&self->globalPosition, &player->globalPosition) <= objData->sporeInhaleRange) &&
            (((DLL_210_Player*)player->dll)->vtbl->func42(player) == 0) &&
            (((DLL_210_Player*)player->dll)->vtbl->func43(player) == 0)
        ) {
            func_8002635C(player, self, 0x15, 1, 0);
            objData->flags |= SHkillermushroom_FLAG_Disable_Spore_Damage;
        }

        //Limit spore damage radius
        if (objData->sporeInhaleRange > INHALE_RADIUS_MAX) {
            objData->sporeInhaleRange = INHALE_RADIUS_MAX;
        }

        //End attack after two seconds
        objData->timer += gUpdateRateF;
        if (objData->timer > SPORE_ATTACK_DURATION) {
            objData->state = SHkillermushroom_STATE_5_Spore_Attack_Outro;
            objData->timer = 0.0f;

            //Stop sound loop
            if (objData->soundHandle != 0) {
                gDLL_6_AMSFX->vtbl->stop(objData->soundHandle);
                objData->soundHandle = 0;
            }
        }

        //Create spore particles
        fxTransform.transl.x = objData->attachX;
        fxTransform.transl.y = objData->attachY;
        fxTransform.transl.z = objData->attachZ;
        for (i = 5; i != 0; i--) {
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_3EB, &fxTransform, PARTFXFLAG_200000 | PARTFXFLAG_1, -1, NULL);
        }
        break;
    case SHkillermushroom_STATE_5_Spore_Attack_Outro:
        self->unkAF &= ~ARROW_FLAG_8_No_Targetting;

        //Return to idle after a waiting period (and after animation's finished)
        objData->timer += gUpdateRateF;
        if ((objData->timer > objSetup->attackCooldown) &&
            (objData->flags & SHkillermushroom_FLAG_Animation_Finished)
        ) {
            objData->state = SHkillermushroom_STATE_0_Idle;
            objData->flags &= ~SHkillermushroom_FLAG_Disable_Spore_Damage;
            objData->sporeInhaleRange = 0.0f;
        }
        break;

    case SHkillermushroom_STATE_1_Regrow:
        //Regrow after being collected/killed

        //Stay invulnerable
        objData->flags &= ~SHkillermushroom_FLAG_Vulnerable;

        //Limit scale speed
        if (self->srt.scale > objData->scaleMax) {
            objData->scaleSpeed /= 1.1f;
        }
        if (objData->scaleSpeed < 0.00001f) {
            objData->scaleSpeed = 0.0f;
        }

        //Increase timer and scale
        objData->timer += gUpdateRateF;
        self->srt.scale += objData->scaleSpeed * gUpdateRateF;

        //Return to idle once grown
        if (objData->timer > objData->growDuration) {
            objData->state = SHkillermushroom_STATE_0_Idle;
        }
        break;

    case SHkillermushroom_STATE_9_Stunned:
        //Configure LockIcon, start stunned sound loop, and pick stun duration
        if (objData->timer <= 0.0f) {
            func_80023BF8(self, 0x19, 0, 0, 0, 6);
            gDLL_6_AMSFX->vtbl->play(self, SOUND_745_Mushroom_Stunned_Loop, MAX_VOLUME, &objData->soundHandle, NULL, 0, NULL);
            objData->timer = rand_next(240, 300);
        }

        //Run down stun timer
        objData->timer -= gUpdateRateF;

        //Return to idle once stun wears off
        if (objData->timer <= 0.0f) {
			objData->timer = 0.0f; //@recomp

            gDLL_13_Expgfx->vtbl->func4(self);

			//@recomp: take a detour to idle through angry state
			if (objData->health > 0) {
				if (objData->health == 1) {
            		objData->state = SHkillermushroom_STATE_7_Angry_Fast;
				} else {
            		objData->state = SHkillermushroom_STATE_8_Angry_Slow;
				}

				objData->flags &= ~(SHkillermushroom_FLAG_Disable_Spore_Damage | SHkillermushroom_FLAG_Delayed_Attack_Done);
				objData->sporeInhaleRange = 0.0f;

				//Anticipate attack with sound
				gDLL_6_AMSFX->vtbl->play(self, SOUND_53A_Spore_Spray_Intro, MAX_VOLUME, NULL, NULL, 0, NULL);
			} else {
				objData->state = SHkillermushroom_STATE_0_Idle;
			}

            func_80023C6C(self);

            //Stop sound loop
            if (objData->soundHandle != 0) {
                gDLL_6_AMSFX->vtbl->stop(objData->soundHandle);
                objData->soundHandle = 0;
            }

        //If still stunned, emit particles periodically
        } else {
            objData->stunFxTimer -= gUpdateRateF;
            if (objData->stunFxTimer <= 0.0f) {
                fxTransform.transl.x = 14.0f;
                fxTransform.transl.y = 25.0f;
                gDLL_17_partfx->vtbl->spawn(self, PARTICLE_51D, &fxTransform, PARTFXFLAG_2, -1, NULL);
                objData->stunFxTimer = 20.0f;
            }

            //Handle player collecting mushroom (@incomplete: doesn't add any inventory items)
            if (self->unkAF & ARROW_FLAG_1_Interacted) {
                //Send message to player, displaying Red Mushroom's tutorial box
                //@bug: should use unique Red Mushroom tutorial gamebit, not the Blue Mushroom gamebit?
                obj_send_mesg(player,
                    0x7000A,
                    self,
                    (void*)BIT_Tutorial_Collected_Blue_Mushroom
                );

                objData->state = SHkillermushroom_STATE_10_Hidden;

                //Stop sound loop
                if (objData->soundHandle != 0) {
                    gDLL_6_AMSFX->vtbl->stop(objData->soundHandle);
                    objData->soundHandle = 0;
                }
            }

            self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
        }
        break;

    case SHkillermushroom_STATE_8_Angry_Slow: //@recomp: handle state
    case SHkillermushroom_STATE_7_Angry_Fast:
        //Stay invulnerable
        objData->flags &= ~SHkillermushroom_FLAG_Vulnerable;

        //Wait a moment before attack
        objData->timer += gUpdateRateF;
        if ((objData->flags & SHkillermushroom_FLAG_Delayed_Attack_Done) == FALSE) {
			if (((objData->state == SHkillermushroom_STATE_8_Angry_Slow) && (objData->timer < ANGRY_SLOW_SPORE_DELAY)) ||
				((objData->state == SHkillermushroom_STATE_7_Angry_Fast) && (objData->timer < ANGRY_FAST_SPORE_DELAY))
			) {
				break;
			}

			objData->flags |= SHkillermushroom_FLAG_Delayed_Attack_Done;
			objData->timer = 0.0f;
		}

        //Start spore sound loop
        if (objData->soundHandle == 0) {
            gDLL_6_AMSFX->vtbl->play(self, SOUND_53B_Spore_Spray_Loop, MAX_VOLUME, &objData->soundHandle, NULL, 0, NULL);
        }

        self->unkAF &= ~ARROW_FLAG_8_No_Targetting;

        //Increase spore damage radius
        objData->sporeInhaleRange += INHALE_RADIUS_SPEED * gUpdateRateF;

        //Limit spore damage radius
        if (objData->sporeInhaleRange > INHALE_RADIUS_MAX) {
            objData->sporeInhaleRange = INHALE_RADIUS_MAX;
        }

        //Harm the player if they're in range
        if (!(objData->flags & SHkillermushroom_FLAG_Disable_Spore_Damage) &&
            (vec3_distance(&self->globalPosition, &player->globalPosition) <= objData->sporeInhaleRange) &&
            (((DLL_210_Player*)player->dll)->vtbl->func42(player) == 0) &&
            (((DLL_210_Player*)player->dll)->vtbl->func43(player) == 0)
        ) {
            func_8002635C(player, self, 0x15, 1, 0);
            objData->flags |= SHkillermushroom_FLAG_Disable_Spore_Damage;
			objData->timer = 0.0f;
        }

		//Return to idle once finished
        if (objData->flags & SHkillermushroom_FLAG_Animation_Finished) {
			objData->flags |= (SHkillermushroom_FLAG_Vulnerable | SHkillermushroom_FLAG_Disable_Spore_Damage);
			
            //Stop sound loop
            if (objData->soundHandle != 0) {
				gDLL_6_AMSFX->vtbl->stop(objData->soundHandle);
                objData->soundHandle = 0;
            }

			if (objData->timer > objSetup->attackCooldown) {
				objData->state = SHkillermushroom_STATE_0_Idle;
				objData->flags &= ~(SHkillermushroom_FLAG_Disable_Spore_Damage | SHkillermushroom_FLAG_Delayed_Attack_Done);
				objData->sporeInhaleRange = 0.0f;
			}
        } else {
			//Create spore particles
			fxTransform.transl.x = objData->attachX;
			fxTransform.transl.y = objData->attachY - 5;
			fxTransform.transl.z = objData->attachZ;
			for (i = 5; i != 0; i--) {
				gDLL_17_partfx->vtbl->spawn(self, PARTICLE_3EB, &fxTransform, PARTFXFLAG_200000 | PARTFXFLAG_1, -1, NULL);
			}
		}

        break;

    case SHkillermushroom_STATE_10_Hidden:
        //Wait to regrow after being collected
        func_800267A4(self);

        objData->timer += gUpdateRateF;
        if (objData->timer > objData->regrowWaitDuration) {
            SHkillermushroom_reset(self, objData, TRUE);
            objData->state = SHkillermushroom_STATE_1_Regrow;
            func_80023C6C(self);
        }
        break;

    case SHkillermushroom_STATE_0_Idle:
    default:
        self->unkAF &= ~ARROW_FLAG_8_No_Targetting;

        dx = player->srt.transl.x - self->srt.transl.x;
        dy = player->srt.transl.y - self->srt.transl.y;
        dz = player->srt.transl.z - self->srt.transl.z;
        playerDistance = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));

        //Attack the player when they're nearby (player can avoid detection by sneaking)
        if (((playerDistance < objSetup->aggroRadius) && ((DLL_210_Player*)player->dll)->vtbl->func56(player) >= 0.54f) ||
			(playerDistance < (objSetup->aggroRadius / 4)) //@recomp: sneaking stops working when touching mushroom
        ) {
            objData->flags &= ~SHkillermushroom_FLAG_Disable_Spore_Damage;
            objData->state = SHkillermushroom_STATE_3_Spore_Attack_Intro;
            objData->timer = 0.0f;
            gDLL_6_AMSFX->vtbl->play(self, SOUND_53A_Spore_Spray_Intro, MAX_VOLUME, NULL, NULL, 0, NULL);
        }
        break;
    }

    //React to attacks
    if ((attackType = func_80025F40(self, &hitBy, &hitSphereID, &hitDamage)) && (objData->flags & SHkillermushroom_FLAG_Vulnerable)) {
        gDLL_6_AMSFX->vtbl->play(self, SOUND_744_Mushroom_Hit, MAX_VOLUME, NULL, NULL, 0, NULL);
        objData->flags &= ~SHkillermushroom_FLAG_Disable_Spore_Damage;

        //Optionally set a gamebit when the mushroom is attacked
        if (objSetup->gamebitAttacked != NO_GAMEBIT) {
            main_set_bits(objSetup->gamebitAttacked, TRUE);
        }

        objData->state = SHkillermushroom_STATE_9_Stunned;
        objData->timer = 0.0f;

		//@recomp: handle health
		if (objData->health == 1) {
			if (recomp_get_config_u32("shkillermushroom_show_hidden_states")) {
				objData->health = 3;
			} else {
				objData->health = objSetup->health;
			}

			objData->state = SHkillermushroom_STATE_6_Dying_Intro;

			//Fall backwards
			dx = player->srt.transl.x - self->srt.transl.x;
			dz = player->srt.transl.z - self->srt.transl.z;
			yaw = atan2f_to_s(-dx, -dz) + M_90_DEGREES;

            //If struck by weapon, fall in the direction of the strike
            if (player->id == OBJ_Sabre) {
                //Sabre
                if (attackType == 12) {
                    //Fall to right when struck by clockwise swing
                    yaw -= M_90_DEGREES;
                } else if ((attackType == 10) && (player->curModAnimId != 0x96)) { 
                    //Fall to left when struck by anticlockwise swing (not Z-lock jump-attack)
                    yaw += M_90_DEGREES;
                } 
            } else {
                //Krystal
                if ((attackType == 11) && (player->curModAnimId != 0x99)) {
                    //Fall to right when struck by clockwise swing (not overhead swing)
                    yaw -= M_90_DEGREES;
                } else if (attackType == 10) { 
                    //Fall to left when struck by anticlockwise swing
                    yaw += M_90_DEGREES;
                } 
            }

            //Randomise angle slightly
            yaw += rand_next(-M_20_DEGREES, M_20_DEGREES);

			//Wrap angle
            CIRCLE_WRAP(yaw);
            self->srt.yaw = yaw;
		}
		objData->health--;

        //Stop active sound loop
        if (objData->soundHandle != 0) {
            gDLL_6_AMSFX->vtbl->stop(objData->soundHandle);
            objData->soundHandle = 0;
        }
    }

    //Change animation if needed
    if (self->curModAnimId != dStateModAnimIDs[objData->state]) {
        func_80023D30(self, dStateModAnimIDs[objData->state], 0.0f, 0);
    }

    //Advance animation
    if (func_80024108(self, dStateAnimSpeeds[objData->state], gUpdateRateF, NULL)) {
        objData->flags |= SHkillermushroom_FLAG_Animation_Finished;
    } else {
        objData->flags &= ~SHkillermushroom_FLAG_Animation_Finished;
    }
}

RECOMP_PATCH void SHkillermushroom_reset(Object* self, SHkillermushroom_Data_Modified* objData, int startAtZeroScale) {
    SHkillermushroom_Setup_Modified* objSetup = (SHkillermushroom_Setup_Modified*)self->setup;

    self->srt.roll = rand_next(-1500, 1500);
    self->srt.pitch = rand_next(-1500, 1500);
    self->srt.yaw = rand_next(-1500, 1500);
    self->opacity = OBJECT_OPACITY_MAX;
    self->srt.flags &= ~OBJFLAG_INVISIBLE;
    self->srt.transl.x = objSetup->base.x;
    self->srt.transl.y = objSetup->base.y;
    self->srt.transl.z = objSetup->base.z;

    if (startAtZeroScale) {
        self->srt.scale = 0.00001f;
        objData->timer = 0.0f;
        objData->growDuration = rand_next(0, 100) + 200.0f;
        objData->scaleMax = (rand_next(-100, 100) * 0.001f) + objData->baseScale;
        objData->scaleSpeed = objData->scaleMax / objData->growDuration;
    }

    //@recomp: reset health
	if (recomp_get_config_u32("shkillermushroom_show_hidden_states")) {
		objData->health = MUSHROOM_DEFAULT_HEALTH;
	} else {
		objData->health = objSetup->health;
	}

    func_8002674C(self);
    func_800264D0(self);
}

RECOMP_PATCH u32 SHkillermushroom_get_data_size(Object *self, u32 a1) {
    return sizeof(SHkillermushroom_Data_Modified);
}
