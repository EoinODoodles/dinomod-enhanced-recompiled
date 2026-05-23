#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "reasset.h"

#include "compression_util.h"
#include "common_objsetups.h"
#include "configs.h"
#include "custom_textable_ids.h"
#include "custom_gamebits.h"
#include "math_util.h"
#include "mod_common.h"
#include "object_util.h"
#include "objects/511_SHboulder.h"

#include "PR/ultratypes.h"
#include "dlls/objects/common/collectable.h"
#include "dlls/objects/325_trigger.h"
#include "dlls/objects/418_DFriverflow.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "game/gamebits.h"
#include "sys/fs.h"
#include "sys/map.h"
#include "sys/map_enums.h"
#include "sys/math.h"
#include "sys/memory.h"
#include "macros.h"

INCBIN(block628, "0628 0274_moon_temple_viewing_tile.bin");
INCBIN(block351, "blocks_0351_SHriver_rocky_waterfall.bin");
INCBIN(block579, "blocks_0579_SHwell_lily_pond_climb.bin");
INCBIN(block580, "blocks_0580_SHwell_lily_pond_vines.bin");
INCBIN(block581, "blocks_0581_SHwell_entrance.bin");
INCBIN(block582, "blocks_0582_SHwell_stalactite_tunnels.bin");
INCBIN(block583, "blocks_0583_SHwell_river_end.bin");
INCBIN(block989, "blocks_0989_DBriver_waterfall_basin_1.bin");
INCBIN(hits989, "hits_0989_DBriver_waterfall_basin_1.bin");
INCBIN(block995, "blocks_0995_DBriver_bend_1.bin");
INCBIN(block994, "blocks_0994_DBriver_waterfall_basin_2.bin");
// INCBIN(block338, "0338 0152.bin");

INCBIN(tex0_kiosk_gold_key,             "tex0_kiosk_gold_key.bin");
INCBIN(tex0_kiosk_silver_key,           "tex0_kiosk_silver_key_custom.bin");
INCBIN(tex0_kiosk_firefly,              "tex0_kiosk_firefly_custom.bin");
// INCBIN(tex0_kiosk_replay_disk,          "tex0_kiosk_replay_disk_custom.bin");
INCBIN(tex0_custom_energy_egg,          "tex0_energy_egg_custom.bin");
INCBIN(tex0_custom_energy_egg_moldy,    "tex0_energy_egg_moldy_custom.bin");
INCBIN(tex0_kiosk_fox,                  "tex0_kiosk_fox_icon_custom.bin");

INCBIN(models_dimtent_burnt,    "models_0886_DIMtent_burnt_opacity.bin");
INCBIN(objects_dimtent,         "objects_0320 0140 DIMTent.bin");

INCBIN(models_purple_mushroom,  "models_purple_mushroom_recreation.bin");
INCBIN(modanim_purple_mushroom, "modanim_purple_mushroom_recreation.bin");
INCBIN(amap_purple_mushroom,    "amap_purple_mushroom_recreation.bin");
INCBIN(objects_purple_mushroom, "objects_0571 023B SHrocketmushroo.bin");

INCBIN(objects_shseqobject,     "objects_0561_SHseqobject.bin");
INCBIN(objects_shboulder,       "objects_0583 0247 SHboulder.bin");

#define BLOCKS_REPLACE_BASE(trkblk, trkblkBaseID, blockID, file) (reasset_blocks_set(trkblk, reasset_base_id(blockID - trkblkBaseID), REASSET_BASE_NAMESPACE, file, file##_end  - file))

#define COORDS_SETUP(coordX, coordY, coordZ) .base.x = coordX, .base.y = coordY, .base.z = coordZ

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

static ReAssetNamespace dinomodNs;

REASSET_ON_INIT void dinomod_reasset_on_init(void) {
    dinomodNs = reasset_namespace("dinomod");
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
}

