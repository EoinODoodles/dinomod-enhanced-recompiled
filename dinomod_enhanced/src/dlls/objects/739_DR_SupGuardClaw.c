#include "modding.h"

#include "macros.h"
#include "PR/ultratypes.h"
#include "game/objects/object.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/objanim.h"
#include "sys/objects.h"
#include "sys/objexpr.h"
#include "sys/objhits.h"
#include "dll.h"
#include "dlls/engine/53_movelib.h"
#include "dlls/objects/210_player.h"

#include "recomp/dlls/objects/739_DR_SupGuardClaw_recomp.h"

typedef struct {
    s16 soundID;
    s16 unk2;
} TalkSounds;

typedef struct {
    ObjSetup base;
    s8 yaw;
    s8 characterType;    //0: SharpClaw, 1: GuardClaw
    s16 unused1A;
    s16 unused1C;
    s16 gamebitFinished; //Gamebit set when the character's interactions are finished (SharpClaw fed, GuardClaw tricked)
} DR_NPC_Setup;

typedef struct {
    u8 _unk0[0x4A9 - 0];
    u8 unk4A9;
    u8 _unk4AA[0x4DC - 0x4AA];
    HeadAnimation headAnimLook;
    HeadAnimation headAnimTalk;
    UnkFunc_80024108Struct animData;
    TalkSounds* soundsTalk;                 //Character sounds played at intervals
    s32 (*behaviourFunction)(Object* self); //Character-specific behaviour called from the control function. Returns TRUE when character's interactions are finished.
    s16* soundsAttack;                      //Sounds to play when attacking (only used by GuardClaw)
    s16* modAnims;                          //An array of modAnimIDs
    f32 animSpeed;                          //Animation playback speed
    s16 talkTimer;                          //Randomised talk countdown timer
    u16 finished : 1;                       //Character sets gamebit and disappears afterwards
} DR_NPC_Data;

typedef enum {
    DR_NPC_SharpClaw,
    DR_NPC_GuardClaw
} DR_NPC_CharacterTypes;

/**
  * (DR_NiceSharpy)
  * Stop cutscene from triggering too early (originally by MusicalProgrammer) 
  */
RECOMP_PATCH int DR_NPC_anim_callback(Object* self, Object*arg1, AnimObj_Data* animData) {
    DR_NPC_Data* objData;
    DR_NPC_Setup* objSetup;
    Object* player;
    s16 modAnimID;
    s32 foodGamebit;
    s32 i;

    objData = self->data;
    player = get_player();
    objSetup = (DR_NPC_Setup*)self->setup;

    func_8002674C(self);
    func_80028D2C(self);

    //GuardClaw sequence subevents
    for (i = 0; i < animData->messageCount; i++) {
        if ((animData->messages[i] == 1) && (objSetup->characterType != DR_NPC_SharpClaw)) {
            obj_free_tick(self);
            func_800267A4(self);
            self->srt.flags |= OBJFLAG_INVISIBLE;
        }
    }

    modAnimID = objData->modAnims[2];
    if (((DLL_53_movelib*)(gTempDLLInsts[1]))->vtbl->func4(self, animData, (MoveLibData*)objData, modAnimID, modAnimID) != 0) {
        return 1;
    }

    //Feeding the SharpClaw
    foodGamebit = ((DLL_210_Player*)player->dll)->vtbl->func74(player);
    if (foodGamebit != NO_GAMEBIT) {
        if ((foodGamebit == BIT_Dino_Egg_Count) || (foodGamebit == BIT_Moldy_Meat_Count)) {
            STUBBED_PRINTF(" Edanble FoodType %i ", foodGamebit);
            // ((DLL_210_Player*)player->dll)->vtbl->func75(player, 1);
            ((DLL_210_Player*)player->dll)->vtbl->func75(player, 0);
            
            //@recomp: don't set gamebit here
            // main_set_bits(objSetup->gamebitFinished, 1);
            // STUBBED_PRINTF(" \n Have Set Bit %i \n", objSetup->gamebitFinished);

            gDLL_3_Animation->vtbl->end_obj_sequence(self->seqSlot);
            return 0;
        } else {
            STUBBED_PRINTF(" FoodType %i ", foodGamebit);
            ((DLL_210_Player*)player->dll)->vtbl->func75(player, 2);
        }
    }

    return 0;
}
