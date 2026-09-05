#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "common.h"
#include "dlls/engine/33_BaddieControl.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/215_SharpClaw.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/gfx/modgfx.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objmsg.h"

#include "recomp/dlls/objects/434_DFSH_ObjCreator_recomp.h"

//TEMPORARY DEFINES
#define DFSH_ObjCreator_obj_Control DFSH_ObjCreator_control
#define DFSH_ObjCreator_obj_GetDataSize DFSH_ObjCreator_get_data_size

#define BIT_DFSH_ObjCreator_Stop 0x589
#define BIT_DF_Shrine_Activate_ObjCreator_1 0xF6
#define BIT_DF_Shrine_Activate_ObjCreator_2 0xF7
#define BIT_DF_Shrine_Activate_ObjCreator_3 0xF8
#define BIT_DF_Shrine_Activate_ObjCreator_4 0xF9
#define BIT_DF_Shrine_SharpClaw_Drop_Magic_Gems 0xFC
#define BIT_DF_Shrine_SharpClaw_Defeated 0x1E7

#define dll_player(obj) (((DLL_210_Player*)obj->dll)->vtbl)
#define dll_SharpClaw(obj) (((DLL_214_SharpClaw*)obj->dll)->vtbl)

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
//END OF TEMPORARY DEFINES

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 gamebit; // Unused in this DLL, but the gamebit used to enable the ObjCreator in the `DFSH_Shrine` DLL is usually the same as the one here.
/*1A*/ u16 _unk1A; // Unused in the DLL, but usually set to 0x1C - maybe intended as hit points?
/*1C*/ u16 _unk1C;
/*1E*/ s8 rotation; // yaw >> 8
/*1F*/ s8 creatorIndex; // objCreator's index around the Krazoa symbol, used to determine which gamebit activates it and the parameters for the SharpClaw it spawns
} DFSH_ObjCreator_Setup;

typedef struct {
/*0*/ s16 timer;
/*2*/ s16 timerRate;
/* RECOMP */
Object* sharpClaw; //Keep track of the SharpClaw that was created
s16 timerFreeSharpClaw;  //Timer for freeing the SharpClaw
} DFSH_ObjCreator_Data;

