#include "configs.h"
#include "dlls/engine/6_amsfx.h"
#include "math_util.h"
#include "modding.h"
#include "object_util.h"
#include "reasset.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "game/objects/interaction_arrow.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "dll.h"
#include "dlls/engine/33_BaddieControl.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "sys/rand.h"

#include "recomp/dlls/objects/428_DFmole_recomp.h"

// #define DEBUG_MOLE

//TEMPORARY DEFINES
#define capy_obj_Control capy_control
#define capy_obj_Free capy_free
#define capy_obj_GetDataSize capy_get_data_size
#define capy_findFoodTarget capy_func_468
#define capy_handlePlayerTarget capy_func_644
#define capy_searchForTarget capy_func_704
#define capy_animCallback capy_anim_callback
#define capy_animState0Standing capy_anim_state_0_standing
#define capy_animState1Burrowed capy_anim_state_1_burrowed
#define capy_animState3Burrow capy_anim_state_3_burrow
#define capy_animState8Walking capy_anim_state_8_walking
#define capy_logicState2GoToDigSpot capy_logic_state_2_go_to_dig_spot
#define capy_logicState4Idle capy_logic_state_4_idle

#define PARTICLE_CA 0xCA
#define PARTICLE_CB 0xCB
//END OF TEMPORARY DEFINES

/*0x0*/ extern  ObjFSA_StateCallback sAnimCallbacks[9];
/*0x28*/ extern ObjFSA_StateCallback sLogicCallbacks[6];

extern void capy_findFoodTarget(Object* self, Baddie* baddie, ObjFSA_Data* fsa);
extern void capy_handlePlayerTarget(Object* self, s32 arg1, Baddie* baddie, ObjFSA_Data* fsa);
extern void capy_searchForTarget(Object* self, Baddie* baddie, ObjFSA_Data* fsa);
extern int capy_animCallback(Object* self, Object* animObj, AnimObj_Data* animData, s8 prevCallbackValue);

typedef struct {
/*00*/ u8 fed;
/*01*/ u8 ateMagicPlant;
/*02*/ u8 turnSpeedFactor;
/*04*/ u16 timer;
/*08*/ u32 soundHandle;
/*0C*/ f32 prevX;
/*10*/ f32 prevZ;
/* RECOMP */
    Object* targetStandIn;  //A temporary stand-in object for the dig logic state's curve nodes, so they can be referenced with fsa->target
    u8 digState;            //Substates for a redesigned implementation of the "GoToDigSpot" logic state
    u16 digTimer;           //Times how long the mole's spent trying to reach dig site, as a failsafe
    u32 soundHandleBurrow1;  //Sound handle for optional dig loop sounds
    u32 soundHandleBurrow2;  //Sound handle for optional dig loop sounds
    u8 digSoundsVolume;    //Volume for fading out optional dig sounds
    f32 prevAnimProgress;
} capy_data;

typedef enum {
    DFMole_DIGSTATE_Find_Dig_Site_Node = 0, //Finds the curve node at the dig site, and positions a temporary target object there
    DFMole_DIGSTATE_Turn_to_Site,           //Mole turns towards the dig site (using the temporary object as an FSA target)
    DFMole_DIGSTATE_Go_to_Site,             //Mole walks over to the dig site, using voxmap checks
    DFMole_DIGSTATE_Find_Wall_Node,         //Finds the next curve node: behind the dig site wall, and places the temporary target there
    DFMole_DIGSTATE_Turn_to_Wall,           //Mole turns to face the wall (using the temporary object as an FSA target)
    DFMole_DIGSTATE_Dig                     //Mole is facing the wall - start scooping!
} DFMole_DigStates;

enum CapyAnimStates {
    CAPY_ASTATE_0_Standing = 0,
    CAPY_ASTATE_1_Burrowed = 1,
    CAPY_ASTATE_2_Unburrow = 2,
    CAPY_ASTATE_3_Burrow = 3,
    CAPY_ASTATE_4_Sniff = 4,
    CAPY_ASTATE_5_Eat = 5,
    CAPY_ASTATE_6_DoneEating = 6,
    CAPY_ASTATE_7_DigWall = 7,
    CAPY_ASTATE_8_Walking = 8
};

