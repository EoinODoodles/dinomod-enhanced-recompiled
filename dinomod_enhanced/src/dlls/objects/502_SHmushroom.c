#include "modding.h"
#include "player_util.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "PR/gbi.h"
#include "macros.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "game/objects/interaction_arrow.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objanim.h"
#include "sys/objects.h"
#include "sys/objmsg.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "sys/rand.h"
#include "sys/segment_53F00.h"
#include "dll.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/common/sidekick.h"
#include "dlls/objects/common/foodbag.h"
#include "dlls/objects/315_sidefoodbag.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/502_SHmushroom.h"
#include "dlls/objects/713_DRearthwalk.h"

#include "recomp/dlls/objects/502_SHmushroom_recomp.h"

/*
	CHANGES:

	Increment inventory gamebit immediately when a mushroom is collected, and set the 
	gamebit preventing the mushroom from appearing at the same time.
	(Previously it would first set the "mushroom doesn't reappear" gamebit, and then 
	there was a half-second or so delay between collecting the mushroom and incrementing the 
	inventory gamebit via player message. If the mushroom unloaded in between the two events, 
	you'd lose a White Mushroom permanently and be softlocked!)

 	Store a gamebitID and objectID for the item collection sequence/tutorial message box. 
	This ensures the item collection sequence's held item animObj isn't uninitialised when the 
	tutorial plays, and allows the Blue Mushrooms and White Mushrooms to have separate configs. 
	(TODO: White Mushrooms still use the Blue Mushroom's gamebit/animObjID currently, need to create unique ones for 'em!)

 	Fix a bug where the Find command would still appear near mushrooms while they're invisible/collected.

 	Delete the mushroom after it enters its hidden state, or when an EarthWalker eats it, or if it's already been collected.

 	Added an way to see mushrooms' hidden dance animation: they now have a random chance of dancing in their idle state!
	(Can be disabled/probabilities adjusted in the mod configs)
*/

typedef struct {
	UnkCurvesStruct curves;		//Curve-following data (e.g. for the lily pond White Mushroom in SwapStone Hollow's well)
	f32 pursuerDistance;        //Distance to player/sidekick
	f32 prevPursuerDistance;    //Distance to player/sidekick on previous tick
	f32 jumpSpeedMultiplier;    //Factor for speed while jumping
	f32 hopSpeedMultiplier;     //Factor for speed while hopping(?) Set during setup but otherwise unused.
	f32 jumpSpeed;              //Jump speed, extracted from jump animation's root motion data
	f32 hopSpeed;               //Hop speed, extracted from hop animation's root motion data
	f32 curvesDelta;            //Curve-following parameter (speed?)
	f32 stunnedTimer;           //Countdown for exiting stunned state
	f32 stunFxTimer;            //Countdown for periodically emitting particles while stunned
	u32 soundHandleStun;        //Controls bird tweeting sound loop while stunned
	s16 fleeAngle;              //Yaw to flee towards (different from mushroom's visual yaw, since that needs to be offset by 90 degrees during jump animation)
	s16 _unk132;
	s16 gamebitInventory;       //Gamebit to increment when collected
	u8 state;                   //State Machine value
	u8 flags;                   //Tracks various conditions for the State Machine (animation finished, moving, hurt, etc.)
	// s32 _unk138;
	/* RECOMP */
	s16 tutorialGamebit;		//Gamebit for the Blue/White mushroom's tutorial box
	s16 tutorialObjectID;		//animObj objectID to show during the item collection sequence
	s16 expireTimer;			//Deletes the mushroom after being hidden for a while
	s16 danceTimer;				//Timer for random chance of dancing while idling
	s8 spinSpeed;				//Which direction to rotate when dancing
	u8 firstTickDone;			//Tracks whether this is the first run through the mushroom's State Machine
} SHmushroom_Data_Extended;

