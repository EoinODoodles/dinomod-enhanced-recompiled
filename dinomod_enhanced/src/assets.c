#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "reasset.h"

#include "core/map.h"
#include "custom/dlls/SHbarrel.h"
#include "custom/dlls/SHbarrelcreator.h"
#include "custom_object_ids.h"
#include "custom_objsetups.h"
#include "compression_util.h"
#include "common_objsetups.h"
#include "configs.h"
#include "custom_textable_ids.h"
#include "custom_gamebits.h"
#include "math_util.h"
#include "mod_common.h"
#include "object_util.h"
#include "objects/307_SeqDoor.h"
#include "objects/511_SHboulder.h"
#include "objects/780_WCBeacon.h"
#include "objects/781_WCPressureSwitch.h"

#include "PR/ultratypes.h"
#include "dlls/objects/common/collectable.h"
#include "dlls/engine/33_BaddieControl.h"
#include "dlls/objects/325_trigger.h"
#include "dlls/objects/418_DFriverflow.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "game/gamebits.h"
#include "macros.h"
#include "sys/pi.h"
#include "sys/map.h"
#include "sys/map_enums.h"
#include "sys/math.h"
#include "sys/memory.h"

INCBIN(block601, "inc/blocks_0601_WC_central_temple_north_east.bin");
INCBIN(block602, "inc/blocks_0602_WC_central_temple_east.bin");
INCBIN(block610, "inc/blocks_0610_WC_central_temple_middle.bin");
INCBIN(block611, "inc/blocks_0611_WC_central_temple_south.bin");
INCBIN(block612, "inc/blocks_0612_WC_central_temple_southmost.bin");
INCBIN(block617, "inc/blocks_0617_WC_central_temple_north_west.bin");
INCBIN(block618, "inc/blocks_0618_WC_central_temple_west.bin");
INCBIN(block619, "inc/blocks_0619_WC_central_temple_south_west.bin");
INCBIN(block628, "inc/blocks_0628_WC_moon_temple_viewing_tile.bin");
INCBIN(block1086, "inc/blocks_1086_WC_boss_corner_sw.bin");
INCBIN(block1087, "inc/blocks_1087_WC_boss_west_corridor_south.bin");
INCBIN(block1088, "inc/blocks_1088_WC_boss_west_corridor_north.bin");
INCBIN(block1089, "inc/blocks_1089_WC_boss_corner_nw.bin");
INCBIN(block1093, "inc/blocks_1093_WC_boss_north_corridor_west.bin");
INCBIN(block1094, "inc/blocks_1094_WC_boss_south_corridor_east.bin");
INCBIN(block1098, "inc/blocks_1098_WC_boss_corner_se.bin");
INCBIN(block1099, "inc/blocks_1099_WC_boss_east_corridor_south.bin");
INCBIN(block1100, "inc/blocks_1100_WC_boss_east_corridor_north.bin");
INCBIN(block1101, "inc/blocks_1101_WC_boss_corner_ne.bin");
INCBIN(block351, "inc/blocks_0351_SHriver_rocky_waterfall.bin");
INCBIN(block358, "inc/blocks_0358_SH_reflection_pool.bin");
INCBIN(block579, "inc/blocks_0579_SHwell_lily_pond_climb.bin");
INCBIN(block580, "inc/blocks_0580_SHwell_lily_pond_vines.bin");
INCBIN(block581, "inc/blocks_0581_SHwell_entrance.bin");
INCBIN(block582, "inc/blocks_0582_SHwell_stalactite_tunnels.bin");
INCBIN(block583, "inc/blocks_0583_SHwell_river_end.bin");
INCBIN(block718, "inc/blocks_0718_DIM1_river_crossing.bin");
INCBIN(block724, "inc/blocks_0724_DIM1_cannon_silo.bin");
INCBIN(block725, "inc/blocks_0725_DIM1_snowball_track_start.bin");
INCBIN(block989, "inc/blocks_0989_DBriver_waterfall_basin_1.bin");
INCBIN(hits989, "inc/hits_0989_DBriver_waterfall_basin_1.bin");
INCBIN(block995, "inc/blocks_0995_DBriver_bend_1.bin");
INCBIN(block994, "inc/blocks_0994_DBriver_waterfall_basin_2.bin");
// INCBIN(block338, "inc/blocks_0338 0152.bin");

INCBIN(tex0_kiosk_gold_key,             "inc/tex0_kiosk_gold_key.bin");
INCBIN(tex0_kiosk_silver_key,           "inc/tex0_kiosk_silver_key_custom.bin");
INCBIN(tex0_kiosk_firefly,              "inc/tex0_kiosk_firefly_custom.bin");
// INCBIN(tex0_kiosk_replay_disk,          "inc/tex0_kiosk_replay_disk_custom.bin");
INCBIN(tex0_custom_energy_egg,          "inc/tex0_energy_egg_custom.bin");
INCBIN(tex0_custom_energy_egg_moldy,    "inc/tex0_energy_egg_moldy_custom.bin");
INCBIN(tex0_kiosk_fox,                  "inc/tex0_kiosk_fox_icon_custom.bin");

INCBIN(models_dimtent_burnt,    "inc/models_0886_DIMtent_burnt_opacity.bin");
INCBIN(objects_dimtent,         "inc/objects_0320_DIMTent.bin");

INCBIN(models_purple_mushroom,  "inc/models_purple_mushroom_recreation.bin");
INCBIN(modanim_purple_mushroom, "inc/modanim_purple_mushroom_recreation.bin");
INCBIN(amap_purple_mushroom,    "inc/amap_purple_mushroom_recreation.bin");
INCBIN(objects_purple_mushroom, "inc/objects_0571_SHrocketmushroo.bin");

INCBIN(objects_shseqobject,     "inc/objects_0561_SHseqobject.bin");
INCBIN(objects_shboulder,       "inc/objects_0583_SHboulder.bin");

INCBIN(objects_shbarrel,        "inc/objects_SHbarrel.bin");
INCBIN(objects_shbarrelcreator, "inc/objects_SHbarrelcreator.bin");

INCBIN(models_wcsuntempledoor,  "inc/models_0938_WCSunTempleDoor.bin");
INCBIN(models_wcmoontempledoor, "inc/models_0939_WCMoonTempleDoor.bin");
INCBIN(models_wcslabdoor,       "inc/models_0942_WCSlabDoor.bin");
INCBIN(models_wcbossdoor,       "inc/models_0943_WCBossDoor.bin");

INCBIN(objects_vampirebat,      "inc/objects_0053_VampireBat.bin");
INCBIN(objects_warppoint,       "inc/objects_1124_WarpPoint.bin");

#define BLOCKS_REPLACE_BASE(trkblk, trkblkBaseID, blockID, file) (reasset_blocks_set(trkblk, reasset_base_id(blockID - trkblkBaseID), REASSET_BASE_NAMESPACE, file, file##_end  - file))
#define MODELS_REPLACE_BASE(modelID, file) (reasset_models_set(modelID, REASSET_BASE_NAMESPACE, file, file##_end  - file))

#define COORDS_SETUP(coordX, coordY, coordZ) .base.x = coordX, .base.y = coordY, .base.z = coordZ
#define TRIGGER_YAW(degrees) ((u8)((float)degrees*0x10/90.0f + 0.5f)) //Yaw for TriggerPlanes etc. (other axes use DEGREES_TO_ANGLE8)
#define TRIGGER_SCALE(scaleFloat) ((u8)(scaleFloat*0x10 + 0.5f))

#define GET_MAPS_OBJECT(mapID, uID) (reasset_map_objects_get(mapID, reasset_base_id(uID), NULL))
#define GET_TRIGGER(mapID, uID) ((Trigger_Setup*)GET_MAPS_OBJECT(mapID, uID))

//A quick way to add ObjectGroup on/off commands to Trigger Objects (enter: on, exit: off)
#define DIRECTIONAL_OBJGROUP_TRIGGER(groupID, triggerObject, cmdSlotIn, cmdSlotOut)\
    triggerObject->commands[cmdSlotIn].condition = CMD_COND_IN | CMD_COND_RE_ENTER;\
    triggerObject->commands[cmdSlotIn].id = TRG_CMD_ENABLE_OBJ_GROUP;\
    triggerObject->commands[cmdSlotIn].paramCombined = groupID;\
    \
    triggerObject->commands[cmdSlotOut].condition = CMD_COND_OUT | CMD_COND_RE_EXIT;\
    triggerObject->commands[cmdSlotOut].id = TRG_CMD_DISABLE_OBJ_GROUP;\
    triggerObject->commands[cmdSlotOut].paramCombined = groupID;

//A quick way to add ObjectGroup on/off commands to Trigger Objects (enter: off, exit: on)
#define DIRECTIONAL_OBJGROUP_TRIGGER_REVERSE(groupID, triggerObject, cmdSlotIn, cmdSlotOut)\
    triggerObject->commands[cmdSlotIn].condition = CMD_COND_IN | CMD_COND_RE_ENTER;\
    triggerObject->commands[cmdSlotIn].id = TRG_CMD_DISABLE_OBJ_GROUP;\
    triggerObject->commands[cmdSlotIn].paramCombined = groupID;\
    \
    triggerObject->commands[cmdSlotOut].condition = CMD_COND_OUT | CMD_COND_RE_EXIT;\
    triggerObject->commands[cmdSlotOut].id = TRG_CMD_ENABLE_OBJ_GROUP;\
    triggerObject->commands[cmdSlotOut].paramCombined = groupID;

//A quick way to add World ObjectGroup on/off commands to Trigger Objects (enter: on, exit: off)
#define DIRECTIONAL_WORLD_OBJGROUP_TRIGGER(groupID, mapID, triggerObject, cmdSlotIn, cmdSlotOut)\
    triggerObject->commands[cmdSlotIn].condition = CMD_COND_IN | CMD_COND_RE_ENTER;\
    triggerObject->commands[cmdSlotIn].id = TRG_CMD_WORLD_ENABLE_OBJ_GROUP;\
    triggerObject->commands[cmdSlotIn].param1 = groupID;\
    triggerObject->commands[cmdSlotIn].param2 = mapID;\
    \
    triggerObject->commands[cmdSlotOut].condition = CMD_COND_OUT | CMD_COND_RE_EXIT;\
    triggerObject->commands[cmdSlotOut].id = TRG_CMD_WORLD_DISABLE_OBJ_GROUP;\
    triggerObject->commands[cmdSlotOut].param1 = groupID;\
    triggerObject->commands[cmdSlotOut].param2 = mapID;

//A quick way to add World ObjectGroup on/off commands to Trigger Objects (enter: off, exit: on)
#define DIRECTIONAL_WORLD_OBJGROUP_TRIGGER_REVERSE(groupID, mapID, triggerObject, cmdSlotIn, cmdSlotOut)\
    triggerObject->commands[cmdSlotIn].condition = CMD_COND_IN | CMD_COND_RE_ENTER;\
    triggerObject->commands[cmdSlotIn].id = TRG_CMD_WORLD_DISABLE_OBJ_GROUP;\
    triggerObject->commands[cmdSlotIn].param1 = groupID;\
    triggerObject->commands[cmdSlotIn].param2 = mapID;\
    \
    triggerObject->commands[cmdSlotOut].condition = CMD_COND_OUT | CMD_COND_RE_EXIT;\
    triggerObject->commands[cmdSlotOut].id = TRG_CMD_WORLD_ENABLE_OBJ_GROUP;\
    triggerObject->commands[cmdSlotOut].param1 = groupID;\
    triggerObject->commands[cmdSlotOut].param2 = mapID;

