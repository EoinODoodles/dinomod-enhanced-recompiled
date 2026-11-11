#include "dlls/engine/29_gplay.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "sys/math.h"
#include "macros.h"

#include "custom_gamebits.h"
#include "chapter_select.h"

// TODO: Most newlfx actions set here are probably wrong. The game doesn't consider which object the
//       action is attached to and always restores it to being on the player, which is often incorrect.
//       Newlfx should probably be patched to not persist non-player actions and then each chapter will
//       need to be updated.
#define CHSELECT_NEWLFX 0

static void ch_stub(void) { }

static void ch_ship_battle(void) { }

// krystal intro
static void ch_wl_1(void) {
    chselect_set_location(PLAYER_KRYSTAL, 13630.91f, 211.0f, -620.5f, M_180_DEGREES, 0);
    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 120;
    light.skylights[1] = 119;
#if CHSELECT_NEWLFX
    light.newlfx = 93;
#endif
    chselect_set_light(PLAYER_KRYSTAL, &light);
    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 165;
    env.unk6 = 166;
    env.unk8 = 168;
    // the rest might be overkill? ship battle leftovers?
    env.unkA = 100;
    env.unkC = 150;
    env.unkE = 151;
    env.unk10 = -11319; env.unk14 = 786; env.unk18 = -14984;
    env.unk1C = -13714; env.unk20 = 79; env.unk24 = -16441;
    env.unk28 = -14363; env.unk2C = 14; env.unk30 = -16480;
    chselect_set_env(PLAYER_KRYSTAL, &env);
    chselect_set_music(PLAYER_KRYSTAL, 54, -1, 72, -1);
    chselect_set_map_state(MAP_WARLOCK_MOUNTAIN, 1, G1);

    // tutorials completed on the Galleon
    mainSetBits(BIT_Tutorial_Magic_Crystal, 1);
    mainSetBits(BIT_Tutorial_Collected_Energy_Egg, 1);

    mainSetBits(BIT_SB_Battle_Started, 1);
    mainSetBits(BIT_SB_Found_Kyte, 1);
    mainSetBits(BIT_Galleon_Arrived_at_Warlock_Mountain, 1);
    mainSetBits(BIT_WL_Load_Unload_Galleon, 1);
}

static void ch_ice_mountain(void) {
    /* ---- during WL ---- */
    // move WL_Column_Top onto the pressure switch
    chselect_move_setup(0x428E0, MAP_WARLOCK_MOUNTAIN, 12357.0f, -83.7f, 154.53f);

    mainSetBits(BIT_Spell_Projectile, 1);
    mainSetBits(BIT_Play_Seq_00EF_Scales_Escapes_With_Kyte, 1);
    mainSetBits(BIT_Krystal_Used_Warp_Crystal, 1);
    mainSetBits(BIT_WM_EarthWalker_Died, 1);
    mainSetBits(BIT_Krystal_Warp_Crystal, 1);
    mainSetBits(BIT_WM_Played_Randorn_First_Meeting, 1);
    mainSetBits(BIT_WL_Load_Unload_Galleon, 0);
    mainSetBits(BIT_WM_Near_Cave_Exit_Krystal_Side, 1); // ?
    mainSetBits(BIT_SP_Foodbag_S, 1);
    mainSetBits(BIT_53A, 1); // what in the world sets this
    mainSetBits(BIT_8B0, 1); // WL_seqpoint (which one)
    mainSetBits(BIT_8F4, 1); // WL_seqpoint cond (which one)
    mainSetBits(BIT_903, 1); // projectile switch
    mainSetBits(BIT_Shown_ZLock_Interact_Message, 1);
    mainSetBits(BIT_Tutorial_Magic_Plant, 1);
    mainSetBits(BIT_Tutorial_ZLock_Talk, 1);

    //chselect_set_map_state(MAP_WARLOCK_MOUNTAIN, 1, G2); // before warp

    mainSetBits(BIT_Kyte_Flight_Curve, 121);

    /* ---- warp to SC ---- */
    mainSetBits(BIT_Talking_to_Rocky, 1);
    chselect_set_map_state(MAP_WARLOCK_MOUNTAIN, 1, G_None);
    chselect_set_map_state(MAP_SWAPSTONE_CIRCLE, 2, G0);

    chselect_set_location(PLAYER_KRYSTAL, -1057.8f, 304.0f, 17436.0f, M_180_DEGREES, 0);
    {
        PlayerLightActions light = DEFAULT_LACTIONS;
        light.skylights[0] = 485;
        light.skylights[1] = 486;
        light.trklights[0] = 484;
#if CHSELECT_NEWLFX
        light.newlfx = 126;
#endif
        chselect_set_light(PLAYER_KRYSTAL, &light);
        PlayerEnvActions env = DEFAULT_ENVACTIONS;
        env.unk4 = 79; // depends on time of day in SC
        env.unk6 = 80;
        env.unk8 = 81;
        chselect_set_env(PLAYER_KRYSTAL, &env);
    }
    chselect_set_music(PLAYER_KRYSTAL, 185, -1, 203, -1);

    /* ---- swap to sabre ---- */
    chselect_set_player(PLAYER_SABRE);
    //chselect_set_location(PLAYER_SABRE, 370.72f, 6382.78f, 2406.9f, 0, 2); // before cutscene

    // skip intro cutscene
    {
        mainSetBits(BIT_Play_Seq_0063_IM_Sabre_Intro, 1);
        mainSetBits(BIT_Played_Seq_0063_IM_Sabre_Intro, 1);
        
        chselect_set_map_state(MAP_ICE_MOUNTAIN_1, 1, G0 | G3 | G4 | G5 | G7);
        
        PlayerLightActions light = DEFAULT_LACTIONS;
        light.skylights[0] = 347;
        light.skylights[1] = 348;
        light.skylights[2] = 380;
        light.skylights[3] = 356;
#if CHSELECT_NEWLFX
        light.newlfx = 6;
#endif
        chselect_set_light(PLAYER_SABRE, &light);
        
        PlayerEnvActions env = DEFAULT_ENVACTIONS;
        env.unk4 = 260;
        env.unk6 = 158;
        env.unk8 = 163;
        env.unkA = 256;
        env.unkC = 42;
        env.unk10 = 496; env.unk14 = 6488; env.unk18 = 2236;
        env.unk1C = 553; env.unk20 = 6485; env.unk24 = 2252;
        env.unk3D = 1;
        chselect_set_env(PLAYER_SABRE, &env);
        
        chselect_set_music(PLAYER_SABRE, 155, -1, 151, -1);

        chselect_set_location(PLAYER_SABRE, 493.75f, 6488.87f, 2232.4f, M_90_DEGREES, 2);
    }
}