enum CapyLogicStates {
    CAPY_LSTATE_0_Init = 0,
    CAPY_LSTATE_1_DigWall = 1,
    CAPY_LSTATE_2_GoToDigSpot = 2,
    CAPY_LSTATE_3_Eating = 3,
    CAPY_LSTATE_4_Idle = 4,
    CAPY_LSTATE_5_Underground = 5
};

enum CapyModAnimIndices {
    CAPY_MODANIM_0_Burrow = 0,
    CAPY_MODANIM_1_Unburrow = 1,
    CAPY_MODANIM_2_DigWall = 2,
    CAPY_MODANIM_3_Standing = 3,
    CAPY_MODANIM_4_Walking = 4,
    CAPY_MODANIM_5_Sniffing = 5,
    CAPY_MODANIM_6_Eating = 6,
    CAPY_MODANIM_7_DoneEating = 7
};

/* Creates a temporary stand-in object representing the current dig site curve node, so its location can easily work with `fsa->target` */
static void DFMole_createTargetStandIn(Object* self, capy_data* objData) {
    AnimObj_Setup* setup;

    if (objData->targetStandIn) {
        return;
    }

    setup = (AnimObj_Setup*)objAllocSetup(sizeof(AnimObj_Setup), OBJ_animbubble);
    setup->base.loadFlags = OBJSETUP_LOAD_MAIN;
    setup->base.fadeFlags = OBJSETUP_FADE_CAMERA;
    setup->base.loadDistance = 0xFF;
    setup->base.fadeDistance = 0xFF;
    setup->base.x = self->setup->x;
    setup->base.y = self->setup->y;
    setup->base.z = self->setup->z;
    setup->sequenceIdBitfield = -1;
    
    objData->targetStandIn = objSetupObject(&setup->base, OBJINIT_STANDALONE | OBJINIT_FLAG4, self->mapID, -1, NULL);
    if (objData->targetStandIn != NULL) {
        objData->targetStandIn->opacity = 0;
    }
}

/* Plays sounds while the mole is burrowing */
static void DFMole_handleBurrowSounds(Object* self) {
    Baddie* baddie;
    capy_data* objData;
    f32 animThreshold;

    baddie = self->data;
    if (baddie == NULL) {
        return;
    }

    objData = baddie->objdata;
    if (objData == NULL) {
        return;
    }

    if (configs_GetMoleBurrowEffects()) {
        switch (baddie->fsa.animState) {
        case CAPY_ASTATE_2_Unburrow:
        case CAPY_ASTATE_3_Burrow:
            //Start sound when the mole burrows down
            if (baddie->fsa.animStateTime < gUpdateRate) {
                if (objData->soundHandleBurrow1) {
                    dll_amSfx->Stop(objData->soundHandleBurrow1);
                    objData->soundHandleBurrow1 = 0;
                }
                objData->soundHandleBurrow1 = dll_amSfx->Play(self, SOUND_48_Dig_Loop, MAX_VOLUME, NULL, NULL, 0, NULL);
                dll_amSfx->SetPitch(objData->soundHandleBurrow1, 1.1f + ((f32)mathRnd(-10, 10) / 100.0f));
                objData->digSoundsVolume = MAX_VOLUME;
            }

            animThreshold = (baddie->fsa.animState == CAPY_ASTATE_3_Burrow) ? 0.5f : 0.1f;

            if ((objData->prevAnimProgress < animThreshold && self->animProgress >= animThreshold) || 
                (animThreshold == 0.0f && self->animProgress == 0.0f)
            ) {
                if (objData->soundHandleBurrow2) {
                    dll_amSfx->Stop(objData->soundHandleBurrow2);
                    objData->soundHandleBurrow2 = 0;
                }
                objData->soundHandleBurrow2 = dll_amSfx->Play(self, SOUND_602_Emerge_Snowy, VOLUME_PERCENT(30), NULL, NULL, 0, NULL);
                dll_amSfx->SetPitch(objData->soundHandleBurrow2, 0.5f + ((f32)mathRnd(-10, 10) / 150.0f));
            }
            break;
        default:
            if (objData->soundHandleBurrow1) {
                //Fade out the burrowing sound if the mole is finished burrowing/unburrowing
                if (objData->digSoundsVolume > gUpdateRate * 4) {
                    objData->digSoundsVolume -= gUpdateRate * 4;
                } else {
                    objData->digSoundsVolume = 0;
                }

                if (objData->digSoundsVolume) {
                    dll_amSfx->SetVol(objData->soundHandleBurrow1, objData->digSoundsVolume);
                } else {
                    dll_amSfx->Stop(objData->soundHandleBurrow1);
                    objData->soundHandleBurrow1 = 0;

                    if (objData->soundHandleBurrow2) {
                        dll_amSfx->Stop(objData->soundHandleBurrow2);
                        objData->soundHandleBurrow2 = 0;
                    }
                }
            }
        }
    } else {
        if (objData->soundHandleBurrow1) {
            dll_amSfx->Stop(objData->soundHandleBurrow1);
            objData->soundHandleBurrow1 = 0;
        }
        if (objData->soundHandleBurrow2) {
            dll_amSfx->Stop(objData->soundHandleBurrow2);
            objData->soundHandleBurrow2 = 0;
        }
    }

    objData->prevAnimProgress = self->animProgress;
}