#define INCFST(fileID, filename, ext) \
    INCBIN(fst_assets_##filename##_##ext, "assets/" #filename "."#ext); \
    reasset_fst_set_static(fileID, fst_assets_##filename##_##ext, fst_assets_##filename##_##ext##_end - fst_assets_##filename##_##ext);

REASSET_ON_FST_SET_LOW_PRIORITY void dinomod_on_reasset_fst_set(void) {
    INCFST(AMAP_BIN, AMAP, bin)
    INCFST(AMAP_TAB, AMAP, tab)

    INCFST(ANIM_BIN, ANIM, bin)
    INCFST(ANIM_TAB, ANIM, tab)

    INCFST(ANIMCURVES_BIN, ANIMCURVES, bin)
    INCFST(ANIMCURVES_TAB, ANIMCURVES, tab)

    INCFST(AUDIO_BIN, AUDIO, bin)

    INCFST(BITTABLE_BIN, BITTABLE, bin)

    INCFST(BLOCKS_BIN, BLOCKS, bin)
    INCFST(BLOCKS_TAB, BLOCKS, tab)

    INCFST(CAMACTIONS_BIN, CAMACTIONS, bin)
    INCFST(ENVFXACT_BIN, ENVFXACT, bin)
    INCFST(FONTS_BIN, FONTS, bin)

    if (recomp_get_config_u32("gametext_flavor") == GAMETEXT_COSMETIC) {
        INCFST(GAMETEXT_BIN, GAMETEXT_cosmetic, bin)
        INCFST(GAMETEXT_TAB, GAMETEXT_cosmetic, tab)
    } else {
        INCFST(GAMETEXT_BIN, GAMETEXT, bin)
        INCFST(GAMETEXT_TAB, GAMETEXT, tab)
    }

    INCFST(GLOBALMAP_BIN, GLOBALMAP, bin)

    INCFST(HITS_BIN, HITS, bin)
    INCFST(HITS_TAB, HITS, tab)

    INCFST(LACTIONS_BIN, LACTIONS, bin)

    INCFST(MAPS_BIN, MAPS, bin)
    INCFST(MAPS_TAB, MAPS, tab)

    INCFST(MODANIM_BIN, MODANIM, bin)
    INCFST(MODANIM_TAB, MODANIM, tab)

    INCFST(MODELS_BIN, MODELS, bin)
    INCFST(MODELS_TAB, MODELS, tab)

    INCFST(MODLINES_BIN, MODLINES, bin)
    INCFST(MODLINES_TAB, MODLINES, tab)

    INCFST(MPEG_BIN, MPEG, bin)
    INCFST(MPEG_TAB, MPEG, tab)

    INCFST(MUSICACTIONS_BIN, MUSICACTIONS, bin)

    INCFST(OBJECTS_BIN, OBJECTS, bin)
    INCFST(OBJECTS_TAB, OBJECTS, tab)
    INCFST(OBJINDEX_BIN, OBJINDEX, bin)

    INCFST(OBJSEQ_BIN, OBJSEQ, bin)
    INCFST(OBJSEQ_TAB, OBJSEQ, tab)
    INCFST(OBJSEQ2CURVE_TAB, OBJSEQ2CURVE, tab)

    INCFST(TABLES_BIN, TABLES, bin)

    INCFST(TEX0_BIN, TEX0, bin)
    INCFST(TEX0_TAB, TEX0, tab)
    INCFST(TEX1_BIN, TEX1, bin)
    INCFST(TEX1_TAB, TEX1, tab)

    INCFST(WARPTAB_BIN, WARPTAB, bin)
}

s32 OBJ_SHbarrel = 1466;
s32 OBJ_SHbarrelcreator = 1467;

static ReAssetNamespace dinomodNs;

static ReAssetID shBarrelIndexID;
static ReAssetID shBarrelcreatorIndexID;

s32 UID_SH_BurrowsSharpClaw = 0x10000;

REASSET_ON_INIT void dinomod_reasset_on_init(void) {
    dinomodNs = reasset_namespace("dinomod");

    shBarrelIndexID = reasset_id(dinomodNs, OBJ_SHbarrel);
    shBarrelcreatorIndexID = reasset_id(dinomodNs, OBJ_SHbarrelcreator);
}

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 unk18;
/*19*/ s8 unk19;
/*1A*/ s16 unk1A;
/*1C*/ u8 _unk1C[0x1E - 0x1C];
/*1E*/ s16 unk1E;
/*20*/ s16 unk20;
} WCApertureSymbol_Setup;

static void walled_city_additions(void) {
    ReAssetID walledCity = reasset_base_id(MAP_WALLED_CITY);

    // Add two FXEmit objects to enable in the moon temple aperture cutscene
    for (s32 i = 0; i < 2; i++) {
        FXEmit_Setup fxemit = {0};
        fxemit.base.objId = OBJ_FXEmit;
        fxemit.base.actExclusions1 = 0;
        fxemit.base.actExclusions2 = 0;
        fxemit.base.loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP;
        fxemit.base.fadeFlags = OBJSETUP_FADE_CAMERA;
        fxemit.base.mapObjGroup = 7;
        fxemit.base.fadeDistance = 50;

        // base off of WCApertureSymbol position
        fxemit.base.x = 3008.52490234375f;
        fxemit.base.y = -694.7548828125f;
        fxemit.base.z = -3690.62060546875f;
        
        switch (i) {
            case 0:
                fxemit.base.x += 0.0f;
                fxemit.base.y += -0.24524f;
                fxemit.base.z += 0.846679f;
                fxemit.unk1A = 0x741; // Circular blue glow inside of aperture
                fxemit.unk1C = -5;
                fxemit.unk27 = 0;
                break;
            case 1:
                fxemit.base.x += -0.266602f;
                fxemit.base.y += -1.635926f - 9.0f;
                fxemit.base.z += 0.492187f + -70.0f;
                fxemit.unk1A = 0x25A; // Blue beams
                fxemit.unk1C = -1;
                fxemit.unk27 = 0;
                break;
        }

        fxemit.unk18 = 0;
        fxemit.unk19 = 0;
        fxemit.toggleGamebit = 0x848; // Active during the aperture cutscene
        fxemit.disableGamebit = -1;
        fxemit.unk22 = 0;
        fxemit.unk23 = 0;
        fxemit.unk24 = 0;
        fxemit.unk25 = 0;
        fxemit.unk26 = 0;
        fxemit.unk28 = 0;
        fxemit.unk29 = 0;
        fxemit.unk2A = 0;

        reasset_map_objects_set(walledCity, reasset_auto_id(dinomodNs), &fxemit, sizeof(fxemit));
    }

    //Add a HitAnimator to ensure ledge grab HITS aren't active until WCSlabDoor moves out of the way
    {
        HitAnimator_Setup hitAnimator = {
            .base = {
                .objId = OBJ_HitAnimator,
                .actExclusions1 = 0,
                .loadFlags = OBJSETUP_LOAD_MAIN,
                .fadeFlags = OBJSETUP_FADE_CAMERA,
                .loadDistance = 32,
                .fadeDistance = 32,
                .x = 959,
                .y = -858,
                .z = -5601
            },
            .mode = HitAnimator_Mode_HITS, //HITS line switched on when gamebit set
            .gamebitActivate = 0x7F8, //WCSlabDoor open
            .hitsAnimatorID = 8 //tag for the ledge grab line
        };

        reasset_map_objects_set(walledCity, reasset_auto_id(dinomodNs), &hitAnimator, sizeof(hitAnimator));
    }
}

static void walled_city_modifications(void) {
    ReAssetID walledCity = reasset_base_id(MAP_WALLED_CITY);
    ReAssetID wcBossRoom = reasset_base_id(MAP_BOSS_KLANADACK);
    ReAssetID wcTrkblk = reasset_base_id(20);
    ReAssetID ktTrkblk = reasset_base_id(53);
    int wcBlocksBase = 585;
    int ktBlocksBase = 1086;

    //TEMPORARY DEFINES (TODO: remove once these are in decomp)
    {
        #define BIT_WC_Sun_Pressure_Switch_Active 0x7ED
        #define BIT_WC_Sun_Beacon_Raised 0x7EF
        #define BIT_WC_Sun_Beacon_Lit 0x7F9
        #define BIT_WC_SlabDoor_Sun_Symbol_Lit 0x7F7

        #define BIT_WC_Moon_Pressure_Switch_Active 0x7EE
        #define BIT_WC_Moon_Beacon_Raised 0x7F0
        #define BIT_WC_Moon_Beacon_Lit 0x7FA
        #define BIT_WC_SlabDoor_Moon_Symbol_Lit 0x802
        
        #define BIT_WC_SlabDoor_Opened 0x813
        #define BIT_WC_Boss_Door_Opened 0x819
        #define BIT_WC_King_EarthWalker_Cage_Opened 0x7F5
        #define BIT_WC_King_EarthWalker_Rescued 0x1DF

        #define BIT_WC_Transporter_Chamber_Rises 0x335
        #define BIT_WC_Transporter_Chamber_Opened 0x235
    }

    //BLOCKS - Central Temple
    {
        /* 
            - Fix gaps in the temple's Moon/Sun Door entrances
            - UV/terrain issues in the tunnels to the beacons
            - Decals for the tunnel's lasers
            - Prevent Tricky from falling into the pits below the beacons
            - Prevent Tricky from falling into the pits below the RedEye statues
            - Collision, UV, and double-sided face fixes around tree border
        */
        BLOCKS_REPLACE_BASE(wcTrkblk, wcBlocksBase, 601, block601); //NE: Prevent Tricky falling into pit below Sun Beacon
        BLOCKS_REPLACE_BASE(wcTrkblk, wcBlocksBase, 602, block602); //E: Fix UV/terrain issues in Sun Beacon tunnel, laser decals
        BLOCKS_REPLACE_BASE(wcTrkblk, wcBlocksBase, 610, block610); //Middle: Fix gaps in temple's Moon/Sun door entrances, beacon tunnel UV fixes, stop Tricky falling below RedEye statues
        BLOCKS_REPLACE_BASE(wcTrkblk, wcBlocksBase, 611, block611); //S: Fix a small gap between WCSlabDoor and its surroundings
        BLOCKS_REPLACE_BASE(wcTrkblk, wcBlocksBase, 612, block612); //Southmost: Fix seams and missing collision on tree border
        BLOCKS_REPLACE_BASE(wcTrkblk, wcBlocksBase, 617, block617); //NW: Fix stretched UVs on top of the wall beside the tree border
        BLOCKS_REPLACE_BASE(wcTrkblk, wcBlocksBase, 618, block618); //W: Fix UV/terrain issues in Moon Beacon tunnel, laser decals
        BLOCKS_REPLACE_BASE(wcTrkblk, wcBlocksBase, 619, block619); //SW: Prevent Tricky falling into pit below Moon Beacon
    }

    //BLOCKS - Boss Room
    {
        BLOCKS_REPLACE_BASE(ktTrkblk, ktBlocksBase, 1086, block1086); //Minor UV fixes: ceiling
        BLOCKS_REPLACE_BASE(ktTrkblk, ktBlocksBase, 1087, block1087); //Minor UV fixes: ceiling
        BLOCKS_REPLACE_BASE(ktTrkblk, ktBlocksBase, 1088, block1088); //Minor UV fixes: ceiling
        BLOCKS_REPLACE_BASE(ktTrkblk, ktBlocksBase, 1089, block1089); //Minor UV fixes: ceiling, floor 
        BLOCKS_REPLACE_BASE(ktTrkblk, ktBlocksBase, 1093, block1093); //Ceiling UV fixes (fix broken range wrap)
        BLOCKS_REPLACE_BASE(ktTrkblk, ktBlocksBase, 1094, block1094); //Ceiling UV fixes (fix broken range wrap)
        BLOCKS_REPLACE_BASE(ktTrkblk, ktBlocksBase, 1098, block1098); //Minor UV fixes: ceiling, floor 
        BLOCKS_REPLACE_BASE(ktTrkblk, ktBlocksBase, 1099, block1099); //Minor UV fixes: ceiling
        BLOCKS_REPLACE_BASE(ktTrkblk, ktBlocksBase, 1100, block1100); //Minor UV fixes: ceiling
        BLOCKS_REPLACE_BASE(ktTrkblk, ktBlocksBase, 1101, block1101); //Minor UV fixes: ceiling
    }

    // WCCageDoor
    // Fix the door playing a sound from far away as you approach Walled City
    {
        SeqDoor_Setup* cageDoor = reasset_map_objects_get(walledCity, 
            reasset_base_id(0x411B0), NULL);
        cageDoor->gamebitOpenA = NO_GAMEBIT;
        cageDoor->gamebitRestoreState = BIT_7F5;
        cageDoor->options = SeqDoor_OPTION_1_Delay_Play_Until_Gamebit_Set | 
                            SeqDoor_OPTION_2_Unload_If_Already_Open | 
                            SeqDoor_OPTION_4_Unload_At_End_of_Sequence;
    }

    // WCPressureSwitch
    {
        // Moon
        {
            //MAPS - Restore Moon switch's model, store Beacon Lit gamebit
            {
                WCPressureSwitch_Setup* pSwitch = reasset_map_objects_get(walledCity, 
                    reasset_base_id(0x411b6), NULL);
                pSwitch->modelIdx = 1;
                pSwitch->gamebitFinished = BIT_WC_Moon_Beacon_Lit;
                pSwitch->gamebitPlayerOnSwitch = DINOMOD_BIT_92E_WC_Player_on_Moon_Pressure_Switch;
            }
        }

        // Sun
        {
            //MAPS - Store Beacon Lit gamebit
            {
                WCPressureSwitch_Setup* pSwitch = reasset_map_objects_get(walledCity, 
                    reasset_base_id(0x411b3), NULL);
                pSwitch->gamebitFinished = BIT_WC_Sun_Beacon_Lit;
                pSwitch->gamebitPlayerOnSwitch = DINOMOD_BIT_92F_WC_Player_on_Sun_Pressure_Switch;
            }
        }
    }

    // WCSunTempleDoor/WCMoonTempleDoor (Fix gaps between the doors and the temple entrances)
    {
        typedef struct {
            ObjSetup base;
            s8 yaw;
            s8 mode;
            s16 offIntervalDuration;
            s16 firingDuration;
            s16 gamebitEnabled;
        } WCSunTempleLaser_Setup;

        // Moon
        {
            //MAPS - Align the door exactly with the temple entrance, and add custom settings
            {
                SeqDoor_Setup* moonDoor = reasset_map_objects_get(walledCity, 
                    reasset_base_id(0x411b4), NULL);
                moonDoor->base.x = 1280;
                moonDoor->base.y = -682;
                moonDoor->base.z = -4800;

                //Don't close on top of the player/sidekick
                moonDoor->options = SeqDoor_OPTION_10_Wait_While_Player_Nearby | SeqDoor_OPTION_20_Wait_While_Sidekick_Nearby | SeqDoor_OPTION_40_3D_Nearby_Check;
                moonDoor->range = 64;
            }

            //MODELS - Fix gaps
            {
                MODELS_REPLACE_BASE(reasset_base_id(939), models_wcmoontempledoor);
            }

            //OBJECTS - Use a modified ObjSeq (Sink down instead of up, to avoid sticking up nonsensically through the next tier's walkway)
            {
                ObjDef* moonTempleDoorDef = reasset_objects_get(reasset_base_id(257), NULL);
                s16* seq = (s16*)((u8*)moonTempleDoorDef + (u32)moonTempleDoorDef->pSeq);
                seq[0] = 0x450;
            }

            //MAPS - Align the Moon Beacon tunnel's lasers exactly with the wall tiles
            //       (so it's easier to predict where they'll appear)
            {
                WCSunTempleLaser_Setup* laser;
                
                laser = reasset_map_objects_get(walledCity, 
                    reasset_base_id(0x4186a), NULL);
                laser->base.x = 1528.388;
                laser->base.y = -939.75;
                laser->base.z = -4900.0;
                laser->yaw = DEGREES_TO_ANGLE8(180);

                laser = reasset_map_objects_get(walledCity, 
                    reasset_base_id(0x41b51), NULL);
                laser->base.x = 1693.126;
                laser->base.y = -939.75;
                laser->base.z = -4900.0;
                laser->yaw = DEGREES_TO_ANGLE8(180);
                //Put this laser into the same objectGroup as its peers 
                //(for some reason it wasn't in an objectGroup before, unlike all the other beacon tunnel lasers)
                laser->base.loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP;
                laser->base.mapObjGroup = 1;

                laser = reasset_map_objects_get(walledCity, 
                    reasset_base_id(0x4186b), NULL);
                laser->base.x = 1834.060;
                laser->base.y = -939.75;
                laser->base.z = -4900.0;
                laser->yaw = DEGREES_TO_ANGLE8(180);
            }
        }

        // Sun
        {
            //MAPS - Align the door exactly with the temple entrance, and add custom settings
            {
                SeqDoor_Setup* sunDoor = reasset_map_objects_get(walledCity, 
                    reasset_base_id(0x411b1), NULL);
                sunDoor->base.x = 640;
                sunDoor->base.y = -682;
                sunDoor->base.z = -4799;

                //Don't close on top of the player/sidekick
                sunDoor->options = SeqDoor_OPTION_10_Wait_While_Player_Nearby | SeqDoor_OPTION_20_Wait_While_Sidekick_Nearby | SeqDoor_OPTION_40_3D_Nearby_Check;
                sunDoor->range = 64;
            }

            //MODELS - Fix gaps, draw underside (since this door moves up instead of down)
            {
                MODELS_REPLACE_BASE(reasset_base_id(938), models_wcsuntempledoor);
            }

            //MAPS - Align the Sun Beacon tunnel's lasers exactly with the wall tiles
            //       (so it's easier to predict where they'll appear)
            {
                WCSunTempleLaser_Setup* laser;
                
                laser = reasset_map_objects_get(walledCity, 
                    reasset_base_id(0x41813), NULL);
                laser->base.x = 391.612;
                laser->base.y = -939.75;
                laser->base.z = -4700.0;
                laser->yaw = 0;

                laser = reasset_map_objects_get(walledCity, 
                    reasset_base_id(0x4183d), NULL);
                laser->base.x = 226.874;
                laser->base.y = -939.75;
                laser->base.z = -4700.0;
                laser->yaw = 0;

                laser = reasset_map_objects_get(walledCity, 
                    reasset_base_id(0x4183c), NULL);
                laser->base.x = 85.940;
                laser->base.y = -939.75;
                laser->base.z = -4700.0;
                laser->yaw = 0;
            }
        }
    }

    // WCBeacons
    {
        // Sun beacon
        {
            WCBeacon_Setup* sunBeacon = reasset_map_objects_get(walledCity, 
                reasset_base_id(0x411B2), NULL);
            sunBeacon->gamebitSwitch = BIT_WC_Sun_Pressure_Switch_Active;
            sunBeacon->gamebitSlab = BIT_WC_SlabDoor_Sun_Symbol_Lit;
        }

        // Moon beacon
        {
            WCBeacon_Setup* sunBeacon = reasset_map_objects_get(walledCity, 
                reasset_base_id(0x411B5), NULL);
            sunBeacon->gamebitSwitch = BIT_WC_Moon_Pressure_Switch_Active;
            sunBeacon->gamebitSlab = BIT_WC_SlabDoor_Moon_Symbol_Lit;
        }
    }

    // WCSlabDoor
    {
        // Fix the small gap between the slab and its surroundings
        {
            //MAPS
            {
                SeqDoor_Setup* slab = reasset_map_objects_get(walledCity, 
                    reasset_base_id(0x411b7), NULL);
                slab->base.x = 959.000;
                slab->base.y = -874.000;
                slab->base.z = -5681.000;
                slab->options = SeqDoor_OPTION_8_Sync_With_State_Gamebit; //Add custom setting too
            }

            //MODELS
            {
                MODELS_REPLACE_BASE(reasset_base_id(942), models_wcslabdoor);
            }
        }

        //Ensure ledge grab HITS aren't active until WCSlabDoor moves out of the way
        {
            //HITS (tag line #6 with an animatorID)
            {
                TrackLine* line = reasset_hits_get(wcTrkblk, reasset_base_id(611 - wcBlocksBase), reasset_base_id(6));
                line->animatorID = 8;
            }
        }
    }

    // WCBossDoor
    // Fix the small gap between the boss door and its surroundings, and tweak the UVs to reduce seams
    {
        //MAPS
        {
            SeqDoor_Setup* door = reasset_map_objects_get(walledCity, 
                reasset_base_id(0x41392), NULL);
            door->base.x = 959.506;
            door->base.y = -1123.000;
            door->base.z = -4800.000;
            door->scale = 65;
            door->options = SeqDoor_OPTION_8_Sync_With_State_Gamebit; //Ensures boss room ramp door is open when Sabre's exiting it with the SpellStone
        }

        //MODELS
        {
            ReAssetID models_wcbossdoor_ID = reasset_base_id(943);
            reasset_models_set(models_wcbossdoor_ID, REASSET_BASE_NAMESPACE, models_wcbossdoor, models_wcbossdoor_end - models_wcbossdoor);
        }
    }

    //WCPressureSwitch (Moon)
    {
        typedef struct {
        /*00*/ ObjSetup base;
        /*18*/ u8 yaw;
        /*19*/ u8 modelIdx;
        /*1A*/ s16 gameBitPressed;             //Gamebit to set when switch is pressed down
        /*1C*/ u8 yOffsetAnimation;            //How far down the switch should move when pressed
        /*1D*/ u8 yThreshold;                  //Threshold for other objects pressing switch
        /*1E*/ u8 distanceGuardCommand;        //Player distance at which Guard sidekick command is selectable
        /*20*/ s16 gamebitActivated;           //Gamebit to check if switch is deactivated
        } PressureSwitch_Setup;

        //MAPS - Restore Moon switch's model
        {
            PressureSwitch_Setup* pSwitch = (PressureSwitch_Setup*)reasset_map_objects_get(walledCity, 
                reasset_base_id(0x411b6), NULL);
            pSwitch->modelIdx = 1;
        }
    }

    // Boss Room Warps
    {
        //Lobby room
        {
            //WarpPoint (handles inbound and outbound warps)
            {
                WarpPoint_Setup* warp = reasset_map_objects_get(walledCity, 
                    reasset_base_id(0x413ae), NULL);
                warp->mode = WarpPoint_CUSTOMMODE_5;
                warp->isInboundWarp = TRUE; //NOTE: it's both inbound and outbound!
                warp->objectSeqIndex = 10;       //Custom sequence (arrival)
                warp->objectSeqIndexDepart = 11; //Custom sequence (departure)

                //Ensures the arrival sequence doesn't overlap/interfere with the post-boss sequence
                warp->gamebit = BIT_WC_King_EarthWalker_Cage_Opened; //Post-boss sequence needs to have been viewed

                warp->gamebitDepart = BIT_81B; //Set via a TriggerPlane to activate the warp, becomes unset afterwards
            
                warp->zShiftArrival = -3;
                warp->zShiftDeparture = -1;
            }

            //TriggerPlane (activates outbound warp into boss room, but only if the entrance ramp is lowered)
            {
                Trigger_Setup* plane = reasset_map_objects_get(walledCity, 
                    reasset_base_id(0x413af), NULL);
                plane->base.z = -4642; //Shift out slightly, so the player isn't too close to the WarpPoint when the departure sequence plays
                plane->rotationX = DEGREES_TO_ANGLE8(336); //Tilt plane backwards, to help ensure player can't drop down from above and miss it
                plane->sizeX = TRIGGER_SCALE(0.64f); //Scale up slightly
                plane->conditionBitFlagIDs[0] = BIT_WC_Boss_Door_Opened; //@recomp: make sure ramp is lowered (sorry speedrunners!)
            }
        }

        //Boss room
        {
            //WarpPoint (handles inbound and outbound warp)
            //Has a custom arrival sequence and departure sequence, which only play on revisits
            {
                WarpPoint_Setup* warp = reasset_map_objects_get(wcBossRoom, 
                    reasset_base_id(0x413B0), NULL);
                warp->base.x = 1376.557;
                warp->base.z = -1680.0;
                warp->mode = WarpPoint_CUSTOMMODE_5;
                warp->isInboundWarp = TRUE; //NOTE: it's both inbound and outbound!
                warp->yaw = DEGREES_TO_ANGLE8(180);
                warp->quarterRange = 120;
                warp->warpID = 91; //Walled City boss lobby
                warp->objectSeqIndex = 10;       //Custom sequence (arrival)
                warp->objectSeqIndexDepart = 11; //Custom sequence (departure)

                //Ensures the arrival sequence only plays on revisits, and doesn't overlap/interfere with the boss intro sequence
                warp->gamebit = BIT_SpellStone_WC;

                warp->gamebitDepart = DINOMOD_BIT_962_WC_Boss_Room_Warp_to_Lobby; //Set via a TriggerPlane to activate the warp, becomes unset afterwards
            }

            //TriggerPlane (activates outbound warp back to lobby)
            {
                Trigger_Setup plane = {
                    .base = {
                        .objId = OBJ_TriggerPlane,
                        .actExclusions1 = MAP_ACT(1), //Only appears after finishing the boss battle
                        .loadFlags = OBJSETUP_LOAD_MAIN,
                        .fadeFlags = OBJSETUP_FADE_CAMERA,
                        .loadDistance = 64,
                        .fadeDistance = 64,
                        .x = 1378.742,
                        .y = 0,
                        .z = -1725.389
                    },
                    .commands[0] = {
                        .condition = CMD_COND_IN | CMD_COND_RE_ENTER,
                        .id = TRG_CMD_BITS,
                        .paramCombined = TRIG_BITS_MODE(TRUE) | DINOMOD_BIT_962_WC_Boss_Room_Warp_to_Lobby //Set gamebit
                    },
                    .sizeX = TRIGGER_SCALE(0.812),
                    .sizeY = 0x10,
                    .sizeZ = 0x10,
                    .conditionBitFlagIDs[0] = NO_GAMEBIT
                };
                reasset_map_objects_set(wcBossRoom, reasset_auto_id(dinomodNs), &plane, sizeof(plane));
            }
        }

        //Add HITS lines allowing the player clamber up from the sides of the ramp 
        //(No real reason for this except that you'd expect to be able to, so shur lookit... why not!)
        {
            ReAssetID blockIDCentralTempleMiddle = reasset_base_id(610 - wcBlocksBase);
            #define BOSS_RAMP_HITS_ANIMATOR 0xB5

            //Uppies
            {
                TrackLine stepUpLines[2] = {
                    { HITS_A(356, -1210, 515), HITS_B(356, -1123, 320), .heightB = 0, .heightA = 87 },
                    { HITS_A(283, -1123, 320), HITS_B(283, -1210, 515), .heightA = 0, .heightB = 87 }
                };

                for (u32 i = 0; i < ARRAYCOUNT(stepUpLines); i++) {
                    stepUpLines[i].settingsA = 0xe;
                    stepUpLines[i].settingsB = TrackLine_SETTINGB_Nonsolid | HITS_Clamber_Up;
                    stepUpLines[i].animatorID = BOSS_RAMP_HITS_ANIMATOR;
                    reasset_hits_set(wcTrkblk, blockIDCentralTempleMiddle, 
                        reasset_auto_id(36 + i), REASSET_BASE_NAMESPACE, &stepUpLines[i]);
                }
            }
        
            //Add HitAnimator for removing these lines
            {
                HitAnimator_Setup hitA = {
                    .base = {
                        .objId = OBJ_HitAnimator,
                        .actExclusions1 = 0, //NOTE: HITS line needs controlling regardless of current Act
                        .loadFlags = OBJSETUP_LOAD_MAIN,
                        .fadeFlags = OBJSETUP_FADE_CAMERA,
                        .loadDistance = 30,
                        .fadeDistance = 30,
                        .x = 953.487,
                        .y = -1123.000,
                        .z = -4605.000
                    },
                    .gamebitActivate = BIT_WC_Boss_Door_Opened,
                    .mode = hitanimator_configure_mode_flags(
                        FALSE, FALSE, FALSE),
                    .hitsAnimatorID = BOSS_RAMP_HITS_ANIMATOR
                };
                reasset_map_objects_set(walledCity, reasset_auto_id(dinomodNs), &hitA, sizeof(hitA));
            }
        }
    }

    //Boss Room - Adding Act values to all boss fight objects (so they can be hidden on revisit)
    {
        
        ReAssetIterator iterator = reasset_map_objects_create_iterator(wcBossRoom);
        ReAssetID id;
        while (reasset_iterator_next(iterator, &id)) {
            ObjSetup* setup = reasset_map_objects_get(wcBossRoom, id, NULL);
            switch (setup->objId) {
            case OBJ_KT_Rex:
            case OBJ_KT_Fallingrocks:
            case OBJ_KT_RexSequences:
            case OBJ_KT_RexFloorSwit:
            case OBJ_KT_Lazerwall:
            case OBJ_KT_Lazerlight:
                setup->actExclusions1 = ~MAP_ACT(1);
            }
        }
        reasset_iterator_destroy(iterator);
    }

    //Sun/Moon Apertures
    {
        // Fix terrain ID of moon temple viewing tile (to let the aperture work correctly)
        BLOCKS_REPLACE_BASE(wcTrkblk, wcBlocksBase, 628, block628);

        // Revert dinomod's removal of the moon temple lift sequences, so it can be used again
        {
            ObjDef* moonTempleLiftDef = reasset_objects_get(reasset_base_id(276), NULL);
            s16* seq = (s16*)((u8*)moonTempleLiftDef + (u32)moonTempleLiftDef->pSeq);
            seq[0] = 0x3D4;
            seq[1] = 0x3D6;
        }

        // Moon door seqobj
        // Revert dinomod's gamebit change, so the door only opens after the aperture sequence
        {
            SeqObj_Setup* seqobj = reasset_map_objects_get(walledCity, 
                reasset_base_id(0x41474), NULL);
            seqobj->gamebitPlay = 0x829;
        }

        // Moon Aperture
        // Set to always enabled (the sun aperture is always enabled as well)
        {
            WCApertureSymbol_Setup* moonAperture = reasset_map_objects_get(walledCity, 
                reasset_base_id(0x41463), NULL);
            moonAperture->unk20 = BIT_ALWAYS_1;
        }
    }

    // Delete DummyObjects in Moon/Sun temple basements (these are objects with an unmapped OBJINDEX.bin entry)
    reasset_map_objects_delete(walledCity, reasset_base_id(0x41AFC));
    reasset_map_objects_delete(walledCity, reasset_base_id(0x41B35));

    //Remove HITS around Central Temple's Krazoa Shrine transporter when risen (fixing a bug where you'd climb over them on your way into the transporter)
    {
        #define TRANSPORTER_CHAMBER_HITS_ANIMATOR 0x54
        #define TRANSPORTER_CHAMBER_HITS_LINE_BASE 32

        //Add animatorID to relevant lines
        {
            TrackLine* line;
            ReAssetID blockIDCentralTempleMiddle = reasset_base_id(610 - wcBlocksBase);

            for (u32 i = 0; i < 4; i++) {
                line = reasset_hits_get(wcTrkblk, blockIDCentralTempleMiddle, 
                reasset_base_id(TRANSPORTER_CHAMBER_HITS_LINE_BASE + i));

                line->animatorID = TRANSPORTER_CHAMBER_HITS_ANIMATOR;
            }
        }
    
        //Add HitAnimator for removing these lines
        {
            HitAnimator_Setup hitA = {
                .base = {
                    .objId = OBJ_HitAnimator,
                    .actExclusions1 = 0,
                    .loadFlags = OBJSETUP_LOAD_MAIN,
                    .fadeFlags = OBJSETUP_FADE_CAMERA,
                    .loadDistance = 200,
                    .fadeDistance = 200,
                    .x = 959.000,
                    .y = -413.000,
                    .z = -4799.000
                },
                .gamebitActivate = BIT_WC_Transporter_Chamber_Opened,
                .mode = hitanimator_configure_mode_flags(
                    TRUE, FALSE, FALSE),
                .hitsAnimatorID = TRANSPORTER_CHAMBER_HITS_ANIMATOR
            };
            reasset_map_objects_set(walledCity, reasset_auto_id(dinomodNs), &hitA, sizeof(hitA));
        }
    }

    // Edit objectGroup toggle TriggerPlanes to be directionally specific
    {
        Trigger_Setup* plane;

        //Approach route
        {
            plane = GET_TRIGGER(walledCity, 0x40beb);
            DIRECTIONAL_OBJGROUP_TRIGGER(5, plane, 0, 4); //90-degree bend after Queen EarthWalker's gateway
        }

        //Central temple
        {
            plane = GET_TRIGGER(walledCity, 0x40dad);
            DIRECTIONAL_OBJGROUP_TRIGGER(0, plane, 2, 5); //Sun beacon temple entrance

            plane = GET_TRIGGER(walledCity, 0x40dae);
            DIRECTIONAL_OBJGROUP_TRIGGER(0, plane, 2, 5); //Sun beacon tunnel exit

            plane = GET_TRIGGER(walledCity, 0x4104c);
            DIRECTIONAL_OBJGROUP_TRIGGER(1, plane, 2, 5); //Moon beacon temple entrance

            plane = GET_TRIGGER(walledCity, 0x4104f);
            DIRECTIONAL_OBJGROUP_TRIGGER(1, plane, 2, 5); //Moon beacon tunnel exit

            plane = GET_TRIGGER(walledCity, 0x41052);
            DIRECTIONAL_OBJGROUP_TRIGGER(4, plane, 2, 3); //Boss lobby
        }

        //Sun temple route
        {
            plane = GET_TRIGGER(walledCity, 0x41312);
            DIRECTIONAL_OBJGROUP_TRIGGER(2, plane, 0, 5); //Sun pushblocks approach
            DIRECTIONAL_OBJGROUP_TRIGGER(6, plane, 6, 7); //Sun pushblocks approach

            plane = GET_TRIGGER(walledCity, 0x41311);
            DIRECTIONAL_OBJGROUP_TRIGGER(2, plane, 0, 5); //Sun temple approach

            plane = GET_TRIGGER(walledCity, 0x41775);
            DIRECTIONAL_OBJGROUP_TRIGGER_REVERSE(6, plane, 0, 1); //Sun temple entry
            DIRECTIONAL_OBJGROUP_TRIGGER(8, plane, 6, 7); //Sun temple entry
        }

        //Moon temple route
        {
            plane = GET_TRIGGER(walledCity, 0x4130e);
            DIRECTIONAL_OBJGROUP_TRIGGER(3, plane, 0, 5); //Moon pushblocks approach
            DIRECTIONAL_OBJGROUP_TRIGGER(7, plane, 6, 7); //Moon pushblocks approach

            plane = GET_TRIGGER(walledCity, 0x4130f);
            DIRECTIONAL_OBJGROUP_TRIGGER(3, plane, 0, 6); //Moon temple approach

            plane = GET_TRIGGER(walledCity, 0x41604);
            DIRECTIONAL_OBJGROUP_TRIGGER_REVERSE(7, plane, 6, 7); //Moon temple entry
            DIRECTIONAL_OBJGROUP_TRIGGER(9, plane, 2, 3); //Moon temple entry
        }
    }
}

static void shrine_fxemit_modifications(void) {
    // Remove 'disable' gamebit 0x5 for some shrine FXEmits. Ensures they don't get disabled after picking up a CCgrub.
    // The other shrines don't have disable gamebits for their emitters.
    static s32 mapIDs[] = { MAP_SHRINE_DISCOVERY_FALLS, MAP_SHRINE_MOON_MOUNTAIN_PASS, MAP_SHRINE_DIAMOND_BAY };

    for (u32 i = 0; i < ARRAYCOUNT(mapIDs); i++) {
        ReAssetID mapID = reasset_base_id(mapIDs[i]);

        ReAssetIterator iterator = reasset_map_objects_create_iterator(mapID);
        ReAssetID id;
        while (reasset_iterator_next(iterator, &id)) {
            ObjSetup* setup = reasset_map_objects_get(mapID, id, NULL);
            if (setup->objId == OBJ_FXEmit) {
                FXEmit_Setup* fxemit = (FXEmit_Setup*)setup;
                if (fxemit->disableGamebit == 0x5) {
                    fxemit->disableGamebit = -1;
                }
            }
        }
        reasset_iterator_destroy(iterator);
    }
}

/** Adding HitAnimators and HITS line tags in WM's main room to toggle the ledge grab lines at WM_Platform's upper destination */
static void warlock_mountain_platform_additions(void) {
    //Repurposing these lift gamebits to keep track of the ledge grab HitAnimators
    #define LIFT_NEAR_TOP_GAMEBIT_KRYSTAL BIT_322
    #define LIFT_NEAR_TOP_GAMEBIT_SABRE   BIT_369

    ReAssetID warlockMountain = reasset_base_id(MAP_WARLOCK_MOUNTAIN);

    // Add two HitAnimators to the upper tier of Warlock Mountain's main chamber, 
    // for toggling the ledge grab lines at the lifts' upper destinations 
    for (s32 i = 0; i < 2; i++) {
        HitAnimator_Setup hitAnimator = {
            .base = {
                .objId = OBJ_HitAnimator,
                .actExclusions1 = 0,
                .loadFlags = OBJSETUP_LOAD_MAIN,
                .fadeFlags = OBJSETUP_FADE_CAMERA,
                .loadDistance = 10,
                .fadeDistance = 10
            },
            .mode = HitAnimator_Mode_HITS | HitAnimator_Mode_Invert, //HITS line switched off when gamebit set
            .hitsAnimatorID = 8 //tag for the ledge grab line
        };

        switch (i) {
            case 0:
                //Krystal side
                hitAnimator.base.x = 1369.1f;
                hitAnimator.base.y = 472.0f;
                hitAnimator.base.z = 2692.4f;
                hitAnimator.gamebitActivate = LIFT_NEAR_TOP_GAMEBIT_KRYSTAL;
                break;
            case 1:
                //Sabre side
                hitAnimator.base.x = 1186.8f;
                hitAnimator.base.y = 472.0f;
                hitAnimator.base.z = 1793.0f;
                hitAnimator.gamebitActivate = LIFT_NEAR_TOP_GAMEBIT_SABRE;
                break;
        }

        reasset_map_objects_set(warlockMountain, reasset_auto_id(dinomodNs), &hitAnimator, sizeof(hitAnimator));
    }
}
static void warlock_mountain_platform_modifications(void) {
    ReAssetID wlTrkblk = reasset_base_id(15);

    // Modify the ledge grab HITS lines at the lifts' upper destinations, adding HitAnimator tags
    TrackLine* line;

    //Krystal's side
    line = reasset_hits_get(wlTrkblk, reasset_base_id(459 - 438), reasset_base_id(0));
    line->animatorID = 8;

    //Sabre's side
    line = reasset_hits_get(wlTrkblk, reasset_base_id(447 - 438), reasset_base_id(0));
    line->animatorID = 8;
}

static void warlock_mountain_modifications(void) {
    ReAssetID wm = reasset_base_id(MAP_WARLOCK_MOUNTAIN);

    // Clear triggered bit field from dinomod WL_seqpoint for spirit 7. This bit won't get set anyway due
    // to the seq warp changing the map before WL_seqpoint gets a chance to set it. We've changed the bit
    // for this anyway for recomp.
    {
        WL_seqpoint_Setup* spirit7Seqpoint = reasset_map_objects_get(wm, reasset_base_id(0x45661), NULL);
        spirit7Seqpoint->triggeredBit = -1;
    }
}

static void dragon_rock_upper_modifications(void) {
    ReAssetID drTop = reasset_base_id(MAP_DRAGON_ROCK_TOP);

    // Remove static spawns of DR_EarthWarrior (they were leftover from testing, the one you save in DR lower remains)
    reasset_map_objects_delete(drTop, reasset_base_id(0x338CB));
    reasset_map_objects_delete(drTop, reasset_base_id(0x33984));
    reasset_map_objects_delete(drTop, reasset_base_id(0x406A1));

    // Only enable callpads after the Earth Warrior has been brought to the surface (normally they are always enabled)
    DR_EarthCallPad_Setup* callpad;
    callpad = reasset_map_objects_get(drTop, reasset_base_id(0x4033C), NULL);
    callpad->gamebitEnabled = 0x656;
    callpad = reasset_map_objects_get(drTop, reasset_base_id(0x4059C), NULL);
    callpad->gamebitEnabled = 0x656;
}

static void dragon_rock_bottom_modifications(void) {
    ReAssetID drBot = reasset_base_id(MAP_DRAGON_ROCK_BOTTOM);

    // Increase barrel load distance so it doesn't despawn before the pressure pads, otherwise the pad will go back
    // up and the player will be unable to activate all three at the same time. Note that this is really only an issue
    // for the pad by the lake with the turrent in the middle. The other pads have load distances similar to the barrel
    // and despawn fast enough to not have a chance to go back up.
    {
        ObjSetup* cfBarrel = reasset_map_objects_get(drBot, reasset_base_id(0x34AD3), NULL);
        cfBarrel->loadDistance = 127;
    }
}

/** Change the fade settings of the GP_ShrinePillar so they're visible across Golden Plains 
  * (they're fairly low-poly, should be grand even on N64!) */
static void golden_plains_modifications(void) {
    ReAssetID mapID = reasset_base_id(MAP_GOLDEN_PLAINS);

    ReAssetIterator iterator = reasset_map_objects_create_iterator(mapID);
    ReAssetID id;
    while (reasset_iterator_next(iterator, &id)) {
        ObjSetup* setup = reasset_map_objects_get(mapID, id, NULL);
        if (setup->objId == OBJ_GP_PillarDoor || setup->objId == OBJ_GP_ShrinePillar) {
            setup->fadeFlags = OBJSETUP_FADE_MANUAL;
        }
    }
    reasset_iterator_destroy(iterator);
}

/** Makes sure collectables have objectIDs for the animObj that should appear when they're collected */
static void collectables_animobj_patch(void) {
    typedef struct {
        u32 objIndexID; //the collectable object
        u32 animObjID;  //the objectID of the animObj to show during the item collection cutscene
    } Object_AnimObj_Pair;

    Object_AnimObj_Pair collectables[] = {
        {OBJ_SC_golden_nugge, OBJ_CCgolden_nugget}, //Shiny Nugget (SwapStone Circle)
        {OBJ_CCgoldnuggetPic, OBJ_CCgolden_nugget}  //Shiny Nugget (Cape Claw)
    };

    ReAssetID objectIndex;
    ObjDef* objDef;
    CollectableDef* collectable;

    for (u32 i = 0; i < ARRAYCOUNT(collectables); i++) {
        reasset_object_indices_get(
            reasset_base_id(collectables[i].objIndexID), 
            &objectIndex
        );

        objDef = reasset_objects_get(objectIndex, NULL);
        if (!objDef) {
            recomp_eprintf("[ASSETS.C] Couldn't find objDef for objIndex %x\n", collectables[i].objIndexID);
            continue;
        }

        if ((u32)objDef->collectableDef == 0) {
            recomp_eprintf("[ASSETS.C] Couldn't find collectableDef for %s\n", objDef->name);
            continue;
        }

        collectable = (CollectableDef*)((u32)objDef + (u32)objDef->collectableDef);
        collectable->seqObjectID = collectables[i].animObjID;
        // recomp_printf("Patched %s!\n", objDef->name);
    } 
}

static void cape_claw_modifications(void) {
    ReAssetID capeClaw = reasset_base_id(MAP_CAPE_CLAW);

    // Causes Cape Claw's Shiny Nugget to increment the same gamebit used by SwapStone Circle's Shiny Nuggets
    {
        Collectable_Setup* gold = (Collectable_Setup*)reasset_map_objects_get(capeClaw, 
                reasset_base_id(0x42DFD), NULL);
        if (gold->base.objId == OBJ_CCgoldnuggetPic) {
            gold->gamebitCount = BIT_627;
        }
    }

    // Stop creating spray particles after Kyte pulls lever (gamebit moved to ObjSetup to make patch more reusable)
    {
        u32 waterFallSprays[] = {0x42E2C, 0x42E2D, 0x42E2E};

        for (u8 i = 0, end = ARRAYCOUNT(waterFallSprays); i < end; i ++) {
            WaterFallSpray_Setup* spray = (WaterFallSpray_Setup*)reasset_map_objects_get(capeClaw, 
                    reasset_base_id(waterFallSprays[i]), NULL);
            if (spray->base.objId == OBJ_WaterFallSpray) {
                spray->gamebit = BIT_144;
            }
        }
    }

    // Change CC riverflow between the fruit tree and the golden nugget cave (along the shore) to ignore logs. This
    // riverflow is great for stopping the player from leaving the shore but makes it unnecessarily difficult to get
    // to the golden nugget cave with the log.
    {
        DFriverflow_Setup* riverflow = reasset_map_objects_get(capeClaw, reasset_base_id(0x300A9), NULL);
        riverflow->flags = 0xFE; // ignore log but not player
    }

    // Edit outer CC riverflows to prevent the player from leaving the area
    {
        DFriverflow_Setup* riverflow;

        // Shift the one close to the golden nugget cave a little closer
        riverflow = reasset_map_objects_get(capeClaw, reasset_base_id(0x300AC), NULL);
        riverflow->base.x = 1804.66f;
        riverflow->base.z = 2110.17f;

        // Crank up the size of the one closer to the hightop
        riverflow = reasset_map_objects_get(capeClaw, reasset_base_id(0x304ED), NULL);
        riverflow->range = 200;

        // New riverflow to fill in a gap
        DFriverflow_Setup newRiverflow = {
            .base = {
                .objId = OBJ_DFriverflow,
                .actExclusions1 = 0xF8,
                .loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP,
                .fadeFlags = OBJSETUP_FADE_CAMERA,
                .mapObjGroup = 1,
                .fadeDistance = 0,
                .x = 1370.0f,
                .y = -277.0f,
                .z = 1960.0f
            },
            .yaw = 0,
            .range = 0xFF,
            .flags = 0xFF,
            .toggleGamebit = -1
        };

        reasset_map_objects_set(capeClaw, reasset_auto_id(dinomodNs), &newRiverflow, sizeof(newRiverflow));
    }
}

/** (CURRENTLY UNUSED) Adds jetbike fuel refills around Golden Plains, only showing up in Act 3 */
PRAGMA_IGNORE_PUSH("-Wunused")
static void golden_plains_fuel_additions(void) {
    ReAssetID mapID = reasset_base_id(MAP_GOLDEN_PLAINS);

    typedef struct {
    /*00*/ ObjSetup base;
    /*18*/ u8 _unk18;
    /*19*/ u8 _unk19;
    /*1A*/ s16 unk1A;
    /*1C*/ u8 _unk1C;
    /*1D*/ u8 _unk1D;
    /*1E*/ s16 gamebit;
    } CRFuelTank_Setup;

    typedef struct {
        Vec3f coords;
    } CustomFuel;

    CustomFuel fuelData[20] = {
        {VEC3F(3638.878, 278.484, 2340.379)},
        {VEC3F(4026.011, 427.872, 1896.975)},
        {VEC3F(4305.506, 450.573, 1843.973)},
        {VEC3F(4555.718, 442.071, 1663.649)},
        {VEC3F(4683.737, 335.360, 2100.510)},
        {VEC3F(4756.052, 316.182, 766.135)},
        {VEC3F(4507.222, 295.304, 598.087)},
        {VEC3F(3078.822, 304.259, 357.186)},
        {VEC3F(2208.539, 310.000, 588.102)},
        {VEC3F(2275.768, 257.000, 1167.243)},
        {VEC3F(2678.663, 246.000, 1656.635)},
        {VEC3F(2935.286, 190.358, 2138.605)},
        {VEC3F(3249.799, 321.815, 2329.280)},
        {VEC3F(2738.000, 362.000, 1538.000)},
        {VEC3F(2586.000, 388.000, 1065.000)},
        {VEC3F(2604.000, 365.000, 640.000)},
        {VEC3F(2391.022, 310.655, 2007.850)},
        {VEC3F(2391.022, 310.655, 2007.850)},
        {VEC3F(2086.857, 120.619, 2888.839)},
        {VEC3F(2681.164, 212.499, 2171.146)},
    };
    u8 count = ARRAYCOUNT(fuelData);

    //Insert the new objects
    for (s32 i = 0; i < count; i++) {
        CRFuelTank_Setup fuel = {
            .base = {
                .objId = OBJ_CRFuelTank,
                .actExclusions1 = ~MAP_ACT(3),
                .loadFlags = OBJSETUP_LOAD_MAIN,
                .fadeFlags = OBJSETUP_FADE_CAMERA,
                .loadDistance = 140,
                .fadeDistance = 140,
                .x = fuelData[i].coords.x,
                .y = fuelData[i].coords.y,
                .z = fuelData[i].coords.z
            },
            .unk1A = 0x12C,
            .gamebit = NO_GAMEBIT
        };
        reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &fuel, sizeof(fuel));
    }
}
PRAGMA_IGNORE_POP()