/*0x0*/ static s16 dStateModAnimIDs[] = {
	SHmushroom_MODANIM_0_Idle_LOOP,         //SHmushroom_STATE_0_Idle
	SHmushroom_MODANIM_1_Jump,              //SHmushroom_STATE_1_Jump,
	SHmushroom_MODANIM_6_Dancing_LOOP,      //SHmushroom_STATE_2_Collected
	SHmushroom_MODANIM_2_Look_Intro,        //SHmushroom_STATE_3_Alert_Intro
	SHmushroom_MODANIM_3_Look_Idle_LOOP,    //SHmushroom_STATE_4_Alert
	SHmushroom_MODANIM_4_Look_Hop_LOOP,     //SHmushroom_STATE_5_Surprised
	SHmushroom_MODANIM_0_Idle_LOOP,         //SHmushroom_STATE_6_Trapped
	SHmushroom_MODANIM_5_Sigh,              //SHmushroom_STATE_7_Relieved_Sigh
	SHmushroom_MODANIM_6_Dancing_LOOP,      //SHmushroom_STATE_8_Hidden
	SHmushroom_MODANIM_7_Stunned_LOOP,      //SHmushroom_STATE_9_Stunned
	SHmushroom_MODANIM_6_Dancing_LOOP      //SHmushroom_STATE_10_Dance (@recomp: custom state)
};
/*0x14*/ static f32 dStateAnimSpeeds[] = {
	0.005f,   //SHmushroom_STATE_0_Idle
	0.01f,    //SHmushroom_STATE_1_Jump,
	0.005f,   //SHmushroom_STATE_2_Collected
	0.01f,    //SHmushroom_STATE_3_Alert_Intro
	0.01f,    //SHmushroom_STATE_4_Alert
	0.015f,   //SHmushroom_STATE_5_Surprised
	0.005f,   //SHmushroom_STATE_6_Trapped
	0.01f,    //SHmushroom_STATE_7_Relieved_Sigh
	0.005f,   //SHmushroom_STATE_8_Hidden
	0.012f,   //SHmushroom_STATE_9_Stunned
	0.005f,  //SHmushroom_STATE_10_Dance (@recomp: custom state)
};

#define SHmushroom_FLAG_Delete_after_Setup 0x20
#define SHmushroom_FLAG_Message_Sent_to_Player 0x40
#define SHmushroom_STATE_10_Dance 10 //@recomp: custom idle state
#define MUSHROOM_SNEAK_THRESHOLD 0.54f
#define DEBUG_MUSHROOM FALSE

extern s16 dStateModAnimIDs[];
extern f32 dStateAnimSpeeds[];

extern s16 SHmushroom_flee_from_player(Object* self, Object* fleeingFrom, SHmushroom_Data_Extended* objData, f32 distance);
extern s16 SHmushroom_flee_along_curve(Object* self, Object* fleeingFrom, SHmushroom_Data_Extended* objData, f32 distance);
extern void SHmushroom_tick_state_machine(Object* self, SHmushroom_Data_Extended* objData, SHmushroom_Setup* objSetup);

/**
  * Used during setup: stores the gamebitID for the mushroom's tutorial textbox,
  * and the animObj objectID to display during the item collection sequence.
  */
static void get_tutorial_gamebit_and_animObjID(Object* self, s16* tutorialGamebit, s16* tutorialObjectID) {
	switch (self->modelInstIdx) {
	case SHmushroom_MODEL_0_Blue_Mushroom:
		*tutorialGamebit = BIT_Tutorial_Collected_Blue_Mushroom;
		*tutorialObjectID = OBJ_SHmushroomanim;
		break;
	case SHmushroom_MODEL_1_White_Mushroom:
		*tutorialGamebit = BIT_Tutorial_Collected_Blue_Mushroom; //TODO: give this a unique gamebit
		*tutorialObjectID = OBJ_SHmushroomanim; //TODO: create a unique animObj with the White Mushroom model
		break;
	default:
		*tutorialGamebit = NO_GAMEBIT;
		*tutorialObjectID = -1;
		break;
	}
}

/** 
  * Splits the mushroom gamebit handling out as a separate reusable function,
  * ensuring the inventory count gamebit gets incremented at the same time as the
  * gamebit that prevents the mushroom from reappearing. 
  *
  * (Fixes a softlock where you could be missing mushrooms for Queen EarthWalker)
  */