/* Creates particle effects while the mole is burrowing */
static void DFMole_handleBurrowParticles(Object* self) {
    Baddie* baddie;
    capy_data* objData;
    SRT fxTransform;
    Vec3f offset = VEC3F(0, 0, 0);
    f32 animThreshold;
    u8 createParticleA = FALSE;
    u8 createParticleB = FALSE;
    s32 yawOffset = 0;

    baddie = self->data;
    if (baddie == NULL) {
        return;
    }

    objData = baddie->objdata;
    if (objData == NULL) {
        return;
    }

    if (configs_GetMoleBurrowEffects() == FALSE) {
        return;
    }

    switch (baddie->fsa.animState) {
    case CAPY_ASTATE_2_Unburrow:
        if (self->animProgress < 0.5f) {
            if (!mathRnd(0, 2)) {
                createParticleA = TRUE;
            }
            if (!mathRnd(0, 2)) {
                createParticleB = TRUE;
            }
            yawOffset = M_180_DEGREES;
            //Position the effects closer to the mole's centre
            offset.z = +15.0f;
        }
        break;
    case CAPY_ASTATE_3_Burrow:
        if (!mathRnd(0, 2)) {
            createParticleA = TRUE;
        }
        if (!mathRnd(0, 2)) {
            createParticleB = TRUE;
        }
        //Position the effects closer to the mole's claws
        offset.z = -10.0f;
        break;
    case CAPY_ASTATE_7_DigWall:
        if (self->animProgress < 0.7f) {
            if (!mathRnd(0, 2)) {
                createParticleA = TRUE;
            }
            if (!mathRnd(0, 2)) {
                createParticleB = TRUE;
            }
            offset.y = lerp_float(self->animProgress/0.7f, 20.0f, 0.0f);
            offset.z = -20.0f;
        }
        break;
    }

    if (createParticleA || createParticleB) {
        fxTransform.transl.x = self->globalPosition.x;
        fxTransform.transl.y = self->globalPosition.y;
        fxTransform.transl.z = self->globalPosition.z;
        fxTransform.yaw = self->srt.yaw;
        if (yawOffset) {
            yawOffset += fxTransform.yaw;
            CIRCLE_WRAP(yawOffset);
            fxTransform.yaw = yawOffset;
        }
        
        if (offset.x || offset.y || offset.z) {
            rotate_point_by_angle_2D(offset.x, offset.z, &offset.x, &offset.z, self->srt.yaw);
            fxTransform.transl.x += offset.x;
            fxTransform.transl.y += offset.y;
            fxTransform.transl.z += offset.z;
        }

        if (createParticleA) {
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_CA, &fxTransform, PARTFXFLAG_1 | PARTFXFLAG_200000, -1, NULL);
        }
        if (createParticleB) {
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_CB, &fxTransform, PARTFXFLAG_1 | PARTFXFLAG_200000, -1, NULL);
        }
    }
}

RECOMP_PATCH void capy_obj_Control(Object* self) {
    Baddie* baddie = self->data;
    
    gDLL_18_objfsa->vtbl->tick(self, &baddie->fsa, 1.0f, 1.0f, sAnimCallbacks, sLogicCallbacks);
    
    capy_findFoodTarget(self, baddie, &baddie->fsa);

    if ((baddie->fsa.target != NULL) || (baddie->fsa.hitpoints == 0)) { 
        capy_handlePlayerTarget(self, 0, baddie, &baddie->fsa);
    } else {
        capy_searchForTarget(self, baddie, &baddie->fsa);
    }

    //@recomp: handle optional sounds and particles
    DFMole_handleBurrowSounds(self);
    DFMole_handleBurrowParticles(self);
}