#define BOULDER_BIT DINOMOD_BIT_920_SH_BoulderBlownUp
#define RIVER_BIT DINOMOD_BIT_921_SH_RiverUnblocked
#define VINES_BIT DINOMOD_BIT_922_SH_Well_LilyPondVinesUnblocked
#define BOULDER_SEQ_BIT DINOMOD_BIT_92C_SH_River_Seq_Has_Played

static void swapstone_hollow_additions(void) {
    ReAssetID mapID = reasset_base_id(MAP_SWAPSTONE_HOLLOW);
    ReAssetID shTrkblk = reasset_base_id(12);
    int sHollowBlocksBase = 345;

    //Add a SHseqobject to play an unused cutscene when Sabre leaves the shop (found by jeebs2kx)
    {
        SeqObj_Setup objSeq = {
            .base = {
                .objId = OBJ_SHseqobject,
                .actExclusions1 = 0,
                .loadFlags = OBJSETUP_LOAD_MAIN,
                .fadeFlags = OBJSETUP_FADE_CAMERA,
                .loadDistance = 50,
                .fadeDistance = 50,
                .x = 2529.7,
                .y = -636,
                .z = 1413.7
            },
            .gamebitPlay = BIT_SP_Exiting_Shop,
            .gamebitHasPlayed = NO_GAMEBIT,
            .yaw = 0,
            .playbackOptions = 8,
            .seqIndex = 9,
            .replayActorMask = 1,
            .warpID = 0
        };
        reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &objSeq, sizeof(objSeq));
    }

    //Add a TriggerTime object as well, to make sure the "Exiting Shop" gamebit unsets while near the SwapStone
    {
        Trigger_Setup tTime = {
            .base = {
                .objId = OBJ_TriggerTime,
                .loadFlags = OBJSETUP_LOAD_MAIN,
                .fadeFlags = OBJSETUP_FADE_CAMERA,
                .loadDistance = 50,
                .fadeDistance = 50,
                .x = 2529.7,
                .y = -616,
                .z = 1413.7
            },
            .commands = {
                {
                    .condition = CMD_COND_IN,
                    .id = TRG_CMD_BITS,
                    .paramCombined = TRIG_BITS_MODE(FALSE) | BIT_SP_Exiting_Shop //Unset gamebit
                },
            },
            .timerDuration = 60
        };

        reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &tTime, sizeof(tTime));
    }

    //Add SHBoulders blocking the waterfall near Rocky 
    //(One for each side of the opening, to give the illusion that you're seeing the back of the boulder)
    {
        SHboulder_Setup boulders[2] = {
            {COORDS_SETUP(2410.9, -654.8, 956.3), .scale = 146, .yaw = DEGREES_TO_ANGLE8(34.3f), .pitch = DEGREES_TO_ANGLE8(0.9f), .invincible = FALSE, .debris = FALSE}, //Inner side (Rocky's pond)
            {COORDS_SETUP(2383.0, -642.7, 922.0), .scale = 141, .yaw = DEGREES_TO_ANGLE8(21.0f), .pitch = 0,                       .invincible = TRUE,  .debris = TRUE},  //Outer side
        };

        for (s32 i = 0, count = ARRAYCOUNT(boulders); i < count; i++) {
            SHboulder_Setup* boulder = &boulders[i];
            boulder->base.objId = OBJ_SHboulder;
            boulder->base.actExclusions1 = ~MAP_ACT(1);
            boulder->base.loadFlags = OBJSETUP_LOAD_MAIN;
            boulder->base.fadeFlags = OBJSETUP_FADE_CAMERA;
            boulder->base.loadDistance = 170;
            boulder->base.fadeDistance = 170;
            boulder->gamebitGone = BOULDER_BIT;
            reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), boulder, sizeof(SHboulder_Setup));
        }
    }

    //Add HITS lines around the SHboulder near Rocky, to help stabilise player collision around it
    {
        ReAssetID blockIDRockyWaterfall = reasset_base_id(351 - sHollowBlocksBase);
        #define BOULDER_HITS_ANIMATOR 0xB0

        Vec3f points[] = {
            VEC3F(531, -691, 296),
            VEC3F(518, -691, 328),
            VEC3F(491, -691, 347),
            VEC3F(459, -691, 349)
        };

        for (int i = 0, count = ARRAYCOUNT(points); i < count - 1; i++) {
            TrackLine line = {
                .Ax = points[i].x,
                .Ay = points[i].y,
                .Az = points[i].z,

                .Bx = points[(i + 1) % count].x,
                .By = points[(i + 1) % count].y,
                .Bz = points[(i + 1) % count].z,

                .heightA = 60,
                .heightB = 60,

                .settingsA = 0,
                .settingsB = HITS_1,

                .animatorID = BOULDER_HITS_ANIMATOR,
            };
            reasset_hits_set(shTrkblk, blockIDRockyWaterfall, reasset_auto_id(40 + i), REASSET_BASE_NAMESPACE, &line);
        }
    
        //Add HitAnimator for removing these lines
        {
            HitAnimator_Setup hitA = {
                .base = {
                    .objId = OBJ_HitAnimator,
                    .actExclusions1 = 0, //NOTE: HITS line needs controlling regardless of current Act
                    .loadFlags = OBJSETUP_LOAD_MAIN,
                    .fadeFlags = OBJSETUP_FADE_CAMERA,
                    .loadDistance = 30,
                    .fadeDistance = 30,
                    .x = 2418.481,
                    .y = -618.471,
                    .z = 967.463
                },
                .gamebitActivate = BOULDER_BIT,
                .mode = hitanimator_configure_mode_flags(
                    TRUE, FALSE, FALSE),
                .hitsAnimatorID = BOULDER_HITS_ANIMATOR
            };
            reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &hitA, sizeof(hitA));
        }
    }

    //Add an ObjectGroup (#21) for the reflection pool cave
    {
        #define OBJGROUP_REFLECTION_POOL 21

        //Add two TriggerPlanes for loading/unloading the group (two to cover different approach routes)
        {
            Trigger_Setup groupPlanes[] = {
                {COORDS_SETUP(2690.560, -628,     1920),     .rotationY = TRIGGER_YAW(180), .sizeX = TRIGGER_SCALE(0.521)},
                {COORDS_SETUP(2939.537, -636.972, 2104.399), .rotationY = TRIGGER_YAW(83),  .sizeX = TRIGGER_SCALE(0.521)},
            };

            for (int i = 0, end = ARRAYCOUNT(groupPlanes); i < end; i++) {
                Trigger_Setup* plane = &groupPlanes[i];
                plane->base.objId = OBJ_TriggerPlane,
                plane->base.loadFlags = OBJSETUP_LOAD_MAIN,
                plane->base.fadeFlags = OBJSETUP_FADE_MAIN,
                plane->base.loadDistance = 0x40,
                plane->base.fadeDistance = 0x40,

                DIRECTIONAL_OBJGROUP_TRIGGER(OBJGROUP_REFLECTION_POOL, plane, 0, 1);

                plane->sizeY = 0x10;
                plane->sizeZ = 0x10;

                reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), plane, sizeof(Trigger_Setup));
            }
        }

        //Add SHbarrelcreator for blowing up the boulder after Tricky learns Flame
        {
            SHBarrelcreator_Setup barrel = {
                .base = {
                    .objId = OBJ_SHbarrelcreator,
                    .actExclusions1 = ~MAP_ACT(1),
                    .loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP,
                    .fadeFlags = OBJSETUP_FADE_CAMERA,
                    .mapObjGroup = OBJGROUP_REFLECTION_POOL,
                    .fadeDistance = 140,
                    .x = 2244.0f,
                    .y = -676.0f,
                    .z = 2299.0f
                },
                .searchDistance = 0xFF,
                .gamebitStop = BOULDER_BIT,
                .yaw = (-13000) >> 8
            };
            reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &barrel, sizeof(barrel));
        }

        //Add a SharpClaw guarding the barrel
        {
            //TO-DO: use Baddie_Setup
            u8 sharpClaw_Setup[] = {
                /*00*/ 0x00, 0x11, 0x0E, 0x00, 
                /*04*/ 0x00, 0x00, 0x7F, 0x60,
                /*08*/ 0x00, 0x00, 0x00, 0x00, 
                /*0C*/ 0x00, 0x00, 0x00, 0x00, 
                /*10*/ 0x00, 0x00, 0x00, 0x00, 
                /*14*/ 0x00, 0x00, 0x00, 0x00, 

                /*18*/ 0xFF, 0xFF, 0xFF, 0xFF, 
                /*1C*/ 0xFF, 0xFF, 0x00, 0x00, 
                /*20*/ 0x00, 0x00, 0x00, 0x01, 
                /*24*/ 0x00, 0x00, 0x00, 0x03, 
                /*28*/ 0x00, 0x0E, 0x00, 0x00, 
                /*2C*/ 0x00, 0x48, 0xFF, 0x01, 
                /*30*/ 0xFF, 0xFF, 0x06, 0x00, 
                /*34*/ 0x00, 0x00, 0x00, 0x00 
            };

            ObjSetup* sharpClaw = (ObjSetup*)sharpClaw_Setup;
            sharpClaw->objId = OBJ_ClubSharpClaw;
            sharpClaw->actExclusions1 = ~MAP_ACT(1);
            sharpClaw->loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP;
            sharpClaw->fadeFlags = OBJSETUP_FADE_CAMERA;
            sharpClaw->mapObjGroup = OBJGROUP_REFLECTION_POOL;
            sharpClaw->fadeDistance = 140;
            sharpClaw->x = 2370.327f; sharpClaw->y = -674.900f; sharpClaw->z = 2259.521f;

            reasset_map_objects_set(mapID, reasset_id(dinomodNs, UID_SH_BurrowsSharpClaw), &sharpClaw_Setup, sizeof(sharpClaw_Setup));
        }
    }

    //Add HITS lines to help pen the SharpClaw in
    {
        ReAssetID blockIDReflectionPool = reasset_base_id(358 - sHollowBlocksBase);

        Vec3f points[] = {
            VEC3F(415, -680, 331),
            VEC3F(437, -680, 376),
            VEC3F(480, -680, 379),
            VEC3F(519, -680, 319),
            VEC3F(500, -680, 278),
            VEC3F(443, -680, 304),
        };
        for (int i = 0, count = ARRAYCOUNT(points); i < count; i++) {
            TrackLine line = {
                .Ax = points[i].x,
                .Ay = points[i].y,
                .Az = points[i].z,

                .Bx = points[(i + 1) % count].x,
                .By = points[(i + 1) % count].y,
                .Bz = points[(i + 1) % count].z,

                .heightA = 80,
                .heightB = 80,

                .settingsA = 0xb,
                .settingsB = HITS_1,
            };
            reasset_hits_set(shTrkblk, blockIDReflectionPool, reasset_auto_id(14 + i), REASSET_BASE_NAMESPACE, &line);
        }
    }    

    //Add a SHseqobject to play a custom sequence
    {
        SeqObj_Setup objSeq = {
            .base = {
                .objId = OBJ_SHseqobject,
                .actExclusions1 = 0,
                .loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP,
                .fadeFlags = OBJSETUP_FADE_MAIN,
                .mapObjGroup = 0,
                .fadeDistance = 0xFF,
                .x = 2432,
                .y = -532,
                .z = 888
            },
            .gamebitPlay = BOULDER_BIT,
            .gamebitHasPlayed = BOULDER_SEQ_BIT,
            .yaw = 0,
            .playbackOptions = SEQOBJ_OPTIONS_AutoHasPlayed_Set_After_Sequence, //wait until end of sequence to set gamebit
            .seqIndex = 10,
            .replayActorMask = 1,
            .warpID = 0
        };
        reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &objSeq, sizeof(objSeq));
    }

    //Add HitAnimators for removing tangible parts of the water
    {
        HitAnimator_Config hitAnimatorData[] = {
            {VEC3F(2369.237, -620,  737.118),   RIVER_BIT, 1, FALSE, TRUE, FALSE}, //block351 (waterfall near Rocky)
            {VEC3F(2119.723, -620,  477.954),   RIVER_BIT, 1, FALSE, TRUE, FALSE}, //block347 (river bend with log dockpoint)
            {VEC3F(1588.987, -620,  436.598),   RIVER_BIT, 1, FALSE, TRUE, FALSE}, //block360 (river section beside 4 White Mushrooms)
            {VEC3F(1158.884, -620,  538.609),   RIVER_BIT, 1, FALSE, TRUE, FALSE}, //block346 (river crossing, Queen EarthWalker side)
            {VEC3F(903.856,  -620,  789.503),   RIVER_BIT, 1, FALSE, TRUE, FALSE}, //block349 (river crossing, well side)
            {VEC3F(590.928,  -620,  965.955),   RIVER_BIT, 1, FALSE, TRUE, FALSE}, //block989 (Diamond Bay waterfall basin 1) (upper river)
            {VEC3F(285.865,  -1000, 994.401),   RIVER_BIT, 3, FALSE, TRUE, FALSE}, //                                         (rapids)
            {VEC3F(557.214,  -825,  1013.432),  RIVER_BIT, 1, TRUE, FALSE, FALSE}, //                                         (ledge-grab HITS line)
            {VEC3F(180.215,  -1000, 1625.786),  RIVER_BIT, 3, FALSE, TRUE, FALSE}, //block995 (Diamond Bay river bend)
            {VEC3F(-263.215, -1000, 1740.963),  RIVER_BIT, 3, FALSE, TRUE, FALSE}, //block994 (Diamond Bay waterfall basin 2) (water)
        };
        u8 count = ARRAYCOUNT(hitAnimatorData);

        for (s32 i = 0; i < count; i++) {
            HitAnimator_Config* data = &hitAnimatorData[i];
            HitAnimator_Setup hitA = {
                .base = {
                    .objId = OBJ_HitAnimator,
                    .actExclusions1 = (data->isBlocksAnimator == FALSE) ? 0 : ~MAP_ACT(1), //NOTE: HITS line needs controlling regardless of current Act
                    .loadFlags = OBJSETUP_LOAD_LEVEL,
                    .fadeFlags = OBJSETUP_FADE_CAMERA,
                    .loadDistance = 140,
                    .fadeDistance = 140,
                    .x = data->coords.x,
                    .y = data->coords.y,
                    .z = data->coords.z
                },
                .gamebitActivate = data->gamebit,
                .mode = hitanimator_configure_mode_flags(
                    data->removeWhenSet, data->isBlocksAnimator, data->blocksFade),
                .hitsAnimatorID =   (data->isBlocksAnimator == FALSE) * data->animatorID,
                .blocksAnimatorID = (data->isBlocksAnimator == TRUE)  * data->animatorID
            };
            reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &hitA, sizeof(hitA));
        }
    }

    //Add VisAnimators for removing intangible parts of the water
    {
        typedef struct {
            Vec3f coords;
            s16 gamebit;
            u8 animatorID;
            u8 removeWhenSet;
        } VisAnimators;

        VisAnimators visAnimatorData[] = {
            {VEC3F(2371.953, -620,  765.635),  RIVER_BIT, 2, FALSE},  //block351 (waterfall near Rocky)
            {VEC3F(501.871,  -620,  981.222),  RIVER_BIT, 2, FALSE},  //block989 (Diamond Bay waterfall basin 1) (waterfall)
            {VEC3F(261.545,  -1000, 995.373),  RIVER_BIT, 4, FALSE},  //                                         (rapids foam)
            {VEC3F(163.287,  -1000, 1638.482), RIVER_BIT, 4, FALSE},  //block995 (Diamond Bay river bend)        (rapids foam)
            {VEC3F(-230.137, -1000, 1740.963), RIVER_BIT, 2, FALSE},  //block994 (Diamond Bay waterfall basin 2) (waterfall)
            {VEC3F(-287.761, -1000, 1740.963), RIVER_BIT, 4, FALSE},  //block994 (Diamond Bay waterfall basin 2) (rapids foam)
        };
        u8 count = ARRAYCOUNT(visAnimatorData);

        //Insert the new objects
        for (s32 i = 0; i < count; i++) {
            VisAnimators* data = &visAnimatorData[i];
            VisAnimator_Setup visA = {
                .base = {
                    .objId = OBJ_VisAnimator,
                    .actExclusions1 = ~MAP_ACT(1),
                    .loadFlags = OBJSETUP_LOAD_LEVEL,
                    .fadeFlags = OBJSETUP_FADE_CAMERA,
                    .loadDistance = 140,
                    .fadeDistance = 140,
                    .x = data->coords.x,
                    .y = data->coords.y,
                    .z = data->coords.z
                },
                .animatorID1 = data->animatorID,
                .gamebitID = data->gamebit,
                .initialVisibility = data->removeWhenSet
            };

            reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &visA, sizeof(visA));
        }
    }

    //Add TexScrolls for the waterfall leading down to Diamond Bay
    {
        TexScroll2_Setup texScrollData[] = {
            {.base.x = 563.975, .base.y = -671.187, .base.z = 961.741, .textureIndex = 30, RIVER_BIT, 0, 3, 0,  7, 31}, //block989 (Diamond Bay waterfall basin 1) (upper river)
            {.base.x = 501.871, .base.y = -671.187, .base.z = 981.222, .textureIndex = 37, RIVER_BIT, 0, 0, 0, -7, -1}, //block989 (Diamond Bay waterfall basin 1) (waterfall top)
            {.base.x = 483.437, .base.y = -638.000, .base.z = 981.222, .textureIndex = 45, RIVER_BIT, 0, 0, 0, -7, -1}, //block989 (Diamond Bay waterfall basin 1) (waterfall main)
        };
        u8 count = ARRAYCOUNT(texScrollData);

        //Insert the new objects
        for (s32 i = 0; i < count; i++) {
            TexScroll2_Setup* scroll = &texScrollData[i];
            scroll->base.objId = OBJ_texscroll2;
            scroll->base.actExclusions1 = ~MAP_ACT(1);
            scroll->base.loadFlags = OBJSETUP_LOAD_LEVEL;
            scroll->base.fadeFlags = OBJSETUP_FADE_CAMERA;
            scroll->base.loadDistance = 140;
            scroll->base.fadeDistance = 140;
            reasset_map_objects_set(mapID, 
                reasset_auto_id(dinomodNs), scroll, sizeof(TexScroll2_Setup)
            );
        }
    }

    //Add WaveAnimators to the waterfall basin (in a special objectGroup just for the sequence)
    {
        typedef struct {
        /*00*/ ObjSetup base;
        /*18*/ s16 unk18;
        /*1A*/ s16 gamebitActivate;
        /*1C*/ u8 frequencyZ;
        /*1D*/ u8 frequencyX;
        /*1E*/ u8 amplitude;
        /*1F*/ s8 verticalOffset;
        /*20*/ u8 animatorID;
        /*21*/ u8 period; //Always set to 60?
        /*22*/ u8 unk22;  //Always set to 6?
        /*23*/ u8 vertexColourEffect; //0: lava, 1: water, 2: orange
        } WaveAnimator_Setup;

        WaveAnimator_Setup waveAnimatorData[] = {
            {COORDS_SETUP(321.844, -1002.904, 1052.198), .frequencyX = 0, .frequencyZ = 1, .animatorID = 3},
            {COORDS_SETUP(321.844, -1002.904, 1052.198), .frequencyX = 0, .frequencyZ = 1, .animatorID = 4},
        };

        for (int i = 0, end = ARRAYCOUNT(waveAnimatorData); i < end; i++) {
            WaveAnimator_Setup* wAnim = &waveAnimatorData[i];
            wAnim->base.objId = OBJ_WaveAnimator;
            wAnim->base.actExclusions1 = 0;
            wAnim->base.loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP;
            wAnim->base.fadeFlags = OBJSETUP_FADE_CAMERA;
            wAnim->base.mapObjGroup = 20;
            wAnim->base.fadeDistance = 140;
            wAnim->amplitude = 3;
            wAnim->verticalOffset = -12,
            wAnim->period = 35;
            wAnim->unk22 = 6;
            wAnim->vertexColourEffect = 1; //water
            reasset_map_objects_set(mapID, 
                reasset_auto_id(dinomodNs), wAnim, sizeof(WaveAnimator_Setup)
            );
        }
    }

    {
        // Add a distract node next to the sleeping log trader thorntail so that tricky
        // can correctly use distract to wake them up. The distract option is vanilla but
        // tricky won't actually use distract without a node like this present.
        // Note: Not sure what a lot of these fields do, this was copied from warlock mountain.
        CurveSetup distractNode = {
            .objId = OBJ_curve,
            .unk3 = 9,
            .pos = {
                .x = 2705.53f,
                .y = -622.83f,
                .z = 1807.94f
            },
            .unk18 = 0,
            .curveType = 0x24,
            .unk1A = 0x3,
            .unk1B = 0,
            .links = {-1, -1, -1, -1},
            .unk2C = 0,
            .unk2D = 0,
            .unk2E = 0x40,
            .unk2F = -1,
            .type24.unk30 = -1,
            .type24.unk32 = BIT_14 // disable after woken up
        };

        reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &distractNode, sizeof(distractNode));
    }

    //Revert the SHboulder blocking Willow Grove back to being a ThornTail (it used be one in older patches) to avoid confusion,
    //since the player might try carrying a barrel to it and be confused about why it can't be destroyed
    //TODO: config for this? 
    {
        //Add a ThornTail (a custom sleeping one)
        {
            typedef struct {
            /*00*/ ObjSetup base;
            /*18*/ u8 thornTailIndex;
            /*19*/ u8 yaw;          //@recomp: custom param
            /*1A*/ s16 gamebitAway; //@recomp: ThornTail doesn't show up when set
            } SHthorntail_Setup;

            SHthorntail_Setup thornTail = {
                .base = {
                    .objId = OBJ_SHthorntail,
                    .actExclusions1 = ~MAP_ACT(1),
                    .loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP,
                    .fadeFlags = OBJSETUP_FADE_CAMERA,
                    .mapObjGroup = 9,
                    .fadeDistance = 100,
                    .x = 3145.765625, 
                    .y = -789,
                    .z = -188.720
                },
                .thornTailIndex = 4,    //Custom ThornTail
                .yaw = DEGREES_TO_ANGLE8(180),
                .gamebitAway = BIT_1E6, //Willow Grove open, maybe the ThornTail wandered off to explore it?
            };
            reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &thornTail, sizeof(thornTail));
        }
    }
}

