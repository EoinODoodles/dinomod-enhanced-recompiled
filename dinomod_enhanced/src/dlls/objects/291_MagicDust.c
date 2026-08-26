#include "configs.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "game/gamebits.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objmsg.h"
#include "sys/print.h"
#include "sys/rand.h"
#include "dll.h"
#include "dlls/objects/290_magicplant.h"
#include "dlls/objects/291_magicdust.h"

#include "recomp/dlls/objects/291_MagicDust_recomp.h"

#define DURATION_DELAY_FALL 60.0f
#define DURATION_LIFETIME 1800.0f
#define DURATION_VANISH 180.0f

#define DINOMOD_MAGIC_Y_MIN -13568.0f

/* The Magic Gem's shadows are removed in ROM patches, though they are used in recomp.
 * 
 * This define is just used to document where the code was changed.
 */
// #define DINOMOD_MAGIC_REMOVE_SHADOW

extern void MagicDust_collect(Object* self, Object* player, MagicDust_Data* objData);

/**
  * - Remove code setting shadow fade-in/fade-out flag (originally by MusicalProgrammer).
  * - Stop Magic Gem from falling past a certain point (originally by MusicalProgrammer).
  * - Stop stationary Magic Gems from rapidly playing sounds when touched (unloads them instead)
  */