/* Free the dig spot state's temporary stand-in object too */
RECOMP_PATCH void capy_obj_Free(Object* self, s32 onlySelf) {
    Baddie* baddie = self->data;

    objFreeObjectType(self, OBJTYPE_Baddie);

    if (self->linkedObject != NULL) {
        objFreeObject(self->linkedObject);
        self->linkedObject = NULL;
    }

    gDLL_33_BaddieControl->vtbl->free(self, baddie, 1);

    {
        capy_data* objData = baddie->objdata;
        
        //@recomp: free the stand-in target object
        if (objData->targetStandIn) {
            objFreeObject(objData->targetStandIn);
            objData->targetStandIn = NULL;
        }

        //@recomp: free soundHandles
        if (objData->soundHandle) {
            dll_amSfx->Stop(objData->soundHandle);
            objData->soundHandle = 0;
        }
        if (objData->soundHandleBurrow1) {
            dll_amSfx->Stop(objData->soundHandleBurrow1);
            objData->soundHandleBurrow1 = 0;
        }
        if (objData->soundHandleBurrow2) {
            dll_amSfx->Stop(objData->soundHandleBurrow1);
            objData->soundHandleBurrow1 = 0;
        }
    }
}

/* Fix framerate dependency */
RECOMP_PATCH s32 capy_animState0Standing(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    s32 count;
    Object** magicPlants;
    s32 i;
    s32 magicPlantNearby;
    Baddie* baddie;
    capy_data* objData;

    baddie = self->data;
    objData = baddie->objdata;

    if (fsa->enteredAnimState) {
        objAnimSet(self, CAPY_MODANIM_3_Standing, 0.0f, 0);
        fsa->unk33A = FALSE;
    }
    fsa->animTickDelta = 0.03f;

    fsa->unk278 = 0.0f;
    fsa->unk27C = 0.0f;
    gDLL_18_objfsa->vtbl->turn_to_target(self, fsa, updateRate, 5);

    if (fsa->enteredAnimState) {
        objData->timer = 0;
    }

    objData->timer += gUpdateRate; //@recomp: fix framerate dependency

    if ((objGetPlayer() == fsa->target) && (objData->timer > 60)) {
        if (objData->ateMagicPlant) {
            fsa->enteredAnimState = TRUE;
            fsa->unk33A = FALSE;
            fsa->logicState = CAPY_LSTATE_5_Underground;
            return FSA_NEXTSTATE_SYNC(CAPY_ASTATE_3_Burrow);
        }

        magicPlants = objGetAllOfType(OBJTYPE_MagicPlant, &count);
        magicPlantNearby = FALSE;
        for (i = 0; i < count; i++) {
            if (!magicPlantNearby && vec3Distance(&self->globalPosition, &magicPlants[i]->globalPosition) < 300.0f) {
                magicPlantNearby = TRUE;
                fsa->target = magicPlants[i];
            }
        }
    }

    return 0;
}

/* Fix framerate dependency, and make sure player collision is off */
RECOMP_PATCH s32 capy_animState1Burrowed(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie = self->data;
    capy_data* objData;

    objData = baddie->objdata;

    if (fsa->enteredAnimState) {
        objAnimSet(self, CAPY_MODANIM_1_Unburrow, 0.0f, 0);
        fsa->unk33A = FALSE;
    }

    if (objData->timer != 0) {
        //@recomp: fix framerate dependency
        if (objData->timer > gUpdateRate) {
            objData->timer -= gUpdateRate;
        } else {
            objData->timer = 0;
        }
    }

    fsa->animTickDelta = 0.0f;
    fsa->unk278 = 0.0f;
    fsa->unk27C = 0.0f;

    //@recomp: ensure collision doesn't affect player while underground
    if (fsa->enteredAnimState) {
        self->objhitInfo->unk58 |= 1;
    }

    return 0;
}

/* Fix an issue where the mole couldn't find the designated unburrow points, and would instead
   stay at the dig spot and unburrow on top of the player while they crawl back into the central cave. */
