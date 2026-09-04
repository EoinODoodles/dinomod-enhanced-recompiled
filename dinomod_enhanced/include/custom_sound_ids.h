#pragma once

#include "PR/ultratypes.h"

// SoundID changes (in AUDIO.bin file0)
/* TODO: The soundIDs with edited parameters are documented here,
   but it'd be good to also document any soundIDs mapping MP3s/wavs that have been edited. */
enum CustomSoundID {
    SOUND_C_Empty = 0xC,    //?

    SOUND_10_Empty = 0x10,  //?
    SOUND_11_Empty = 0x11,  //?

    SOUND_19_SwapStone_You_Cannot_Swap = 0x19,          //Rubble: "You cannot swap while holding a SpellStone or carrying a Spirit."
    SOUND_1A_SwapStone_Are_You_Ready_to_Warp = 0x1A,    //Rubble: "Are you ready to warp to Warlock Mountain?"
    SOUND_1B_WM_Rock_Slide_Sabre = 0x1B,                //Sabre: "What is it?!"
    SOUND_1C_WM_Rock_Slide_Tricky = 0x1C,               //Tricky: "I can feel something strange..."
    
    SOUND_26_SwapStone_You_Cannot_Go_to_WM = 0x26,      //Rubble: "You cannot go to Warlock Mountain until you are carrying a Spirit!"
    SOUND_27_CRF_Galleon_Spying = 0x27,                 //Scales: (laughing), SharpClaw: (laughing) "...oh."
    SOUND_28_DFPT_Scales_Nooo = 0x28,                   //Scales: "Nooooo!" (Reverb)

    SOUND_C8_CC_HighTop_Warn = 0xC8,             //HighTop: "Do not swim in these waters. The currents are strong, they will drag you straight under."
    SOUND_C9_WM_Crystal_Sun_Cutscene_1 = 0xC9,   //Quan Ata Lachu: "We are Quan Ata Lachu, six stars of eight..."
    SOUND_CA_WM_Crystal_Sun_Cutscene_2 = 0xCA,   //Sabre: "Do you mean General Scales?"
    SOUND_CB_WM_Crystal_Sun_Cutscene_3 = 0xCB,   //Quan Ata Lachu: "Not Scales..."
    SOUND_CC_WM_Crystal_Sun_Cutscene_4 = 0xCC,   //Quan Ata Lachu: "When we are one..."
    SOUND_CD_WM_Crystal_Sun_Cutscene_5 = 0xCD,   //Sabre: "Majestic Eight? ..."
    SOUND_CE_WM_Crystal_Sun_Cutscene_6 = 0xCE,   //Quan Ata Lachu: "Dinosaur Planet and your own world..."
    SOUND_CF_WM_Crystal_Sun_Cutscene_7 = 0xCF,   //Sabre: "You mean they actually exist?"
    SOUND_D0_WM_Crystal_Sun_Cutscene_8 = 0xD0,   //Quan Ata Lachu: "The Krazoa fought..."
    SOUND_D1_WM_Crystal_Sun_Cutscene_9 = 0xD1,   //Quan Ata Lachu: "If evil succeeds..."
    SOUND_D2_WM_Crystal_Sun_Cutscene_10 = 0xD2,  //Quan Ata Lachu: "GO TO THE KRAZOA..."
    SOUND_D3_WM_Crystal_Sun_Cutscene_11 = 0xD3,  //Sabre: "So that's why Krystal saw the mirage! ..."
    
    SOUND_141_SH_ThornTail_Chat_1 = 0x141,  //ThornTail: "Have you woken the SwapStone yet?"
    
    SOUND_159_DFP_Kyte_SpellStone_Holder = 0x159,  //Kyte: "Krystal! It's the SpellStone holder!"
    
    SOUND_1C7_CRF_Prison_Guard_Alert = 0x1C7,  //SharpClaw: "Wha? Who's there!? Guards!"
    
    SOUND_218_SH_Tricky_Hears_The_Queen_1 = 0x218,  //Queen EarthWalker: "Ohhh! *roar*"

    SOUND_21A_SH_Tricky_Hears_The_Queen_2 = 0x21A,  //Tricky: "That's my mom!"
    
    SOUND_389_DIMExplosion = 0x389,  //NOTE: already exists in the prototype, but the volume and falloff distance were increased so it's audible.
    
    SOUND_3F1_DIM_Sabre_Belina = 0x3F1,  //Sabre: "Belina!"

    SOUND_481_WM_Reminiscing_Arrival_1 = 0x481,  //Randorn: "Could be something to do with the Krazoa! ..."
    
    SOUND_48D_WM_Reminiscing_Arrival_2 = 0x48D,  //Krystal: "Then the forcefield enveloped the planet..."