RECOMP_PATCH void MagicDust_control(Object* self) {
    Object* player;
    MagicDust_Data* objData;
    f32 playerDistance;
    DLL27_Data* collision;
    Vec3f negativeS;
    Vec3f vReflect;
    Vec3f nSurface;
    f32 absSpeed;
    f32 pad[2];
    f32 volumeMultiplier;
    f32 distance;
    f32 coefficient;
    u8 i;
    u8 fxParam;
    u32 outMesgID;
    
    player = objGetPlayer();
    objData = self->data;
    collision = &objData->collision;
    
    //Wait for the "You have picked up a… MAGIC GEM!" sequence to end (if it's active)
    if (objData->flags & MagicDust_FLAG_Tutorial_Sequence) {
        while (objRecvMesg(self, &outMesgID, NULL, NULL)) {
            if (outMesgID == 0x7000B) {
                MagicDust_collect(self, player, objData);
                objData->flags &= ~MagicDust_FLAG_Tutorial_Sequence;
            }
        }
        
        if (objData->flags & MagicDust_FLAG_Tutorial_Sequence) {
            return;
        }
    }
    
    //Create glow effects
    if (self->unkDC == 0) {
        fxParam = 0;
        gDLL_17_partfx->vtbl->spawn(self, objData->fxIDGlow, NULL, 0x10002, -1, &fxParam);
        fxParam = 1;
        gDLL_17_partfx->vtbl->spawn(self, objData->fxIDGlow, NULL, 0x10002, -1, &fxParam);
        fxParam = 2;
        gDLL_17_partfx->vtbl->spawn(self, objData->fxIDGlow, NULL, 0x10002, -1, &fxParam);
        self->unkDC = 1;
    }
    
    //Handle behaviour when at rest
    if (objData->flags & MagicDust_FLAG_On_Ground) {
        //Spin
        self->srt.yaw += gUpdateRate << 8;

        //Play a ringing sound at intervals
        objData->soundTimer -= gUpdateRate;
        if (objData->soundTimer < 0) {
            dll_amSfx->Play(self, SOUND_BA1_MagicDust_Twinkle, MAX_VOLUME, NULL, NULL, 0, NULL);
            objData->soundTimer = mathRnd(240, 300);
        }
    }
    
    //Handle motion
    if (self->srt.flags & OBJFLAG_OWNS_SETUP) {
        //Do nothing when parented to MagicPlant
        if (self->unkC4 != NULL) {
            //@rom-patch: remove shadow updating (shadows are used in recomp, though!)
			#ifndef DINOMOD_ROM_PATCH
            self->shadow->flags |= OBJ_SHADOW_FLAG_FADE_OUT; 
			#endif

            gDLL_27->vtbl->reset(self, collision);
            return;
        }

        //@rom-patch: remove shadow updating (shadows are used in recomp, though!)
		#ifndef DINOMOD_ROM_PATCH
        self->shadow->flags &= ~OBJ_SHADOW_FLAG_FADE_OUT;
		#endif
        
        collision->mode = 1;

        //Apply drag and gravity when falling
        if (!(objData->flags & (MagicDust_FLAG_Delay_Fall | MagicDust_FLAG_On_Ground))) {
            self->velocity.x *= 0.99f;
            self->velocity.z *= 0.99f;
            self->velocity.y -= (0.1f * gUpdateRateF);
        }
        
        //Decrement timer
        objData->timer -= gUpdateRateF;

        //FALL DELAY
        if (objData->flags & MagicDust_FLAG_Delay_Fall) {
            //Set "fell" flag when fall delay timer's up
            if (objData->timer <= 0.0f) {
                objData->flags &= ~MagicDust_FLAG_Delay_Fall;
                objData->flags |= MagicDust_FLAG_Fell;
                objData->timer = DURATION_LIFETIME;
                self->opacity = OBJECT_OPACITY_MAX;
            }
            
            //Create sparkles (if not parented)
            if (self->parent == NULL) {
                gDLL_17_partfx->vtbl->spawn(self, objData->fxIDSparkle, NULL, 1, -1, NULL);
                gDLL_17_partfx->vtbl->spawn(self, objData->fxIDSparkle, NULL, 1, -1, NULL);
            }

        //FALLING/ON GROUND
        } else if (objData->flags & MagicDust_FLAG_Fell) {
            //Vanish when the gem's lifetime is up
            if (objData->timer <= 0.0f) {
                objData->flags &= ~MagicDust_FLAG_Fell;
                objData->flags |= MagicDust_FLAG_Vanish;
                objData->timer = DURATION_VANISH;
                gDLL_13_Expgfx->vtbl->func5(self);

                //Create lots of sparkles
                if (self->parent == NULL) {
                    for (i = 30; i != 0; i--) {
                        gDLL_17_partfx->vtbl->spawn(self, objData->fxIDSparkle, NULL, 1, -1, &i);
                    }
                }

                self->opacity = 1;
                dll_amSfx->Play(self, SOUND_B2B_Magic_Chime, MAX_VOLUME, NULL, NULL, 0, NULL);
            }

            //Move
            objMove(self, 
                self->velocity.x * gUpdateRateF, 
                self->velocity.y * gUpdateRateF, 
                self->velocity.z * gUpdateRateF
            );

			//@recomp: limit how far gems can fall
			if (self->srt.transl.y < DINOMOD_MAGIC_Y_MIN) {
				self->srt.transl.y = DINOMOD_MAGIC_Y_MIN;
			}
        } else {
            //Destroy self when vanish timer's up
            if (objData->timer <= 0.0f) {
                if (self->srt.flags & OBJFLAG_OWNS_SETUP) {
                    objFreeObject(self);
                } else {
                    objDisable(self);
                    self->srt.flags |= OBJFLAG_INVISIBLE;
                }
            }
            return;
        }
        
        //Handle falling
        if (!(objData->flags & (MagicDust_FLAG_Delay_Fall | MagicDust_FLAG_On_Ground))) {
            gDLL_27->vtbl->func_1E8(self, collision, gUpdateRateF);
            gDLL_27->vtbl->func_5A8(self, collision);
            gDLL_27->vtbl->func_624(self, collision, gUpdateRateF);

            //Bounce when colliding with ground
            if (collision->unk25D != 0) {
                negativeS.x = -self->velocity.x;
                negativeS.y = -self->velocity.y;
                negativeS.z = -self->velocity.z;
                absSpeed = VECTOR_MAGNITUDE(negativeS);

                //Play a "ting!" sound scaling volume by speed
                volumeMultiplier = absSpeed;
                if (absSpeed > 0.5f) {
                    if (absSpeed > 2.0f) {
                        volumeMultiplier = 2.0f;
                    }
                    dll_amSfx->Play(self, SOUND_66E_Ting, volumeMultiplier * 32.0f, NULL, NULL, 0, NULL);
                }
                
                //Reflect speed vector off surface normal (@recomp: make use of unused reflect calculation!)
				if (configs_GetMagicGemReflectBounce()) {
					//FANCY REFLECTED BOUNCE

                    //R = V - 2*(V ⋅ N)*N

                    //Get negative velocity unit vector
                    if (absSpeed != 0.0f) {
                        coefficient = 1.0f / absSpeed;
                        negativeS.f[0] *= coefficient;
                        negativeS.f[1] *= coefficient;
                        negativeS.f[2] *= coefficient;
                    }

                    //Get dot product between surface normal and negative velocity unit vector
                    nSurface.f[0] = collision->unk68.unk0[0].f[0];
                    nSurface.f[1] = collision->unk68.unk0[0].f[1];
                    nSurface.f[2] = collision->unk68.unk0[0].f[2];
                    coefficient = 2.0f * DOT_PRODUCT(negativeS, nSurface);

                    //Get reflected bounce vector
                    vReflect.f[0] = nSurface.f[0] * coefficient;
                    vReflect.f[1] = nSurface.f[1] * coefficient;
                    vReflect.f[2] = nSurface.f[2] * coefficient;
                    vReflect.f[0] -= negativeS.f[0];
                    vReflect.f[1] -= negativeS.f[1];
                    vReflect.f[2] -= negativeS.f[2];

					//@recomp: use reflected bounce
					self->velocity.x = vReflect.f[0]*absSpeed*0.85f;
					self->velocity.y = vReflect.f[1]*absSpeed*0.85f;
					self->velocity.z = vReflect.f[2]*absSpeed*0.85f;

					//Stop after 6 bounces off the ground
					if (nSurface.f[1] >= 0.707f) {
						objData->bounces++;
						if (objData->bounces >= 6) {
							objData->flags |= MagicDust_FLAG_On_Ground;
							self->velocity.x = 0;
							self->velocity.y = 0;
							self->velocity.z = 0;
						}
					}
                } else {
					//BASIC BOUNCE
                
					//@recomp: get vertical component of surface normal
					nSurface.f[1] = collision->unk68.unk0[0].f[1];

					//Bounce upwards if the ground is mostly level
					if (nSurface.f[1] >= 0.707f) {
						self->velocity.y = -self->velocity.y;
						self->velocity.y *= 0.85f;

						//Stop after 6 bounces
						objData->bounces++;
						if (objData->bounces >= 6) {
							objData->flags |= MagicDust_FLAG_On_Ground;
							self->velocity.x = 0;
							self->velocity.y = 0;
							self->velocity.z = 0;
						}
					//Otherwise bounce laterally and lose vertical momentum
					} else {
						self->velocity.x = -self->velocity.x;
						self->velocity.z = -self->velocity.z;
						self->velocity.x *= 0.85f;
						self->velocity.z *= 0.85f;
					}
				}
            }
        }
    }

    //@recomp: fix a bug where the stationary gems would rapidly play sounds while being touched
    /* TODO: double-check this is safe! 
       (I'm not sure what the "OBJFLAG_OWNS_SETUP" SRT flag does, or
       how/if it might interact negatively with freeing the gem here) */
    if (objData->flags & MagicDust_FLAG_Vanish) {
        objData->timer -= gUpdateRateF;
        if (objData->timer < 0.0f) {
            objFreeObject(self);
        }
        return;
    }

    //Collect the gem when close
    distance = self->srt.transl.y - player->srt.transl.y;
    if (distance < 0.0f) {
        distance = -distance;
    } 
    if (distance < 20.0f) {
        playerDistance = vec3DistanceXZSquared(&self->globalPosition, &player->globalPosition);
        distance = objData->collisionRadius + 8.0f;
        if (playerDistance < SQ(distance)) {
            //Display a tutorial box when the player first collects a Magic Crystal
            if (mainGetBits(BIT_Tutorial_Magic_Crystal) == 0) {
                gDLL_3_Animation->vtbl->set_variable_obj(objData->animObjectID, NULL, 0);
                outMesgID = 0;
                objSendMesg(player, 0x7000A, self, NULL);
                mainSetBits(BIT_Tutorial_Magic_Crystal, 1);
                objData->flags |= MagicDust_FLAG_Tutorial_Sequence;

                //@recomp: open queue so the player's reply is received 
                //(fixes a bug where magic didn't increase after the tutorial sequence)
                objInitMesgQueue(self, 1);
            } else {
                MagicDust_collect(self, player, objData);
            }
            
            self->opacity = 1;
            gDLL_13_Expgfx->vtbl->func5(self);
            for (i = 10; i != 0; i--) {
                gDLL_17_partfx->vtbl->spawn(self, objData->fxIDSparkle, NULL, 1, -1, NULL);
            }
            
            dll_amSfx->Play(self, objData->soundID, MAX_VOLUME, NULL, NULL, 0, NULL);
        }
    }
}

/**
  * @recomp: Fix a potential crash in the MagicPlant object's print function,
  * by clearing its references to this MagicDust object when freed.
  */
RECOMP_PATCH void MagicDust_free(Object* self, s32 onlySelf) {
	Object* parent;
	MagicPlant_Data* magicPlantData;

	//Check if the MagicDust is parented
	if (!self || !self->unkC4){
		return;
	}
	parent = self->unkC4;

	//Do nothing if the gem's parent object is deleted
	if (parent->stateFlags & OBJSTATE_DESTROYED){
		return;
	}

	//If the gem's parent is a MagicPlant, check if it's still referencing this gem
	if (parent->id == OBJ_MagicPlant){
		magicPlantData = parent->data;
		if (magicPlantData && magicPlantData->magic == self){
			//Clear the reference to avoid crashes in the MagicPlant's print function
			magicPlantData->magic = NULL;
		}
	}
}