RECOMP_PATCH s32 capy_animState3Burrow(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie;
    capy_data* objData;
    CurveSetup* curveSetup;
    s32 curveUID;
    s32 curveType;

    baddie = self->data;
    objData = baddie->objdata;

    if (fsa->enteredAnimState) {
        objAnimSet(self, CAPY_MODANIM_0_Burrow, 0.0f, 0);
        fsa->unk33A = FALSE;
    }
    fsa->animTickDelta = 0.04f;

    fsa->unk278 = 0.0f;
    fsa->unk27C = 0.0f;

    if (fsa->unk33A) {
        curveType = 0x24; //@recomp: fix wrong curve type being used for unburrow spots
        curveUID = gDLL_26_Curves->vtbl->func_1E4(self->srt.transl.f[0], self->srt.transl.f[1], self->srt.transl.f[2], &curveType, 1, 0);
        if (curveUID != -1) {
            curveSetup = gDLL_26_Curves->vtbl->func_39C(curveUID);
            self->srt.transl.x = curveSetup->pos.x;
            self->srt.transl.z = curveSetup->pos.z;
        }
        objData->timer = 80;

        return FSA_NEXTSTATE_SYNC(CAPY_ASTATE_1_Burrowed);
    }

    return 0;
}

/* Fix some framerate dependencies */
RECOMP_PATCH s32 capy_animState8Walking(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    Baddie* baddie;
    capy_data* objData;
    f32 dz;
    f32 dx;
    
    baddie = self->data;
    objData = baddie->objdata;

    if (fsa->enteredAnimState) {
        objAnimSet(self, CAPY_MODANIM_4_Walking, 0.0f, 0);
        fsa->unk33A = FALSE;
    }

    if (fsa->enteredAnimState) {
        objData->timer = 0;
        objData->prevX = self->srt.transl.x;
        objData->prevZ = self->srt.transl.z;
    }

    //Get current speed from position delta (@framerate-dependent)
    dx = self->srt.transl.x - objData->prevX;
    dz = self->srt.transl.z - objData->prevZ;
    objData->prevX = self->srt.transl.x;
    objData->prevZ = self->srt.transl.z;

    //@recomp: fix framerate dependency
    if (((SQ(dx) + SQ(dz))/gUpdateRateF < 0.2f) && (fsa->logicState != CAPY_LSTATE_2_GoToDigSpot)) {
        objData->timer += gUpdateRate;
    } else {
        objData->timer = 0;
    }

    if (objData->timer >= (13 * 2)) {
        fsa->enteredAnimState = TRUE;
        fsa->unk33A = FALSE;
        fsa->logicState = CAPY_LSTATE_5_Underground;
        return FSA_NEXTSTATE_SYNC(CAPY_ASTATE_3_Burrow);
    }

    fsa->animTickDelta = 0.08f;
    gDLL_18_objfsa->vtbl->func7(self, fsa, updateRate, 1);
    gDLL_33_BaddieControl->vtbl->func3(self, fsa, baddie, 2.0f, 12.0f);

    return 0;
}

