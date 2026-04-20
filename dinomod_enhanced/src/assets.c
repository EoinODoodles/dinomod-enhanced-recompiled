#include "recompconfig.h"
#include "recomputils.h"

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
#include "asset_repacker.h"
#include "common_objsetups.h"
#include "object_util.h"
#include "custom_uids.h"

typedef enum {
    GAMETEXT_VANILLA,
    GAMETEXT_COSMETIC,
} GametextFlavor;

#define INCFST(fileID, filename, ext) \
    INCBIN(fst_assets_##filename##_##ext, "assets/" #filename "."#ext); \
    repacker_set_fst_file_replacement(fileID, fst_assets_##filename##_##ext, fst_assets_##filename##_##ext##_end - fst_assets_##filename##_##ext);

REPACKER_ON_LOAD_FST_REPLACEMENTS_CALLBACK void dinomod_repacker_fst_replacements(void) {
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

INCBIN(block628, "0628 0274_moon_temple_viewing_tile.bin");

REPACKER_ON_LOAD_REPLACEMENTS_CALLBACK void dinomod_repacker_replacements(void) {
    // Fix terrain ID of moon temple viewing tile (to let the aperture work correctly)
    repacker_blocks_set_replacement(628, block628, block628_end - block628);
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

static void walled_city_modifications(void) {
    // Revert dinomod's removal of the moon temple lift sequences, so it can be used again
    {
        ObjDef *moonTempleLiftDef = repacker_objects_get(276, NULL);
        s16 *seq = (s16*)((u32)moonTempleLiftDef + (u32)moonTempleLiftDef->pSeq);
        seq[0] = 0x3D4;
        seq[1] = 0x3D6;
    }

    // Moon temple fixes
    {
        MapHeader *header = repacker_maps_get(MAP_WALLED_CITY, 0, NULL);
        void *objects = repacker_maps_get(MAP_WALLED_CITY, 4, NULL);

        ObjSetup *setup = (ObjSetup*)objects;

        for (s32 i = 0; i < header->objectInstanceCount; i++) {
            if (setup->uID == 0x41474) {
                // Moon door seqobj
                // Revert dinomod's gamebit change, so the door only opens after the aperture sequence
                SeqObj_Setup *seqobj = (SeqObj_Setup*)setup;
                seqobj->gamebitFinished = 0x829;
            } else if (setup->uID == 0x41463) {
                // Moon Aperture
                // Set to always enabled (the sun aperture is always enabled as well)
                WCApertureSymbol_Setup *moonAperture = (WCApertureSymbol_Setup*)setup;
                moonAperture->unk20 = BIT_ALWAYS_1;
            }

            setup = objsetup_next(setup);
        }

        // Add two FXEmit objects to enable in the moon temple aperture cutscene
        u32 fxEmitSetupSize = mmAlign4(sizeof(FXEmit_Setup));
        u32 addedSize = fxEmitSetupSize * 2;
        u32 newSize = header->objectInstancesFileLength + addedSize;
        objects = repacker_maps_resize(MAP_WALLED_CITY, 4, newSize);
        setup = (ObjSetup*)objects;
        ObjSetup *lastGroup7 = maps_find_object_group_endpoint(header, setup, 7);

        if (lastGroup7 != NULL) {
            setup = lastGroup7;
            bcopy((void*)setup, (void*)((u32)setup + addedSize), 
                header->objectInstancesFileLength - ((u32)setup - (u32)objects));
        } else {
            recomp_error_message_box("Failed to find WC object group 7 setups!");
        }

        for (s32 i = 0; i < 2; i++) {
            FXEmit_Setup *fxemit = (FXEmit_Setup*)setup;
            fxemit->base.objId = OBJ_FXEmit;
            fxemit->base.quarterSize = fxEmitSetupSize >> 2;
            fxemit->base.actExclusions1 = 0;
            fxemit->base.actExclusions2 = 0;
            fxemit->base.loadFlags = OBJSETUP_LOAD_IN_MAP_OBJGROUP;
            fxemit->base.fadeFlags = OBJSETUP_FADE_CAMERA;
            fxemit->base.mapObjGroup = 7;
            fxemit->base.fadeDistance = 50;

            // base off of WCApertureSymbol position
            fxemit->base.x = 3008.52490234375f;
            fxemit->base.y = -694.7548828125f;
            fxemit->base.z = -3690.62060546875f;
            
            switch (i) {
                case 0:
                    fxemit->base.x += 0.0f;
                    fxemit->base.y += -0.24524f;
                    fxemit->base.z += 0.846679f;
                    fxemit->unk1A = 0x741; // Circular blue glow inside of aperture
                    fxemit->unk1C = -5;
                    fxemit->unk27 = 0;
                    fxemit->base.uID = UID_WC_Moon_Temple_Aperture_FXEmit_Glow;
                    break;
                case 1:
                    fxemit->base.x += -0.266602f;
                    fxemit->base.y += -1.635926f - 9.0f;
                    fxemit->base.z += 0.492187f + -70.0f;
                    fxemit->unk1A = 0x25A; // Blue beams
                    fxemit->unk1C = -1;
                    fxemit->unk27 = 0;
                    fxemit->base.uID = UID_WC_Moon_Temple_Aperture_FXEmit_Beam;
                    break;
            }

            fxemit->unk18 = 0;
            fxemit->unk19 = 0;
            fxemit->toggleGamebit = 0x848; // Active during the aperture cutscene
            fxemit->disableGamebit = -1;
            fxemit->unk22 = 0;
            fxemit->unk23 = 0;
            fxemit->unk24 = 0;
            fxemit->unk25 = 0;
            fxemit->unk26 = 0;
            fxemit->unk28 = 0;
            fxemit->unk29 = 0;
            fxemit->unk2A = 0;

            setup = objsetup_next(setup);
        }

        header->objectInstancesFileLength = newSize;
        header->objectInstanceCount += 2;
    }
}

static void shrine_fxemit_modifications(void) {
    // Remove 'disable' gamebit 0x5 for some shrine FXEmits. Ensures they don't get disabled after picking up a CCgrub.
    // The other shrines don't have disable gamebits for their emitters.
    static s32 mapIDs[] = { MAP_SHRINE_DISCOVERY_FALLS, MAP_SHRINE_MOON_MOUNTAIN_PASS, MAP_SHRINE_DIAMOND_BAY };

    for (u32 i = 0; i < ARRAYCOUNT(mapIDs); i++) {
        s32 mapID = mapIDs[i];

        MapHeader *header = repacker_maps_get(mapID, 0, NULL);
        void *objects = repacker_maps_get(mapID, 4, NULL);

        ObjSetup *setup = (ObjSetup*)objects;

        for (s32 i = 0; i < header->objectInstanceCount; i++) {
            if (setup->objId == OBJ_FXEmit) {
                FXEmit_Setup *fxemit = (FXEmit_Setup*)setup;
                if (fxemit->disableGamebit == 0x5) {
                    fxemit->disableGamebit = -1;
                }
            }

            setup = objsetup_next(setup);
        }
    }
}

/** Adding HitAnimators and HITS line tags in WM's main room to toggle the ledge grab lines at WM_Platform's upper destination */
static void warlock_mountain_platform_modifications(void) {
    //Repurposing these lift gamebits to keep track of the ledge grab HitAnimators
    #define LIFT_NEAR_TOP_GAMEBIT_KRYSTAL BIT_322
    #define LIFT_NEAR_TOP_GAMEBIT_SABRE   BIT_369

    u32 mapID = MAP_WARLOCK_MOUNTAIN;

    // MAPS.bin
    // Add two HitAnimators to the upper tier of Warlock Mountain's main chamber, 
    // for toggling the ledge grab lines at the lifts' upper destinations 
    {
        MapHeader *header = repacker_maps_get(mapID, 0, NULL);
        void *objects = repacker_maps_get(mapID, 4, NULL);

        ObjSetup *setup = (ObjSetup*)objects;

        //Resize the MAPS file, adding space for two HitAnimators
        u32 hitAnimatorSetupSize = mmAlign4(sizeof(HitAnimator_Setup));
        u32 addedSize = hitAnimatorSetupSize * 2;
        u32 newSize = header->objectInstancesFileLength + addedSize;
        objects = repacker_maps_resize(mapID, 4, newSize);
        setup = (ObjSetup*)objects;

        
        // Find the end of the map's generic group
        ObjSetup *endOfGenericGroup = maps_find_generic_group_endpoint(header, setup);

        //Move subsequent objects to make enough room for the new ones
        if (endOfGenericGroup != NULL) {
            bcopy((void*)setup, (void*)((u32)setup + addedSize), 
                header->objectInstancesFileLength - ((u32)setup - (u32)objects));
        } else {
            recomp_error_message_box("WM: Failed to find end of generic group!");
        }

        //Insert two new HitAnimators
        for (s32 i = 0; i < 2; i++) {
            HitAnimator_Setup *hitAnimator = (HitAnimator_Setup*)setup;
            hitAnimator->base.objId = OBJ_HitAnimator;
            hitAnimator->base.quarterSize = hitAnimatorSetupSize >> 2;
            hitAnimator->base.actExclusions1 = 0;
            hitAnimator->base.loadFlags = OBJSETUP_LOAD_MAIN;
            hitAnimator->base.fadeFlags = OBJSETUP_FADE_CAMERA;
            hitAnimator->base.loadDistance = 10;
            hitAnimator->base.fadeDistance = 10;

            switch (i) {
                case 0:
                    //Krystal side
                    hitAnimator->base.x = 1369.1f;
                    hitAnimator->base.y = 472.0f;
                    hitAnimator->base.z = 2692.4f;
                    hitAnimator->base.uID = UID_WM_Lift_HitAnimator_Krystal;
                    hitAnimator->gamebitActivate = LIFT_NEAR_TOP_GAMEBIT_KRYSTAL;
                    break;
                case 1:
                    //Sabre side
                    hitAnimator->base.x = 1186.8f;
                    hitAnimator->base.y = 472.0f;
                    hitAnimator->base.z = 1793.0f;
                    hitAnimator->base.uID = UID_WM_Lift_HitAnimator_Sabre;
                    hitAnimator->gamebitActivate = LIFT_NEAR_TOP_GAMEBIT_SABRE;
                    break;
            }

            hitAnimator->mode = HitAnimator_Mode_HITS | HitAnimator_Mode_Invert; //HITS line switched off when gamebit set
            hitAnimator->hitsAnimatorID = 8; //tag for the ledge grab line

            setup = objsetup_next(setup);
        }

        header->objectInstancesFileLength = newSize;
        header->objectInstanceCount += 2;
    }

    // HITS.bin
    // Modify the ledge grab HITS lines at the lifts' upper destinations, adding HitAnimator tags
    {
        HitsLine *lines;

        //Krystal's side
        lines = repacker_hits_get(459, NULL);
        lines[0].animatorID = 8;

        //Sabre's side
        lines = repacker_hits_get(447, NULL);
        lines[0].animatorID = 8;
    }
}

static void dragon_rock_upper_modifications(void) {
    MapHeader *header = repacker_maps_get(MAP_DRAGON_ROCK_TOP, 0, NULL);
    void *objects = repacker_maps_get(MAP_DRAGON_ROCK_TOP, 4, NULL);

    ObjSetup *setup = (ObjSetup*)objects;

    for (s32 i = 0; i < header->objectInstanceCount; i++) {
        // Remove static spawns of DR_EarthWarrior (they were leftover from testing, the one you save in DR lower remains)
        if (setup->uID == 0x338CB || setup->uID == 0x33984 || setup->uID == 0x406A1) {
            setup->objId = OBJ_animator;
        }

        setup = objsetup_next(setup);
    }
}

/** Change the fade settings of the GP_ShrinePillar so they're visible across Golden Plains 
  * (they're fairly low-poly, should be grand even on N64!) */
static void golden_plains_modifications(void) {
    u32 mapID = MAP_GOLDEN_PLAINS;

    // MAPS.bin
    {
        MapHeader *header = repacker_maps_get(mapID, 0, NULL);
        void *objects = repacker_maps_get(mapID, 4, NULL);

        ObjSetup *setup = (ObjSetup*)objects;

        for (s32 i = 0; i < header->objectInstanceCount; i++) {
            if (setup->objId == OBJ_GP_PillarDoor){
                setup->fadeFlags = OBJSETUP_FADE_MANUAL;
            } else if (setup->objId == OBJ_GP_ShrinePillar) {
                setup->fadeFlags = OBJSETUP_FADE_MANUAL;
            }            

            setup = objsetup_next(setup);
        }
    }
}

/** (CURRENTLY UNUSED) Adds jetbike fuel refills around Golden Plains, only showing up in Act 3 */
PRAGMA_IGNORE_PUSH("-Wunused")
static void golden_plains_fuel_modifications(void) {
    u32 mapID = MAP_GOLDEN_PLAINS;

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
        u32 uID;
        Vec3f coords;
    } CustomFuel;

    CustomFuel fuelData[20] = {
        {UID_GP_FuelTank_00, 3638.878, 278.484, 2340.379},
        {UID_GP_FuelTank_01, 4026.011, 427.872, 1896.975},
        {UID_GP_FuelTank_02, 4305.506, 450.573, 1843.973},
        {UID_GP_FuelTank_03, 4555.718, 442.071, 1663.649},
        {UID_GP_FuelTank_04, 4683.737, 335.360, 2100.510},
        {UID_GP_FuelTank_05, 4756.052, 316.182, 766.135},
        {UID_GP_FuelTank_06, 4507.222, 295.304, 598.087},
        {UID_GP_FuelTank_07, 3078.822, 304.259, 357.186},
        {UID_GP_FuelTank_08, 2208.539, 310.000, 588.102},
        {UID_GP_FuelTank_09, 2275.768, 257.000, 1167.243},
        {UID_GP_FuelTank_10, 2678.663, 246.000, 1656.635},
        {UID_GP_FuelTank_11, 2935.286, 190.358, 2138.605},
        {UID_GP_FuelTank_12, 3249.799, 321.815, 2329.280},
        {UID_GP_FuelTank_13, 2738.000, 362.000, 1538.000},
        {UID_GP_FuelTank_14, 2586.000, 388.000, 1065.000},
        {UID_GP_FuelTank_15, 2604.000, 365.000, 640.000},
        {UID_GP_FuelTank_16, 2391.022, 310.655, 2007.850},
        {UID_GP_FuelTank_17, 2391.022, 310.655, 2007.850},
        {UID_GP_FuelTank_18, 2086.857, 120.619, 2888.839},
        {UID_GP_FuelTank_19, 2681.164, 212.499, 2171.146},
    };
    u8 count = ARRAYCOUNT(fuelData);

    {
        MapHeader *header = repacker_maps_get(mapID, 0, NULL);
        void *objects = repacker_maps_get(mapID, 4, NULL);

        ObjSetup *setup = (ObjSetup*)objects;

        //Resize the MAPS file, adding space for the fuel objects
        u32 fuelSetupSize = mmAlign4(sizeof(CRFuelTank_Setup));
        u32 addedSize = fuelSetupSize * count;
        u32 newSize = header->objectInstancesFileLength + addedSize;
        objects = repacker_maps_resize(mapID, 4, newSize);
        setup = (ObjSetup*)objects;
        
        // Find the end of the map's generic group
        ObjSetup *endOfGenericGroup = maps_find_generic_group_endpoint(header, setup);

        //Move subsequent objects to make enough room for the new ones
        if (endOfGenericGroup != NULL) {
            bcopy((void*)setup, (void*)((u32)setup + addedSize), 
                header->objectInstancesFileLength - ((u32)setup - (u32)objects));
        } else {
            recomp_error_message_box("GP: Failed to find end of generic group!");
        }

        //Insert the new objects
        for (s32 i = 0; i < count; i++) {
            CRFuelTank_Setup *fuel = (CRFuelTank_Setup*)setup;

            fuel->base.objId = OBJ_CRFuelTank;
            fuel->base.quarterSize = fuelSetupSize >> 2;
            fuel->base.actExclusions1 = ~MAP_ACT(3);
            fuel->base.loadFlags = OBJSETUP_LOAD_MAIN;
            fuel->base.fadeFlags = OBJSETUP_FADE_CAMERA;
            fuel->base.loadDistance = 140;
            fuel->base.fadeDistance = 140;
            fuel->base.x = fuelData[i].coords.x;
            fuel->base.y = fuelData[i].coords.y;
            fuel->base.z = fuelData[i].coords.z;
            fuel->base.uID = fuelData[i].uID;
            fuel->unk1A = 0x12C;
            fuel->gamebit = NO_GAMEBIT;

            setup = objsetup_next(setup);
        }

        header->objectInstancesFileLength = newSize;
        header->objectInstanceCount += count;
    }
}
PRAGMA_IGNORE_POP()

static void cc_lightfoot_patch(void) {
    // Change CClightfoot model from chief to normal red-colored LightFoot
    ObjDef *ccLightfootObjDef = repacker_objects_get(430, NULL);
    u32 *ccLightfootModelList = (u32*)((u32)ccLightfootObjDef + (u32)ccLightfootObjDef->pModelList);
    ccLightfootModelList[0] = 0x00CB;
}

REPACKER_ON_LOAD_MODIFICATIONS_CALLBACK void dinomod_repacker_modifications(void) {
    walled_city_modifications();
    shrine_fxemit_modifications();
    warlock_mountain_platform_modifications();
    dragon_rock_upper_modifications();
    golden_plains_modifications();
    // golden_plains_fuel_modifications();
    cc_lightfoot_patch();
}