static void walled_city_modifications(void) {
    ReAssetID walledCity = reasset_base_id(MAP_WALLED_CITY);
    ReAssetID wcTrkblk = reasset_base_id(20);

    // Fix terrain ID of moon temple viewing tile (to let the aperture work correctly)
    reasset_blocks_set(wcTrkblk, reasset_base_id(628 - 585), REASSET_BASE_NAMESPACE, block628, block628_end - block628);

    // Revert dinomod's removal of the moon temple lift sequences, so it can be used again
    {
        ObjDef *moonTempleLiftDef = reasset_objects_get(reasset_base_id(276), NULL);
        s16 *seq = (s16*)((u8*)moonTempleLiftDef + (u32)moonTempleLiftDef->pSeq);
        seq[0] = 0x3D4;
        seq[1] = 0x3D6;
    }

    // Moon door seqobj
    // Revert dinomod's gamebit change, so the door only opens after the aperture sequence
    {
        SeqObj_Setup *seqobj = (SeqObj_Setup*)reasset_map_objects_get(walledCity, 
            reasset_base_id(0x41474), NULL);
        seqobj->gamebitPlay = 0x829;
    }

    // Moon Aperture
    // Set to always enabled (the sun aperture is always enabled as well)
    {
        WCApertureSymbol_Setup *moonAperture = (WCApertureSymbol_Setup*)reasset_map_objects_get(walledCity, 
            reasset_base_id(0x41463), NULL);
        moonAperture->unk20 = BIT_ALWAYS_1;
    }

    // Delete DummyObjects in Moon/Sun temple basements (these are objects with an unmapped OBJINDEX.bin entry)
    reasset_map_objects_delete(walledCity, reasset_base_id(0x41AFC));
    reasset_map_objects_delete(walledCity, reasset_base_id(0x41B35));
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
            ObjSetup *setup = reasset_map_objects_get(mapID, id, NULL);
            if (setup->objId == OBJ_FXEmit) {
                FXEmit_Setup *fxemit = (FXEmit_Setup*)setup;
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
    HitsLine *line;

    //Krystal's side
    line = reasset_hits_get(wlTrkblk, reasset_base_id(459 - 438), reasset_base_id(0));
    line->animatorID = 8;

    //Sabre's side
    line = reasset_hits_get(wlTrkblk, reasset_base_id(447 - 438), reasset_base_id(0));
    line->animatorID = 8;
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

/** Change the fade settings of the GP_ShrinePillar so they're visible across Golden Plains 
  * (they're fairly low-poly, should be grand even on N64!) */
static void golden_plains_modifications(void) {
    ReAssetID mapID = reasset_base_id(MAP_GOLDEN_PLAINS);

    ReAssetIterator iterator = reasset_map_objects_create_iterator(mapID);
    ReAssetID id;
    while (reasset_iterator_next(iterator, &id)) {
        ObjSetup *setup = reasset_map_objects_get(mapID, id, NULL);
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
    ObjDef *objDef;
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
        Collectable_Setup *gold = (Collectable_Setup*)reasset_map_objects_get(capeClaw, 
                reasset_base_id(0x42DFD), NULL);
        if (gold->base.objId == OBJ_CCgoldnuggetPic) {
            gold->gamebitCount = BIT_627;
        }
    }

    // Stop creating spray particles after Kyte pulls lever (gamebit moved to ObjSetup to make patch more reusable)
    {
        u32 waterFallSprays[] = {0x42E2C, 0x42E2D, 0x42E2E};

        for (u8 i = 0, end = ARRAYCOUNT(waterFallSprays); i < end; i ++) {
            WaterFallSpray_Setup *spray = (WaterFallSpray_Setup*)reasset_map_objects_get(capeClaw, 
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
            .gamebitPlayed = NO_GAMEBIT,
            .yaw = 0,
            .playbackOptions = 8,
            .seqIndex = 9,
            .unk22 = 1,
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
                    TRIG_PARAMS_COMBINED( TRIG_BITS_MODE(0) | BIT_SP_Exiting_Shop ) //Unset gamebit
                },
            },
            .timerDuration = 60
        };

        reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &tTime, sizeof(tTime));
    }

    //Add SHBoulders blocking the waterfall near Rocky 
    //(One for each side of the opening, to give the illusion that you're seeing the back of the boulder)
    {
        typedef struct {
            Vec3f coords;
            u16 scale;
            s16 yaw;
            s16 pitch;
            s16 roll;
        } SHBoulders;

        SHBoulders boulderData[2] = {
            {VEC3F(2410.9, -654.8, 956.3), 146, DEGREES_TO_ANGLE8(34.3f), DEGREES_TO_ANGLE8(0.9f), 0},
            {VEC3F(2383.0, -642.7, 922.0), 141, DEGREES_TO_ANGLE8(21.0f), 0, 0},
        };
        u8 count = ARRAYCOUNT(boulderData);

        //Insert the new objects
        for (s32 i = 0; i < count; i++) {
            SHboulder_Setup boulder = {
                .base = {
                    .objId = OBJ_SHboulder,
                    .actExclusions1 = ~MAP_ACT(1),
                    .loadFlags = OBJSETUP_LOAD_MAIN,
                    .fadeFlags = OBJSETUP_FADE_CAMERA,
                    .loadDistance = 140,
                    .fadeDistance = 140,
                    .x = boulderData[i].coords.x,
                    .y = boulderData[i].coords.y,
                    .z = boulderData[i].coords.z
                },
                .scale = boulderData[i].scale,
                .yaw = boulderData[i].yaw,
                .pitch = boulderData[i].pitch,
                .roll = boulderData[i].roll,
                .gamebitGone = BOULDER_BIT
            };
            reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &boulder, sizeof(boulder));
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
            HitsLine line = {
                .Ax = points[i].x,
                .Ay = points[i].y,
                .Az = points[i].z,

                .Bx = points[(i + 1) % count].x,
                .By = points[(i + 1) % count].y,
                .Bz = points[(i + 1) % count].z,

                .heightA = 60,
                .heightB = 60,

                .settingsA = 0,
                .settingsB = 1,

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
                    .loadFlags = OBJSETUP_LOAD_LEVEL,
                    .fadeFlags = OBJSETUP_FADE_CAMERA,
                    .loadDistance = 140,
                    .fadeDistance = 140,
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

    //Add CFbarrel for blowing up the boulder after Tricky learns Flame
    //TODO: improve this, check how respawning barrels are usually handled elsewhere!
    {
        typedef struct {
            ObjSetup base;
            s8 unk18;
            s8 unk19;
            s32 unk1C;
            s32 unk20;
        } CFBarrel_Setup;

        CFBarrel_Setup barrel = {
            .base = {
                .objId = OBJ_CFbarrel,
                .actExclusions1 = ~MAP_ACT(1),
                .loadFlags = OBJSETUP_LOAD_MAIN,
                .fadeFlags = OBJSETUP_FADE_CAMERA,
                .loadDistance = 140,
                .fadeDistance = 140,
                .x = 2244.0f,
                .y = -676.0f,
                .z = 2299.0f
            }
        };
        reasset_map_objects_set(mapID, reasset_id(dinomodNs, 0x100000), &barrel, sizeof(barrel));
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

    //Tag Blocks shapes with animatorIDs, so they can be removed with HitAnimators
    reasset_blocks_set(shTrkblk, reasset_base_id(351 - 345), REASSET_BASE_NAMESPACE, block351, block351_end - block351);

    //Revert changes to SHboulder's DLL usage
    {
        ReAssetID objects_shboulder_id = reasset_base_id(583); //OBJ_SHboulder
        reasset_objects_set(objects_shboulder_id, REASSET_BASE_NAMESPACE, objects_shboulder, objects_shboulder_end - objects_shboulder);
    }

    //Edit the SHboulder blocking Willow Grove, so it can't be destroyed
    {
        SHboulder_Setup *boulder = (SHboulder_Setup*)reasset_map_objects_get(sHollow, 
            reasset_base_id(0x307F3), NULL);
        boulder->scale = 177;
        boulder->invincible = TRUE;
    }

    // Move river sfx TriggerPoints into obj group 11
    {
        ObjSetup *triggerPoint;
        
        triggerPoint = reasset_map_objects_get(sHollow, reasset_base_id(0x31CDD), NULL);
        triggerPoint->loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP;
        triggerPoint->mapObjGroup = 11;
        
        triggerPoint = reasset_map_objects_get(sHollow, reasset_base_id(0x31CCA), NULL);
        triggerPoint->loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP;
        triggerPoint->mapObjGroup = 11;
    }

    // Move dockpoint into obj group 12
    {
        ObjSetup *dockpoint = reasset_map_objects_get(sHollow, reasset_base_id(0x42BAA), NULL);
        dockpoint->loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP;
        dockpoint->mapObjGroup = 12;
    }

    // Edit trigger plane at the start of the DB side of the river to enable the DB
    // river object group that contains river flows etc.
    {
        Trigger_Setup *trigger = reasset_map_objects_get(sHollow, reasset_base_id(0x18EB), NULL);
        trigger->commands[1].id = TRG_CMD_WORLD_ENABLE_OBJ_GROUP;
        trigger->commands[1].condition = CMD_COND_IN | CMD_COND_RE_ENTER;
        trigger->commands[1].param1 = 0;
        trigger->commands[1].param2 = MAP_DIAMOND_BAY;
        trigger->commands[2].id = TRG_CMD_WORLD_DISABLE_OBJ_GROUP;
        trigger->commands[2].condition = CMD_COND_OUT | CMD_COND_RE_EXIT;
        trigger->commands[2].param1 = 0;
        trigger->commands[2].param2 = MAP_DIAMOND_BAY;
    }

    //Add a HITS line so you dangle off SwapStone Hollow's waterfall when attempting to run off it
    {
        ReAssetID dbTrkblk = reasset_base_id(48);
        HitsLine line = {
            .Ax = 538,
            .Ay = -825,
            .Az = 308,

            .Bx = 575,
            .By = -825,
            .Bz = 434,

            .heightA = 0x28,
            .heightB = 0x28,

            .settingsA = 0xe,
            .settingsB = 0x82,

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
                    .loadFlags = OBJSETUP_LOAD_LEVEL,
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
                .loadFlags = OBJSETUP_LOAD_LEVEL,
                .fadeFlags = OBJSETUP_FADE_CAMERA,
                .loadDistance = 140,
                .fadeDistance = 140,
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
                .loadFlags = OBJSETUP_LOAD_LEVEL,
                .fadeFlags = OBJSETUP_FADE_CAMERA,
                .loadDistance = 140,
                .fadeDistance = 140,
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
    u32 shWellBlocks;

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
            HitsLine line = {
                .Ax = 247,
                .Ay = -1125,
                .Az = 575,

                .Bx = 460,
                .By = -1109,
                .Bz = 579,

                .heightA = 0,
                .heightB = 56,

                .settingsA = 0x8e,
                .settingsB = 0x01,

                .animatorID = 1,
            };
            reasset_hits_set(shWellTrkblk, blockIDLilyPondVines, reasset_base_id(32), REASSET_BASE_NAMESPACE, &line);
        }

        //Add an equivalent HITS line at the opposite side of the vines, so the collision's more stable at that side too
        {
            ReAssetID blockIDLilyPondVines = reasset_base_id(580 - shWellBlocksBase);
            HitsLine line = {
                .Ax = 454,
                .Ay = -1127,
                .Az = 640,

                .Bx = 238,
                .By = -1127,
                .Bz = 640,

                .heightA = 0,
                .heightB = 56,

                .settingsA = 0x8e,
                .settingsB = 0x01,

                .animatorID = 1,
            };
            reasset_hits_set(shWellTrkblk, blockIDLilyPondVines, reasset_auto_id(33), REASSET_BASE_NAMESPACE, &line);
        }

        // Allow ledge-grabs on two lines near the top of the climbable vines in the lily pond cave
        // (they were set up just as invisible walls, probably by mistake)
        {
            ReAssetID blockIDLilyPondClimb = reasset_base_id(579 - shWellBlocksBase);
            
            HitsLine line;

            //Line 50
            ReAssetID hitsVineTopEdge1 = reasset_base_id(50);
            bcopy(reasset_hits_get(shWellTrkblk, blockIDLilyPondClimb, hitsVineTopEdge1), &line, sizeof(HitsLine));
            line.settingsA = 0;
            line.settingsB = 0x82;
            reasset_hits_set(shWellTrkblk, blockIDLilyPondClimb, hitsVineTopEdge1, REASSET_BASE_NAMESPACE, &line);
            
            //Line 51
            ReAssetID hitsVineTopEdge2 = reasset_base_id(51);
            bcopy(reasset_hits_get(shWellTrkblk, blockIDLilyPondClimb, hitsVineTopEdge2), &line, sizeof(HitsLine));
            line.settingsA = 0;
            line.settingsB = 0x82;
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
                HitsLine line = {
                    .Ax = points[i].x,
                    .Ay = points[i].y,
                    .Az = points[i].z,

                    .Bx = points[(i + 1) % count].x,
                    .By = points[(i + 1) % count].y,
                    .Bz = points[(i + 1) % count].z,

                    .heightA = 40,
                    .heightB = 40,

                    .settingsA = 0,
                    .settingsB = 0x85
                };
                reasset_hits_set(shWellTrkblk, blockIDStalactiteCave, reasset_auto_id(127 + i), REASSET_BASE_NAMESPACE, &line);
            }
        }
    }

}

static void cc_lightfoot_patch(void) {
    // Change CClightfoot model from chief to normal red-colored LightFoot
    ObjDef *ccLightfootObjDef = reasset_objects_get(reasset_base_id(430), NULL);
    u32 *ccLightfootModelList = (u32*)((u8*)ccLightfootObjDef + (u32)ccLightfootObjDef->pModelList);
    ccLightfootModelList[0] = 0x00CB;
}

/** Fix DIM and Galadon related music actions. Original patch by nuggs. */
static void music_actions_patch(void) {
    // Galadon
    MusicAction *action103 = reasset_music_actions_get(reasset_base_id(103 - 1));
    action103->seqID = 60;
    MusicAction *action104 = reasset_music_actions_get(reasset_base_id(104 - 1));
    action104->seqID = 60;
    MusicAction *action106 = reasset_music_actions_get(reasset_base_id(106 - 1));
    action106->seqID = 60;
    MusicAction *action108 = reasset_music_actions_get(reasset_base_id(108 - 1));
    action108->seqID = 60;
    
    // DIM
    MusicAction *action109 = reasset_music_actions_get(reasset_base_id(109 - 1));
    action109->seqID = 66;
    MusicAction *action110 = reasset_music_actions_get(reasset_base_id(110 - 1));
    action110->seqID = 66;
    MusicAction *action111 = reasset_music_actions_get(reasset_base_id(111 - 1));
    action111->seqID = 66;
    MusicAction *action135 = reasset_music_actions_get(reasset_base_id(135 - 1));
    action135->seqID = 66;
    MusicAction *action136 = reasset_music_actions_get(reasset_base_id(136 - 1));
    action136->seqID = 66;
    MusicAction *action138 = reasset_music_actions_get(reasset_base_id(138 - 1));
    action138->seqID = 66;
    MusicAction *action140 = reasset_music_actions_get(reasset_base_id(140 - 1));
    action140->seqID = 66;
}

PRAGMA_IGNORE_PUSH("-Wunused")
static void df_patches_shinx(void) {
    ReAssetID df = reasset_base_id(MAP_DISCOVERY_FALLS);
    ReAssetID dfTrkblk = reasset_base_id(11);
    
    // Fix block shapes that are missing the fog render flag
    ReAssetID blockID = reasset_base_id(338 - 319);
    u32 blockDataSize;
    u8 *blockData = reasset_blocks_get(dfTrkblk, blockID, &blockDataSize);
    blockData = dinomod_block_decompress(blockData, blockDataSize, &blockDataSize);
    Block *block = (Block*)(blockData + 8);
    BlockShape *shapes = (BlockShape*)((u8*)block + (u32)block->shapes);
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
        Trigger_Setup drownTrigger = {
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

        reasset_map_objects_set(db, reasset_auto_id(dinomodNs), &drownTrigger, sizeof(drownTrigger));
    }

    // Add another drown trigger after the second drop in the river (the deep spot where the vines can knock you down into)
    {
        Trigger_Setup drownTrigger = {
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

        reasset_map_objects_set(db, reasset_auto_id(dinomodNs), &drownTrigger, sizeof(drownTrigger));
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
        Trigger_Setup *trigger = reasset_map_objects_get(db, reasset_base_id(0x41EF0), NULL);
        trigger->sizeX = 34; // scale 0.75 -> 2.125
        trigger->base.loadDistance = 48; // increase load dist to compensate
    }

    // Add a gamebit to the SideLoad at the start of the DB river
    // (stops Tricky from warping down there when river's missing)
    {
        SideLoad_Setup *sideLoad = reasset_map_objects_get(db, reasset_base_id(0x42B6F), NULL);
        sideLoad->gamebitUnlocked = RIVER_BIT;
    }

    // Add a gamebit to the WaterFallSpray at the start of the DB river
    {
        WaterFallSpray_Setup *spray = reasset_map_objects_get(db, reasset_base_id(0x4205B), NULL);
        spray->gamebit = RIVER_BIT;
        spray->invertGamebit = TRUE;
    }
}

static void discovery_falls_hit_edits(void) {
    ReAssetID df = reasset_base_id(MAP_DISCOVERY_FALLS);
    ReAssetID dfTrkblk = reasset_base_id(11);
    const int dfTrkblkBase = 319;

    HitsLine *hit;

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

REASSET_ON_SET_LOW_PRIORITY void dinomod_reasset_on_set(void) {
    walled_city_additions();
    warlock_mountain_platform_additions();
    swapstone_hollow_additions();
    swapstone_hollow_well_additions();
    //golden_plains_fuel_additions();
    diamond_bay_additions();
}

REASSET_ON_MODIFY_LOW_PRIORITY void dinomod_reasset_on_modify(void) {
    music_actions_patch();
    collectables_animobj_patch();
    cmdmenu_icons_patch();
    purple_mushroom_patch();

    shrine_fxemit_modifications();
    warlock_mountain_platform_modifications();
    swapstone_hollow_modifications();
    swapstone_hollow_well_modifications();
    cc_lightfoot_patch();
    cape_claw_modifications();
    darkice_mines_modifications();
    golden_plains_modifications();
    walled_city_modifications();
    dragon_rock_upper_modifications();
    golden_plains_modifications();
    // golden_plains_fuel_modifications();
    music_actions_patch();
    // df_patches_shinx();
    // df_modifications();
    diamond_bay_modifications();
    discovery_falls_hit_edits();
}