    SOUND_490_WM_Randorn_Weakening = 0x490,  //Randorn: "As more and more magic is extracted..."
    
    SOUND_499_WM_Randorn_Fed = 0x499,  //Randorn: "Thank you my dear! Take this in return."
    
    /* NOTE: the Shackled SnowHorn's lines are already mapped in the prototype - their volume was just adjusted here */
    SOUND_594_DIM_Shackled_SnowHorn_Freed_1 = 0x594,  //(VOLUME CHANGE) Shackled SnowHorn: "I cannot thank you enough for releasing me..."
    SOUND_595_DIM_Shackled_SnowHorn_Freed_2 = 0x595,  //(VOLUME CHANGE) Shackled SnowHorn: "I found it whilst working yesterday..."
    SOUND_596_DIM_Shackled_SnowHorn_Freed_3 = 0x596,  //(VOLUME CHANGE) Sabre: "Thanks! This may come in useful."
    SOUND_597_DIM_Shackled_SnowHorn_Freed_4 = 0x597,  //(VOLUME CHANGE) Shackled SnowHorn: "If you're heading for the mine, please look out for the Guardian's daughter!"
    SOUND_598_DIM_Shackled_SnowHorn_Freed_5 = 0x598,  //(VOLUME CHANGE) Shackled SnowHorn: "Her bravery alone saved us from certain death, at the hands of General Scales."
    
    SOUND_5A4_WM_Randorn_Refuse_Food = 0x5A4,  //Randorn: "No thanks, Krystal. I don't feel so good."
    
    SOUND_6A5_SW_Geyser_Area_Sabre_Shouts_Tricky = 0x6A5,  //Sabre: "Tricky!!!"
    
    SOUND_74D_CC_Shrine_Whispers = 0x74D,  //Test of Character: "Yes... kill him..." (Custom recording by LaminGaming)
    
    SOUND_ACE_WG_Meeting_Shabunga_1 = 0xACE,  //Shabunga: "I am Shabunga, thank you for helping me."
    SOUND_ACF_WG_Meeting_Shabunga_2 = 0xACF,  //Sabre: "We weren't exactly sure that you needed help."
    SOUND_AD0_WG_Meeting_Shabunga_3 = 0xAD0,  //Tricky: "Yeah, you look kinda scary!"
    SOUND_AD1_WG_Meeting_Shabunga_4 = 0xAD1,  //Shabunga: "DO NOT BE SHOCKED BY MY APPEARANCE..."
    SOUND_AD2_WG_Meeting_Shabunga_5 = 0xAD2,  //Shabunga: "THE THORNTAILS ARE A BUNCH OF COWARDS..."
    SOUND_AD3_WG_Meeting_Shabunga_6 = 0xAD3,  //Shabunga: "This belt gives great powers to whoever wears it."
    SOUND_AD4_WG_Meeting_Shabunga_7 = 0xAD4,  //Shabunga: "THAT IS A KRAZOA STAR..."
    SOUND_AD5_WG_Meeting_Shabunga_8 = 0xAD5,  //Tricky: "So what use is it to us?"
    SOUND_AD6_WG_Meeting_Shabunga_9 = 0xAD6,  //Shabunga: "Hm. Maybe it has just enough power..."
    
    SOUND_AEF_WG_Shabunga_Chat_1 = 0xAEF,  //Shabunga: "I NEED THREE PODS TO MAKE A POTION."
    SOUND_AF0_WG_Shabunga_Chat_2 = 0xAF0,  //Shabunga: "WAIT! RETURN TO ME WITH THE SPELLSTONE..."
    SOUND_AF1_WG_Shabunga_Chat_3 = 0xAF1,  //Shabunga: "WHERE IS... THE SPELLSTONE?"
    SOUND_AF2_WG_Shabunga_Chat_4 = 0xAF2,  //Shabunga: "Hurry! Take this to the volcano..."
    
    SOUND_B5A_Wooden_Pulley_LOOP = 0xB5B,       //Same as 0x776, but zero min volume
    SOUND_B5B_Mechanism_Unlocking_Low = 0xB5B,  //Same sample as 0x1CB, but pitched way down

    SOUND_BC0_DIM_Tent_Burn_LOOP = 0xBC0,        //Same as 0x50B, but with falloff increased
    SOUND_BC1_DIM_Cannon_Rotate_LOOP = 0xBC1,    //Same as 0x1D4, but with falloff increased
    SOUND_BC2_SharpClaw_Laugh = 0xBC2,      //Same as 0x8D2, but with falloff increased
    SOUND_BC3_SharpClaw_Nyeh = 0xBC3        //Same as 0xB26, but with falloff increased
};