/* Optionally use a revamped version of the dig spot state, hopefully avoiding issues where the mole spends a long time trying to find the wall. */
RECOMP_PATCH s32 capy_logicState2GoToDigSpot(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    s32 _pad[2];
    capy_data* objData;
    UnkCurvesStruct* baddieCurves;
    Baddie* baddie;
    s32 startDig;
    f32 dYaw;
    f32 dYawAbs;
    s32 curveAngle;
    /* RECOMP */
    u8 useRevampedDigState = configs_GetMoleDigFix();

    baddie = self->data;
    baddieCurves = baddie->unk3F8;
    objData = baddie->objdata;

    //@recomp: set up dig State Machine
    if (fsa->enteredLogicState) {
        objData->digState = DFMole_DIGSTATE_Find_Dig_Site_Node;
        objData->digTimer = 0;
    }

    //Run the curve logic regardless of which version of this state we're using
    startDig = curves_func_800053B0(&baddieCurves->unk0, 5.0f / (SQ(baddieCurves->unk0.unk68.f[2] - self->srt.transl.f[2]) + SQ(baddieCurves->unk0.unk68.f[0] - self->srt.transl.f[0])));

    //Pick between original/revamped version of this state
    if (useRevampedDigState == FALSE) {
        if ((startDig || baddieCurves->unk0.unk10) && gDLL_26_Curves->vtbl->func_4704(baddieCurves)) {
            fsa->animState = CAPY_ASTATE_7_DigWall;
            fsa->enteredAnimState = FALSE;
            fsa->unk33A = FALSE;
            fsa->logicState = CAPY_LSTATE_1_DigWall;
            return 0;
        } else {
            //Get angle from curve tangent
            curveAngle = (mathAtan2f(baddieCurves->unk0.unk74, baddieCurves->unk0.unk7C) & 0xFFFF);

            //Get yaw diff (choose shortest angular path)
            dYaw = (((curveAngle - ((u16)self->srt.yaw & 0xFFFF))) + M_180_DEGREES);
            if (dYaw > M_180_DEGREES) {
                dYaw = -(M_360_DEGREES - 1) + dYaw;
            }
            if (dYaw < -M_180_DEGREES) {
                dYaw = (M_360_DEGREES - 1) + dYaw;
            }

            //Get magnitude of yaw diff
            if (dYaw < 0.0f) {
                dYawAbs = -dYaw;
            } else {
                dYawAbs = dYaw;
            }

            //Set turn speed based on yaw diff
            fsa->unk278 = 1.0f - (dYawAbs / (M_180_DEGREES - 1.0f));
            if (fsa->unk278 < 0.01f) {
                fsa->unk278 = 0.01f;
            }
            fsa->unk278 *= objData->turnSpeedFactor / 100.0f;

            //Use turn speed as movement speed too
            fsa->speed = fsa->unk278;

            gDLL_18_objfsa->vtbl->func6(self, fsa, baddieCurves->unk0.unk68.f[0], baddieCurves->unk0.unk68.f[2], 0, 0, 60.0f);
            fsa->animState = CAPY_ASTATE_8_Walking;

            return 0;
        }
    } else {
        Unk80009024* voxData;
        Vec3f d;
        s32 angleToTarget;
        s32 yawDiff;
        u8 curveFound;

        fsa->xAnalogInput = 0.0f;
        fsa->yAnalogInput = 0.0f;

        //Create a temporary stand-in Object representing the current curve node (in `fsa->target`able form)
        if (objData->targetStandIn == NULL) {
            DFMole_createTargetStandIn(self, objData);
        }

#if DEBUG_MOLE
        diPrintf("Dig spot state: %d\n", objData->digState);
#endif

        objData->digTimer += gUpdateRate;
        if (objData->digTimer > 60*10) {
#ifdef DEBUG_MOLE
            recomp_printf("DFmole spent ages trying to find the wall - skipping to dig!\n");
#endif
            objData->digState = DFMole_DIGSTATE_Dig;
        }

        switch (objData->digState) {
        case DFMole_DIGSTATE_Find_Dig_Site_Node:
            fsa->animState = CAPY_ASTATE_0_Standing;
            //Find the curve node just in front of the dig site
            if (baddieCurves->unkA0 && objData->targetStandIn) {
                //Put the target stand-in at its position (ignore Y, since the mole's curves are positioned in the air for some reason)
                objData->targetStandIn->srt.transl.x = baddieCurves->unkA0->pos.x;
                objData->targetStandIn->srt.transl.z = baddieCurves->unkA0->pos.z;

#if DEBUG_MOLE
                objData->targetStandIn->setup->uID = baddieCurves->unkA0->uID; //NOTE: just for debugging!
#endif
                fsa->target = objData->targetStandIn;
                objData->digState = DFMole_DIGSTATE_Turn_to_Site;
            }
            break;
        case DFMole_DIGSTATE_Turn_to_Site:
            gDLL_18_objfsa->vtbl->turn_to_target(self, fsa, updateRate, 5);

            //Get the angle from the mole to the target
            d.x = fsa->target->srt.transl.x - self->globalPosition.x;
            d.y = 0.0f;
            d.z = fsa->target->srt.transl.z - self->globalPosition.z;
            vec3Normalize(&d);

            angleToTarget = mathAtan2f(d.x, d.z) - M_180_DEGREES;
            CIRCLE_WRAP(angleToTarget);

            //Get the angle difference between current yaw and that direction
            yawDiff = angleToTarget - self->srt.yaw;
            CIRCLE_WRAP(yawDiff);
            if (yawDiff < 0) {
                yawDiff = -yawDiff;
            }

            if (yawDiff < M_45_DEGREES) {
                objData->digState = DFMole_DIGSTATE_Go_to_Site;
            }

            break;
        case DFMole_DIGSTATE_Go_to_Site:
            //Walk over to the wall, until getting very close to the target stand-in
            fsa->animState = CAPY_ASTATE_8_Walking;

            gDLL_18_objfsa->vtbl->turn_to_target(self, fsa, updateRate, 5);

            voxData = &baddie->unk34C;
            bcopy(&self->srt.transl, &voxData->unk0, sizeof(Vec3f));
            bcopy(&fsa->target->srt.transl, &baddie->unk34C.unkC, sizeof(Vec3f));
            vox_func_80009024(voxData, &baddie->unk374);

            //Store lateral target distance
            {
                d.x = fsa->target->srt.transl.x - self->globalPosition.x;
                d.y = 0.0f;
                d.z = fsa->target->srt.transl.z - self->globalPosition.z;
                fsa->targetDist = sqrtf(SQ(d.x) + SQ(d.z));
            }

#if DEBUG_MOLE
            diPrintf("targetDist: %f\n", &fsa->targetDist);
#endif

            if (fsa->targetDist < 9.0f) {
                objData->digState = DFMole_DIGSTATE_Find_Wall_Node;
                break;
            }

            if (voxData->unk25 == 0) {
                gDLL_18_objfsa->vtbl->func6(self, fsa, voxData->unk18.x, voxData->unk18.z, 0.0f, 0.0f, 60.0f);
            } else {
                gDLL_18_objfsa->vtbl->func6(self, fsa, voxData->unk18.x, voxData->unk18.z, 1.0f, 1.0f, 60.0f);
            }
            break;
        case DFMole_DIGSTATE_Find_Wall_Node:
            fsa->animState = CAPY_ASTATE_0_Standing;
            //Find the curve node behind the dig site's wall, to use as a reference for the dig direction
            if (baddieCurves->unkA4 && objData->targetStandIn) {
                //Put the target stand-in at its position (ignore Y again, since these curve nodes are also in the air for some reason)
                objData->targetStandIn->srt.transl.x = baddieCurves->unkA4->pos.x;
                objData->targetStandIn->srt.transl.z = baddieCurves->unkA4->pos.z;

#if DEBUG_MOLE
                objData->targetStandIn->setup->uID = baddieCurves->unkA4->uID; //NOTE: just for debugging!
#endif

                fsa->target = objData->targetStandIn;
                objData->digState = DFMole_DIGSTATE_Turn_to_Wall;
            }
            break;
        case DFMole_DIGSTATE_Turn_to_Wall:
            //Turn to face the digging direction, and start digging when very close to facing the correct way
            gDLL_18_objfsa->vtbl->turn_to_target(self, fsa, updateRate, 5);

            //Get the angle from the mole to the target
            d.x = fsa->target->srt.transl.x - self->globalPosition.x;
            d.y = 0.0f;
            d.z = fsa->target->srt.transl.z - self->globalPosition.z;
            vec3Normalize(&d);

            angleToTarget = mathAtan2f(d.x, d.z) - M_180_DEGREES;
            CIRCLE_WRAP(angleToTarget);

            //Get the angle difference between current yaw and that direction
            yawDiff = angleToTarget - self->srt.yaw;
            CIRCLE_WRAP(yawDiff);
            if (yawDiff < 0) {
                yawDiff = -yawDiff;
            }

            if (yawDiff < M_1_DEGREE) {
                objData->digState = DFMole_DIGSTATE_Dig;
            }
            break;
        case DFMole_DIGSTATE_Dig:
            //Dig it! And importantly: clear the mole's target
            fsa->animState = CAPY_ASTATE_7_DigWall;
            fsa->enteredAnimState = FALSE;
            fsa->unk33A = FALSE;
            fsa->logicState = CAPY_LSTATE_1_DigWall;
            fsa->target = NULL;
            break;
        }
    }

    return 0;
}

/* Extend objData */
RECOMP_PATCH u32 capy_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(Baddie) + sizeof(capy_data);
}