static void swapstone_hollow_modifications(void) {
    ReAssetID sHollow = reasset_base_id(MAP_SWAPSTONE_HOLLOW);
    ReAssetID shTrkblk = reasset_base_id(12);
    int sHollowBlocksBase = 345;

    //Add extra sequences to SHseqobject
    {
        ReAssetID objects_shseqobject_id = reasset_base_id(561); //OBJ_SHseqobject
        reasset_objects_set(objects_shseqobject_id, REASSET_BASE_NAMESPACE, objects_shseqobject, objects_shseqobject_end - objects_shseqobject);
    }

    //BLOCKS edits (Tag Blocks shapes with animatorIDs, so they can be removed with HitAnimators)
    {
        BLOCKS_REPLACE_BASE(shTrkblk, sHollowBlocksBase, 351, block351);
        BLOCKS_REPLACE_BASE(shTrkblk, sHollowBlocksBase, 358, block358); //add light source
    }

    //Revert changes to SHboulder's DLL usage
    {
        ReAssetID objects_shboulder_id = reasset_base_id(583); //OBJ_SHboulder
        reasset_objects_set(objects_shboulder_id, REASSET_BASE_NAMESPACE, objects_shboulder, objects_shboulder_end - objects_shboulder);
    }

    //Delete the SHboulder blocking Willow Grove (reverting to a ThornTail like in older patches, 
    //to avoid confusion where players might carry the explosive barrel over to it)
    //TODO: config for this, choosing between boulder/ThornTail/something else?
    {
        ReAssetID shBoulderWillowGrove = reasset_base_id(0x307F3);
        reasset_map_objects_delete(sHollow, shBoulderWillowGrove);
    }

    // Move river sfx TriggerPoints into obj group 11
    {
        ObjSetup* triggerPoint;
        
        triggerPoint = reasset_map_objects_get(sHollow, reasset_base_id(0x31CDD), NULL);
        triggerPoint->loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP;
        triggerPoint->mapObjGroup = 11;
        
        triggerPoint = reasset_map_objects_get(sHollow, reasset_base_id(0x31CCA), NULL);
        triggerPoint->loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP;
        triggerPoint->mapObjGroup = 11;
    }

    // Move dockpoint into obj group 12
    {
        ObjSetup* dockpoint = reasset_map_objects_get(sHollow, reasset_base_id(0x42BAA), NULL);
        dockpoint->loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP;
        dockpoint->mapObjGroup = 12;
    }

    // Edit trigger plane at the start of the DB side of the river to enable the DB
    // river object group that contains river flows etc.
    {
        Trigger_Setup* plane = reasset_map_objects_get(sHollow, reasset_base_id(0x18EB), NULL);
        DIRECTIONAL_WORLD_OBJGROUP_TRIGGER(0, MAP_DIAMOND_BAY, plane, 1, 2);
    }

    //Add a HITS line so you dangle off SwapStone Hollow's waterfall when attempting to run off it
    {
        ReAssetID dbTrkblk = reasset_base_id(48);
        TrackLine line = {
            HITS_A(538, -825, 308),
            HITS_B(575, -825, 434),

            .heightA = 40,
            .heightB = 40,

            .settingsA = 0xe,
            .settingsB = TrackLine_SETTINGB_Nonsolid | HITS_Precipice,

            .animatorID = 1,
        };
        reasset_hits_set(dbTrkblk, reasset_base_id(989 - 974), reasset_auto_id(13), REASSET_BASE_NAMESPACE, &line);
    }

    // Edit SHplantspore leading to the SwapStone to only become enabled after saving the queen EarthWalker
    // (in vanilla, this enables after the Thorntail by the well moves. it's just looking for the wrong Thorntail basically)
    {
        DLL506_Setup* plantSpore = reasset_map_objects_get(sHollow, reasset_base_id(0x34DF5), NULL);
        plantSpore->unk20 = BIT_SH_Move_Thorntail_Blocking_Swapstone; // 0x8D4
    }

    // Edit TriggerCylinder around Rocky, so it unsets the "Exiting the Shop" gamebit
    {
        Trigger_Setup* cylinder = GET_TRIGGER(sHollow, 0xbe040a9);
        cylinder->commands[1].id = TRG_CMD_BITS;
        cylinder->commands[1].condition = (CMD_COND_IN | CMD_COND_OUT | CMD_COND_RE_ENTER | CMD_COND_RE_EXIT);
        cylinder->commands[1].paramCombined = BIT_SP_Exiting_Shop;
    }

    // Edit TriggerPlane approaching Rocky, so it unsets the "Exiting the Shop" gamebit too (just in case)
    {
        Trigger_Setup* plane = GET_TRIGGER(sHollow, 0x34732);
        plane->commands[7].id = TRG_CMD_BITS;
        plane->commands[7].condition = (CMD_COND_IN | CMD_COND_OUT | CMD_COND_RE_ENTER | CMD_COND_RE_EXIT);
        plane->commands[7].paramCombined = BIT_SP_Exiting_Shop;
    }

    // Edit TriggerPlane just inside the gateway to Walled City, so 
    // the river crossing area's objects are loaded when approaching from Walled City
    {
        Trigger_Setup* plane = GET_TRIGGER(sHollow, 0x40B75);
        DIRECTIONAL_OBJGROUP_TRIGGER_REVERSE(6, plane, 1, 2);
    }
}