static void ch_snowhorn_wastes_1(void) {
    /* ice mountain/race */
    mainSetBits(BIT_6F, 1); // IMMultiSeq
    mainSetBits(BIT_IM_MultiSeq_2, 1);
    mainSetBits(BIT_IM_MultiSeq_3, 1);
    mainSetBits(BIT_377, 1); // IMPrisonGate restore bit
    mainSetBits(BIT_382, 1); // ice mountain projectile switch
    mainSetBits(BIT_3AE, 1); // IMMultiSeq
    mainSetBits(BIT_3B9, 1); // IMFence openA
    mainSetBits(BIT_3BA, 1); // IMIcicle enabled bit

    chselect_set_map_state(MAP_ICE_MOUNTAIN_1, 2, G1);

    /* snowhorn wastes */
    mainSetBits(BIT_Play_Seq_00EA_IM_Sabre_Falls_Into_Hot_Spring, 1);
    mainSetBits(BIT_37A, 1); // BIT_Played_Seq_00EA_IM_Sabre_Falls_Into_Hot_Spring
    mainSetBits(BIT_IM_Race_Ended, 1);
    mainSetBits(BIT_Tricky_Talk_Sequence, 1);
    mainSetBits(BIT_4E3, 255); // tricky related

    chselect_set_map_state(MAP_SNOWHORN_WASTES, 1, G_None);

    chselect_set_location(PLAYER_SABRE, -2420.59f, -73.43f, 936.64f, M_90_DEGREES, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 389;
    light.skylights[1] = 390;
    light.skylights[2] = 394;
    light.skylights[3] = 356;
    light.trklights[0] = 392;
#if CHSELECT_NEWLFX
    light.newlfx = 145;
#endif
    light.cam = 1;
    chselect_set_light(PLAYER_SABRE, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 180;
    env.unk6 = 182;
    env.unk8 = 181;
    env.unkA = 38;
    env.unkC = 42;
    env.unk10 = 162; env.unk14 = 6530; env.unk18 = 1921;
    env.unk1C = 553; env.unk20 = 6485; env.unk24 = 2252;
    chselect_set_env(PLAYER_SABRE, &env);

    chselect_set_music(PLAYER_SABRE, 66, -1, 151, -1);
}

static void ch_swapstone_hollow_1(void) {
    chselect_set_min_magic(PLAYER_SABRE, 5);

    /* snowhorn wastes tutorial area */
    mainSetBits(BIT_6, 1); // NWtree 0xF0C falls
    mainSetBits(BIT_7, 1); // NWtree 0x30312 falls
    mainSetBits(BIT_8, 1); // set by NWtricky after the tutorial SharpClaw is defeated, enables some TriggerPoint in every shrine?
    mainSetBits(BIT_SW_Grumpy_SnowHorn_Stopped_Geyser, 1);
    mainSetBits(BIT_SW_Tricky_Ball_Collected, 1);
    mainSetBits(BIT_SnowHorn_Tutorial_GotAlpineRoot1, 1);
    mainSetBits(BIT_A8, 1); // WallAnimator 0x30637 dug bit
    mainSetBits(BIT_AB, 1); // WallAnimator 0x30A21 dug bit
    mainSetBits(BIT_SnowHorn_Tutorial_Defeated_SharpClaw, 1);
    mainSetBits(BIT_SW_Tricky_Toy_Unearthed, 1);
    mainSetBits(BIT_112, 1); // GroundAnimator 0x1743 dug bit
    mainSetBits(BIT_113, 1); // GroundAnimator 0x3047F dug bit
    mainSetBits(BIT_SnowHorn_Tutorial_GotAlpineRoot2, 1);
    mainSetBits(BIT_Tutorial_Collected_Blue_Mushroom, 1);
    mainSetBits(BIT_188, 1); // snowhorn wastes levelname shown
    mainSetBits(BIT_Tricky_Ball_Unlocked, 1);
    mainSetBits(BIT_4D3, 1); // NWboulder 0x30398
    mainSetBits(BIT_4D4, 1); // set/used by NWtricky
    mainSetBits(BIT_Tricky_Talk_Sequence, 145);
    mainSetBits(BIT_4E3, 1); // tricky related
    mainSetBits(BIT_Tricky_Unlocked_Sidekick_Commands, 1);
    mainSetBits(BIT_SnowHorn_Tutorial_NumRootsFed, 2);
    mainSetBits(BIT_587, 1); // ?? what sets this
    mainSetBits(BIT_Map_SW, 1);
    mainSetBits(BIT_90F, 1); // ?? what sets this
    mainSetBits(BIT_Tutorial_Collected_Scarab, 1);
    mainSetBits(BIT_913, 1); // push/pull tutorial?
    mainSetBits(BIT_916, 1); // GroundAnimator 0x42E3F dug bit
    mainSetBits(BIT_UI_Scarab_Counter_Enabled, 1);

    chselect_set_min_scarabs(PLAYER_SABRE, 10);

    chselect_set_map_state(MAP_ICE_MOUNTAIN_1, 2, G_None);
    chselect_set_map_state(MAP_SNOWHORN_WASTES, 1, G_None);

    /* snowhorn wastes */
    mainSetBits(BIT_26, 1); // NWtreebridge

    /* tunnel to swapstone hollow */
    chselect_set_location(PLAYER_SABRE, -6984.36f, -830.41f, -569.07f, M_90_DEGREES, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 389;
    light.skylights[1] = 390;
    light.skylights[2] = 394;
    light.skylights[3] = 356;
    light.trklights[0] = 392;
#if CHSELECT_NEWLFX
    light.newlfx = 145;
#endif
    light.cam = 61;
    chselect_set_light(PLAYER_SABRE, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 180;
    env.unk6 = 182;
    env.unk8 = 181;
    env.unkA = 100;
    //env.unkC = 42; // the evil snow that never goes away
    env.unk10 = -2497; env.unk14 = -55; env.unk18 = 983;
    chselect_set_env(PLAYER_SABRE, &env);

    chselect_set_music(PLAYER_SABRE, 281, -1, 65, -1);
}

static void ch_swapstone_circle_1(void) {
    /* swapstone hollow 1 (queen earthwalker quest) */
    mainSetBits(BIT_13, 1); // SHbluemushroom 0x308F3 collected bit
    mainSetBits(BIT_A1, 1); // DINOMOD SPECIFIC: stops SHseqobject 0x34841 from replaying
    mainSetBits(BIT_SH_Queen_EW_Quest_Progress, 7);
    mainSetBits(BIT_SH_Move_Thorntail_Blocking_Hollow_Log, 1);
    mainSetBits(BIT_SH_Queen_EW_White_Mushrooms_Eaten, 10);
    mainSetBits(BIT_F4, 1); // SHbluemushroom 0x308F6 collected bit
    mainSetBits(BIT_F5, 1); // SHbluemushroom 0x308F2 collected bit
    mainSetBits(BIT_15C, 1); // SHbluemushroom 0x308F4 collected bit
    mainSetBits(BIT_177, 1); // SHbluemushroom 0x308F5 collected bit
    //mainSetBits(BIT_2B2, 1); // SC_sequences 0x2733 played bit
    mainSetBits(BIT_460, 1); // SHbluemushroom 0x307D5 collected bit
    mainSetBits(BIT_461, 1); // SHbluemushroom 0x429E9 collected bit
    mainSetBits(BIT_462, 1); // SHbluemushroom 0x307D4 collected bit
    mainSetBits(BIT_463, 1); // SHbluemushroom 0x30832 collected bit
    mainSetBits(BIT_464, 1); // SHbluemushroom 0x30844 collected bit
    mainSetBits(BIT_484, 1); // VFP SHseqobject 0x30AFA played bit
    mainSetBits(BIT_485, 1); // VFP SHseqobject 0x30AFC played bit
    mainSetBits(BIT_Play_Seq_0298_Queen_Shows_VFPT, 1); // VFP SHseqobject 0x30AFA played bit
    mainSetBits(BIT_Play_Seq_0299_Queen_Shows_SpellStones, 1); // VFP SHseqobject 0x30AFC played bit
    mainSetBits(BIT_Tricky_Talk_Sequence, 201);
    mainSetBits(BIT_4E3, 255); // tricky related
    mainSetBits(BIT_658, 1); // ExplodeAnimator 0x348E6
    mainSetBits(BIT_659, 1); // ExplodeAnimator 0x348E9
    mainSetBits(BIT_65A, 1); // ExplodeAnimator 0x348EE
    mainSetBits(BIT_8D2, 1); // SwapStone Hollow levelname shown
    mainSetBits(BIT_SH_Move_Thorntail_Blocking_Swapstone, 1);

    //chselect_set_map_state(MAP_SWAPSTONE_HOLLOW, 1, G0);
    chselect_set_map_state(MAP_VOLCANO_FORCE_POINT_TEMPLE, 11, G_None);

    chselect_set_min_scarabs(PLAYER_SABRE, 30);

    chselect_set_location(PLAYER_SABRE, -6571.54f, -632.0f, -1607.57f, 0, 0);

    {
        PlayerLightActions light = DEFAULT_LACTIONS;
        light.skylights[0] = 389;
        light.skylights[1] = 401;
        light.skylights[2] = 402;
        light.skylights[3] = 403;
        light.trklights[0] = 392;
#if CHSELECT_NEWLFX
        light.newlfx = 2;
#endif
        light.cam = 1;
        chselect_set_light(PLAYER_SABRE, &light);

        PlayerEnvActions env = DEFAULT_ENVACTIONS;
        env.unk4 = 97;
        env.unk6 = 95;
        env.unk8 = 96;
        env.unkA = 100;
        env.unk10 = -2497; env.unk14 = -55; env.unk18 = 983;
        chselect_set_env(PLAYER_SABRE, &env);

        chselect_set_music(PLAYER_SABRE, 281, -1, 1 /*5*/, -1);
    }

    mainSetBits(BIT_Talked_to_Rocky, 1);
    mainSetBits(BIT_883, 1); // ?
    mainSetBits(BIT_Play_Seq_035F_Rocky_Intro, 1);
    mainSetBits(BIT_Talking_to_Rocky, 1);

    /* swap to krystal */
    chselect_set_player(PLAYER_KRYSTAL);

    mainSetBits(BIT_2B2, 0);

    chselect_set_map_state(MAP_SWAPSTONE_HOLLOW, 1, G_None);
    chselect_set_map_state(MAP_SWAPSTONE_CIRCLE, 2, G0);

    chselect_set_location(PLAYER_KRYSTAL, -1048.82f, 307.0f, 17424.86f, 0, 0);

    {
        PlayerLightActions light = DEFAULT_LACTIONS;
        light.skylights[0] = 485;
        light.skylights[1] = 486;
        light.trklights[0] = 484;
#if CHSELECT_NEWLFX
        light.newlfx = 126;
#endif
        light.cam = 1;
        chselect_set_light(PLAYER_KRYSTAL, &light);

        PlayerEnvActions env = DEFAULT_ENVACTIONS;
        env.unk4 = 79;
        env.unk6 = 80;
        env.unk8 = 81;
        chselect_set_env(PLAYER_KRYSTAL, &env);

        chselect_set_music(PLAYER_KRYSTAL, 185, -1, 18, -1);
    }
}

static void ch_cape_claw_1(void) {
    /* swapstone circle */
    mainSetBits(BIT_Talking_to_Rocky, 0);
    mainSetBits(BIT_Kyte_Flight_Curve, 50);
    mainSetBits(BIT_8C3, 1); // SwapStone Circle levelname shown

    chselect_set_map_state(MAP_SWAPSTONE_CIRCLE, 2, G1 | G6);
    
    chselect_set_min_scarabs(PLAYER_KRYSTAL, 20);

    /* cc start */
    chselect_set_location(PLAYER_KRYSTAL, 1403.51f, 158.35f, 16944.01f, -M_90_DEGREES, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 485;
    light.skylights[1] = 486;
    light.trklights[0] = 484;
#if CHSELECT_NEWLFX
    light.newlfx = 480;
#endif
    light.cam = 1;
    chselect_set_light(PLAYER_KRYSTAL, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 434;
    env.unk6 = 435;
    env.unk8 = 9;
    chselect_set_env(PLAYER_KRYSTAL, &env);

    chselect_set_music(PLAYER_KRYSTAL, 218, -1, 187, -1);
}

static void ch_cape_claw_1_courtyard(void) {
    // gas room "CCboulder"s
    chselect_move_setup(0x42DEA, MAP_CAPE_CLAW, 3563.9375f,   -102.0f, 18277.742188f);
    chselect_move_setup(0x42DE9, MAP_CAPE_CLAW, 3606.336914f, -102.0f, 18309.619141f);
    chselect_move_setup(0x42DE8, MAP_CAPE_CLAW, 3531.695313,  -102.0f, 18309.619141f);
    chselect_move_setup(0x42DE7, MAP_CAPE_CLAW, 3563.9375f,   -102.0f, 18342.292969f);

    mainSetBits(BIT_CC_Bribed_GuardClaw, 1);
    mainSetBits(BIT_8B, 1); // CCseqpoint 0x2CD9
    mainSetBits(BIT_CC_Completed_Gas_Chamber_Puzzle, 1);
    mainSetBits(BIT_168, 1); // CClogpush
    mainSetBits(BIT_187, 1); // Cape Claw levelname shown
    mainSetBits(BIT_Spell_Forcefield, 1);
    mainSetBits(BIT_Played_Seq_022F_CC_Lightfoot_Gives_Spellpage, 1);
    mainSetBits(BIT_246, 1); // ?? what sets this
    mainSetBits(BIT_295, 1); // ?? what sets this
    mainSetBits(BIT_Gold_Nugget_CC, 1);
    mainSetBits(BIT_3A0, 1); // ?
    mainSetBits(BIT_Play_Seq_022F_CC_Lightfoot_Gives_Spellpage, 1);
    mainSetBits(BIT_3B8, 1); // CCclawdoor restore bit
    mainSetBits(BIT_3C1, 1); // CCclawdoor open bit B
    mainSetBits(BIT_3C2, 1); // CCclawdoor open bit A
    mainSetBits(BIT_CC_Shot_Down_Lightfoot_Cage, 1);
    mainSetBits(BIT_3E6, 1); // CCdockdoor1 0x3032D
    mainSetBits(BIT_3E7, 1); // CCdockdoor2 0x3435D
    mainSetBits(BIT_Kyte_Flight_Curve, 1);
    mainSetBits(BIT_603, 1); // CCseqobject 0x34046 played bit
    mainSetBits(BIT_627, 1); // golden nugget count

    //mainSetBits(BIT_CC_Courtyard_Gate_Left_Lever_or_Lantern, 1);
    //mainSetBits(BIT_CC_Courtyard_Gate_Right_Lever_or_Lantern, 1);
    //mainSetBits(BIT_CC_Gate_Opened, 1);

    chselect_set_map_state(MAP_CAPE_CLAW, 1, G3 | G4 | G13);
    chselect_set_map_state(MAP_SWAPSTONE_CIRCLE, 2, G_None);

    /* courtyard start (before gate) */
    chselect_set_location(PLAYER_KRYSTAL, 4336.79f, -8.0f, 17994.89f, -M_90_DEGREES, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 485;
    light.skylights[1] = 498;
    light.trklights[0] = 484;
    light.cam = 1;
    chselect_set_light(PLAYER_KRYSTAL, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 302;
    env.unk6 = 304;
    env.unk8 = 303;
    chselect_set_env(PLAYER_KRYSTAL, &env);

    chselect_set_music(PLAYER_KRYSTAL, 218, -1, 115, -1);
}

// LightFoot ambush
static void ch_lightfoot_village_1(void) {
    /* cape claw courtyard */
    mainSetBits(BIT_CC_Courtyard_Gate_Left_Lever_or_Lantern, 1);
    mainSetBits(BIT_CC_Courtyard_Gate_Right_Lever_or_Lantern, 1);
    mainSetBits(BIT_CC_Gate_Opened, 1);

    mainSetBits(BIT_5, 1); // grub tutorial bit (will break shrine FXemits without dinomod patches)
    mainSetBits(BIT_CC_Rescued_Kyte, 1);
    //mainSetBits(BIT_372, 1); // sharpclaw 0x34566 defeated
    //mainSetBits(BIT_390, 1); // sharpclaw 0x34561 defeated
    mainSetBits(BIT_CC_Kyte_Pulled_All_Four_Levers, 1);
    mainSetBits(BIT_3CB, 1); // kyte commands enabled
    mainSetBits(BIT_CC_Cell_Door_Key, 1);
    mainSetBits(BIT_Kyte_Flight_Curve, 50);
    mainSetBits(BIT_Kyte_Flight_Talk_Sequence, 4);
    mainSetBits(BIT_CC_Courtyard_Gate_Reopened, 1);

    /* cape claw exit */
    chselect_set_map_state(MAP_CAPE_CLAW, 2, G1); // set on the way out
    chselect_set_map_state(MAP_SWAPSTONE_CIRCLE, 2, G1 | G6);

    chselect_set_location(PLAYER_KRYSTAL, 1403.51f, 158.35f, 16944.01f, M_90_DEGREES, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 485;
    light.skylights[1] = 497;
    light.trklights[0] = 484;
    light.cam = 1;
    chselect_set_light(PLAYER_KRYSTAL, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 302;
    env.unk6 = 304;
    env.unk8 = 303;
    chselect_set_env(PLAYER_KRYSTAL, &env);

    chselect_set_music(PLAYER_KRYSTAL, 218, -1, 280, -1);
}

static void ch_swapstone_circle_2(void) {
    /* lightfoot ambush */
    mainSetBits(BIT_SC_Encountered_Baby_Lightfoot, 1);
    mainSetBits(BIT_2BB, 1); // TriggerPlane 0x33977 restore bit
    mainSetBits(BIT_2CC, 1); // SC_sequences 0x281F played bit
    mainSetBits(BIT_2D0, 1); // SC_sequences 0x2829 played bit
    mainSetBits(BIT_Kyte_Flight_Curve, 8);
    mainSetBits(BIT_SC_Ambushed, 1);
    chselect_set_map_state(MAP_SWAPSTONE_CIRCLE, 3, G0);

    /* path from lfv to sc */
    chselect_set_location(PLAYER_KRYSTAL, -287.468f, 286.740f, 16947.609f, 17920, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 485;
    light.skylights[1] = 486;
    light.trklights[0] = 484;
    light.cam = 1;
    chselect_set_light(PLAYER_KRYSTAL, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 79;
    env.unk6 = 80;
    env.unk8 = 9;
    env.unkA = 451;
    env.unk10 = -1073; env.unk14 = 394; env.unk18 = 17380;
    chselect_set_env(PLAYER_KRYSTAL, &env);

    chselect_set_music(PLAYER_KRYSTAL, 185, -1, 18, -1);
}

static void ch_discovery_falls(void) {
    /* swapstone circle beacons */
    mainSetBits(BIT_SC_Beacon_Lit_1, 1);
    mainSetBits(BIT_SC_Beacon_Lit_2, 1);
    mainSetBits(BIT_SC_Beacon_Lit_3, 1);
    mainSetBits(BIT_SC_Beacon_Added_Twigs_1, 1);
    mainSetBits(BIT_SC_Beacon_Added_Twigs_2, 1);
    mainSetBits(BIT_SC_Beacon_Added_Twigs_3, 1);
    mainSetBits(BIT_SC_All_Beacons_Lit, 1);
    mainSetBits(BIT_2D8, 1); // set by seq 0x77
    mainSetBits(BIT_2D9, 1); // HitAnimator 0xBE0400D activate bit

    chselect_set_map_state(MAP_CAPE_CLAW, 2, G_None); // for some reason this gets cleared during the beacon quest?

    /* discovery falls start */
    mainSetBits(BIT_Kyte_Flight_Curve, 61);

    chselect_set_map_state(MAP_DISCOVERY_FALLS, 1, G1);
    chselect_set_map_state(MAP_SWAPSTONE_CIRCLE, 3, G_None);

    chselect_set_location(PLAYER_KRYSTAL, -2097.08f, 160.27f, 18309.26f, 17408, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 485;
    light.skylights[1] = 36;
    light.trklights[0] = 484;
#if CHSELECT_NEWLFX
    light.newlfx = 90;
#endif
    light.cam = 1;
    chselect_set_light(PLAYER_KRYSTAL, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 55;
    env.unk6 = 53;
    env.unk8 = 54;
    env.unkA = 451;
    env.unk10 = -1073; env.unk14 = 394; env.unk18 = 17380;
    chselect_set_env(PLAYER_KRYSTAL, &env);

    chselect_set_music(PLAYER_KRYSTAL, 218, -1, 13, -1);
}

static void ch_df_shrine(void) {
    /* discovery falls */
    mainSetBits(BIT_DF_Demolition_Cave_Destroy_Whirlpool_Wall_2, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroyed_Whirlpool_Wall_2, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroy_Whirlpool_Wall_3, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroyed_Whirlpool_Wall_3, 1);
    mainSetBits(BIT_DF_Shrine_Door_Light_Activated_One, 1);
    mainSetBits(BIT_DF_Shrine_Door_Light_Activated_Two, 1);
    mainSetBits(BIT_DF_Shrine_Door_Light_Activated_Three, 1);
    mainSetBits(BIT_DF_Shrine_Door_Light_Activated_Four, 1);
    mainSetBits(BIT_DF_Played_Seq_0167_Arrival_So_This_Is_Discovery_Falls, 1);
    mainSetBits(BIT_DF_Played_Seq_0168_Krystal_Meets_HighTop_Archaeologist, 1);
    mainSetBits(BIT_DF_Cradle_Powered, 1);
    mainSetBits(BIT_DF_Toxic_Cave_Destroy_Gas_Vent, 1);
    mainSetBits(BIT_DF_Toxic_Cave_Destroyed_Gas_Vent, 1);
    mainSetBits(BIT_DF_Played_Seq_003E_Mole_Cave_SharpClaw_Hides, 1);
    mainSetBits(BIT_DF_Toxic_Cave_Destroy_Wall, 1);
    mainSetBits(BIT_DF_Toxic_Cave_Destroyed_Wall, 1);
    mainSetBits(BIT_DF_Played_Seq_002E_Demolition_Cave_SharpClaw_Antics, 1);
    mainSetBits(BIT_DF_Played_Seq_002C_Mole_Cave_Hatch_Opens, 1);
    mainSetBits(BIT_DF_Whirlpool_Cave_Wall_Demolished, 1);
    mainSetBits(BIT_DF_Whirlpool_Cave_Wall_Demolition_Finished, 1);
    mainSetBits(BIT_DF_Shrine_Door_Opened, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroy_Crate_Wall, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroyed_Crate_Wall, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroy_Whirlpool_Wall_1, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroyed_Whirlpool_Wall_1, 1);
    mainSetBits(BIT_Krystal_Foodbag_S, 1);
    mainSetBits(BIT_Krystal_Foodbag_M, 1);
    mainSetBits(BIT_Krystal_Foodbag_L, 1);
    mainSetBits(BIT_190, 1); // this was a type 0 bit before, does it do anything?
    mainSetBits(BIT_DF_Played_Seq_000F_Kyte_Secures_Rope_Upper_Falls, 1);
    mainSetBits(BIT_DF_Mole_Cave_Destroyed_Door, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroy_Whirlpool_Wall_4, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroy_Plant_Wall_1, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroyed_Plant_Wall_1, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroy_Plant_Wall_2, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroyed_Plant_Wall_2, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroyed_Whirlpool_Wall_4, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroy_Far_Wall, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroyed_Far_Wall, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroy_SharpClaw_Wall, 1);
    mainSetBits(BIT_DF_Demolition_Cave_Destroyed_SharpClaw_Wall, 1);
    mainSetBits(BIT_Kyte_Flight_Curve, 201);
    mainSetBits(BIT_Kyte_Flight_Talk_Sequence, 255);
    mainSetBits(BIT_DF_Seq_0035_Shrine_Door_Opened, 1);
    mainSetBits(BIT_DF_Played_Seq_02AD_Krystal_Meets_Foodbag_SharpClaw, 1);
    mainSetBits(BIT_CapyTunnel1, 1);
    mainSetBits(BIT_CapyTunnel2, 1);
    mainSetBits(BIT_CapyTunnel3, 1);
    mainSetBits(BIT_7DF, 1); // ?? what sets this
    //mainSetBits(BIT_879, 1); // SharpClaw 0x2043 defeated bit
    //mainSetBits(BIT_87A, 1); // SharpClaw 0x2042 defeated bit
    //mainSetBits(BIT_888, 1); // SharpClaw 0x2056 defeated bit
    //mainSetBits(BIT_889, 1); // SharpClaw 0x2055 defeated bit
    mainSetBits(BIT_8B1, 1); // Discovery Falls levelname shown
    // we don't want the shrine entrance sharpy to be there when exiting the shrine
    // (a type 0 bit controls their defeated state, so only a timesave can stop them from spawning)
    chselect_set_timesave(0x1F82, 72 * 60); // shrine entrance sharpy

    chselect_set_min_scarabs(PLAYER_KRYSTAL, 30);

    chselect_set_map_state(MAP_DISCOVERY_FALLS, 1, G3 | G7 | G11);

    /* shrine entrance */
    // chselect_set_location(PLAYER_KRYSTAL, -4048.695313f, 218.0f, 16215.4375f, -8192, 0);

    // PlayerLightActions light = DEFAULT_LACTIONS;
    // light.skylights[0] = 485;
    // light.skylights[1] = 35;
    // light.trklights[0] = 495;
//#if CHSELECT_NEWLFX
    // light.newlfx = 493;
//#endif
    // light.flags = PL_LIGHT_ACTION_FLAG_IS_INSIDE;
    // light.cam = 1;
    // chselect_set_light(PLAYER_KRYSTAL, &light);

    // PlayerEnvActions env = DEFAULT_ENVACTIONS;
    // env.unk4 = 394;
    // env.unk6 = 53;
    // env.unk8 = 54;
    // env.unkA = 451;
    // env.unk10 = -1073; env.unk14 = 394; env.unk18 = 17380;
    // chselect_set_env(PLAYER_KRYSTAL, &env);

    // chselect_set_music(PLAYER_KRYSTAL, 23, -1, 13, -1);

    /* warp */
    chselect_set_min_magic(PLAYER_KRYSTAL, 15);

    chselect_set_location(PLAYER_KRYSTAL, 15847.602f, 56.0f, -16792.164f, 0, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 19;
    light.skylights[1] = 18;
    light.trklights[0] = 495;
#if CHSELECT_NEWLFX
    light.newlfx = 493;
#endif
    light.flags = PL_LIGHT_ACTION_FLAG_IS_INSIDE;
    light.cam = 1;
    chselect_set_light(PLAYER_KRYSTAL, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 120;
    env.unk6 = 53;
    env.unk8 = 121;
    env.unkA = 451;
    env.unk10 = -1073; env.unk14 = 394; env.unk18 = 17380;
    chselect_set_env(PLAYER_KRYSTAL, &env);

    chselect_set_music(PLAYER_KRYSTAL, 41, -1, 13, -1);
}

static void ch_warlock_mountain_spirit_1(void) {
    /* discovery falls/the walk back to sc */
    mainSetBits(BIT_DF_Play_Seq_003C_Krystal_Returns_From_The_Shrine, 1);
    mainSetBits(BIT_DF_Played_Seq_003C_Krystal_Returns_From_The_Shrine, 1);
    mainSetBits(BIT_DF_Shrine_SharpClaw_Drop_Magic_Gems, 1);
    mainSetBits(BIT_Play_Seq_0107_Rocky_Intro_Unused, 1);
    mainSetBits(BIT_Kyte_Flight_Curve, 117);
    mainSetBits(BIT_Spirit_Bits, (1 << 0));
    mainSetBits(BIT_Talking_to_Rocky, 1);

    chselect_set_map_state(MAP_WARLOCK_MOUNTAIN, 2, G_None);
    chselect_set_map_state(MAP_SWAPSTONE_CIRCLE, 8, G0);
    //chselect_set_map_state(MAP_ICE_MOUNTAIN_1, 1, G_None); // this seems like a mistake/temp thing in the swapstone dll
    chselect_set_map_state(MAP_DISCOVERY_FALLS, 1, G_None);

    /* warlock mountain krystal transporter room */
    chselect_set_location(PLAYER_KRYSTAL, 13711.855469f, 210.710876f, 144.656250f, M_90_DEGREES, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 108;
    light.skylights[1] = 119;
    light.trklights[0] = 484;
#if CHSELECT_NEWLFX
    light.newlfx = 90;
#endif
    light.cam = 1;
    chselect_set_light(PLAYER_KRYSTAL, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 517;
    env.unk6 = 516;
    env.unk8 = 518;
    env.unkA = 451;
    env.unk10 = -1073; env.unk14 = 394; env.unk18 = 17380; // leftovers from a while ago now?
    chselect_set_env(PLAYER_KRYSTAL, &env);

    chselect_set_music(PLAYER_KRYSTAL, 266, -1, 187, -1);
}

// sabre DIM act start
static void ch_swapstone_hollow_2(void) {
    /* warlock mountain spirit 1 */
    mainSetBits(BIT_Play_Seq_0170_WM_Return_to_Randorn_Quan_Ata_Lachu_Speaks, 1);
    mainSetBits(BIT_Set_During_Spirit_Release_1, 1);
    mainSetBits(BIT_Play_Seq_0180_Release_Spirit_1, 1);
    mainSetBits(BIT_Spirit_Bits, 0);
    mainSetBits(BIT_4CF, 1); // WL_seqpoint 0x30D75 condition bit (set by seq 0x170)

    /* swapstone circle*/
    mainSetBits(BIT_Kyte_Flight_Curve, 55);

    {
        // krystal's last state
        chselect_set_location(PLAYER_KRYSTAL, -1048.82f, 307.0f, 17424.86f, 0, 0);

        PlayerLightActions light = DEFAULT_LACTIONS;
        light.skylights[0] = 485;
        light.skylights[1] = 486;
        light.trklights[0] = 484;
#if CHSELECT_NEWLFX
        light.newlfx = 126;
#endif
        light.cam = 1;
        chselect_set_light(PLAYER_KRYSTAL, &light);

        PlayerEnvActions env = DEFAULT_ENVACTIONS;
        env.unk4 = 434;
        env.unk6 = 435;
        env.unk8 = 436;
        env.unkA = 141;
        env.unk10 = 13685; env.unk14 = 202; env.unk18 = -169;
        chselect_set_env(PLAYER_KRYSTAL, &env);

        chselect_set_music(PLAYER_KRYSTAL, 185, -1, 5, -1);
    }

    /* swap */
    chselect_set_player(PLAYER_SABRE);

    /* swapstone hollow */
    mainSetBits(BIT_Play_Seq_01FD_Rocky_Teaches_Distract, 1);
    mainSetBits(BIT_Tricky_Learned_Distract, 1);
    mainSetBits(BIT_Play_Seq_035F_Rocky_Intro, 0); // unset by the swapstone during the swap?
    mainSetBits(BIT_2B2, 1); // SC_sequences 0x2733 played bit

    chselect_set_map_state(MAP_SWAPSTONE_HOLLOW, 1, G0);

    {
        chselect_set_location(PLAYER_SABRE, -6571.54f, -632.0f, -1607.57f, 0, 0);

        PlayerLightActions light = DEFAULT_LACTIONS;
        light.skylights[0] = 389;
        light.skylights[1] = 401;
        light.skylights[2] = 402;
        light.skylights[3] = 403;
#if CHSELECT_NEWLFX
        light.newlfx = 1;
#endif
        light.cam = 1;
        chselect_set_light(PLAYER_SABRE, &light);

        PlayerEnvActions env = DEFAULT_ENVACTIONS;
        env.unk4 = 97;
        env.unk6 = 438;
        env.unk8 = 439;
        env.unkA = 100;
        chselect_set_env(PLAYER_SABRE, &env);

        chselect_set_music(PLAYER_SABRE, 218, -1, 1, -1);
    }
}

static void ch_darkice_1(void) {
    /* swapstone hollow */
    mainSetBits(BIT_Talking_to_Rocky, 0);

    /* snowhorn wastes */
    mainSetBits(BIT_Garunda_Te_Fed, 1);
    mainSetBits(BIT_104, 1); // NWminedoor
    mainSetBits(BIT_Garunda_Te_Quest_Progress, 5);
    mainSetBits(BIT_11A, 1); // NWMultiSeq 0x32D52 played
    mainSetBits(BIT_120, 1); // SnowClaw 0x41D81 defeated bit
    mainSetBits(BIT_121, 1); // SnowClaw 0x30A70 defeated bit
    mainSetBits(BIT_Garunda_Te_Weeds_Eaten, 10);
    mainSetBits(BIT_Tricky_Talk_Sequence, 142);
    mainSetBits(BIT_763, 142); // HitAnimator 0x15F4

    chselect_set_map_state(MAP_SWAPSTONE_HOLLOW, 1, G_None);
    chselect_set_map_state(MAP_DARK_ICE_MINES_1, 1, G0);
    chselect_set_map_state(MAP_SNOWHORN_WASTES, 1, G12);

    /* mine entrance */
    chselect_set_location(PLAYER_SABRE, -6808.006348f, -1202.555176f, 1838.698242f, M_180_DEGREES, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 389;
    light.skylights[1] = 390;
    light.skylights[2] = 394;
    light.skylights[3] = 403;
    light.trklights[0] = 392;
#if CHSELECT_NEWLFX
    light.newlfx = 204;
#endif
    light.cam = 1;
    chselect_set_light(PLAYER_SABRE, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 180;
    env.unk6 = 182;
    env.unk8 = 181;
    env.unkA = 183;
    env.unk3D = 0;
    chselect_set_env(PLAYER_SABRE, &env);

    chselect_set_music(PLAYER_SABRE, 281, -1, 77, -1);
}

static void ch_darkice_2(void) {
    /* dim1 */
    chselect_move_setup(0x16FC, MAP_DARK_ICE_MINES_1, -8113.85f, -1492.0f, 4288.9f); // DIMPushBlock
    chselect_move_setup(0x30104, MAP_DARK_ICE_MINES_1, -7470.120605f, -1079.900024f, 6638.58252f); // DIMBoulder

    mainSetBits(BIT_DIM_Famished_SnowHorn_Met, 1);
    mainSetBits(BIT_DIM_Famished_SnowHorn_Fed_One_Alpine_Root, 1);
    mainSetBits(BIT_DIM_Used_Mine_Key, 1);
    mainSetBits(BIT_DIM_Mine_Key, 1);
    mainSetBits(BIT_33, 1); // AlphaAnimator 0x1659
    mainSetBits(BIT_35, 1); // AlphaAnimator 0x1699
    mainSetBits(BIT_3A, 1); // DIMLavaBallGene 0x1570
    mainSetBits(BIT_3B, 1); // DIMLavaBallGene 0x186E
    mainSetBits(BIT_DIM_Shackled_SnowHorn_Freed, 1);
    mainSetBits(BIT_147, 1); // DIMLavaBallGene 0x1878
    mainSetBits(BIT_155, 1); // DIMBridgeCogCol 0x1BE0 discovered
    mainSetBits(BIT_156, 1); // DIMSeqObject 0x1BB0 played
    mainSetBits(BIT_15B, 1); // DIMShackleKey 0x1642 discovered
    mainSetBits(BIT_16D, 1); // DIMAlpineRoot 0x16A4 collected
    mainSetBits(BIT_16E, 1); // DIMAlpineRoot 0x16AA collected
    mainSetBits(BIT_DIM_Famished_SnowHorn_Fed_Two_Alpine_Roots, 1);
    mainSetBits(BIT_DIM_Defeated_SharpClaw_Hurting_Fallen_SnowHorn, 1);
    mainSetBits(BIT_17A, 1); // DIMLever 0x1B15 used
    mainSetBits(BIT_DIM_Gear_1, 1);
    mainSetBits(BIT_17C, 1); // DIMBridge 0x1B4D play bit
    mainSetBits(BIT_17D, 1); // DIMBridge 0x1B4D played bit
    mainSetBits(BIT_DIM_Gear_2, 1);
    mainSetBits(BIT_DIM_Gear_3, 1);
    mainSetBits(BIT_DIM_Gear_4, 1);
    mainSetBits(BIT_DIM_Used_Gear_1, 1);
    mainSetBits(BIT_DIM_Used_Gear_2, 1);
    mainSetBits(BIT_DIM_Used_Gear_3, 1);
    mainSetBits(BIT_DIM_Used_Gear_4, 1);
    mainSetBits(BIT_1DB, 1); // DIMLever 0x1CCB used
    mainSetBits(BIT_1DC, 1); // DIMSeqObject 0x1BB0 play bit
    mainSetBits(BIT_1E1, 1); // DIMLever 0x1CCB enabled
    mainSetBits(BIT_1E2, 1); // DIMWoodDoor 0x1CE1 destroyed
    mainSetBits(BIT_1E3, 1); // DIMLever 0x1DDD used
    mainSetBits(BIT_1E4, 1); // DIMBridge 0x1DD9 play
    mainSetBits(BIT_1E5, 1); // DIMBridge 0x1DD9 played
    mainSetBits(BIT_1E8, 1); // DIMSeqObject 0x1EA1 play
    mainSetBits(BIT_DIM_Magic_Bridge_Visible, 1);
    mainSetBits(BIT_Horn_of_Truth, 1);
    mainSetBits(BIT_1EF, 1); // ?
    mainSetBits(BIT_1FF, 1); // DIMTruthHornPli 0x20C7 play bit
    mainSetBits(BIT_Tricky_Learned_Flame, 1);
    mainSetBits(BIT_3CF, 1); // cannon claw defeated
    mainSetBits(BIT_3E5, 1); // DIMPushBlock 0x16FC
    mainSetBits(BIT_Tricky_Dug_Up_Horn_of_Truth_Pad, 1);
    //mainSetBits(BIT_DIM_Gear_3_Random_Tent, 11);
    mainSetBits(BIT_4A8, 1); // DIMBridgeCogCol 0x30CC8 discovered
    mainSetBits(BIT_Burned_DIMTent_5, 1);
    mainSetBits(BIT_Burned_DIMTent_4, 1);
    mainSetBits(BIT_Burned_DIMTent_3, 1);
    mainSetBits(BIT_Burned_DIMTent_2, 1);
    mainSetBits(BIT_Burned_DIMTent_1, 1);
    mainSetBits(BIT_Burned_DIMTent_0, 1);
    mainSetBits(BIT_Burned_DIMTent_11, 1);
    mainSetBits(BIT_Burned_DIMTent_10, 1);
    mainSetBits(BIT_Burned_DIMTent_9, 1);
    mainSetBits(BIT_Burned_DIMTent_8, 1);
    mainSetBits(BIT_Burned_DIMTent_7, 1);
    mainSetBits(BIT_Burned_DIMTent_6, 1);
    mainSetBits(BIT_4CE, 3); // DIMboulder times hit
    mainSetBits(BIT_Tricky_Talk_Sequence, 44);
    mainSetBits(BIT_Tricky_Spawns, 0); // bye bye
    mainSetBits(BIT_540, 1); // ?
    mainSetBits(BIT_DIM_Lit_Igloo_Fire_One, 1);
    mainSetBits(BIT_DIM_Lit_Igloo_Fire_Two, 1);
    mainSetBits(BIT_DIM_Lit_Igloo_Fire_Three, 1);
    mainSetBits(BIT_DIM_Lit_All_Igloo_Fires, 1);
    mainSetBits(BIT_8A5, 1); // DIMSeqObject 0x426E0 played bit

    chselect_set_map_state(MAP_DARK_ICE_MINES_1, 1, G6 | G8 | G14);

    /* before bike */
    chselect_set_location(PLAYER_SABRE, -9106.515f, -909.450f, 6638.054f, M_180_DEGREES, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 160;
    light.skylights[1] = 430;
    light.skylights[2] = 433;
    light.skylights[3] = 403;
    light.trklights[0] = 392;
    light.trklights[1] = 443;
#if CHSELECT_NEWLFX
    light.newlfx = 2;
#endif
    light.cam = 1;
    chselect_set_light(PLAYER_SABRE, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 224;
    env.unk6 = 292;
    env.unk8 = 9;
    env.unkA = 291;
    env.unkC = 300;
    env.unk10 = -9322; env.unk14 = -763; env.unk18 = 7062;
    env.unk1C = -7345; env.unk20 = -994; env.unk24 = 7317;
    chselect_set_env(PLAYER_SABRE, &env);

    chselect_set_music(PLAYER_SABRE, 281, -1, 96, -1);
}

static void ch_galadon(void) {
    /* dim 2*/
    chselect_move_setup(0x2724, MAP_DARK_ICE_MINES_2, -7440.065f, -2647.811f, 6830.156f); // DIMPushBlock (to boss room)

    mainSetBits(BIT_91, 1); // sideload 0x331F1 unlocked (tricky's cell?)
    mainSetBits(BIT_Belina_Te_Cell_Key, 1);
    mainSetBits(BIT_Tricky_Cell_Key, 1);
    mainSetBits(BIT_1F5, 1); // DIM2SilverKey 0x216D discovered
    mainSetBits(BIT_1FA, 1); // DIM2SeqObject 0x1E98 play
    mainSetBits(BIT_1FB, 1); // DIM2SeqObject 0x1E98 played
    mainSetBits(BIT_207, 1); // DIM2CellLever 0x2182 used
    mainSetBits(BIT_208, 1); // DIM2CellLever 0x2181 used
    mainSetBits(BIT_215, 1); // DIM2CellDoorLef 0x219C restore
    mainSetBits(BIT_217, 1); // DIMIceWall 0x21FD
    mainSetBits(BIT_218, 1); // HitAnimator 0x2DDE
    mainSetBits(BIT_219, 1); // DIM2CellDoorLef 0x219C open
    mainSetBits(BIT_21A, 1); // DIM2CellDoorLef 0x219A open
    mainSetBits(BIT_223, 1); // DIM2SeqObject 0x2061 play
    mainSetBits(BIT_224, 1); // DIM2SeqObject 0x2061 played
    mainSetBits(BIT_22F, 1); // ExplodeAnimator 0x225B trigger
    mainSetBits(BIT_230, 1); // ExplodeAnimator 0x225B exploded
    mainSetBits(BIT_240, 1); // DIM2PuzzleKey 0x2233 discovered
    mainSetBits(BIT_DIM_Door_Key_1, 1);
    mainSetBits(BIT_24B, 1); // DIM2UseObject 0x33EEB used
    mainSetBits(BIT_254, 1); // DIM2Icicle 0x241F fell
    mainSetBits(BIT_255, 1); // DIM2Icicle 0x241E fell
    mainSetBits(BIT_26C, 1); // DIM2Icicle 0x241A fell
    mainSetBits(BIT_26D, 1); // DIM2Icicle 0x2419 fell
    mainSetBits(BIT_26E, 1);
    mainSetBits(BIT_26F, 1);
    mainSetBits(BIT_270, 1);
    mainSetBits(BIT_271, 1);
    mainSetBits(BIT_DIM_Pushed_Ice_Block_Into_Lake, 1);
    mainSetBits(BIT_278, 1); // ! used by DIMlever and SC_meterblock
    mainSetBits(BIT_279, 1); // DIM2IceGrateDoo 0x2451 open
    mainSetBits(BIT_27A, 1); // DIM2IceGrateDoo 0x2451 restore
    mainSetBits(BIT_27B, 1);
    mainSetBits(BIT_27C, 1);
    mainSetBits(BIT_27D, 1);
    mainSetBits(BIT_27E, 1);
    mainSetBits(BIT_27F, 1); // ProjectileSwitc 0x245E
    mainSetBits(BIT_280, 1); // ProjectileSwitc 0x245F
    mainSetBits(BIT_281, 1); // ProjectileSwitc 0x2460
    mainSetBits(BIT_DIM_Door_Key_2, 1);
    mainSetBits(BIT_285, 1); // DIM2PuzzleDoor 0x33EEE open
    mainSetBits(BIT_286, 1); // DIM2PuzzleKey 0x2472 discovered
    mainSetBits(BIT_28C, 1);
    mainSetBits(BIT_2C2, 1); // DIM2SeqObject 0x33EEA play
    mainSetBits(BIT_2C3, 1); // DIM2SeqObject 0x33EEA played
    mainSetBits(BIT_35F, 1); // IMSnowBike 0x3037B
    mainSetBits(BIT_40A, 1); // DIMBikeDoor 0x303B9
    mainSetBits(BIT_Tricky_Talk_Sequence, 71);
    mainSetBits(BIT_Tricky_Spawns, 1);
    mainSetBits(BIT_354, 1); // WallAnimator 0x2723 dug

    chselect_set_map_state(MAP_DARK_ICE_MINES_1, 1, G_None);
    chselect_set_map_state(MAP_DARK_ICE_MINES_2, 0, G6); // why act 0?
    chselect_set_map_state(MAP_BOSS_GALADON, 1, G0 | G1 | G6);

    chselect_set_min_scarabs(PLAYER_SABRE, 50);

    /* boss room */
    chselect_set_location(PLAYER_SABRE, -7474.890, -2974.000, 7299.699, M_180_DEGREES, -2);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 220;
    light.skylights[1] = 241;
    light.skylights[2] = 322;
    light.skylights[3] = 274;
    light.trklights[0] = 333;
    light.trklights[1] = 443;
    light.flags = PL_LIGHT_ACTION_FLAG_IS_INSIDE;
    light.cam = 74;
    chselect_set_light(PLAYER_SABRE, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 220;
    env.unk6 = 292;
    env.unk8 = 219;
    env.unkA = 291;
    env.unkC = 300;
    env.unk10 = -9322; env.unk14 = -763; env.unk18 = 7062;
    env.unk1C = -7345; env.unk20 = -994; env.unk24 = 7317;
    chselect_set_env(PLAYER_SABRE, &env);

    // Note: Usually you get here with ambience 93 (lava sounds) but without it actually playing?
    //       Using silent ambience from earlier in DIM2 instead.
    chselect_set_music(PLAYER_SABRE, /*93*/ 281, -1, 135, -1);
}

static void ch_swapstone_hollow_river(void) {
    /* boss galadon */
    mainSetBits(BIT_SpellStone_DIM, 1);
    mainSetBits(BIT_210, 1);

    /* dim1/snowhorn wastes (trek back) */
    mainSetBits(BIT_Garunda_Te_Quest_Progress, 7);
    mainSetBits(BIT_186, 273); // ? map objbits?
    mainSetBits(BIT_SpellStone_DIM_Activated, 1);
    mainSetBits(BIT_22E, 1);
    mainSetBits(BIT_Tricky_Talk_Sequence, 145);

    /* snowhown wastes -> swapstone hollow tunnel */
    chselect_set_map_state(MAP_SNOWHORN_WASTES, 2, G19);
    chselect_set_map_state(MAP_DARK_ICE_MINES_2, 0, G_None); // why act 0?
    chselect_set_map_state(MAP_BOSS_GALADON, 1, G_None);

    chselect_set_location(PLAYER_SABRE, -6984.36f, -800.41f, -569.07f, M_90_DEGREES, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 389;
    light.skylights[1] = 390;
    light.skylights[2] = 394;
    light.skylights[3] = 274;
    light.trklights[0] = 392;
    light.trklights[1] = 443;
#if CHSELECT_NEWLFX
    light.newlfx = 203;
#endif
    light.cam = 61;
    chselect_set_light(PLAYER_SABRE, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 180;
    env.unk6 = 182;
    env.unk8 = 181;
    env.unkA = 100;
    env.unkC = 300;
    env.unk10 = -7184; env.unk14 = -1180; env.unk18 = 2851;
    env.unk1C = -7345; env.unk20 = -994; env.unk24 = 7317;
    chselect_set_env(PLAYER_SABRE, &env);

    chselect_set_music(PLAYER_SABRE, 281, -1, 65, -1);
}

static void ch_diamond_bay_1(void) {
    /* swapstone hollow */
    mainSetBits(BIT_14, 1); // SHseqobject 0x1CEB played bit
    mainSetBits(BIT_Tricky_Talk_Sequence, 56);
    mainSetBits(DINOMOD_BIT_920_SH_BoulderBlownUp, 1);
    mainSetBits(DINOMOD_BIT_921_SH_RiverUnblocked, 1);
    mainSetBits(DINOMOD_BIT_92C_SH_River_Seq_Has_Played, 1);

    /* before log to diamond bay */
    chselect_set_map_state(MAP_SWAPSTONE_HOLLOW, 1, G7 | G8 | G11 | G12);

    chselect_set_location(PLAYER_SABRE, -6746.6f, -804.093f, -2728.939f, 0x2F00, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 389;
    light.skylights[1] = 401;
    light.skylights[2] = 402;
    light.skylights[3] = 403;
    light.trklights[0] = 392;
    light.trklights[1] = 443;
#if CHSELECT_NEWLFX
    light.newlfx = 2;
#endif
    light.cam = 1;
    chselect_set_light(PLAYER_SABRE, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 97;
    env.unk6 = 95;
    env.unk8 = 96;
    env.unkA = 100;
    env.unkC = 300;
    env.unk10 = -7184; env.unk14 = -1180; env.unk18 = 2851;
    env.unk1C = -7345; env.unk20 = -994; env.unk24 = 7317;
    chselect_set_env(PLAYER_SABRE, &env);

    chselect_set_music(PLAYER_SABRE, 281, -1, 1, -1);
}

static void ch_volcano_force_point_1(void) {
    /* diamond bay */
    mainSetBits(BIT_186, 0); // ?? map objbits?
    mainSetBits(BIT_39B, 1); // ClubSharpClaw 0x300BF defeated
    mainSetBits(BIT_DB_Gave_Pointback_Egg_One, 1);
    mainSetBits(BIT_3A4, 1);
    mainSetBits(BIT_3A5, 1);
    mainSetBits(BIT_3A6, 1);
    mainSetBits(BIT_3A7, 1);
    mainSetBits(BIT_3C0, 1);
    mainSetBits(BIT_DB_Picked_Up_Egg, 1);
    mainSetBits(BIT_406, 1);
    mainSetBits(BIT_407, 1);
    mainSetBits(BIT_408, 1);
    mainSetBits(BIT_DB_Tricky_Drained_Pool_One, 1);
    mainSetBits(BIT_DB_Tricky_Drained_Pool_Two, 1);
    mainSetBits(BIT_DB_Tricky_Drained_Pool_Three, 1);
    mainSetBits(BIT_411, 1);
    mainSetBits(BIT_DB_Egg_Flood_Level_One, 1);
    mainSetBits(BIT_DB_Egg_Flood_Level_Two, 1);
    mainSetBits(BIT_DB_Egg_Flood_Level_Three, 1);
    mainSetBits(BIT_44B, 1);
    mainSetBits(BIT_44C, 1);
    mainSetBits(BIT_44D, 1);
    mainSetBits(BIT_4B4, 1);
    mainSetBits(BIT_Tricky_Talk_Sequence, 210);
    mainSetBits(BIT_4E6, 1);
    mainSetBits(BIT_83F, 1);
    mainSetBits(BIT_DB_Bay_Diamond, 1);
    mainSetBits(BIT_8A6, 1);
    mainSetBits(BIT_8AF, 1);
    mainSetBits(BIT_8EB, 1);

    /* volcano force point entrance (bridge) */
    chselect_set_map_state(MAP_DIAMOND_BAY, 1, G6 | G11);
    chselect_set_map_state(MAP_VOLCANO_FORCE_POINT_TEMPLE, 11, G_None); // still act 11 from the queen's cutscene?
    chselect_set_map_state(MAP_SWAPSTONE_HOLLOW, 1, G6 | G8 | G11 | G12);

    chselect_set_location(PLAYER_SABRE, -12298.08f, -1447.0f, -2237.22f, M_90_DEGREES, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 389;
    light.skylights[1] = 618;
    light.skylights[2] = 619;
    light.skylights[3] = 403;
    light.trklights[0] = 616;
    light.trklights[1] = 443;
    light.cam = 1;
    chselect_set_light(PLAYER_SABRE, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 487;
    env.unk6 = 488;
    env.unk8 = 489;
    env.unkA = 100;
    env.unkC = 300;
    env.unk10 = -7184; env.unk14 = -1180; env.unk18 = 2851;
    env.unk1C = -7345; env.unk20 = -994; env.unk24 = 7317;
    chselect_set_env(PLAYER_SABRE, &env);

    chselect_set_music(PLAYER_SABRE, 208, -1, 195, -1);
}

static void ch_db_shrine(void) {
    /* volcano force point/killbot area */
    mainSetBits(BIT_SpellStone_DIM, 1);
    mainSetBits(BIT_Tricky_Talk_Sequence, 122);
    mainSetBits(BIT_4E7, 1);
    mainSetBits(BIT_DB_Unlock_Act_Two, 1);
    mainSetBits(BIT_4EC, 1);
    mainSetBits(BIT_4ED, 1);
    mainSetBits(BIT_4EE, 1);
    mainSetBits(BIT_4FD, 1);
    mainSetBits(BIT_503, 1);
    mainSetBits(BIT_504, 1);
    mainSetBits(BIT_506, 1);
    mainSetBits(BIT_507, 1);
    mainSetBits(BIT_508, 1);
    mainSetBits(BIT_509, 1);
    mainSetBits(BIT_50A, 1);
    mainSetBits(BIT_512, 1);
    mainSetBits(BIT_513, 1);
    mainSetBits(BIT_VFP_Door_Seal_Break, 1);
    mainSetBits(BIT_51D, 1);
    mainSetBits(BIT_521, 1);
    mainSetBits(BIT_VFP_Flamethrower_Turned_Off_2, 1);
    mainSetBits(BIT_523, 1);
    mainSetBits(BIT_541, 1);
    mainSetBits(BIT_84F, 1);
    mainSetBits(BIT_86B, 1);
    mainSetBits(BIT_870, 1);
    mainSetBits(BIT_871, 1);
    mainSetBits(BIT_877, 1);
    mainSetBits(BIT_87D, 1);
    mainSetBits(BIT_87E, 1);
    mainSetBits(BIT_87F, 1);
    mainSetBits(BIT_8B6, 1);
    mainSetBits(BIT_8B7, 1);
    mainSetBits(BIT_8F0, 1);
    mainSetBits(BIT_VFP_Flamethrower_Turned_Off, 1);

    chselect_set_map_state(MAP_DIAMOND_BAY, 2, G3 | G11);
    chselect_set_map_state(MAP_VOLCANO_FORCE_POINT_TEMPLE, 1, G_None);

    /* warp */
    chselect_set_map_state(MAP_SHRINE_DIAMOND_BAY, 0, G1);

    chselect_set_location(PLAYER_SABRE, 12007.602f, 56.0f, -16792.164f, 0x1800, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 232;
    light.skylights[1] = 233;
    light.skylights[2] = 359;
    light.skylights[3] = 374;
    light.trklights[0] = 616;
    light.trklights[1] = 443;
#if CHSELECT_NEWLFX
    light.newlfx = 413;
#endif
    light.flags = PL_LIGHT_ACTION_FLAG_IS_INSIDE;
    light.cam = 1;
    chselect_set_light(PLAYER_SABRE, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 154;
    env.unk6 = 325;
    env.unk8 = 212;
    env.unkA = 100;
    env.unkC = 300;
    env.unk10 = -7184; env.unk14 = -1180; env.unk18 = 2851;
    env.unk1C = -7345; env.unk20 = -994; env.unk24 = 7317;
    chselect_set_env(PLAYER_SABRE, &env);

    chselect_set_music(PLAYER_SABRE, 41, -1, 163, -1);
}

// DB spirit
static void ch_warlock_mountain_spirit_2(void) {
    /* DB shrine */
    mainSetBits(BIT_21F_Spirit_Collected, 1);
    mainSetBits(BIT_Spirit_Bits, (2 << 0));

    chselect_set_map_state(MAP_WARLOCK_MOUNTAIN, 3, G_None);
    chselect_set_map_state(MAP_SHRINE_DIAMOND_BAY, 0, G_None);

    /* diamond bay -> swapstone hollow */
    mainSetBits(BIT_21F_Spirit_Collected, 1);
    mainSetBits(BIT_Tricky_Talk_Sequence, 201);
    mainSetBits(BIT_86F, 1);
    mainSetBits(BIT_872, 1);
    mainSetBits(BIT_873, 1);
    mainSetBits(BIT_Play_Seq_035F_Rocky_Intro, 1);
    mainSetBits(BIT_Talking_to_Rocky, 1);

    chselect_set_map_state(MAP_DIAMOND_BAY, 2, G_None);
    chselect_set_map_state(MAP_SWAPSTONE_HOLLOW, 1, G0);

    /* warp */
    chselect_set_map_state(MAP_SNOWHORN_WASTES, 1, G19); // reset act from swapstone?
    chselect_set_map_state(MAP_ICE_MOUNTAIN_1, 1, G_None); // reset act from swapstone?

    chselect_set_location(PLAYER_SABRE, 12384.348f, -289.000f, 2953.051f, -M_90_DEGREES, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 232;
    light.skylights[1] = 632;
    light.skylights[2] = 402;
    light.skylights[3] = 374;
    light.trklights[0] = 629;
    light.trklights[1] = 443;
#if CHSELECT_NEWLFX
    light.newlfx = 1;
#endif
    light.cam = 1;
    chselect_set_light(PLAYER_SABRE, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 229;
    env.unk6 = 228;
    env.unk8 = 96;
    env.unkA = 100;
    env.unkC = 300;
    env.unk10 = -7184; env.unk14 = -1180; env.unk18 = 2851;
    env.unk1C = -7345; env.unk20 = -994; env.unk24 = 7317;
    chselect_set_env(PLAYER_SABRE, &env);

    chselect_set_music(PLAYER_SABRE, 41, -1, 271, -1);
}

// CRF start
static void ch_swapstone_circle_3(void) {
    /* warlock mountain (sabre, spirit 2) */
    mainSetBits(BIT_29A, 1);
    mainSetBits(BIT_29B, 1);
    mainSetBits(BIT_2A7, 1);
    mainSetBits(BIT_2A8, 1);
    mainSetBits(BIT_2A9, 1);
    mainSetBits(BIT_2B2, 0);
    mainSetBits(BIT_Spirit_2_Release_Sabre, 1);
    mainSetBits(BIT_Play_Seq_0107_Rocky_Intro_Unused, 0);
    mainSetBits(BIT_Spirit_Bits, 0);
    mainSetBits(BIT_Tricky_Talk_Sequence, 35);
    mainSetBits(BIT_WM_Sabre_Transporter_Visible, 1);
    mainSetBits(BIT_Sabre_Warp_Crystal, 1);
    mainSetBits(BIT_8A1, 1);
    mainSetBits(BIT_8A3, 1);

    chselect_set_map_state(MAP_SWAPSTONE_HOLLOW, 1, G_None);

    /* final sabre state */
    {
        chselect_set_location(PLAYER_SABRE, -6571.54, -632.0f, -1607.57f, 0, 0);

        PlayerLightActions light = DEFAULT_LACTIONS;
        light.skylights[0] = 232;
        light.skylights[1] = 401;
        light.skylights[2] = 402;
        light.skylights[3] = 374;
        light.trklights[0] = 629;
        light.trklights[1] = 443;
        light.cam = 1;
        chselect_set_light(PLAYER_SABRE, &light);

        PlayerEnvActions env = DEFAULT_ENVACTIONS;
        env.unk4 = 97;
        env.unk6 = 95;
        env.unk8 = 96;
        env.unkA = 424;
        env.unkC = 300;
        env.unk10 = -6571; env.unk14 = -631; env.unk18 = -1526;
        env.unk1C = -7345; env.unk20 = -994; env.unk24 = 7317;
        chselect_set_env(PLAYER_SABRE, &env);

        chselect_set_music(PLAYER_SABRE, 41, -1, 5, -1);
    }

    /* swap */
    chselect_set_player(PLAYER_KRYSTAL);
    chselect_set_map_state(MAP_SWAPSTONE_CIRCLE, 9, G0);

    {
        chselect_set_location(PLAYER_KRYSTAL, -1048.82f, 307.0f, 17424.86f, 0, 0);

        PlayerLightActions light = DEFAULT_LACTIONS;
        light.skylights[0] = 485;
        light.skylights[1] = 486;
        light.trklights[0] = 484;
#if CHSELECT_NEWLFX
        light.newlfx = 126;
#endif
        light.cam = 1;
        chselect_set_light(PLAYER_KRYSTAL, &light);

        PlayerEnvActions env = DEFAULT_ENVACTIONS;
        env.unk4 = 434;
        env.unk6 = 435;
        env.unk8 = 436;
        env.unkA = 141;
        env.unk10 = 13685; env.unk14 = 202; env.unk18 = -169;
        chselect_set_env(PLAYER_KRYSTAL, &env);

        chselect_set_music(PLAYER_KRYSTAL, 185, -1, 18, -1);
    }
}

static void ch_crf_start(void) {
    /* swapstone circle/mmp */
    mainSetBits(BIT_186, 0); // ? mapobj bits?
    mainSetBits(BIT_38A, 1);
    mainSetBits(BIT_38B, 1);
    mainSetBits(BIT_41C, 1);
    mainSetBits(BIT_453, 1);
    mainSetBits(BIT_Kyte_Flight_Curve, 160);
    mainSetBits(BIT_60F, 1);
    mainSetBits(BIT_CRF_Prison_Key_2, 1);
    mainSetBits(BIT_Talking_to_Rocky, 0);
    mainSetBits(BIT_8AA, 1);
    mainSetBits(BIT_8AB, 1);
    mainSetBits(BIT_8AC, 1);
    mainSetBits(BIT_8C2, 1);
    mainSetBits(DINOMOD_BIT_972_DF_Open_Door_to_MMP, 1);
    mainSetBits(DINOMOD_BIT_975_DF_Kyte_Can_Fly_Through_Door_to_MMP, 1);

    chselect_set_map_state(MAP_MOON_MOUNTAIN_PASS, 1, G1);
    chselect_set_map_state(MAP_DISCOVERY_FALLS, 1, G19);
    chselect_set_map_state(MAP_SWAPSTONE_CIRCLE, 9, G_None);

    /* cloudrunner fortress entrance */
    chselect_set_location(PLAYER_KRYSTAL, -8388.96f, 1109.62f, 18926.75f, 0x3C00, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 485;
    light.skylights[1] = 476;
    light.trklights[0] = 494;
    light.cam = 1;
    chselect_set_light(PLAYER_KRYSTAL, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 312;
    env.unk6 = 314;
    env.unk8 = 313;
    env.unkA = 141;
    env.unk10 = 13685; env.unk14 = 202; env.unk18 = -169;
    chselect_set_env(PLAYER_KRYSTAL, &env);

    chselect_set_music(PLAYER_KRYSTAL, 15, -1, 206, -1);
}

static void ch_crf_dungeon_start(void) {
    /* cloudrunner fortress */
    mainSetBits(BIT_Played_Seq_0041_Scales_Kills_The_Queen, 1);
    mainSetBits(BIT_Play_Seq_017C_CF_This_Is_CloudRunner_Fortress, 1);
    mainSetBits(BIT_Played_Seq_017C_CF_This_Is_CloudRunner_Fortress, 1);
    mainSetBits(BIT_213, 1); // crf entrance stairwell seq play
    mainSetBits(BIT_214, 1); // crf entrance stairwell seq played
    mainSetBits(BIT_356, 1);
    mainSetBits(BIT_Kyte_Trapped, 1);
    mainSetBits(BIT_Kyte_Flight_Curve, 17);
    mainSetBits(BIT_CF_Entrance_Trigger_Passed, 1); // load entrance stairwell
    mainSetBits(BIT_580, 1); // TriggerPoint 0x32253 restore bit
    mainSetBits(BIT_584, 1); // ClubSharpClaw 0x322B0 defeated

    chselect_set_map_state(MAP_CLOUDRUNNER_FORTRESS, 1, G7);
    chselect_set_map_state(MAP_CLOUDRUNNER_DUNGEON, 1, G3);

    /* dungeon player cell */
    chselect_set_location(PLAYER_KRYSTAL, -12055.83f, 1299.43f, 20132.37f, 0, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 506;
    light.skylights[1] = 22;
    light.trklights[0] = 494;
#if CHSELECT_NEWLFX
    light.newlfx = 49;
#endif
    // bug: should be inside light
    light.cam = 37;
    chselect_set_light(PLAYER_KRYSTAL, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 61;
    env.unk6 = 62;
    env.unk8 = 63;
    env.unkA = 70;
    env.unkC = 67;
    env.unkE = 92;
    env.unk10 = -10703; env.unk14 = 1716; env.unk18 = 18241;
    env.unk1C = -10884; env.unk20 = 1876; env.unk24 = 18921;
    env.unk28 = -10448; env.unk2C = 1501; env.unk30 = 18255;
    chselect_set_env(PLAYER_KRYSTAL, &env);

    chselect_set_music(PLAYER_KRYSTAL, 295, -1, 3, -1);
}

static void ch_crf_race(void) {
    /* cloudrunner dungeon */
    chselect_move_setup(0xA69, MAP_CLOUDRUNNER_DUNGEON, -11908.346680f, 1309.0f, 20109.648438f); // CFDungeonBlock

    mainSetBits(BIT_Spell_Illusion, 1);
    mainSetBits(BIT_CRF_Prison_Key_1, 1);
    mainSetBits(BIT_CRF_Prison_Guardian_Cell_Door_Unlocked, 1);
    mainSetBits(BIT_CRF_BoneHead_Guardian_Freed, 1);
    //mainSetBits(BIT_CFGuardian_State, 4);
    mainSetBits(BIT_CRF_Prison_GreenScarab_Cage_Unlocked, 1);
    mainSetBits(BIT_CRF_CloudRunner_Uncle_Freed, 1);
    mainSetBits(BIT_CRF_Prison_RedScarab_Cage_Unlocked, 1);
    mainSetBits(BIT_CRF_Prison_Duster_Cage_Unlocked, 1);
    mainSetBits(BIT_CRF_CloudRunner_Uncle_Escaped, 1);
    //mainSetBits(BIT_CRF_Power_Room_Key, 1);
    mainSetBits(BIT_CRF_Prison_Guardian_Cell_Door_Open, 1);
    mainSetBits(BIT_360, 1);
    mainSetBits(BIT_583, 1);

    /* restored power + rescued kyte */
    mainSetBits(BIT_CF_Floor_Destroyed, 1);
    //mainSetBits(BIT_CFGuardian_State, 8);
    mainSetBits(BIT_CRF_Red_PowerBase, 1);
    mainSetBits(BIT_CRF_Green_PowerBase, 1);
    mainSetBits(BIT_CRF_Blue_PowerBase, 1);
    mainSetBits(BIT_CRF_WindLifts_Powered, 1);
    mainSetBits(BIT_CRF_Power_Room_Unlocked, 1);
    //mainSetBits(BIT_CRF_Power_Room_Key, 0);
    mainSetBits(BIT_186, 0); // ??????
    mainSetBits(BIT_209, 1);
    mainSetBits(BIT_CRF_BlastedRoof_Blasted, 1);
    mainSetBits(BIT_302, 1);
    mainSetBits(BIT_306, 1);
    mainSetBits(BIT_307, 1);
    mainSetBits(BIT_34B, 1);
    mainSetBits(BIT_34C, 1);
    mainSetBits(BIT_Play_Seq_01D7_CF_Kyte_Freed, 1);
    mainSetBits(BIT_Played_Seq_01D7_CF_Kyte_Freed, 1);
    mainSetBits(BIT_35A, 1);
    mainSetBits(BIT_35C, 1);
    mainSetBits(BIT_Kyte_Trapped, 0);
    //mainSetBits(BIT_Kyte_Flight_Curve, 18);
    mainSetBits(BIT_4DF, 1);
    mainSetBits(BIT_Tricky_Talk_Sequence, 0); // wat ??
    //mainSetBits(BIT_CRF_Player_In_Treasure_Room, 1); // also the throne room apparently
    mainSetBits(BIT_8B2, 1);
    mainSetBits(BIT_8B3, 1);
    mainSetBits(BIT_8B4, 1);
    mainSetBits(BIT_8B8, 1);
    mainSetBits(BIT_8B9, 1);
    mainSetBits(BIT_8BA, 1);
    mainSetBits(BIT_8BB_Kyte_Distract_Command, 1);
    mainSetBits(BIT_CRF_Courtyard_Drop_Ladder, 1);
    mainSetBits(BIT_8C9, 1);
    mainSetBits(BIT_8CA, 1);
    mainSetBits(BIT_CRF_Prison_Guardian_Exited_WindLift, 1);
    mainSetBits(BIT_8FD, 1);
    mainSetBits(BIT_8FF, 1);
    mainSetBits(BIT_CRF_BlastedRoof_Damage_Counter, 3);
    mainSetBits(BIT_909, 1);

    /* cfbaby rescue */
    mainSetBits(BIT_CFGuardian_State, 15);
    mainSetBits(BIT_1F8, 1);
    mainSetBits(BIT_1F9, 1);
    mainSetBits(BIT_Play_Seq_01CF_CF_Baby_Cloudrunner_Lands_On_Perch_One, 1);
    mainSetBits(BIT_Play_Seq_01CF_CF_Baby_Cloudrunner_Lands_On_Perch_Two, 1);
    mainSetBits(BIT_Play_Seq_01CF_CF_Baby_Cloudrunner_Lands_On_Perch_Three, 1);
    mainSetBits(BIT_Play_Seq_01CF_CF_Baby_Cloudrunner_Lands_On_Perch_Four, 1);
    mainSetBits(BIT_Play_Seq_01CF_CF_Baby_Cloudrunner_Lands_On_Perch_Five, 1);
    mainSetBits(BIT_Played_Seq_01CF_CF_Baby_Cloudrunner_Lands_On_Perch_One, 1);
    mainSetBits(BIT_Played_Seq_01CF_CF_Baby_Cloudrunner_Lands_On_Perch_Two, 1);
    mainSetBits(BIT_Played_Seq_01CF_CF_Baby_Cloudrunner_Lands_On_Perch_Three, 1);
    mainSetBits(BIT_Played_Seq_01CF_CF_Baby_Cloudrunner_Lands_On_Perch_Four, 1);
    mainSetBits(BIT_Played_Seq_01CF_CF_Baby_Cloudrunner_Lands_On_Perch_Five, 1);
    mainSetBits(BIT_CRF_Dock_Left_BigDoorSwitch_State, 1);
    mainSetBits(BIT_305, 1);
    mainSetBits(BIT_CRF_Play_Seq_005D_Treasure_Room_Door_Opening_Sequences, 1);
    mainSetBits(BIT_30E, 1);
    mainSetBits(BIT_323, 1);
    mainSetBits(BIT_324, 1);
    mainSetBits(BIT_330, 1);
    mainSetBits(BIT_CF_Free_Cloudrunner_From_Chest, 1);
    mainSetBits(BIT_33A, 1);
    mainSetBits(BIT_CRF_First_Four_Baby_CloudRunners_Rescued, 1);
    mainSetBits(BIT_350, 1);
    mainSetBits(BIT_35B, 1);
    mainSetBits(BIT_361, 1);
    mainSetBits(BIT_362, 1);
    mainSetBits(BIT_CRF_Galleon_Rescued_Baby_CloudRunner, 1);
    mainSetBits(BIT_Kyte_Flight_Curve, 23);
    mainSetBits(BIT_475, 1);
    mainSetBits(BIT_476, 1);
    mainSetBits(BIT_477_CRF_Treasure_Room_Doors_Opened, 1);
    mainSetBits(BIT_CRF_Throne_Room_Quest_Complete, 1);
    mainSetBits(BIT_4BF, 1);
    mainSetBits(BIT_Played_Seq_02B2_CF_Courtyard_Water_Drains, 1);
    mainSetBits(BIT_Play_Seq_02C6_CF_Sharpclaw_Only_Four_Chests_Left, 1);
    mainSetBits(BIT_Play_Seq_042D_CF_Baby_CloudRunner_Trapped_In_Chest, 1);
    mainSetBits(BIT_8CC, 1);
    mainSetBits(BIT_8CD, 1);
    mainSetBits(BIT_8CE, 1);
    mainSetBits(BIT_8D3, 1);
    mainSetBits(BIT_CRF_Galleon_Force_Low_Detail, 1);
    mainSetBits(BIT_CRF_Num_Rescued_Baby_CloudRunners, 4); // hmm not 5?

    chselect_set_map_state(MAP_INSIDE_GALLEON, 0, G1);
    chselect_set_map_state(MAP_CLOUDRUNNER_FORTRESS, 1, G30);
    chselect_set_map_state(MAP_CLOUDRUNNER_TREASURE, 1, G6);
    chselect_set_map_state(MAP_CLOUDRUNNER_RACETRACK, 0, G3);

    chselect_move_setup(0x2A4F, MAP_CLOUDRUNNER_DUNGEON, -11625.26f, 1877.79f, 19031.80f); // CFGuardian

    /* before race */
    chselect_set_location(PLAYER_KRYSTAL, -12478.511f, 1313.0f, 19042.547f, M_90_DEGREES, -2);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 506;
    light.skylights[1] = 22;
    light.skylights[3] = 511;
    light.trklights[0] = 54;
    // bug: should be inside light
    light.cam = 1;
    chselect_set_light(PLAYER_KRYSTAL, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 400;
    env.unk6 = 86;
    env.unk8 = 9;
    env.unkA = 70;
    env.unkC = 67;
    env.unkE = 92;
    env.unk10 = -10703; env.unk14 = 1716; env.unk18 = 18241;
    env.unk1C = -10884; env.unk20 = 1876; env.unk24 = 18921;
    env.unk28 = -10448; env.unk2C = 1501; env.unk30 = 18255;
    chselect_set_env(PLAYER_KRYSTAL, &env);

    chselect_set_music(PLAYER_KRYSTAL, 296, -1, 16, -1);
}

static void ch_golden_plains_1(void) {
    /* cloudrunner race */
    mainSetBits(BIT_SpellStone_CRF, 1);
    mainSetBits(BIT_498, 1);
    mainSetBits(BIT_Play_Seq_02A9_CF_Race_End, 1);
    mainSetBits(BIT_Play_Seq_0205_CF_Race_Scales_Intro, 1);
    mainSetBits(BIT_Played_Seq_0205_CF_Race_Scales_Intro, 1);
    mainSetBits(BIT_Played_Seq_02A9_CF_Race_End, 1);
    mainSetBits(BIT_4A4, 1);
    mainSetBits(BIT_Played_Seq_02AB_CF_SpellStone_Activation, 1);
    mainSetBits(BIT_Play_Seq_02AB_CF_SpellStone_Activation, 1);
    mainSetBits(BIT_SpellStone_CRF_Activated, 1);

    chselect_move_setup(0x30C20, MAP_CLOUDRUNNER_RACETRACK, -13341.44043f, 1261.983276f, 18904.962891f); // CFGuardian

    /* walk back to swapstone circle */
    mainSetBits(BIT_38B, 0);
    mainSetBits(BIT_Kyte_Flight_Curve, 94);
    mainSetBits(BIT_CRF_Player_Leaving_Via_Entrance_Bridge, 1);
    mainSetBits(BIT_Play_Seq_0210_CF_Exit_Cutscene, 1);
    mainSetBits(BIT_4DE, 1);
    mainSetBits(BIT_CF_Entrance_Trigger_Passed, 0);
    mainSetBits(BIT_63E, 7); // tumbleweeds?

    chselect_set_map_state(MAP_SWAPSTONE_CIRCLE, 5, G4);
    chselect_set_map_state(MAP_MOON_MOUNTAIN_PASS, 1, G_None);
    chselect_set_map_state(MAP_CLOUDRUNNER_FORTRESS, 1, G30);
    chselect_set_map_state(MAP_CLOUDRUNNER_TREASURE, 1, G_None);
    chselect_set_map_state(MAP_CLOUDRUNNER_DUNGEON, 1, G0);
    chselect_set_map_state(MAP_CLOUDRUNNER_RACETRACK, 0, G_None);
    chselect_set_map_state(MAP_DISCOVERY_FALLS, 1, G_None);

    /* golden plains entrance */
    chselect_set_map_state(MAP_GOLDEN_PLAINS, 1, G1);

    mainSetBits(BIT_GP_Sharpclaw_Jetbike_Cutscene2, 1); // !!!

    chselect_set_location(PLAYER_KRYSTAL, -941.265564f, 121.0f, 15331.285156f, 0, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 485;
    light.skylights[1] = 486;
    light.skylights[3] = 570;
    light.trklights[0] = 484;
#if CHSELECT_NEWLFX
    light.newlfx = 90;
#endif
    light.cam = 1;
    chselect_set_light(PLAYER_KRYSTAL, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 341;
    env.unk6 = 485;
    env.unk8 = 484;
    env.unkA = 451;
    env.unkC = 67;
    env.unkE = 92;
    env.unk10 = -1073; env.unk14 = 394; env.unk18 = 17380;
    env.unk1C = -10884; env.unk20 = 1876; env.unk24 = 18921;
    env.unk28 = -10448; env.unk2C = 1501; env.unk30 = 18255;
    chselect_set_env(PLAYER_KRYSTAL, &env);

    chselect_set_music(PLAYER_KRYSTAL, 218, -1, 238, -1);
}

static void ch_desert_force_point_1(void) {
    /* golden plains */
    chselect_move_setup(0x33871, MAP_GOLDEN_PLAINS, -935.105469f, 113.410797f, 14225.819336f); // DIMPushBlock

    mainSetBits(BIT_Kyte_Flight_Curve, 93);
    mainSetBits(BIT_Spell_Portal, 1);
    mainSetBits(BIT_63E, 0); // tumbleweed meter?
    mainSetBits(BIT_795, 1);
    mainSetBits(BIT_796, 1);
    mainSetBits(BIT_7A6, 1);
    mainSetBits(BIT_7D6, 1);
    mainSetBits(BIT_8F9, 1);
    mainSetBits(DINOMOD_BIT_976_DFPT_Lift_Lowered, 1);

    chselect_set_map_state(MAP_GOLDEN_PLAINS, 1, G4);
    chselect_set_map_state(MAP_DESERT_FORCE_POINT_TEMPLE_TOP, 1, G0 | G5 | G10);
    chselect_set_map_state(MAP_SWAPSTONE_CIRCLE, 5, G_None);

    /* dfp top */
    chselect_set_location(PLAYER_KRYSTAL, 1388.446f, -168.0f, 10541.898f, M_180_DEGREES, 0);

    PlayerLightActions light = DEFAULT_LACTIONS;
    light.skylights[0] = 485;
    light.skylights[1] = 486;
    light.skylights[2] = 358;
    light.skylights[3] = 570;
    light.trklights[0] = 484;
    // no inside light?
    light.cam = 1;
    chselect_set_light(PLAYER_KRYSTAL, &light);

    PlayerEnvActions env = DEFAULT_ENVACTIONS;
    env.unk4 = 341;
    env.unk6 = 485;
    env.unk8 = 415;
    env.unkA = 451;
    env.unkC = 67;
    env.unkE = 92;
    env.unk10 = -1073; env.unk14 = 394; env.unk18 = 17380;
    env.unk1C = -10884; env.unk20 = 1876; env.unk24 = 18921;
    env.unk28 = -10448; env.unk2C = 1501; env.unk30 = 18255;
    chselect_set_env(PLAYER_KRYSTAL, &env);

    chselect_set_music(PLAYER_KRYSTAL, 218, -1, 237, -1);
}

// WC1 start
static void ch_swapstone_hollow_4(void) {
    /* desert force point */
    mainSetBits(BIT_2B2, 1);
    mainSetBits(BIT_SpellStone_CRF, 0);
    mainSetBits(BIT_DFPT_Load_SpellStone_Holder_1_Interior, 1);
    mainSetBits(BIT_DFP_Place_Spellstone_One, 1);
    mainSetBits(BIT_DFPT_Load_Block_Slide_Puzzle_1, 1);
    mainSetBits(DFPT_Load_Block_Slide_Puzzle_2, 1);
    mainSetBits(DINOMOD_BIT_976_DFPT_Lift_Lowered, 0);

    chselect_set_map_state(MAP_INSIDE_GALLEON, 0, G_None); // interesting
    chselect_set_map_state(MAP_CLOUDRUNNER_FORTRESS, 1, G_None);
    chselect_set_map_state(MAP_CLOUDRUNNER_DUNGEON, 1, G_None);

    /* walk back to swapstone circle */
    mainSetBits(BIT_Kyte_Flight_Curve, 55);
    // some of these might have been during dfp
    mainSetBits(BIT_5E3, 1);
    mainSetBits(BIT_5E6, 17);
    mainSetBits(BIT_5E8, 1);
    mainSetBits(BIT_5EA, 1);
    mainSetBits(BIT_5EB, 1);
    mainSetBits(BIT_5ED, 1);
    mainSetBits(BIT_5F6, 1);
    mainSetBits(BIT_652, 1);
    mainSetBits(BIT_Discovered_Duster_1, 1); // uhhhh i didnt do this tho
    mainSetBits(BIT_766, 1);
    mainSetBits(BIT_GP_ShrinePillar_Raised_1, 1);
    mainSetBits(BIT_8EE, 1);
    
    chselect_set_map_state(MAP_SWAPSTONE_CIRCLE, 5, G0);
    chselect_set_map_state(MAP_GOLDEN_PLAINS, 1, G_None);
    chselect_set_map_state(MAP_DESERT_FORCE_POINT_TEMPLE_TOP, 1, G_None);

    {
        // krystal's last state
        chselect_set_location(PLAYER_KRYSTAL, -1119.43f, 302.10f, 17307.94f, 0x1400, 0);

        PlayerLightActions light = DEFAULT_LACTIONS;
        light.skylights[0] = 485;
        light.skylights[1] = 486;
        light.skylights[2] = 358;
        light.skylights[3] = 570;
        light.trklights[0] = 484;
        light.trklights[1] = 634;
        //light.flags = PL_LIGHT_ACTION_FLAG_IS_INSIDE; // !! bug! should not be set here
        light.cam = 1;
        chselect_set_light(PLAYER_KRYSTAL, &light);

        PlayerEnvActions env = DEFAULT_ENVACTIONS;
        env.unk4 = 79;
        env.unk6 = 80;
        env.unk8 = 9;
        env.unkA = 451;
        env.unkC = 67;
        env.unkE = 92;
        env.unk10 = -1073; env.unk14 = 394; env.unk18 = 17380;
        env.unk1C = -10884; env.unk20 = 1876; env.unk24 = 18921;
        env.unk28 = -10448; env.unk2C = 1501; env.unk30 = 18255;
        chselect_set_env(PLAYER_KRYSTAL, &env);

        chselect_set_music(PLAYER_KRYSTAL, 185, -1, 5, -1);
    }

    /* swap */
    chselect_set_player(PLAYER_SABRE);

    mainSetBits(BIT_Play_Seq_0107_Rocky_Intro_Unused, 1);
    mainSetBits(BIT_Tricky_Talk_Sequence, 201);
    mainSetBits(BIT_Play_Seq_035F_Rocky_Intro, 0);

    chselect_set_map_state(MAP_SWAPSTONE_HOLLOW, 1, G0);

    {
        chselect_set_location(PLAYER_SABRE, -6571.54, -632.0f, -1607.57f, 0, 0);

        PlayerLightActions light = DEFAULT_LACTIONS;
        light.skylights[0] = 232;
        light.skylights[1] = 401;
        light.skylights[2] = 402;
        light.skylights[3] = 374;
        light.trklights[0] = 629;
        light.trklights[1] = 443;
        light.cam = 1;
        chselect_set_light(PLAYER_SABRE, &light);

        PlayerEnvActions env = DEFAULT_ENVACTIONS;
        env.unk4 = 97;
        env.unk6 = 95;
        env.unk8 = 96;
        env.unkA = 424;
        env.unkC = 300;
        env.unk10 = -6571; env.unk14 = -631; env.unk18 = -1526;
        env.unk1C = -7345; env.unk20 = -994; env.unk24 = 7317;
        chselect_set_env(PLAYER_SABRE, &env);

        chselect_set_music(PLAYER_SABRE, 218, -1, 1, -1);
    }
}

static Chapter chapters[] = {
    // Krystal Act 1 (Intro)
    { "Krystal's Intro", NULL },
    { "Ship Battle", ch_ship_battle },
    { "Warlock Mountain 1", ch_wl_1 },
    
    // Sabre Act 1 (Intro)
    { "Sabre's Intro", NULL },
    { "Ice Mountain", ch_ice_mountain },
    { "SnowHorn Wastes 1", ch_snowhorn_wastes_1 },
    { "SwapStone Hollow 1", ch_swapstone_hollow_1 },
    
    // Krystal Act 2 (Spirit 1)
    { "Kyte Rescue", NULL },
    { "SwapStone Circle 1", ch_swapstone_circle_1 },
    { "Cape Claw 1", ch_cape_claw_1 },
    { "Cape Claw 1 Courtyard", ch_cape_claw_1_courtyard },
    { "LightFoot Village Ambush", ch_lightfoot_village_1 },
    { "SwapStone Circle 2 (Beacons)", ch_swapstone_circle_2 },
    { "Discovery Falls", ch_discovery_falls },
    { "Test of Combat", ch_df_shrine },
    { "Warlock Mountain Spirit 1", ch_warlock_mountain_spirit_1 },
    
    // Sabre Act 2 (SpellStone 1, Spirit 2)
    { "SnowHorn Peril", NULL },
    { "SwapStone Hollow 2", ch_swapstone_hollow_2 },
    { "DarkIce Mines 1", ch_darkice_1 },
    { "DarkIce Mines 2", ch_darkice_2 },
    { "Boss Galadon", ch_galadon },
    { "SwapStone Hollow 3 (River)", ch_swapstone_hollow_river },
    { "Diamond Bay 1", ch_diamond_bay_1 },
    { "Volcano Force Point 1", ch_volcano_force_point_1 },
    { "Test Of Strength", ch_db_shrine },
    { "Warlock Mountain Spirit 2", ch_warlock_mountain_spirit_2 },

    // Krystal Act 3 (SpellStone 2)
    { "A Birdy Reunion", NULL },
    { "SwapStone Circle 3", ch_swapstone_circle_3 },
    { "CloudRunner Fortress", ch_crf_start },
    { "CloudRunner Dungeon", ch_crf_dungeon_start },
    { "CloudRunner Race", ch_crf_race },
    { "Golden Plains 1", ch_golden_plains_1 },
    { "Desert Force Point 1", ch_desert_force_point_1 },

    // Sabre Act 3 (SpellStone 3)
    { "RedEye Rampage", NULL },
    { "SwapStone Hollow 4", ch_swapstone_hollow_4 },
    
    // Krystal Act 4 (Spirit 3)
    //{ "To The Moon", NULL },

    // Sabre Act 4 (Spirit 4)
    //{ "Sun and Moon", NULL },
    
    // Krystal Act 5 (SpellStone 4, Spirit 5)
    //{ "Chief's Trials", NULL },

};

Chapter* chselect_get_chapters(s32* count) {
    if (count != NULL) {
        *count = ARRAYCOUNT(chapters);
    }
    
    return chapters;
}