static void add_to_inventory(Object* self, SHmushroom_Data_Extended* objData, SHmushroom_Setup* objSetup, Object* player, int showPopup) {
	Object* dinoFoodBag;
	s32 count;

	recomp_eprintf("ADDING TO INVENTORY: %s\n", (self->def ? self->def->name : ""));

	//Set this mushroom's collection gamebit, so it won't reappear
	if (objSetup->gamebitCollected != NO_GAMEBIT) {
		main_set_bits(objSetup->gamebitCollected, TRUE);
	}

	if (self->modelInstIdx == SHmushroom_MODEL_0_Blue_Mushroom) {
		dinoFoodBag = ((DLL_210_Player*)player->dll)->vtbl->func66(player, 16);

		//If the player has a sidekick foodbag, store Blue Mushrooms there
		if (dinoFoodBag && ((DLL_315_SideFoodbag*)dinoFoodBag->dll)->vtbl->is_obtained(dinoFoodBag)) {
			((DLL_315_SideFoodbag*)dinoFoodBag->dll)->vtbl->collect_food(dinoFoodBag, SIDEFOOD_Blue_Mushrooms);

			recomp_eprintf("Blue Mushroom: added to sidekick food bag.\n");
		//Otherwise store Blue Mushrooms directly in the inventory
		} else {
			count = main_increment_bits(objData->gamebitInventory);

			recomp_eprintf("Blue Mushroom: added directly to inventory (not in food bag)! (%d)\n", count);
		}
	} else {
		//Other mushroom types (White Mushrooms) are always stored directly in the inventory
		count = main_increment_bits(objData->gamebitInventory);

		recomp_eprintf("White Mushroom: added to inventory! (%d)\n", count);
	}

	//Optionally show an item collection pop-up
	if (recomp_get_config_u32("cmdmenu_info_popup_expand") &&
		showPopup &&
		count && 
		objData->gamebitInventory != NO_GAMEBIT
	) {
		gDLL_1_cmdmenu->vtbl->info_show(
			objData->gamebitInventory, 
			300, //5 seconds
			count
		);
	}
}

RECOMP_PATCH void SHmushroom_setup(Object* self, SHmushroom_Setup* setup, s32 arg2) {
	SHmushroom_Data_Extended* objData;
	UnkFunc_80024108Struct animInfo;
	s32 curveEndpoint;
	Object* player;
	ObjectShadow* shadow;
	f32 distanceToPlayer;

	objData = self->data;
	curveEndpoint = 25; //Matches unk18 on the initial curve node for SwapStone Hollow's lily pond mushroom (uID 0x3081c)
	player = get_player();
	self->unkB0 |= 0x6000;

	if (main_get_bits(setup->gamebitCollected)) {
		objData->state = SHmushroom_STATE_8_Hidden;
		self->objhitInfo->unk58 &= ~1;
		self->srt.flags |= OBJFLAG_INVISIBLE;
		objData->flags |= SHmushroom_FLAG_Delete_after_Setup;
	}

	//Choose model index
	self->modelInstIdx = setup->modelIdx;
	if (self->modelInstIdx >= self->def->numModels) {
		self->modelInstIdx = SHmushroom_MODEL_0_Blue_Mushroom;
	}

	//@recomp: store tutorial box gamebit and animObj
	get_tutorial_gamebit_and_animObjID(self,
		&objData->tutorialGamebit, 
		&objData->tutorialObjectID
	);

	//Set up shadow
	shadow = self->shadow;
	shadow->maxDistScale = 2.0f * shadow->scale;
	shadow = self->shadow;
	shadow->flags |= OBJ_SHADOW_FLAG_TOP_DOWN | OBJ_SHADOW_FLAG_CUSTOM_DIR;

	objData->jumpSpeedMultiplier = setup->jumpSpeed / 255.0f;
	objData->hopSpeedMultiplier = (setup->hopSpeed / 255.0f) * 0.2f;

	self->srt.scale = self->def->scale;

	//Get root motion speed from jump animation?
	func_80023D30(self, SHmushroom_MODANIM_1_Jump, 0.0f, 0);
	func_80024108(self, 1.0f, 1.0f, &animInfo);
	objData->jumpSpeed = animInfo.unk0[0];
	if (objData->jumpSpeed < 0.0f) {
		objData->jumpSpeed = -objData->jumpSpeed;
	}
	objData->jumpSpeed *= objData->jumpSpeedMultiplier;
	objData->jumpSpeed += 20.0f;

	//Get root motion speed from hop animation?
	func_80023D30(self, SHmushroom_MODANIM_4_Look_Hop_LOOP, 0.0f, 0);
	func_80024108(self, 1.0f, 1.0f, &animInfo);
	objData->hopSpeed = animInfo.unk0[2];
	if (objData->hopSpeed < 0.0f) {
		objData->hopSpeed = -objData->hopSpeed;
	}
	objData->hopSpeed += 20.0f;

	//Set up message queue
	obj_init_mesg_queue(self, 1);

	//Optionally set the mushroom to follow curves (only affects specific mushrooms)
	if ((setup->index == 4) || //White Mushroom around lily pond in SwapStone Hollow well
		(setup->index == 5)    //Unknown
	) {
		objData->flags |= SHmushroom_FLAG_Follow_Curve;
		gDLL_26_Curves->vtbl->func_4288(&objData->curves, self, 1000.0f, &curveEndpoint, -1);
		self->srt.transl.x = objData->curves.unk68.x;
		self->srt.transl.z = objData->curves.unk68.z;
	}

	objData->curvesDelta = 5.0f;

	//Set initial pursuer distance
	if (player != NULL) {
		distanceToPlayer = vec3_distance(&player->globalPosition, &self->globalPosition);
		objData->prevPursuerDistance = distanceToPlayer;
		objData->pursuerDistance = distanceToPlayer;
	} else {
		objData->pursuerDistance = 200.0f;
		objData->prevPursuerDistance = 200.0f;
	}

	obj_add_object_type(self, OBJTYPE_51);

	//Set up inventory gamebit (value incremented when collected)
	if (self->modelInstIdx == SHmushroom_MODEL_0_Blue_Mushroom) {
		objData->gamebitInventory = BIT_Inventory_Blue_Mushrooms;
	} else {
		objData->gamebitInventory = BIT_Inventory_White_Mushrooms;
	}

	func_80023D08(self, self->modelInstIdx);

	//@recomp: start at a random point in the initial animation (desync nearby mushrooms)
	func_80023D30(self, dStateModAnimIDs[objData->state], rand_next(0, 3)*0.25f, 0);
}