RECOMP_PATCH void DFSH_ObjCreator_obj_Control(Object* self) {
    DFSH_ObjCreator_Setup* setup = (DFSH_ObjCreator_Setup*)self->setup;
    DFSH_ObjCreator_Data* objdata = self->data;
    Baddie_Setup* sharpClawSetup;
    Object* sharpClaw;
    Baddie* sharpClawBaddie;
    DLL_IModgfx* modgfx;

    //@recomp: send a message to DFSH_Shrine when the SharpClaw is defeated 
    //(instead of using gamebits to track when a SharpClaw dies, just in case multiple SharpClaw are defeated simultaneously etc.)
    if (objdata->sharpClaw) {
        if (objdata->sharpClaw->stateFlags & OBJSTATE_DESTROYED) {
            objdata->sharpClaw = NULL;
            return;
        }

        objdata->sharpClaw->unkDC = 0;

        if (objdata->timerFreeSharpClaw) {
            objdata->timerFreeSharpClaw += gUpdateRate;
        } else {
            s32 sharpClawState = dll_SharpClaw(objdata->sharpClaw)->GetLogicState(objdata->sharpClaw);
            if (sharpClawState == SharpClaw_LSTATE_10_Dead) {
                objdata->timerFreeSharpClaw += gUpdateRate;
            } else if (sharpClawState == SharpClaw_LSTATE_9_Dying) {
                mainSetBits(BIT_DF_Shrine_SharpClaw_Defeated, TRUE);

                //@recomp: drop magic when dying, if the player's low on it 
                //(inspired by unfinished code further down in this function, maybe to encourage people to try the Forcefield Spell)
                Object* player = objGetPlayer();
                if (player) {
                    if (dll_player(player)->get_magic(player) < (dll_player(player)->get_magic_max(player) / 4)) {
                        sharpClawBaddie = objdata->sharpClaw->data;
                        sharpClawBaddie->unk3E0 = BaddieDrop_9_MagicDust_Huge;
                    } else {
                        sharpClawBaddie = objdata->sharpClaw->data;
                        sharpClawBaddie->unk3E0 = -1;
                    }
                }
            }
        }

        if (objdata->timerFreeSharpClaw > 100) {
            //Free the SharpClaw after its dying sounds finish playing
            objFreeObject(objdata->sharpClaw);
            objdata->sharpClaw = NULL;

            //Send a message to DFSH_Shrine, so it knows a SharpClaw was defeated
            objSendMesgMany(OBJ_DFSH_Shrine, OBJMSG_SEND_FILTER_ID, self, 0xDEAD, (void*)&setup->creatorIndex);
        }
        return;
    }

    //Check if the ObjCreators are deactivated
    if (mainGetBits(BIT_DFSH_ObjCreator_Stop)) {
        self->unkE0 = 0;
        return;
    }

    if ((self->unkE0 == 0) && mainGetBits(BIT_DF_Shrine_Activate_ObjCreator_1 + setup->creatorIndex)) {
        modgfx = dllLoad(DLL_ID_146, 1);
        modgfx->vtbl->func0(self, 0, 0, 1, -1, 0);
        modgfx->vtbl->func0(self, 1, 0, 1, -1, 0);
        dll_amSfx->Play(NULL, SOUND_303, MAX_VOLUME, NULL, NULL, 0, NULL);
        dllFree(modgfx);
        
        objdata->timerRate = 1;
        self->unkE0 = 1;

        //@recomp: unset the objCreator's gamebit now that a SharpClaw will be created
        mainSetBits(BIT_DF_Shrine_Activate_ObjCreator_1 + setup->creatorIndex, FALSE);
    }

    if (objdata->timerRate != 0) {
        objdata->timer -= objdata->timerRate * gUpdateRate; //@recomp: use integer rate
    }

    if (objdata->timer > 0) {
        return;
    }

    sharpClawSetup = objAllocSetup(sizeof(Baddie_Setup), OBJ_ClubSharpClaw);
    sharpClawSetup->base.x = setup->base.x;
    sharpClawSetup->base.y = setup->base.y;
    sharpClawSetup->base.z = setup->base.z;
    sharpClawSetup->base.loadFlags = setup->base.loadFlags;
    sharpClawSetup->base.byte5 = setup->base.byte5;
    sharpClawSetup->base.byte6 = setup->base.byte6;
    sharpClawSetup->base.fadeDistance = setup->base.fadeDistance;
    sharpClawSetup->initialWeaponID = 3;
    // sharpClawSetup->unk18 = BIT_DF_Shrine_SharpClaw_Defeated; //@recomp: remove gamebit here to stop the SharpClaw respawning via its own logic
    sharpClawSetup->unk30 = -1;
    sharpClawSetup->unk2A = self->srt.yaw >> 8;
    sharpClawSetup->unk2B = 2;
    sharpClawSetup->unk2B |= 8; //@recomp: don't reset position while dying (caused sparkles to jump to home position)

    //@recomp: set a uID so that later rounds' SharpClaw spawn correctly
    sharpClawSetup->base.uID = 0xBE0DF00;

    //@recomp: comment out this logic, and instead drop a Magic Gem if the player is low on magic when a SharpClaw dies
    // if (mainGetBits(BIT_DF_Shrine_SharpClaw_Drop_Magic_Gems)) {
    //     //@bug: this seems to be using an objectID, but it should be using an index from `BaddieDrop_IDs`
    //     sharpClawSetup->unk22 = BaddieDrop_9_MagicDust_Huge; //@recomp: fix
    // } else {
    //     sharpClawSetup->unk22 = -1;
    // }

    sharpClawSetup->unk29 = 0xFF;
    sharpClawSetup->unk2E = -1;
    sharpClawSetup->unk34 = 0xFFFF;

    switch (setup->creatorIndex) {
    default:
        sharpClawSetup->quarterHitpoints = 3;
        break;
    case 0:
        sharpClawSetup->quarterHitpoints = 5;
        break;
    case 1:
        sharpClawSetup->quarterHitpoints = 3;
        break;
    case 2:
        sharpClawSetup->quarterHitpoints = 4;
        break;
    case 3:
        sharpClawSetup->quarterHitpoints = 3;
        break;
    }

    sharpClaw = objSetupObject(&sharpClawSetup->base, OBJINIT_FLAG4 | OBJINIT_STANDALONE, self->mapID, -1, self->parent);
    if (sharpClaw != NULL) {
        //@recomp: keep track of the SharpClaw that was created
        objdata->sharpClaw = sharpClaw;

        //@recomp: make sure the SharpClaw doesn't try to play a sequence when spawning
        objdata->sharpClaw->unkE0 = 1;
        objdata->sharpClaw->unkDC = 0;

        sharpClawBaddie = sharpClaw->data;
        if (sharpClawBaddie != NULL) {
            switch (setup->creatorIndex) {
            default:
                sharpClawBaddie->unk3B0 = 0x20;
                break;
            case 0:
                sharpClawBaddie->unk3B0 = 0x20;
                break;
            case 1:
                sharpClawBaddie->unk3B0 = 0x20;
                break;
            case 2:
                //@recomp: only use the extra-tough guarding flags on the last SharpClaw, so it isn't annoyingly difficult
                // sharpClawBaddie->unk3B0 = 0x80 | 0x20;
                sharpClawBaddie->unk3B0 = 0x20; 
                break;
            case 3:
                sharpClawBaddie->unk3B0 = 0x80 | 0x20;
                break;
            }
        }

    }
    
    objdata->timer = 100;
    objdata->timerRate = 0;

    //@recomp: reset modGfx and timerFreeSharpClaw too
    objdata->timerFreeSharpClaw = 0;
    self->unkE0 = 0;
}

/* Extend objData */
RECOMP_PATCH u32 DFSH_ObjCreator_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(DFSH_ObjCreator_Data);
}