static void swapstone_hollow_well_additions(void) {
    ReAssetID mapID = reasset_base_id(MAP_SWAPSTONE_HOLLOW_2);

    /*  Add SHVine blocking the route to SnowHorn Wastes' river
        (Intended to be burnt away by Tricky from the SnowHorn Wastes side after 
        he learns Flame, so he can be brought down into the well to dig up MoonSeeds.) */
    {
        typedef struct {
            ObjSetup base;
            s8 yaw;
            s8 roll;  //@recomp: repurposing unused field
            u8 scale; //@recomp: repurposing unused field
            s16 flameDistance;
            s16 gamebitBurnt;
        } SHvines_Setup;

        typedef struct {
            Vec3f coords;
            u8 yaw;
            u8 roll;
            u8 scale;
            s16 flameDistance;
        } SHvines_Config;

        SHvines_Config vines[] = {
            // {VEC3F(983.343, -1129.392, 545.166),  DEGREES_TO_ANGLE8(356.640), DEGREES_TO_ANGLE8(9.581),  295/2,  0},   //Lily pond side (replaced with Blocks edit for visual improvements)
            {VEC3F(975.102, -1129.448, 670.665), DEGREES_TO_ANGLE8(182),     DEGREES_TO_ANGLE8(359),    149,    150}, //Stalactite cave
        };

        for (int i = 0, end = ARRAYCOUNT(vines); i < end; i++) {
            SHvines_Setup vine = {
                .base = {
                    .objId = OBJ_SHvines,
                    .actExclusions1 = 0,
                    .loadFlags = OBJSETUP_LOAD_MAIN,
                    .fadeFlags = OBJSETUP_FADE_CAMERA,
                    .loadDistance = 255, //Needs to be visible from quite far away due to the long approach from the ice floe river
                    .fadeDistance = 255,
                    .x = vines[i].coords.x,
                    .y = vines[i].coords.y,
                    .z = vines[i].coords.z
                },
                .yaw = vines[i].yaw,
                .roll = vines[i].roll,
                .scale = vines[i].scale,
                .flameDistance = vines[i].flameDistance,
                .gamebitBurnt = VINES_BIT
            };
            reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &vine, sizeof(vine));
        }
    }

    //Add HitAnimator for the lily pond's HITS lines blocking the river route between SwapStone Hollow Well and SnowHorn Wastes
    {
        HitAnimator_Setup hitA = {
            .base = {
                .objId = OBJ_HitAnimator,
                .actExclusions1 = 0,
                .loadFlags = OBJSETUP_LOAD_MAIN,
                .fadeFlags = OBJSETUP_FADE_CAMERA,
                .loadDistance = 100,
                .fadeDistance = 100,
                .x = 979.246,
                .y = -1041.493,
                .z = 609.779
            },
            .gamebitActivate = VINES_BIT,
            .mode = hitanimator_configure_mode_flags(
                TRUE, FALSE, FALSE),
            .hitsAnimatorID = 1,
        };
        reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &hitA, sizeof(hitA));
    }

    // Add HitAnimator for burning away the vines on the lily pond side of the blockade
    // (This side needed to be part of the BLOCKS model, because of transparency sorting issues between Objects/Blocks)
    {
        HitAnimator_Setup hitA = {
            .base = {
                .objId = OBJ_HitAnimator,
                .actExclusions1 = 0,
                .loadFlags = OBJSETUP_LOAD_MAIN,
                .fadeFlags = OBJSETUP_FADE_CAMERA,
                .loadDistance = 100,
                .fadeDistance = 100,
                .x = 979.246,
                .y = -1049.981,
                .z = 609.779
            },
            .gamebitActivate = VINES_BIT,
            .mode = hitanimator_configure_mode_flags(
                TRUE, TRUE, FALSE),
            .blocksAnimatorID = 3,
        };
        reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &hitA, sizeof(hitA));
    }

    // Add GroundAnimators for the dig spots in the stalactite cave
    // (Just placeholders for now since there's no way for Tricky to navigate to them, but they do work!)
    {
        GroundAnimator_Setup groundAnimators[] = {
            {COORDS_SETUP( 981, -1121, 902),  .gamebitDug = DINOMOD_BIT_926_SH_Well_Stalactite_Cave_Tricky_Dug_1, .animatorID = 50},
            {COORDS_SETUP(1129, -1121, 1122), .gamebitDug = DINOMOD_BIT_927_SH_Well_Stalactite_Cave_Tricky_Dug_2, .animatorID = 51},
            {COORDS_SETUP( 747, -1121, 986),  .gamebitDug = DINOMOD_BIT_928_SH_Well_Stalactite_Cave_Tricky_Dug_3, .animatorID = 52}
        };

        // TODO: patch the GroundAnimators so they don't do anything/don't show Find until a gamebit is set
        // Use `DINOMOD_BIT_923_SH_Well_Stalactite_Cave_Cracked_Ground_1`, 2, 3
        for (int i = 0, end = ARRAYCOUNT(groundAnimators); i < end; i++) {
            GroundAnimator_Setup* groundA = &groundAnimators[i]; 
            groundA->base.objId = OBJ_GroundAnimator;
            groundA->base.actExclusions1 = 0;
            groundA->base.loadFlags = OBJSETUP_LOAD_MAIN;
            groundA->base.fadeFlags = OBJSETUP_FADE_CAMERA;
            groundA->base.loadDistance = 140;
            groundA->base.fadeDistance = 140;
            groundA->digDepthMax = 16;
            groundA->soundIndex = 0;
            groundA->findCommandRadius = 75;
            groundA->falloffRadius = 29;
            groundA->collectableDepth = 10;
            reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), groundA, sizeof(GroundAnimator_Setup));
        }
    }
}