RECOMP_PATCH void SHmushroom_control(Object* self) {
	Object* dinoFoodBag;
	SHmushroom_Setup* objSetup;
	s32 i;
	Object* sidekick;
	Object* hitBy;
	s32 pad;
	f32 playerDistanceSquared;
	s32 count;
	Func_80057F1C_Struct** spBC;
	SHmushroom_Data_Extended* objData;
	f32 sidekickDistanceSquared;
	Object* player;
	u32 outMesgID;
	Func_80059C40_Struct sp58;
	s32 temp;

	objData = self->data;

	//@recomp: delete immediately if already collected
	if (objData->flags & SHmushroom_FLAG_Delete_after_Setup) {
		playerUtil_clear_collected_object(get_player(), self);
		obj_destroy_object(self);
	}

	objSetup = (SHmushroom_Setup*)self->setup;
	player = get_player();
	sidekick = get_sidekick();

	if (DEBUG_MUSHROOM) {
		diPrintf("\nSTATE: %d\n", objData->state);
		diPrintf("FLAGS: %02x\n", objData->flags);
		diPrintf("TIMER: %d\n", objData->expireTimer);
	}

	//@recomp: do nothing else when hidden (fixes bug where Find command appears near invisible mushrooms)
	if ((self->srt.flags & OBJFLAG_INVISIBLE) || (objData->state == SHmushroom_STATE_8_Hidden)) {
		//@recomp: delete self after being hidden for a while
		objData->expireTimer += gUpdateRate;
		if ((objData->expireTimer >= 500) && ((objData->flags & SHmushroom_FLAG_Message_Sent_to_Player) == FALSE)) {
			playerUtil_clear_collected_object(player, self);
			obj_destroy_object(self);
		}

		//@recomp: wait for player message, but only use it to delete the mushroom
		while (obj_recv_mesg(self, &outMesgID, NULL, NULL)) {
			if (outMesgID == 0x7000B) {
				DEBUG_MUSHROOM && recomp_eprintf("Message received!\n");
				objData->flags &= ~SHmushroom_FLAG_Message_Sent_to_Player;
				break;
			}
		}

		return;
	}

	//Handle being chased by player/sidekick (get distance to whoever's nearest)
	objData->prevPursuerDistance = objData->pursuerDistance;

	playerDistanceSquared = vec3_distance_squared(&player->globalPosition, &self->globalPosition);
	if (sidekick == NULL) {
		objData->pursuerDistance = sqrtf(playerDistanceSquared);
	} else {
		sidekickDistanceSquared = vec3_distance_squared(&sidekick->globalPosition, &self->globalPosition);
		if (playerDistanceSquared < sidekickDistanceSquared) {
			objData->pursuerDistance = sqrtf(playerDistanceSquared);
		} else {
			objData->pursuerDistance = sqrtf(sidekickDistanceSquared);
		}

		//Show Find command option when sidekick nearby
		if (objData->pursuerDistance < objSetup->alertRange) {
			((DLL_ISidekick*)sidekick->dll)->vtbl->func14(sidekick, Sidekick_Command_INDEX_1_Find);
		}
	}

	//React to attacks
	if (func_80025F40(self, &hitBy, NULL, NULL) != 0) {
		gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_744_Mushroom_Hit, MAX_VOLUME, NULL, NULL, 0, NULL);

		//Get eaten when attacked by an EarthWalker
		if (hitBy->id == OBJ_DR_EarthWarrior) {
			objData->state = SHmushroom_STATE_8_Hidden;
			((DLL_713_DR_EarthWarrior*)hitBy->dll)->vtbl->func20(hitBy, 1, self);
			self->srt.flags |= OBJFLAG_INVISIBLE;
			func_800267A4(self);

			playerUtil_clear_collected_object(player, self);
		} else {
			objData->flags |= SHmushroom_FLAG_Hurt;
		}
	}

	//@recomp: comment out unused switch
	/*
	switch (objSetup->index) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		break;
	}
	*/

	SHmushroom_tick_state_machine(self, objData, objSetup);

	if (objData->flags & SHmushroom_FLAG_Moving) {
		//Snap Y to ground
		count = func_80057F1C(self, self->srt.transl.x, self->srt.transl.y, self->srt.transl.z, &spBC, 0, 0);
		for (i = 0; i < count; i++) {
			if (spBC[i]->unk0[0] < (self->srt.transl.y + 10.0f)) {
				self->srt.transl.y = spBC[i]->unk0[0];
				break;
			}
		}

		/* Stop fleeing under some circumstances? (TODO: investigate!)

		   Should only affect curve-following mushroom around lily pond in SwapStone Hollow - Well.

		   Considering the "MUSHROOM: trapped" string, maybe the mushroom's intended to stop fleeing
		   if its path around the pond is blocked by dropping the cave stalactites?
		*/
		temp = func_80059C40(&self->prevLocalPosition, &self->srt.transl, 6.0f, 2, &sp58, self, 8, -1, 0xFF, 0x14);
		if ((objSetup->index == 4) && (temp) && (sp58.unk50 == 0xD)) {
			objData->flags |= SHmushroom_FLAG_No_Fleeing;
			STUBBED_PRINTF("MUSHROOM: trapped!!!!\n");
		}
	}
}

