#include "modding.h"

#include "common.h"
#include "dlls/engine/53_movelib.h"
#include "game/gametexts.h"
#include "sys/gfx/animseq.h"
#include "sys/objanim.h"
#include "sys/segment_1050.h"

#include "recomp/dlls/objects/713_DR_EarthWarrior_recomp.h"

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 rotation;
/*19*/ s8 unk19;
} DRearthwalk_Setup;

typedef struct {
/*000*/ ObjFSA_Data fsa;
/*34C*/ u8 _unk34C[0x370 - 0x34C];
/*370*/ HeadAnimation unk370;
/*394*/ HeadAnimation unk394;
/*3B8*/ MoveLibData movedata;
/*870*/ UnkCurvesStruct unk870;
/*978*/ Vec3f unk978[4];
/*9A8*/ u8 _unk9A8[0x9B4 - 0x9A8];
/*9B4*/ f32 unk9B4;
/*9B8*/ f32 unk9B8;
/*9BC*/ f32 unk9BC;
/*9C0*/ u8 _unk9C0[0xA52 - 0x9C0];
/*A52*/ s16 unkA52;
/*A54*/ u8 _unkA54[0xA56 - 0xA54];
/*A56*/ s16 energy;
/*A58*/ u16 unkA58;
/*A5A*/ u8 unkA5A;
/*A5B*/ u8 _unkA5B;
/*A5C*/ u8 unkA5C;
/*A5D*/ u8 _unkA5D;
/*A5E*/ u8 unkA5E;
/*A5F*/ u8 unkA5F;
/*A60_0*/ u8 unkA60_0 : 1;
/*A60_1*/ u8 unkA60_1 : 1;
/*A60_2*/ u8 unkA60_2 : 1;
/*A60_3*/ u8 unkA60_3 : 1;
/*A60_4*/ u8 rideable : 1;
/*A60_5*/ u8 unkA60_5 : 1;
/*A61*/ s8 unkA61;
/*A62*/ s8 talkSeq;
/*A63*/ s8 unkA63;
} DRearthwalk_Data;

enum DREWSeq {
    EWSEQ_0_EatBlueMushroom_Laying = 0,
    EWSEQ_1_EatBlueMushroom_Standing = 1,
    EWSEQ_2_LetsGoStopTheDragon = 2,
    EWSEQ_3_INeedEnergy = 3,
    EWSEQ_4 = 4,
    EWSEQ_5_SabreHopOn_Right = 5,
    EWSEQ_6_SabreHopOn_Left = 6,
    EWSEQ_7_SabreHopOff_Left = 7,
    EWSEQ_8_SabreHopOff_Right = 8,
    EWSEQ_9_IfICouldGetOutOfTheseChains = 9,
    EWSEQ_10_TeethChattering = 10
};

/** Allow a summoned Earth Warrior to be ridden if he was already brought to the surface. */
RECOMP_HOOK_DLL(DRearthwalk_setup) void dll_713_setup_hook(Object* self, DRearthwalk_Setup* setup, s32 arg2) {
    if (main_get_bits(BIT_DR_EarthWarriorBroughtToSurface)) { // bit 0x656
        setup->unk19 = 0;
    }
}

//Stops the shackled EarthWalker from going dark too early when entering tunnels (originally by MusicalProgrammer)
RECOMP_PATCH s32 DRearthwalk_func_32EC(Object* self, u8 arg1) {
    DRearthwalk_Data* objdata;

    objdata = self->data;
    switch (arg1) {
        case 5:
            objdata->unkA58 |= 0x80;
            objdata->talkSeq = EWSEQ_10_TeethChattering;
            //Set Mind Read text
            gDLL_22_Subtitles->vtbl->func_21C0(self->id, GAMETEXT_0CF_DR_Mind_Read_messages_4);
            break; //@recomp: don't fallthrough
        case 1:
            objdata->talkSeq = EWSEQ_2_LetsGoStopTheDragon;
            func_80000450(self, self, 0x22C, 0, 0, 0);
            break;
        case 2:
            func_80000450(self, self, 0x22E, 0, 0, 0);
            objdata->unkA58 &= ~0x80;
            //Set Mind Read text
            gDLL_22_Subtitles->vtbl->func_21C0(self->id, GAMETEXT_0D0_DR_Mind_Read_messages_5);
    }

    return 1;
}