static void swapstone_hollow_well_modifications(void) {
    ReAssetID shWellTrkblk = reasset_base_id(19);
    u32 shWellBlocksBase = 579;

    //SwapStone Hollow Well: Fix mesh holes, fog issues (on dig spot decals), vines, stalactite animatorIDs, UVs
    BLOCKS_REPLACE_BASE(shWellTrkblk, shWellBlocksBase, 579, block579);
    BLOCKS_REPLACE_BASE(shWellTrkblk, shWellBlocksBase, 580, block580);
    BLOCKS_REPLACE_BASE(shWellTrkblk, shWellBlocksBase, 581, block581);
    BLOCKS_REPLACE_BASE(shWellTrkblk, shWellBlocksBase, 582, block582);
    BLOCKS_REPLACE_BASE(shWellTrkblk, shWellBlocksBase, 583, block583);

    //HITS edits
    {
        /* Reinstate Rare's HITS line (now edited to be controlled by HitAnimator), blocking you from 
           taking the well's exit onto the Garunda Te area earlier than you're intended to.

           (You're not supposed to reach Garunda Te until after learning Distract, and the ice floe 
           river route from SnowHorn Wastes into SwapStone Hollow Well isn't supposed to be unblocked
           until Tricky learns Flame.) */
        {
            ReAssetID blockIDLilyPondVines = reasset_base_id(580 - shWellBlocksBase);
            TrackLine line = {
                HITS_A(247, -1125, 575),
                HITS_B(460, -1109, 579),

                .heightUnified = 56,
                .settingsA = TrackLine_SETTINGA_Unified_Height | 0xe,
                .settingsB = HITS_1,
                .animatorID = 1,
            };
            reasset_hits_set(shWellTrkblk, blockIDLilyPondVines, reasset_base_id(32), REASSET_BASE_NAMESPACE, &line);
        }

        //Add an equivalent HITS line at the opposite side of the vines, so the collision's more stable at that side too
        {
            ReAssetID blockIDLilyPondVines = reasset_base_id(580 - shWellBlocksBase);
            TrackLine line = {
                HITS_A(454, -1127, 640),
                HITS_B(238, -1127, 640),

                .heightUnified = 56,
                .settingsA = TrackLine_SETTINGA_Unified_Height | 0xe,
                .settingsB = HITS_1,
                .animatorID = 1,
            };
            reasset_hits_set(shWellTrkblk, blockIDLilyPondVines, reasset_auto_id(33), REASSET_BASE_NAMESPACE, &line);
        }

        // Allow ledge-grabs on two lines near the top of the climbable vines in the lily pond cave
        // (they were set up just as invisible walls, probably by mistake)
        {
            ReAssetID blockIDLilyPondClimb = reasset_base_id(579 - shWellBlocksBase);
            
            TrackLine* line;

            //Line 50
            ReAssetID hitsVineTopEdge1 = reasset_base_id(50);
            line = reasset_hits_get(shWellTrkblk, blockIDLilyPondClimb, hitsVineTopEdge1);
            line->settingsA = 0;
            line->settingsB = TrackLine_SETTINGB_Nonsolid | HITS_Precipice;
            reasset_hits_set(shWellTrkblk, blockIDLilyPondClimb, hitsVineTopEdge1, REASSET_BASE_NAMESPACE, &line);
            
            //Line 51
            ReAssetID hitsVineTopEdge2 = reasset_base_id(51);
            line = reasset_hits_get(shWellTrkblk, blockIDLilyPondClimb, hitsVineTopEdge2);
            line->settingsA = 0;
            line->settingsB = TrackLine_SETTINGB_Nonsolid | HITS_Precipice;
            reasset_hits_set(shWellTrkblk, blockIDLilyPondClimb, hitsVineTopEdge2, REASSET_BASE_NAMESPACE, &line);
        }

        //Add HITS lines so you can jump from the central island in the stalactite cave
        {
            ReAssetID blockIDStalactiteCave = reasset_base_id(582 - shWellBlocksBase);

            Vec3f points[] = {
                VEC3F(344, -1127, 325),
                VEC3F(376, -1127, 306),
                VEC3F(396, -1127, 262),
                VEC3F(376, -1127, 223),
                VEC3F(334, -1127, 200),
                VEC3F(301, -1127, 208),
                VEC3F(283, -1127, 256),
                VEC3F(296, -1127, 294)
            };
            for (int i = 0, count = ARRAYCOUNT(points); i < count; i++) {
                TrackLine line = {
                    .Ax = points[i].x,
                    .Ay = points[i].y,
                    .Az = points[i].z,

                    .Bx = points[(i + 1) % count].x,
                    .By = points[(i + 1) % count].y,
                    .Bz = points[(i + 1) % count].z,

                    .heightA = 40,
                    .heightB = 40,

                    .settingsA = 0,
                    .settingsB = TrackLine_SETTINGB_Nonsolid | HITS_Jump_or_Precipice
                };
                reasset_hits_set(shWellTrkblk, blockIDStalactiteCave, reasset_auto_id(127 + i), REASSET_BASE_NAMESPACE, &line);
            }
        }
    }

}

static void cc_lightfoot_patch(void) {
    // Change CClightfoot model from chief to normal red-colored LightFoot
    ObjDef* ccLightfootObjDef = reasset_objects_get(reasset_base_id(430), NULL);
    u32* ccLightfootModelList = (u32*)((u8*)ccLightfootObjDef + (u32)ccLightfootObjDef->pModelList);
    ccLightfootModelList[0] = 0x00CB;
}

/** Fix DIM and Galadon related music actions. Original patch by nuggs. */
static void music_actions_patch(void) {
    // Galadon
    MusicAction* action103 = reasset_music_actions_get(reasset_base_id(103 - 1));
    action103->seqID = 60;
    MusicAction* action104 = reasset_music_actions_get(reasset_base_id(104 - 1));
    action104->seqID = 60;
    MusicAction* action106 = reasset_music_actions_get(reasset_base_id(106 - 1));
    action106->seqID = 60;
    MusicAction* action108 = reasset_music_actions_get(reasset_base_id(108 - 1));
    action108->seqID = 60;
    
    // DIM
    MusicAction* action109 = reasset_music_actions_get(reasset_base_id(109 - 1));
    action109->seqID = 66;
    MusicAction* action110 = reasset_music_actions_get(reasset_base_id(110 - 1));
    action110->seqID = 66;
    MusicAction* action111 = reasset_music_actions_get(reasset_base_id(111 - 1));
    action111->seqID = 66;
    MusicAction* action135 = reasset_music_actions_get(reasset_base_id(135 - 1));
    action135->seqID = 66;
    MusicAction* action136 = reasset_music_actions_get(reasset_base_id(136 - 1));
    action136->seqID = 66;
    MusicAction* action138 = reasset_music_actions_get(reasset_base_id(138 - 1));
    action138->seqID = 66;
    MusicAction* action140 = reasset_music_actions_get(reasset_base_id(140 - 1));
    action140->seqID = 66;
}

PRAGMA_IGNORE_PUSH("-Wunused")
static void df_patches_shinx(void) {
    ReAssetID df = reasset_base_id(MAP_DISCOVERY_FALLS);
    ReAssetID dfTrkblk = reasset_base_id(11);
    
    // Fix block shapes that are missing the fog render flag
    ReAssetID blockID = reasset_base_id(338 - 319);
    u32 blockDataSize;
    u8* blockData = reasset_blocks_get(dfTrkblk, blockID, &blockDataSize);
    blockData = dinomod_block_decompress(blockData, blockDataSize, &blockDataSize);
    Block* block = (Block*)(blockData + 8);
    BlockShape* shapes = (BlockShape*)((u8*)block + (u32)block->shapes);
    // Shape index 2 is also missing fog but i don't know where it is?
    shapes[26].flags |= RENDER_FOG_ACTIVE; // The shapes around the climbable bit at the start of DF
    shapes[27].flags |= RENDER_FOG_ACTIVE;
    reasset_blocks_set(dfTrkblk, blockID, REASSET_BASE_NAMESPACE, blockData, blockDataSize);
    recomp_free(blockData);
}
PRAGMA_IGNORE_POP()