RECOMP_PATCH void SHmushroom_tick_state_machine(Object* self, SHmushroom_Data_Extended* objData, SHmushroom_Setup* objSetup) {
	f32 speed;
	f32 dx;
	f32 dz;
	UnkFunc_80024108Struct animInfo;
	SRT fxTransform;
	Object* player;
	/* @recomp */
	s8 tutorialSeen = FALSE;
	s8 configDanceChance = recomp_get_config_u32("shmushroom_dance_chance");
	s8 percentThreshold;

	player = get_player();

	objData->flags &= ~SHmushroom_FLAG_Moving;

	if (objData->flags & SHmushroom_FLAG_No_Fleeing) {
		objData->state = SHmushroom_STATE_6_Trapped;
	}

	//Get player/sidekick's approach speed
	speed = (objData->prevPursuerDistance - objData->pursuerDistance) / gUpdateRateF;

	self->unkAF |= ARROW_FLAG_8_No_Targetting;

	switch (objData->state) {
	case SHmushroom_STATE_0_Idle:
		if (objData->flags & SHmushroom_FLAG_Hurt) {
			objData->state = SHmushroom_STATE_9_Stunned;
		} else {
			objData->spinSpeed = 0;

			//@recomp: random chance of dancing each time idle anim loops
			//(10% by default, but can be disabled/controlled via mod configs)
			if (configDanceChance) {
				if (configDanceChance >= 100) {
					objData->state = SHmushroom_STATE_10_Dance;
					//Start at a random point in the initial animation (desync nearby mushrooms)
					if (objData->firstTickDone == FALSE) {
						func_80023D30(self, 
							dStateModAnimIDs[objData->state], 
							rand_next(0, 3)*0.25f, 
							0
						);
					}
				} else {
					if (objData->danceTimer <= 0) {
						if (objData->flags & SHmushroom_FLAG_Animation_Finished) {
							percentThreshold = MAX(0, MIN(configDanceChance - 1, 100 - 1));

							if (rand_next(0, 99) <= percentThreshold) {
								//random delay to desync nearby mushrooms' anims
								objData->danceTimer = rand_next(1, 60); 
							}
						}
					} else {
						objData->danceTimer += gUpdateRate;
						if (objData->danceTimer > 60) {
							objData->danceTimer = 0;
							objData->state = SHmushroom_STATE_10_Dance;
						}
					}
				}
			}

			//Flee if the player/sidekick are very close
			if (objData->pursuerDistance < objSetup->fleeRange) {
				if (objData->flags & SHmushroom_FLAG_Follow_Curve) {
					objData->fleeAngle = SHmushroom_flee_along_curve(self, player, objData, objData->jumpSpeed);
				} else {
					objData->fleeAngle = SHmushroom_flee_from_player(self, player, objData, objData->jumpSpeed);
				}

				objData->state = SHmushroom_STATE_1_Jump;
				gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_53C_Mushroom_Bounce, MAX_VOLUME, NULL, NULL, 0, NULL);
				self->srt.yaw = objData->fleeAngle - M_90_DEGREES;

			//Enter alert state if the player/sidekick are close
			} else {
				if (objData->pursuerDistance < objSetup->alertRange) {
					objData->state = SHmushroom_STATE_3_Alert_Intro;
				}
			}
		}
		break;
	case SHmushroom_STATE_1_Jump:
		if (objData->flags & SHmushroom_FLAG_Hurt) {
			objData->state = SHmushroom_STATE_9_Stunned;
		} else {
			//Fleeing
			objData->flags |= SHmushroom_FLAG_Moving;
			if (objData->flags & SHmushroom_FLAG_Animation_Finished) {
				objData->state = SHmushroom_STATE_0_Idle;
			}
		}
		break;
	case SHmushroom_STATE_3_Alert_Intro:
	case SHmushroom_STATE_7_Alert_Outro:
		if (objData->flags & SHmushroom_FLAG_Hurt) {
			objData->state = SHmushroom_STATE_9_Stunned;
			break;
		} else if (objData->flags & SHmushroom_FLAG_Animation_Finished) {
			//Alert: animate head tilting back, then advance state and stay in a poised loop
			if (objData->state == SHmushroom_STATE_3_Alert_Intro) {
				objData->state = SHmushroom_STATE_4_Alert;

			//Relieved: go back to idle when finished sighing
			} else {
				objData->state = SHmushroom_STATE_0_Idle;
			}
			break;
		}
	case SHmushroom_STATE_4_Alert:
		if (objData->flags & SHmushroom_FLAG_Hurt) {
			objData->state = SHmushroom_STATE_9_Stunned;
		} else {
			//Alert state: staying still in poised pose, facing towards threat
			dx = self->srt.transl.x - player->srt.transl.x;
			dz = self->srt.transl.z - player->srt.transl.z;
			self->srt.yaw = arctan2_f(-dx,-dz);

			//If pursuer backs off, play a little "phew!" animation
			if ((objSetup->alertRange + 10.0f) < objData->pursuerDistance) {
				objData->state = SHmushroom_STATE_7_Alert_Outro;
			} else {
				//Otherwise: try to flee
				if (objData->pursuerDistance < objSetup->fleeRange) {
					//Flee by jumping (if the player/sidekick are approaching quickly)
					if (speed >= MUSHROOM_SNEAK_THRESHOLD) {
						if (objData->flags & SHmushroom_FLAG_Follow_Curve) {
							objData->fleeAngle = SHmushroom_flee_along_curve(self, player, objData, objData->jumpSpeed);
						} else {
							objData->fleeAngle = SHmushroom_flee_from_player(self, player, objData, objData->jumpSpeed);
						}

						objData->state = SHmushroom_STATE_1_Jump;
						gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_53C_Mushroom_Bounce, MAX_VOLUME, NULL, NULL, 0, NULL);
						self->srt.yaw = objData->fleeAngle - M_90_DEGREES;

					//Hop in surprise (if the player/sidekick are sneaking up)
					} else {
						if (objData->flags & SHmushroom_FLAG_Follow_Curve) {
							objData->fleeAngle = SHmushroom_flee_along_curve(self, player, objData, objData->hopSpeed);
						} else {
							objData->fleeAngle = SHmushroom_flee_from_player(self, player, objData, objData->hopSpeed);
						}

						objData->state = SHmushroom_STATE_5_Surprised_Hop;
						gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_53C_Mushroom_Bounce, MAX_VOLUME, NULL, NULL, 0, NULL);
						self->srt.yaw = objData->fleeAngle;
					}
				}
			}
		}

		break;
	case SHmushroom_STATE_5_Surprised_Hop:
		//When hurt, finish playing the hop animation before advancing to stunned state
		if ((objData->flags & (SHmushroom_FLAG_Hurt | SHmushroom_FLAG_Animation_Finished)) == (SHmushroom_FLAG_Hurt | SHmushroom_FLAG_Animation_Finished)) {
			objData->state = SHmushroom_STATE_9_Stunned;
		}

		objData->flags |= SHmushroom_FLAG_Moving;

		//If threat backs off and hop finished, return to alert state
		if (((objSetup->fleeRange + 10.0f) < objData->pursuerDistance) && (objData->flags & SHmushroom_FLAG_Animation_Finished)) {
			objData->state = SHmushroom_STATE_4_Alert;

		//Otherwise, flee if the player/sidekick are no longer sneaking up
		} else if (speed >= MUSHROOM_SNEAK_THRESHOLD) {
			if (objData->flags & SHmushroom_FLAG_Follow_Curve) {
				objData->fleeAngle = SHmushroom_flee_along_curve(self, player, objData, objData->jumpSpeed);
			} else {
				objData->fleeAngle = SHmushroom_flee_from_player(self, player, objData, objData->jumpSpeed);
			}

			objData->state = SHmushroom_STATE_1_Jump;
			gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_53C_Mushroom_Bounce, MAX_VOLUME, NULL, NULL, 0, NULL);
			self->srt.yaw = objData->fleeAngle - M_90_DEGREES;
		}
		break;

	case SHmushroom_STATE_2_Collected:
		//NOTE: plays an unseen dancing animation, but the mushroom immediately becomes invisible
		if (objData->flags & SHmushroom_FLAG_Animation_Finished) {
			objData->state = SHmushroom_STATE_8_Hidden;
			self->srt.flags |= OBJFLAG_INVISIBLE;
			func_800267A4(self);
		}
		break;

	case SHmushroom_STATE_9_Stunned:
		//Start stunned sound loop
		if (objData->stunnedTimer <= 0) {
			gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_745_Mushroom_Stunned_Loop, MAX_VOLUME, &objData->soundHandleStun, NULL, 0, NULL);
			objData->stunnedTimer = rand_next(240, 300);
		}

		//Run down stun timer
		objData->stunnedTimer -= gUpdateRateF;

		//Return to idle once stun wears off
		if (objData->stunnedTimer <= 0) {
			gDLL_13_Expgfx->vtbl->func4(self);
			objData->state = SHmushroom_STATE_0_Idle;
			objData->flags &= ~SHmushroom_FLAG_Hurt;

			//Stop sound loop
			if (objData->soundHandleStun != 0) {
				gDLL_6_AMSFX->vtbl->func_A1C(objData->soundHandleStun);
				objData->soundHandleStun = 0;
			}

		//If still stunned, emit particles periodically
		} else {
			objData->stunFxTimer -= gUpdateRateF;
			if (objData->stunFxTimer <= 0) {
				fxTransform.transl.x = 10.0f;
				fxTransform.transl.y = 12.0f;
				gDLL_17_partfx->vtbl->spawn(self, PARTICLE_51D, &fxTransform, PARTFXFLAG_2, -1, NULL);
				objData->stunFxTimer = 20.0f;
			}

			self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
		}
		break;

	case SHmushroom_STATE_6_Trapped:
		//Won't flee, and can be collected immediately without stunning
		if (objData->flags & SHmushroom_FLAG_Hurt) {
			objData->state = SHmushroom_STATE_9_Stunned;
		}

		self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
		break;

	//@recomp: add custom state showing off unseen dance animation
	case SHmushroom_STATE_10_Dance:
		if (objData->flags & SHmushroom_FLAG_Hurt) {
			objData->state = SHmushroom_STATE_9_Stunned;
		} else {
			//Handle returning to idle at end of animation
			if (!configDanceChance) {
				objData->state = SHmushroom_STATE_0_Idle;
			} else if (configDanceChance < 100) {
				//66% chance of continuing dance, 33% chance of returning to idle
				if ((objData->flags & SHmushroom_FLAG_Animation_Finished) && 
					!rand_next(0, 2)
				) {
					objData->state = SHmushroom_STATE_0_Idle;
				}
			}

			//Pick direction and spin
			if (objData->spinSpeed == 0) {
				objData->spinSpeed = rand_next(0, 1) ? -100 : 100;
			}
			self->srt.yaw += objData->spinSpeed * gUpdateRate;

			//Flee if the player/sidekick are very close
			if (objData->pursuerDistance < objSetup->fleeRange) {
				if (objData->flags & SHmushroom_FLAG_Follow_Curve) {
					objData->fleeAngle = SHmushroom_flee_along_curve(self, player, objData, objData->jumpSpeed);
				} else {
					objData->fleeAngle = SHmushroom_flee_from_player(self, player, objData, objData->jumpSpeed);
				}

				objData->state = SHmushroom_STATE_1_Jump;
				gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_53C_Mushroom_Bounce, MAX_VOLUME, NULL, NULL, 0, NULL);
				self->srt.yaw = objData->fleeAngle - M_90_DEGREES;

			//Enter alert state if the player/sidekick are close
			} else {
				if (objData->pursuerDistance < objSetup->alertRange) {
					objData->state = SHmushroom_STATE_3_Alert_Intro;
				}
			}
		}
		break;
	}

	//Handle player collecting mushroom (@recomp: increment inventory gamebit immediately)
	if (self->unkAF & ARROW_FLAG_1_Interacted) {
		//Check if tutorial already seen
		if ((objData->tutorialGamebit != NO_GAMEBIT) && main_get_bits(objData->tutorialGamebit)) {
			tutorialSeen = TRUE;
		}

		//Set item collection sequence animObjID (@recomp: ensures White Mushroom sets animObjID)
		if ((tutorialSeen == FALSE) && (objData->tutorialObjectID >= 0)) {
			gDLL_3_Animation->vtbl->func30(objData->tutorialObjectID, NULL, 0);
		}

		//Send message to player, collecting and displaying Blue Mushroom's tutorial box
		obj_send_mesg(player,
			0x7000A,
			self,
			(void*)(s32)objData->tutorialGamebit
		);
		objData->flags |= SHmushroom_FLAG_Message_Sent_to_Player;

		add_to_inventory(self, objData, objSetup, player, (tutorialSeen == TRUE));

		self->unkAF |= ARROW_FLAG_8_No_Targetting;

		objData->state = SHmushroom_STATE_2_Collected;

		//Stop sound loop
		if (objData->soundHandleStun != 0) {
			gDLL_6_AMSFX->vtbl->func_A1C(objData->soundHandleStun);
			objData->soundHandleStun = 0;
		}
	}

	//Change animation when needed
	if (self->curModAnimId != dStateModAnimIDs[objData->state]) {
		func_80023D30(self, 
			dStateModAnimIDs[objData->state], 
			(objData->state == SHmushroom_STATE_10_Dance) ? 0.0f : 0.25f, 
			0
		);
	}

	//Advance animation
	if (func_80024108(self, dStateAnimSpeeds[objData->state], gUpdateRateF, &animInfo)) {
		objData->flags |= SHmushroom_FLAG_Animation_Finished;
	} else {
		objData->flags &= ~SHmushroom_FLAG_Animation_Finished;
	}

	//Set speed based on current state
	if (objData->state == SHmushroom_STATE_1_Jump) {
		speed = (animInfo.unk0[0] / gUpdateRateF) * objData->jumpSpeedMultiplier;
	} else if (objData->state == SHmushroom_STATE_5_Surprised_Hop) {
		speed = animInfo.unk0[2] / gUpdateRateF;
	} else {
		speed = 0.0f;
	}

	//Move
	self->velocity.x = fsin16_precise(objData->fleeAngle) * speed;
	self->velocity.z = fcos16_precise(objData->fleeAngle) * speed;
	obj_move(self, self->velocity.x * gUpdateRateF, 0.0f, self->velocity.z * gUpdateRateF);

	if (objData->firstTickDone == FALSE) {
		objData->firstTickDone = TRUE;
	}
}

RECOMP_PATCH u32 SHmushroom_get_data_size(Object *self, u32 a1) {
	return sizeof(SHmushroom_Data_Extended);
}
