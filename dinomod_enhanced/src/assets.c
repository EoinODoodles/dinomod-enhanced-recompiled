#include "recompconfig.h"
#include "recomputils.h"
#include "reasset.h"

#include "PR/ultratypes.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "game/gamebits.h"
#include "sys/fs.h"
#include "sys/map.h"
#include "sys/map_enums.h"
#include "sys/memory.h"
#include "macros.h"

#include "mod_common.h"
#include "common_objsetups.h"
#include "object_util.h"

typedef enum {
    GAMETEXT_VANILLA,
    GAMETEXT_COSMETIC,
} GametextFlavor;

INCBIN(block628, "0628 0274_moon_temple_viewing_tile.bin");

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
        seqobj->gamebitFinished = 0x829;
    }

    // Moon Aperture
    // Set to always enabled (the sun aperture is always enabled as well)
    {
        WCApertureSymbol_Setup *moonAperture = (WCApertureSymbol_Setup*)reasset_map_objects_get(walledCity, 
            reasset_base_id(0x41463), NULL);
        moonAperture->unk20 = BIT_ALWAYS_1;
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
        HitAnimator_Setup hitAnimator = {0};
        hitAnimator.base.objId = OBJ_HitAnimator;
        hitAnimator.base.actExclusions1 = 0;
        hitAnimator.base.loadFlags = OBJSETUP_LOAD_MAIN;
        hitAnimator.base.fadeFlags = OBJSETUP_FADE_CAMERA;
        hitAnimator.base.loadDistance = 10;
        hitAnimator.base.fadeDistance = 10;

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

        hitAnimator.mode = HitAnimator_Mode_HITS | HitAnimator_Mode_Invert; //HITS line switched off when gamebit set
        hitAnimator.hitsAnimatorID = 8; //tag for the ledge grab line

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
}

// /** Change the fade settings of the GP_ShrinePillar so they're visible across Golden Plains 
//   * (they're fairly low-poly, should be grand even on N64!) */
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

/** (CURRENTLY UNUSED) Adds jetbike fuel refills around Golden Plains, only showing up in Act 3 */
PRAGMA_IGNORE_PUSH("-Wunused")
static void golden_plains_fuel_modifications(void) {
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
        {3638.878, 278.484, 2340.379},
        {4026.011, 427.872, 1896.975},
        {4305.506, 450.573, 1843.973},
        {4555.718, 442.071, 1663.649},
        {4683.737, 335.360, 2100.510},
        {4756.052, 316.182, 766.135},
        {4507.222, 295.304, 598.087},
        {3078.822, 304.259, 357.186},
        {2208.539, 310.000, 588.102},
        {2275.768, 257.000, 1167.243},
        {2678.663, 246.000, 1656.635},
        {2935.286, 190.358, 2138.605},
        {3249.799, 321.815, 2329.280},
        {2738.000, 362.000, 1538.000},
        {2586.000, 388.000, 1065.000},
        {2604.000, 365.000, 640.000},
        {2391.022, 310.655, 2007.850},
        {2391.022, 310.655, 2007.850},
        {2086.857, 120.619, 2888.839},
        {2681.164, 212.499, 2171.146},
    };
    u8 count = ARRAYCOUNT(fuelData);

    //Insert the new objects
    for (s32 i = 0; i < count; i++) {
        CRFuelTank_Setup fuel = {0};
        fuel.base.objId = OBJ_CRFuelTank;
        fuel.base.actExclusions1 = ~MAP_ACT(3);
        fuel.base.loadFlags = OBJSETUP_LOAD_MAIN;
        fuel.base.fadeFlags = OBJSETUP_FADE_CAMERA;
        fuel.base.loadDistance = 140;
        fuel.base.fadeDistance = 140;
        fuel.base.x = fuelData[i].coords.x;
        fuel.base.y = fuelData[i].coords.y;
        fuel.base.z = fuelData[i].coords.z;
        fuel.unk1A = 0x12C;
        fuel.gamebit = NO_GAMEBIT;

        reasset_map_objects_set(mapID, reasset_auto_id(dinomodNs), &fuel, sizeof(fuel));
    }
}
PRAGMA_IGNORE_POP()

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

REASSET_ON_SET_LOW_PRIORITY void dinomod_reasset_on_set(void) {
    walled_city_additions();
    warlock_mountain_platform_additions();
}

REASSET_ON_MODIFY_LOW_PRIORITY void dinomod_reasset_on_modify(void) {
    walled_city_modifications();
    shrine_fxemit_modifications();
    warlock_mountain_platform_modifications();
    dragon_rock_upper_modifications();
    golden_plains_modifications();
    // golden_plains_fuel_modifications();
    cc_lightfoot_patch();
    music_actions_patch();
}