PRAGMA_IGNORE_PUSH("-Wunused")
static void df_modifications(void) {
    ReAssetID discoveryFalls = reasset_base_id(MAP_DISCOVERY_FALLS);
    ReAssetID dfTrkblk = reasset_base_id(11);

    // reasset_blocks_set(dfTrkblk, reasset_base_id(338 - 319), REASSET_BASE_NAMESPACE, block338, block338_end - block338);
}
PRAGMA_IGNORE_POP()

static void darkice_mines_modifications(void) {
    ReAssetID dim1MapID = reasset_base_id(MAP_DARK_ICE_MINES_1);

    //BLOCKS edits
    {
        ReAssetID dim1Trkblk = reasset_base_id(26);
        u32 dim1BlocksBase = 711;

        //Fix the river crossing area's invisible rock climbing decal setup (it was being drawn in the wrong order and getting erased by the underlay)
        BLOCKS_REPLACE_BASE(dim1Trkblk, dim1BlocksBase, 718, block718);
        
        //Adjust the invisible terrain around the cannon silo, so Sabre doesn't stand in midair above it while it's closed
        BLOCKS_REPLACE_BASE(dim1Trkblk, dim1BlocksBase, 724, block724);

        //Fix a seam in the mountain walls, just beside the SnowHorn platform in the cannon/tents area
        BLOCKS_REPLACE_BASE(dim1Trkblk, dim1BlocksBase, 725, block725);
    }

    //Fix the cannon silo's broken HitAnimator setups
    {
        #define GAMEBIT_DIM1_CannonClaw_Retreated_into_Silo 0x157

        HitAnimator_Setup* siloFlatHitAnim = reasset_map_objects_get(dim1MapID, reasset_base_id(0x1D2D), NULL);
        siloFlatHitAnim->gamebitActivate = GAMEBIT_DIM1_CannonClaw_Retreated_into_Silo;
        siloFlatHitAnim->mode = hitanimator_configure_mode_flags(TRUE, TRUE, FALSE);

        HitAnimator_Setup* siloRaisedHitAnim = reasset_map_objects_get(dim1MapID, reasset_base_id(0x1D2E), NULL);
        siloRaisedHitAnim->gamebitActivate = GAMEBIT_DIM1_CannonClaw_Retreated_into_Silo;
        siloRaisedHitAnim->mode = hitanimator_configure_mode_flags(FALSE, TRUE, FALSE);
    }

    //Fix DIMCannonCover1's objSeq handling 
    //(there was a bug where it was starting off in its closed position when the CannonClaw has already appeared)
    {
        typedef struct {
            ObjSetup base;
            s16 unk18;
            s16 gamebitRestoreState;
            s16 preemptTime;
            s8 objSeqIdx;
            u8 yaw;
            u8 enabledActors;
            u8 scale;
            s16 unk22;
            s16 unk24;
            s16 unk26;
            u8 unk28;
            u8 unk29;
        } DLL307_Setup; //TODO: use SeqDoor_Setup

        DLL307_Setup* cannonCover = reasset_map_objects_get(dim1MapID, reasset_base_id(0x17F4), NULL);
        cannonCover->gamebitRestoreState = NO_GAMEBIT;
    }

    //Delete a DIMExplosion (causes random explosion sound when approaching tent area)
    {
        //Maybe they placed this here temporarily to help debug the mistake in DIMCannonBall's DIMExplosion-creating code?
        ReAssetID dimExplosion = reasset_base_id(0x1DA4);
        reasset_map_objects_delete(dim1MapID, dimExplosion);
    }

    //Edit DIMTent's burnt model, adding draw modes for handling opacity
    {
        ReAssetID models_dimtent_burnt_ID = reasset_base_id(886);
        reasset_models_set(models_dimtent_burnt_ID, REASSET_BASE_NAMESPACE, models_dimtent_burnt, models_dimtent_burnt_end - models_dimtent_burnt);
    }

    //Reference DIMTent's unused burnt tent model in its Objects file
    {
        ReAssetID objects_dimtent_ID = reasset_base_id(320); //OBJ_DIMTent
        reasset_objects_set(objects_dimtent_ID, REASSET_BASE_NAMESPACE, objects_dimtent, objects_dimtent_end - objects_dimtent);
    }
}

/** 
  * Adds new HUD textures into tex0/textable:
  * - Leftover kiosk icons for DIM's Gold Key (unedited) and Silver Key (adapted for N64) 
  * - Leftover kiosk redesign of the Firefly Lantern icon (adapted for N64)
  * - Leftover DP-style kiosk portrait of Fox (adapted for N64)
  * - Custom icons for the Energy Egg (based on Nick Southam's PointBack minigame egg icons)
  *
  * (TODO: include these edits directly in tex0.xdelta, textable.xdelta)
  */
static void cmdmenu_icons_patch(void) {
    //TEX0 (TODO: append to the folder's end)
    ReAssetID tex0_kiosk_fox_ID = reasset_auto_id(dinomodNs);
    reasset_textures_set(TEX_BANK_0, tex0_kiosk_fox_ID, 1, tex0_kiosk_fox, tex0_kiosk_fox_end - tex0_kiosk_fox);

    ReAssetID tex0_kiosk_gold_key_ID    = reasset_auto_id(dinomodNs);
    ReAssetID tex0_kiosk_silver_key_ID  = reasset_auto_id(dinomodNs);
    ReAssetID tex0_kiosk_firefly_ID     = reasset_auto_id(dinomodNs);
    // ReAssetID tex0_kiosk_replay_disk_ID = reasset_auto_id(dinomodNs);
    reasset_textures_set(TEX_BANK_0, tex0_kiosk_gold_key_ID,    1, tex0_kiosk_gold_key,     tex0_kiosk_gold_key_end - tex0_kiosk_gold_key);
    reasset_textures_set(TEX_BANK_0, tex0_kiosk_silver_key_ID,  1, tex0_kiosk_silver_key,   tex0_kiosk_silver_key_end - tex0_kiosk_silver_key);
    reasset_textures_set(TEX_BANK_0, tex0_kiosk_firefly_ID,     1, tex0_kiosk_firefly,      tex0_kiosk_firefly_end - tex0_kiosk_firefly);
    // reasset_textures_set(TEX_BANK_0, tex0_kiosk_replay_disk_ID, 1, tex0_kiosk_replay_disk,  tex0_kiosk_replay_disk_end - tex0_kiosk_replay_disk);
    
    ReAssetID tex0_energy_egg_ID = reasset_auto_id(dinomodNs);
    ReAssetID tex0_energy_egg_moldy_ID = reasset_auto_id(dinomodNs);
    reasset_textures_set(TEX_BANK_0, tex0_energy_egg_ID,       1, tex0_custom_energy_egg,       tex0_custom_energy_egg_end - tex0_custom_energy_egg);
    reasset_textures_set(TEX_BANK_0, tex0_energy_egg_moldy_ID, 1, tex0_custom_energy_egg_moldy, tex0_custom_energy_egg_moldy_end - tex0_custom_energy_egg_moldy);

    //TEXTABLE (reference new tex0 icons)
    reasset_texture_table_set(reasset_base_id(TEXTABLE_25C_Kiosk_Gold_Key_Icon),    TEX_BANK_0, tex0_kiosk_gold_key_ID);
    reasset_texture_table_set(reasset_base_id(TEXTABLE_25D_Kiosk_Silver_Key_Icon),  TEX_BANK_0, tex0_kiosk_silver_key_ID);
    reasset_texture_table_set(reasset_base_id(TEXTABLE_25E_Kiosk_Firefly_Icon),     TEX_BANK_0, tex0_kiosk_firefly_ID);
    // reasset_texture_table_set(reasset_base_id(TEXTABLE_25F_Kiosk_Replay_Disk_Icon), TEX_BANK_0, tex0_kiosk_replay_disk_ID);
    reasset_texture_table_set(reasset_base_id(TEXTABLE_260_Energy_Egg_Icon),        TEX_BANK_0, tex0_energy_egg_ID);
    reasset_texture_table_set(reasset_base_id(TEXTABLE_261_Energy_Egg_Moldy_Icon),  TEX_BANK_0, tex0_energy_egg_moldy_ID);

    reasset_texture_table_set(reasset_base_id(TEXTABLE_266_Kiosk_Fox_Icon),         TEX_BANK_0, tex0_kiosk_fox_ID);
}

/* Adds a reconstructed Purple Mushroom model, and appends it to the `SHrocketmushroom` Object's model list. */
static void purple_mushroom_patch(void) {
    //Add new model for Purple Mushroom (recreated by using the leftover Purple Mushroom textures, and copying the vertex colours from "SHmushroombit"'s model pieces)
    {
        ReAssetID models_purple_mushroom_ID = reasset_base_id(0x24B); //TODO: append to Models instead of replacing this
        reasset_models_set(models_purple_mushroom_ID, REASSET_BASE_NAMESPACE, models_purple_mushroom, models_purple_mushroom_end - models_purple_mushroom);
        reasset_models_set_modanims(models_purple_mushroom_ID, modanim_purple_mushroom, modanim_purple_mushroom_end - modanim_purple_mushroom);
        reasset_models_set_amap(models_purple_mushroom_ID, amap_purple_mushroom, amap_purple_mushroom_end - amap_purple_mushroom);
    }

    //Reference the Purple Mushroom model in the `SHrocketmushroom` Object, so it can optionally be shown
    {
        ReAssetID objects_purple_mushroom_ID = reasset_base_id(571); //OBJ_SHrocketmushroom
        reasset_objects_set(objects_purple_mushroom_ID, REASSET_BASE_NAMESPACE, objects_purple_mushroom, objects_purple_mushroom_end - objects_purple_mushroom);
    }
}

//Revert changes to VampireBat object, allowing lock-on
static void vampire_bat_patch(void) {
    ReAssetID objects_vampirebat_id = reasset_base_id(53); //OBJ_VampireBat
    reasset_objects_set(objects_vampirebat_id, REASSET_BASE_NAMESPACE, objects_vampirebat, objects_vampirebat_end - objects_vampirebat);
}

//Add extra objSeqs to WarpPoint
static void warp_point_patch(void) {
    ReAssetID objects_warppoint_id = reasset_base_id(1124); //OBJ_WarpPoint
    reasset_objects_set(objects_warppoint_id, REASSET_BASE_NAMESPACE, objects_warppoint, objects_warppoint_end - objects_warppoint);
}

static void diamond_bay_additions(void) {
    ReAssetID db = reasset_base_id(MAP_DIAMOND_BAY);

    // Add fall reset EffectBox to the start of the DB river to prevent players from accessing
    // DB and VFP earlier than intended. Disables itself after the SH river is unblocked.
    {
        EffectBox_Setup riverFallResetBox = {
            .base = {
                .objId = OBJ_EffectBox,
                .loadFlags = OBJSETUP_LOAD_LEVEL,
                .fadeFlags = OBJSETUP_FADE_MANUAL,
                .loadDistance = 0xFF,
                .fadeDistance = 0xFF,
                .x = 1141.85f,
                .y = -956.03f,
                .z = -1551.03f
            },
            .unk18 = (19832 / 256), // yaw
            .unk19 = 0, // pitch
            .unk1A = 200, // x radius
            .unk1B = 30, // y radius
            .unk1C = 100, // z radius
            .effect = 0, // fall reset
            .gamebitDisableValue = 1,
            .gamebit = RIVER_BIT, // disable when the SwapStone Hollow river is unblocked
            .target = 0 // player
        };

        reasset_map_objects_set(db, reasset_auto_id(dinomodNs), &riverFallResetBox, sizeof(riverFallResetBox));
    }

    // Add dangerous water trigger at the start of the DB river to prevent players from going
    // this way without a log. The fall reset trigger only applies when the river is dry, so
    // we need something to stop players from fixing the river but not getting the DIM SpellStone.
    {
        Trigger_Setup drownArea = {
            .base = {
                .objId = OBJ_TriggerArea,
                .loadFlags = OBJSETUP_LOAD_LEVEL,
                .fadeFlags = OBJSETUP_FADE_MANUAL,
                .loadDistance = 0xFF,
                .fadeDistance = 0xFF,
                .x = 957.0f,
                .y = -1095.0f,
                .z = -1461.0f
            },
            .commands = {
                {
                    .condition = CMD_COND_IN | CMD_COND_RE_ENTER,
                    .id = TRG_CMD_HAZARD,
                    .param1 = 9, // dangerous water
                    .param2 = 0
                },
                {
                    .condition = CMD_COND_OUT | CMD_COND_RE_EXIT,
                    .id = TRG_CMD_HAZARD,
                    .param1 = 10, // safe water
                    .param2 = 0
                }
            },
            .sizeX = 255,
            .sizeY = 16,
            .sizeZ = 170,
            .rotationY = 0,
            .rotationX = 0
        };

        reasset_map_objects_set(db, reasset_auto_id(dinomodNs), &drownArea, sizeof(drownArea));
    }

    // Add another drown trigger after the second drop in the river (the deep spot where the vines can knock you down into)
    {
        Trigger_Setup drownArea = {
            .base = {
                .objId = OBJ_TriggerArea,
                .loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP,
                .fadeFlags = OBJSETUP_FADE_MANUAL,
                .mapObjGroup = 0,
                .fadeDistance = 0xFF,
                .x = 354.0f,
                .y = -1440.0f,
                .z = -813.0f
            },
            .commands = {
                {
                    .condition = CMD_COND_IN | CMD_COND_RE_ENTER,
                    .id = TRG_CMD_HAZARD,
                    .param1 = 9, // dangerous water
                    .param2 = 0
                },
                {
                    .condition = CMD_COND_OUT | CMD_COND_RE_EXIT,
                    .id = TRG_CMD_HAZARD,
                    .param1 = 10, // safe water
                    .param2 = 0
                }
            },
            .sizeX = 255,
            .sizeY = 16,
            .sizeZ = 255,
            .rotationY = (12680 >> 8),
            .rotationX = 0
        };

        reasset_map_objects_set(db, reasset_auto_id(dinomodNs), &drownArea, sizeof(drownArea));
    }

    //Add HitAnimator for the top of SwapStone Hollow's waterfall
    //(There's one in SH's map too, but having a copy in Diamond Bay's objects seems to 
    // help this behave reliably, with this spot being on the boundary between two maps)
    {
        HitAnimator_Config hitAnimatorData[] = {
            {VEC3F(1197.214,  -813,  -1546.568),  RIVER_BIT, 1, TRUE, FALSE, FALSE}, //block989 (Diamond Bay waterfall basin 1) (ledge-grab HITS line)
        };

        HitAnimator_Config* data = &hitAnimatorData[0];
        HitAnimator_Setup hitA = {
            .base = {
                .objId = OBJ_HitAnimator,
                .actExclusions1 = 0, //NOTE: HITS line needs controlling regardless of current Act
                .loadFlags = OBJSETUP_LOAD_LEVEL,
                .fadeFlags = OBJSETUP_FADE_CAMERA,
                .loadDistance = 140,
                .fadeDistance = 140,
                .x = data->coords.x,
                .y = data->coords.y,
                .z = data->coords.z
            },
            .gamebitActivate = data->gamebit,
            .mode = hitanimator_configure_mode_flags(
                data->removeWhenSet, data->isBlocksAnimator, data->blocksFade),
            .hitsAnimatorID =   ((data->isBlocksAnimator == FALSE) ? data->animatorID : 0),
            .blocksAnimatorID = ((data->isBlocksAnimator == TRUE) ? data->animatorID : 0)
        };

        reasset_map_objects_set(db, reasset_auto_id(dinomodNs), &hitA, sizeof(hitA));
    }

}

static void diamond_bay_modifications(void) {
    ReAssetID db = reasset_base_id(MAP_DIAMOND_BAY);
    ReAssetID dbTrkblk = reasset_base_id(48);
    int dbTrkblkBase = 974;

    //Diamond Bay river: BLOCKS edits
    {
        //Adjust animatorIDs for toggling river, plus seam fixes at the SH connection
        BLOCKS_REPLACE_BASE(dbTrkblk, dbTrkblkBase, 989, block989); //Waterfall basin 1 (dropping down from SwapStone Hollow)
        BLOCKS_REPLACE_BASE(dbTrkblk, dbTrkblkBase, 995, block995); //River bend 1 (near SwapStone Hollow)
        BLOCKS_REPLACE_BASE(dbTrkblk, dbTrkblkBase, 994, block994); //Waterfall basin 2 (second drop along river, approaching where rocks fall)
    }

    // Delete the dockpoints at the start of the DB river
    {
        ReAssetID riverDockpoint1 = reasset_base_id(0x41F04);
        ReAssetID riverDockpoint2 = reasset_base_id(0x30006);

        reasset_map_objects_delete(db, riverDockpoint1);
        reasset_map_objects_delete(db, riverDockpoint2);
    }

    // Increase the size of the trigger plane at the start of the DB river that
    // sets tricky's goal point and sets up some envfx. In vanilla, it's very easy to miss it.
    {
        Trigger_Setup* plane = reasset_map_objects_get(db, reasset_base_id(0x41EF0), NULL);
        plane->sizeX = 34; // scale 0.75 -> 2.125
        plane->base.loadDistance = 48; // increase load dist to compensate
    }

    // Add a gamebit to the SideLoad at the start of the DB river
    // (stops Tricky from warping down there when river's missing)
    {
        SideLoad_Setup* sideLoad = reasset_map_objects_get(db, reasset_base_id(0x42B6F), NULL);
        sideLoad->gamebitUnlocked = RIVER_BIT;
    }

    // Add a gamebit to the WaterFallSpray at the start of the DB river
    {
        WaterFallSpray_Setup* spray = reasset_map_objects_get(db, reasset_base_id(0x4205B), NULL);
        spray->gamebit = RIVER_BIT;
        spray->invertGamebit = TRUE;
    }
}

static void discovery_falls_hit_edits(void) {
    ReAssetID df = reasset_base_id(MAP_DISCOVERY_FALLS);
    ReAssetID dfTrkblk = reasset_base_id(11);
    const int dfTrkblkBase = 319;

    TrackLine* hit;

    // Rough edits to make it possible to go down to the waterfall leading to the shrine. These hits
    // in vanilla are for an older DF layout so this patch adjusts them so they don't block the path.
    // TODO: replace with a more polished patch
    {
        ReAssetID waterfallRiverBlock = reasset_base_id(324 - dfTrkblkBase);
        hit = reasset_hits_get(dfTrkblk, waterfallRiverBlock, reasset_base_id(2));
        hit->Bx = 237;
        hit->Bz = 264;
        hit = reasset_hits_get(dfTrkblk, waterfallRiverBlock, reasset_base_id(3));
        hit->Ax = 237;
        hit->Az = 264;

        hit = reasset_hits_get(dfTrkblk, waterfallRiverBlock, reasset_base_id(5));
        hit->Ax = 359;
        hit->Az = 186;
        hit->Bx = 372;
        hit->Bz = 115;

        hit = reasset_hits_get(dfTrkblk, waterfallRiverBlock, reasset_base_id(6));
        hit->Ax = 372;
        hit->Az = 115;

        reasset_hits_delete(dfTrkblk, waterfallRiverBlock, reasset_base_id(8));
        reasset_hits_delete(dfTrkblk, waterfallRiverBlock, reasset_base_id(9));
        reasset_hits_delete(dfTrkblk, waterfallRiverBlock, reasset_base_id(10));

        hit = reasset_hits_get(dfTrkblk, waterfallRiverBlock, reasset_base_id(11));
        hit->Ax = 220;
        hit->Az = 95;
        hit->Bx = 180;
        hit->Bz = 160;

        hit = reasset_hits_get(dfTrkblk, waterfallRiverBlock, reasset_base_id(12));
        hit->Ax = 180;
        hit->Az = 160;
    }
}

static void custom_objects(void) {
    // SHbarrel
    {
        ReAssetID shBarrelID = reasset_auto_id(dinomodNs);
        reasset_objects_set(shBarrelID, dinomodNs, objects_shbarrel, objects_shbarrel_end - objects_shbarrel);
        reasset_object_indices_set(shBarrelIndexID, shBarrelID);
    }

    // SHbarrelcreator
    {
        ReAssetID shBarrelcreatorID = reasset_auto_id(dinomodNs);
        reasset_objects_set(shBarrelcreatorID, dinomodNs, objects_shbarrelcreator, objects_shbarrelcreator_end - objects_shbarrelcreator);
        reasset_object_indices_set(shBarrelcreatorIndexID, shBarrelcreatorID);
    }
}

static void custom_dlls(void) {
    // SHbarrel
    reasset_dlls_set(reasset_id(dinomodNs, 0x824C), DLL_BANK_OBJECTS, 
        /*exportCount*/ 7, (void*)SHbarrel_ctor, (void*)SHbarrel_dtor, &DLL_SHbarrel_vtbl);

    // SHbarrelcreator
    reasset_dlls_set(reasset_id(dinomodNs, 0x824D), DLL_BANK_OBJECTS, 
        /*exportCount*/ 7, (void*)SHbarrelcreator_ctor, (void*)SHbarrelcreator_dtor, &DLL_SHbarrelcreator_vtbl);
}

/** Give WCTrex hit spheres similar to KT_Rex so they can actually do damage. */
static void add_wctrex_hit_spheres(void) {
    ReAssetID id = reasset_base_id(100);

    u32 size;
    void* data = reasset_models_get(id, &size);
    data = dinomod_model_decompress(data, size, &size);
    u32 oldSize = size;
    {
        u32 newSize = size + (sizeof(HitSphere) * 3);
        void* newdata = recomp_alloc(newSize);
        bcopy(data, newdata, oldSize);
        bzero((u8*)newdata + oldSize, newSize - oldSize);
        recomp_free(data);
        data = newdata;
        size = newSize;
    }

    Model* model = (Model*)((u8*)data + 0xC);
    model->hitSpheres = (HitSphere*)(oldSize - 0xC);
    model->hitSphereCount = 3;

    HitSphere* sphere;

    // Note: These could probably be placed/size better
    // Mouth
    sphere = &((HitSphere*)((u8*)model + (u32)model->hitSpheres))[0];
    sphere->jointIndex = 26;
    sphere->unk2 = 160; // radius
    sphere->x = 0;
    sphere->y = 0;
    sphere->z = 0;
    sphere->unkA = 0;
    sphere->unkC = 0; // index
    sphere->unkD = 0; // group

    // Left foot
    sphere = &((HitSphere*)((u8*)model + (u32)model->hitSpheres))[1];
    sphere->jointIndex = 15;
    sphere->unk2 = 130; // radius
    sphere->x = 0;
    sphere->y = 15;
    sphere->z = 0;
    sphere->unkA = 0;
    sphere->unkC = 1; // index
    sphere->unkD = 1; // group

    // Right foot
    sphere = &((HitSphere*)((u8*)model + (u32)model->hitSpheres))[2];
    sphere->jointIndex = 9;
    sphere->unk2 = 130; // radius
    sphere->x = 0;
    sphere->y = 15;
    sphere->z = 0;
    sphere->unkA = 0;
    sphere->unkC = 2; // index
    sphere->unkD = 2; // group

    reasset_models_set(id, REASSET_BASE_NAMESPACE, data, size);
    recomp_free(data);
}

static void vfp_modifications(void) {
    ReAssetID vfp = reasset_base_id(MAP_VOLCANO_FORCE_POINT_TEMPLE);
    
    // Shrink vision range of VFP ScorpionRobots as they see waaaaaaaaaay too far and end up shooting through walls.
    // This edit is pretty subjective.
    {
        Baddie_Setup* scorpRobo;

        scorpRobo = reasset_map_objects_get(vfp, reasset_base_id(0x41852), NULL);
        scorpRobo->unk29 = 320 / 8;
        scorpRobo = reasset_map_objects_get(vfp, reasset_base_id(0x41851), NULL);
        scorpRobo->unk29 = 320 / 8;
    }
}

static void nw_modifications(void) {
    ReAssetID nw = reasset_base_id(MAP_SNOWHORN_WASTES);
    
    // Fix NWMultiSeq so Garunda Te cutscene won't loop if gamebit 0 is set
    {
        NWMultiSeq_Setup* multiseq = reasset_map_objects_get(nw, reasset_base_id(0x32D52), NULL);
        // Set bit/seq indices to -1 (instead of 0) so the cutscene won't replay
        // the first seq is bit 0 is set (there's only 6 seqs for this cutscene).
        multiseq->unk28[6] = -1;
        multiseq->unk28[7] = -1;
        multiseq->unk40[6] = -1;
        multiseq->unk40[7] = -1;
    }

    // Change the bit the blue SnowHorn hit animator looks for (this is a custom dinomod map obj). We're giving it
    // a custom bit since the rom patches are accidentically using a CC vis anim bit.
    {
        HitAnimator_Setup* blueSnowHornHitAnim = reasset_map_objects_get(nw, reasset_base_id(0x412E1), NULL);
        blueSnowHornHitAnim->gamebitActivate = DINOMOD_BIT_92D_Blue_SnowHorn_HitAnimator;
    }
}

static void gpsh_modifications(void) {
    ReAssetID gpsh = reasset_base_id(MAP_SHRINE_GOLDEN_PLAINS);

    // Set warp ID for completing the Test of Knowledge (and generally just fix the WarpPoint)
    {
        WarpPoint_Setup* completionWarp = reasset_map_objects_get(gpsh, reasset_base_id(0x2C8D), NULL);
        completionWarp->base.objId = OBJ_WarpPoint; // WM_WarpPoint -> WarpPoint (same DLL, but the other shrines use WarpPoint)
        completionWarp->warpID = 31; // go to GP -> GPSH transporter
        completionWarp->isInboundWarp = FALSE; // this is an exit warp (why is this 1 normally?? you never warp to here?)
        completionWarp->mode = 2; // warp only when bit is set (the bit is correctly 0xFD but the condition field is wrong)
    }

    // Edit trigger planes so the player cannot go back into the entrance hall while the test is active.
    // Bit 0x129 will be 0 while the test is active and 1 otherwise (except after completion where it remains 0
    // but the warp will take the player out of the shrine anyway so it's OK).
    {
        Trigger_Setup* plane;

        plane = reasset_map_objects_get(gpsh, reasset_base_id(0x32974), NULL);
        plane->conditionBitFlagIDs[0] = 0x129;

        plane = reasset_map_objects_get(gpsh, reasset_base_id(0x32975), NULL);
        plane->conditionBitFlagIDs[0] = 0x129;
    }

    // Fix flybaddie creators fade distance so the flybaddies (which inherit the distance) are correctly visible.
    // The third creator is actually correct, so just fix the other two.
    {
        ObjSetup* creator;

        creator = reasset_map_objects_get(gpsh, reasset_base_id(0x32951), NULL);
        creator->loadDistance = 254;
        creator->fadeDistance = 254;

        creator = reasset_map_objects_get(gpsh, reasset_base_id(0x32952), NULL);
        creator->loadDistance = 254;
        creator->fadeDistance = 254;
    }
}

static void mmsh_modifications(void) {
    ReAssetID mmsh = reasset_base_id(MAP_SHRINE_MOON_MOUNTAIN_PASS);

    // Fix transporter so the player can leave
    {
        Transporter_Setup* transporter = reasset_map_objects_get(mmsh, reasset_base_id(0xC5D), NULL);
        transporter->gamebit = NO_GAMEBIT; // 0 -> -1 (so it's always enabled)
        transporter->warpID = 73; // 77 -> 73 (MMP shrine transporter)
    }
}

static void ecsh_modifications(void) {
    ReAssetID ecsh = reasset_base_id(MAP_SHRINE_WALLED_CITY);

    // Fix transporter so the player can leave
    {
        Transporter_Setup* transporter = reasset_map_objects_get(ecsh, reasset_base_id(0xC5D), NULL);
        transporter->gamebit = NO_GAMEBIT; // 0 -> -1 (so it's always enabled)
    }
}

static void ccsh_modifications(void) {
    ReAssetID ccsh = reasset_base_id(MAP_SHRINE_CAPE_CLAW);

    // Edit trigger planes so the player cannot go back into the entrance hall while the test is active.
    // Bit 0x129 will be 0 while the test is active and 1 otherwise (except after completion where it remains 0
    // but the warp will take the player out of the shrine anyway so it's OK).
    {
        Trigger_Setup* plane;

        plane = reasset_map_objects_get(ccsh, reasset_base_id(0x32F01), NULL);
        plane->conditionBitFlagIDs[0] = 0x129;

        plane = reasset_map_objects_get(ccsh, reasset_base_id(0x32F02), NULL);
        plane->conditionBitFlagIDs[0] = 0x129;
    }

    // Fix transporter so the player can leave
    {
        Transporter_Setup* transporter = reasset_map_objects_get(ccsh, reasset_base_id(0xC5D), NULL);
        transporter->gamebit = NO_GAMEBIT; // 0 -> -1 (so it's always enabled)
    }
}

static void wgsh_modifications(void) {
    ReAssetID wgsh = reasset_base_id(MAP_SHRINE_WILLOW_GROVE);

    // Edit trigger planes so the player cannot go back into the entrance hall while the test is active.
    // Bit 0x129 will be 0 while the test is active and 1 otherwise (except after completion where it remains 0
    // but the warp will take the player out of the shrine anyway so it's OK).
    {
        Trigger_Setup* plane;

        plane = reasset_map_objects_get(wgsh, reasset_base_id(0x32893), NULL);
        plane->conditionBitFlagIDs[0] = 0x129;

        plane = reasset_map_objects_get(wgsh, reasset_base_id(0x32894), NULL);
        plane->conditionBitFlagIDs[0] = 0x129;
    }

    // Fix transporter so the player can leave
    {
        Transporter_Setup* transporter = reasset_map_objects_get(wgsh, reasset_base_id(0xC5D), NULL);
        transporter->gamebit = NO_GAMEBIT; // 0 -> -1 (so it's always enabled)
    }
}

static void nwsh_modifications(void) {
    ReAssetID nwsh = reasset_base_id(MAP_SHRINE_SNOWHORN_WASTES);

    // Fix transporter so the player can leave
    {
        Transporter_Setup* transporter = reasset_map_objects_get(nwsh, reasset_base_id(0xC5D), NULL);
        transporter->gamebit = 0x129; // 0 -> 0x129 (so it's enabled while the test isn't active)
    }
}

REASSET_ON_SET_LOW_PRIORITY void dinomod_reasset_on_set(void) {
    custom_objects();
    custom_dlls();

    walled_city_additions();
    warlock_mountain_platform_additions();
    swapstone_hollow_additions();
    swapstone_hollow_well_additions();
    //golden_plains_fuel_additions();
    diamond_bay_additions();
}

/** Fixes problems in the seqJoint jointID mapping for each of Sabre's models */
static void sabre_seqjoints_patch(void) {
    ReAssetID objectIndex;
    ObjDef* objDef;
    u8* seqJointDef;

    //Get OBJECTS.bin entry
    {
        reasset_object_indices_get(
            reasset_base_id(0), 
            &objectIndex
        );

        objDef = reasset_objects_get(objectIndex, NULL);
        if (!objDef) {
            return;
        }
    }

    if ((u32)objDef->pSequenceBones == 0) {
        return;
    }

    seqJointDef = (u8*)((u32)objDef + (u32)objDef->pSequenceBones);

    //Iterate over the seqJointDefs, and tweak relevant seqJointIDs' model jointIDs
    for (u32 i = 0, seqJointID; i < objDef->numSequenceBones; i++) {
        seqJointID = seqJointDef[0];
        seqJointDef++; 

        switch (seqJointID) {
        case 1: //Jaw seqJoint
            seqJointDef[0] = 0x13; //gameplay model
            break;
        case 4: //Ear R seqJoint
            seqJointDef[0] = 0x12; //gameplay model
            seqJointDef[1] = 0x12; //cutscene model
            break;
        case 5: //Ear L seqJoint
            seqJointDef[0] = 0x11; //gameplay model
            seqJointDef[1] = 0x13; //cutscene model
            break;
        }

        seqJointDef += objDef->numModels;
    }
}

REASSET_ON_MODIFY_LOW_PRIORITY void dinomod_reasset_on_modify(void) {
    music_actions_patch();
    sabre_seqjoints_patch();
    collectables_animobj_patch();
    cmdmenu_icons_patch();
    purple_mushroom_patch();
    vampire_bat_patch();
    warp_point_patch();

    shrine_fxemit_modifications();
    warlock_mountain_platform_modifications();
    warlock_mountain_modifications();
    swapstone_hollow_modifications();
    swapstone_hollow_well_modifications();
    cc_lightfoot_patch();
    cape_claw_modifications();
    darkice_mines_modifications();
    golden_plains_modifications();
    walled_city_modifications();
    dragon_rock_upper_modifications();
    dragon_rock_bottom_modifications();
    golden_plains_modifications();
    // golden_plains_fuel_modifications();
    music_actions_patch();
    // df_patches_shinx();
    // df_modifications();
    diamond_bay_modifications();
    discovery_falls_hit_edits();
    add_wctrex_hit_spheres();
    vfp_modifications();
    nw_modifications();
    gpsh_modifications();
    mmsh_modifications();
    ecsh_modifications();
    ccsh_modifications();
    wgsh_modifications();
    nwsh_modifications();
}

REASSET_ON_RESOLVE void dinomod_reasset_on_resolve(void) {
    ReAssetResolveMap objIndexResolveMap = reasset_object_indices_get_resolve_map();
    OBJ_SHbarrel = reasset_resolve_map_lookup(objIndexResolveMap, shBarrelIndexID);
    OBJ_SHbarrelcreator = reasset_resolve_map_lookup(objIndexResolveMap, shBarrelcreatorIndexID);
}
