#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"
#include "custom_object_ids.h"
#include "configs.h"

#include "common.h"
#include "game/gamebits.h"
#include "game/objects/object_id.h"
#include "game/objects/interaction_arrow.h"
#include "sys/main.h"
#include "sys/newshadows.h"
#include "sys/objects.h"
#include "sys/objmsg.h"
#include "sys/objtype.h"
#include "dlls/objects/common/collectable.h"
#include "dlls/objects/common/foodbag.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/272_collectable.h"

#include "objects/314_foodbag.h"

#include "recomp/dlls/objects/272_collectable_recomp.h"

#define DEBUG_COLLECTABLE TRUE
#define OBJ_EnergyEgg OBJ_meatPickup

typedef struct {
    u32 soundHandle;            //Cleared on free, but not used for any sound calls
    f32 distanceToPlayer;  
    f32 interactionRadius;
    f32 timerDestroy;           //Countdown after receiving collection message from sender
    s8 sidekickArgBase;         //Base arg value used for sidekick-related collectables
    u8 objHitsValue;
    u8 unused12;
    u8 pause;                   //Control function ends early when this is set
    s16 gamebitCollected;       //Set when collected: collectable vanishes when set
    s16 tutorialObjectID;       //@recomp: animObj objectID to show during the item collection sequence
    s16 gamebitShow;            //(Optional) Collectable only shows up once gamebit set
    u8 shadowOpacity;           //For magic
    u8 unused1B;
    s32 areaValue;              //Received from Area object, if the collectable is inside one
    s8 delayCollect;            //Timer, can only collect object once at 0
    u8 moving;
    u8 isHidden;
    u32 uID;
    Vec3f savedPosition;
    f32 pitchAnimate;           //Rotation for Dino Eggs' rattle animation
    s16 soundTimer;             //Timer for Dino Eggs' rattle sound/animation
    u8 useColourMultiplier;     //Toggles colour multiplier
    u8 interactFlags;           //Toggles ability to collect
    u8 multiplyR;               //Colour multiplier for model
    u8 multiplyG;               //Colour multiplier for model
    u8 multiplyB;               //Colour multiplier for model
    u8 unused3F;
    s16 rootTimer;              //Affects opacity of Alpine Root
    u16 gamebitInventory;       //@recomp: repurposed as inventory item gamebit
} Collectable_Data_Recomp;

typedef enum {
    Collectable_FLAG_Interaction_Off = 1
} Collectable_Flags;

extern int collectable_func_B24(Object* self, Object* animObj, AnimObj_Data* animobjData);
extern void collectable_handle_animation_and_fx(Object* self);
extern void collectable_handle_motion(Object* self);
extern void collectable_collect(Object* self);

/** Returns the player foodbag inventory gamebit associated with a particular food collectable */
static s16 get_food_inventory_gamebit(Object* self) {
    switch (self->id) {
    case OBJ_EnergyEgg:
        return BIT_Dino_Egg_Count;
    case OBJ_applePickup:
        //TODO: handle green apples?
        return BIT_Red_Apple_Count;
        break;
    case OBJ_beanPickup:
        //TODO: handle blue/brown beans?
        return BIT_Red_Bean_Count;
    }

    return NO_GAMEBIT;
}

/** For the item info pop-up: figures out what count to show when collecting an item */
static s8 collectable_get_cmdmenu_popup_item_count(Object* self, Collectable_Setup* objSetup, Collectable_Data_Recomp* objData) {
    Object* player;
    Object* foodbag;
    s8 count = 0;
    s16 foodGamebit;

    switch (self->id) {
        //Unique items (don't show pop-up, since item collect sequence will appear instead)
        case OBJ_NWtrickyball:
        case OBJ_NWreplaymedal:
        case OBJ_CCCellKey:
        case OBJ_DIMShackleKey:
        case OBJ_DIMTruthHorn:
        case OBJ_DIM2PuzzleKey:
        case OBJ_DIM2SilverKey:
        case OBJ_DIM2GoldKey:
        case OBJ_DIM2CellKey:
        case OBJ_CFPrisonKey:
        case OBJ_CFPowerCrystal1:
        case OBJ_CFPowerCrystal2:
        case OBJ_CFPowerCrystal3:
        case OBJ_CFPowerRoomKey:
        case OBJ_CFExplosiveKey:
        case OBJ_WCSunStone:
        case OBJ_WCMoonStone:
        case OBJ_WMconsolestone:
        case OBJ_WM_PureMagic:
        case OBJ_Spellstone:
        case OBJ_PortalSpell:
        case OBJ_ProjectileSpell:
        case OBJ_IllusionSpell:
        case OBJ_FireSpell:
        case OBJ_EarthQuakSpell:
        case OBJ_WeaponUp:
        case OBJ_foodbagSmall:
        case OBJ_foodbagMedium:
        case OBJ_foodbagLarge:
        case OBJ_GuardPass:
        case OBJ_Dynamite:
        case OBJ_PadlockKey:
        case OBJ_fishingnetColle:
            return 0;

        //Stackable items
        case OBJ_NWalpineroot:
        case OBJ_DIMAlpineRoot:
        case OBJ_WCTrexTooth:       //NOTE: not used, separate from Gold Tooth/Silver Tooth
        // case OBJ_MoonSeed:
        case OBJ_FireNut:           //NOTE: not used, Fire Crystal became a separate DLL
        // case OBJ_BlackEyedPod:
        default:
            if ((objSetup->gamebitCount != NO_GAMEBIT)) {
                return main_get_bits(objSetup->gamebitCount) + 1;
            }
            break;

        //Stackable items (food)
        case OBJ_EnergyEgg:
        case OBJ_applePickup:
        case OBJ_beanPickup:
            player = get_player();
            if (!player) {
                break;
            }
            foodbag = ((DLL_210_Player*)player->dll)->vtbl->func66(player, 15);
            if (!foodbag) {
                break;
            }

            //If the food went into the food bag, count how many are now in the bag
            if (Foodbag_will_collected_food_be_stored(foodbag, Foodbag_get_food_type_from_objectID(self))) {
                foodGamebit = get_food_inventory_gamebit(self);

                //Get the food's count
                if (foodGamebit != NO_GAMEBIT) {
                    return main_get_bits(foodGamebit) + 1;
                }
            }
            break;

        //Special cases
        case OBJ_BoneDust:
            //No need for pop-up, magic meter shows up instead
            return 0;
        case OBJ_DIMAlpineRoot2:
            //Not regular collectables: used when riding SnowHorn through blizzard
            return 0;
        case OBJ_CCgoldnuggetPic:
            //Show how many have been found in total
            count += main_get_bits(BIT_Gold_Nugget_GP);
            count += main_get_bits(BIT_Gold_Nugget_LFV);
            count += main_get_bits(BIT_Gold_Nugget_CC);
            return count + 1;
        case OBJ_DIMBridgeCogCol:
            //Show how many are currently held
            if (!main_get_bits(BIT_DIM_Used_Gear_1)) {
                count += main_get_bits(BIT_DIM_Gear_1);
            }
            if (!main_get_bits(BIT_DIM_Used_Gear_2)) {
                count += main_get_bits(BIT_DIM_Gear_2);
            }
            if (!main_get_bits(BIT_DIM_Used_Gear_3)) {
                count += main_get_bits(BIT_DIM_Gear_3);
            }
            if (!main_get_bits(BIT_DIM_Used_Gear_4)) {
                count += main_get_bits(BIT_DIM_Gear_4);
            }
            return count + 1;
    }

    //NOTE: `count + 1` is used because the item that's currently being collected hasn't had its gamebit set/incremented yet

    return 0;
}

/** Handles special cases like the DIM's 4 Bridge Gears and SC/CC's 3 Shiny Nuggets, 
  * ensuring the item collection sequence only plays the first time one of these items is collected.
  *
  * (This can also be switched off if preferred, so it still shows the sequence each time)
  */
static s16 collectable_override_tutorial_gamebitID(Object* self, Collectable_Setup* objSetup) {
    s16 defaultGamebit = objSetup->animMessage;

    //Check if repeat collections should be overridden
    if (recomp_get_config_u32("cmdmenu_info_popup_expand") < POPUP_CONFIG_OVERRIDE_TUTORIAL_ON_REPEAT) {
        return defaultGamebit;
    }

    switch (self->id) {
    case OBJ_DIMBridgeCogCol:
        /* Ignore Gear 1: 
           it doesn't show a tutorial since it's acquired during 
           the cutscene where you free the shackled SnowHorn
        */
        // if (main_get_bits(BIT_DIM_Gear_1)) {
        //     return BIT_DIM_Gear_1;
        // }
        if (main_get_bits(BIT_DIM_Gear_2)) {
            return BIT_DIM_Gear_2;
        }
        if (main_get_bits(BIT_DIM_Gear_3)) {
            return BIT_DIM_Gear_3;
        }
        if (main_get_bits(BIT_DIM_Gear_4)) {
            return BIT_DIM_Gear_4;
        }
    case OBJ_CCgoldnuggetPic:
        if (main_get_bits(BIT_Gold_Nugget_GP)) {
            return BIT_Gold_Nugget_GP;
        }
        if (main_get_bits(BIT_Gold_Nugget_LFV)) {
            return BIT_Gold_Nugget_LFV;
        }
        if (main_get_bits(BIT_Gold_Nugget_CC)) {
            return BIT_Gold_Nugget_CC;
        }
    }

    return defaultGamebit;
}

/** Check whether the item collection sequence will be shown on collecting an object */
static int collectable_will_tutorial_be_shown(Object* self, Collectable_Setup* objSetup) {
    s16 tutorialGamebit;

    //Special case: Energy Eggs
    if (self->id == OBJ_EnergyEgg) {
        return (main_get_bits(BIT_90E) == FALSE);
    }

    tutorialGamebit = collectable_override_tutorial_gamebitID(self, objSetup);
    if (tutorialGamebit > (NO_GAMEBIT + 1)) {
        return (main_get_bits(tutorialGamebit) == FALSE);
    } else {
        return TRUE;
    }
}

/** For the item info popup: returns the gamebitID to use for finding the item's icon texture */
static s16 collectable_get_popup_icon_gamebit(Object* self, Collectable_Setup* objSetup) {
    u8 stackableGold;

    //Handle special cases
    switch (self->id) {
    case OBJ_EnergyEgg:
    case OBJ_applePickup:
    case OBJ_beanPickup:
        return get_food_inventory_gamebit(self);
    case OBJ_CCgoldnuggetPic:
        stackableGold = recomp_get_config_u32("cmdmenu_stack_shiny_nuggets");
        if (stackableGold) {
            return objSetup->gamebitCount;
        } else {
            return objSetup->gamebitCollected;
        }
    }

    //If the collectable has an inventory gamebit defined, use that
    if (objSetup->gamebitCount > (NO_GAMEBIT + 1)) {
        return objSetup->gamebitCount;
    }

    //If the collectable has a collection gamebit defined (for unique collectable instances), use that
    if (objSetup->gamebitCollected > (NO_GAMEBIT + 1)) {
        return objSetup->gamebitCollected;
    }

    return NO_GAMEBIT;
}

/** Log info about the collectable */
static void collectable_log_info(Object* self, Collectable_Setup* objSetup, Collectable_Data_Recomp* objData) {
    recomp_printf("\nCollecting %s!\n", self->def->name);
    recomp_printf("Inventory gamebit: %x\n", objSetup->gamebitCount);
    recomp_printf("Collected gamebit: %x\n", objData->gamebitCollected);
    recomp_printf("Tutorial gamebit: %x\n", objSetup->animMessage);
    recomp_printf("Tutorial animObjID: %d\n", self->def->collectableDef->seqObjectID);
    recomp_printf("Show tutorial: %s\n", collectable_will_tutorial_be_shown(self, objSetup) ? "YES" : "NO");
    recomp_printf("Current count: %d\n", collectable_get_cmdmenu_popup_item_count(self, objSetup, objData));
    recomp_printf("Icon gamebit: %x\n", collectable_get_popup_icon_gamebit(self, objSetup));
}

/** Optionally shows an item info pop-up when collecting the item (not shown when the tutorial sequence appears instead) */
static void collectable_handle_popup(Object* self, Collectable_Setup* objSetup, Collectable_Data_Recomp* objData) {
    s8 count;
    s16 iconGamebit;
 
    //Log info about the collectable
    #if DEBUG_COLLECTABLE
    collectable_log_info(self, objSetup, objData);
    #endif

    //Do nothing if the mod config isn't enabled
    if (recomp_get_config_u32("cmdmenu_info_popup_expand") == FALSE) {
        return;
    }

    //Don't show the pop-up if the item collection tutorial box sequence will be shown instead
    if (collectable_will_tutorial_be_shown(self, objSetup)) {
        return;
    }

    //Show the item collection pop-up
    count = collectable_get_cmdmenu_popup_item_count(self, objSetup, objData);
    iconGamebit = collectable_get_popup_icon_gamebit(self, objSetup);
    if (count && (iconGamebit > 0)) {
        gDLL_1_cmdmenu->vtbl->info_show(
            iconGamebit,
            INFO_POPUP_DURATION,
            count
        );
    }
}

/**
  * Delete collectable if it's already been collected.
  * Fixes a bug where its collision would persist when revisiting an area with a collected collectable!
  */
RECOMP_PATCH void collectable_setup(Object* self, Collectable_Setup* objSetup, s32 arg2) {
    s32 pad1;
    CollectableDef* collectableDef;
    LightAction lfxAction;
    s32 index;
    s16 id;
    Collectable_Data_Recomp* objData;

    objData = self->data;
    obj_add_object_type(self, OBJTYPE_5);
    obj_init_mesg_queue(self, 2);

    self->srt.yaw = objSetup->yaw << 8;
    self->srt.pitch = objSetup->pitch << 8;
    self->srt.roll = objSetup->roll << 8;
    self->srt.scale = self->def->scale;

    self->animCallback = (void*)&collectable_func_B24;
    self->modelInstIdx = objSetup->modelIdx;
    self->unkB0 |= 0x2000;

    objData->sidekickArgBase = objSetup->messageArgBase;
    objData->objHitsValue = objSetup->objHitsValue;
    objData->pause = FALSE;
    objData->areaValue = -2;
    objData->moving = 0;
    objData->delayCollect = 60;
    objData->gamebitShow = objSetup->gamebitSecondary;
    objData->uID = objSetup->base.uID;
    objData->savedPosition.x = self->srt.transl.x;
    objData->savedPosition.y = self->srt.transl.y;
    objData->savedPosition.z = self->srt.transl.z;
    objData->useColourMultiplier = objSetup->applyColourMultiplier;

    //Check if hidden via gamebit
    if (objData->gamebitShow != NO_GAMEBIT) {
        objData->isHidden = main_get_bits(objData->gamebitShow) == 0;

        //@recomp: remove collision
        func_800267A4(self);
    }

    //Check if already collected
    objData->gamebitCollected = objSetup->gamebitCollected;
    if (objData->gamebitCollected != NO_GAMEBIT) {
        self->unkDC = main_get_bits(objData->gamebitCollected);
    } else {
        self->unkDC = 0;
    }

    //@recomp: handle deletion when already collected
    if (self->unkDC) {
        self->unkE0 = 1;
        return;
    }

    //Create particles for Magic collectables
    collectableDef = self->def->collectableDef;
    if (collectableDef && collectableDef->type == Collectable_Type_Magic) {
        if (arg2 == 0) {
            gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_8E_Magic_Chime, MAX_VOLUME, 0, 0, 0, 0);
        }

        for (index = 10; index > 0; index--){
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_42, 0, 4, -1, 0);
        }

        self->velocity.y = -1.2f;
    }

    //Setup interaction radius
    if (collectableDef) {
        objData->interactionRadius = collectableDef->interactionRadius;
    } else {
        objData->interactionRadius = 15.0f;
    }

    if (self->def->lockdata) {
        objData->interactionRadius = self->def->lockdata->interactRadius * 4;
    }

    //Setup light effects (TODO: update with cleaner match when available)
    if (self->def->unk87 & 0x10) {
        lfxAction.unk12.asByte = 25;
        lfxAction.unke = 0;
        lfxAction.unk15 = 0;
        lfxAction.unk16 = 0;
        lfxAction.unk17 = 0;
        lfxAction.unk18 = *(u8*)((u32)self->def + 0x89); 
        lfxAction.unk19 = *(u8*)((u32)self->def + 0x8A);
        lfxAction.unk1a = *(u8*)((u32)self->def + 0x8B);
        lfxAction.unk10 = 0xFFFE;
        lfxAction.unk1b = 0;
        lfxAction.unk1c = 0;
        lfxAction.unk0 = 0;
        lfxAction.unk2 = 0;
        lfxAction.unk8 = 0;
        lfxAction.unk6 = 0;
        lfxAction.unk4 = 0;
        lfxAction.unka = 0x54;
        lfxAction.unkc = 0x25;
        lfxAction.unk1d = 0xFF;
        lfxAction.unk1e = 0;
        lfxAction.unk1f = self->def->lightIdx;
        lfxAction.unk20 = self->def->nLights;
        lfxAction.unk21 = 0;
        gDLL_11_Newlfx->vtbl->func0(self, self, &lfxAction, 0, 0, 0);
        self->unkD6 = lfxAction.unk10;
    }

    //Setup shadow
    if (self->shadow){
        self->shadow->flags = OBJ_SHADOW_FLAG_TOP_DOWN | OBJ_SHADOW_FLAG_CUSTOM_DIR;
        id = self->id;
        if (id == OBJ_BoneDust || id == OBJ_WM_PureMagic){
            self->shadow->flags |= OBJ_SHADOW_FLAG_CUSTOM_OPACITY;
        }
    }

    //Setup vertex colour multiplier
    if ((self->def->flags & 0x10000) && objData->useColourMultiplier) {
        objData->multiplyR = objSetup->multiplyR;
        objData->multiplyG = objSetup->multiplyG;
        objData->multiplyB = objSetup->multiplyB;
    }
}

/**
  * - Optionally show item info pop-up when collecting (after tutorial sequenence has been seen).
  * - Add/remove collision when unhidden/hidden.
  */
RECOMP_PATCH void collectable_control(Object* self) {
    Collectable_Data_Recomp* objData;
    Collectable_Setup* objSetup;
    Object* messageSender;
    Object* player;
    CollectableDef* collectableDef;
    f32 distance;
    u32 outMessage;
    s32 messageArg;
    s32 index;
    f32 newdayValue;
    /* RECOMP */
    s32 gamebitValue;

    objData = self->data;
    objSetup = (Collectable_Setup*)self->setup;

    collectableDef = self->def->collectableDef;

    newdayValue = 1.0f;

    //Countdown to destroy after receiving collection message
    if (objData->timerDestroy != 0.0f) {
        objData->timerDestroy -= gUpdateRateF;
        if (objData->timerDestroy <= 0.0f) {
            obj_destroy_object(self);
            objData->timerDestroy = 0.0f;
        }
        return;
    }

    gDLL_7_Newday->vtbl->func5(&newdayValue);
    self->unkAF |= ARROW_FLAG_8_No_Targetting;

    //Handle being unhidden/hidden
    if (objData->gamebitShow != NO_GAMEBIT) {
        gamebitValue = main_get_bits(objData->gamebitShow);
    
        //@recomp: add collision when unhidden
        if (objData->isHidden & gamebitValue) {
            func_8002674C(self);

        //@recomp: remove collision when hidden
        } else if (!objData->isHidden & !gamebitValue) {
            func_800267A4(self);
        }

        objData->isHidden = gamebitValue == 0;
    }

    //End early if hidden/paused
    if (objData->isHidden || objData->pause) {
        return;
    }

    //Destroy self when lifetime runs out
    if (self->unkE0) {
        self->unkE0 -= gUpdateRate * 3;
        if (self->unkE0 <= 0) {
            obj_destroy_object(self);
            return;
        }
    }

    //Count down until can be collected
    objData->delayCollect -= gUpdateRate;
    if (objData->delayCollect < 0) {
        objData->delayCollect = 0;
    }

    //Check for collection message
    while (obj_recv_mesg(self, &outMessage, &messageSender, 0)) {
        if (outMessage == 0x7000B) {
            objData->timerDestroy = 180.0f;
            collectable_collect(self);
        }
    }

    //Timer for Alpine Roots
    if (self->id == OBJ_DIMAlpineRoot2) {
        if (objData->rootTimer) {
            objData->rootTimer -= gUpdateRate;
            if (objData->rootTimer <= 0) {
                objData->rootTimer = 0;
                self->opacity = OBJECT_OPACITY_MAX;
                self->unkDC = 0;
            }
        }
    }

    //Handle being invisible
    if (self->srt.flags & OBJFLAG_INVISIBLE) {
        return;
    }

    //Handle behaviour after being collected
    if (self->unkDC) {
        //Remove collision
        if (self->objhitInfo) {
            self->objhitInfo->unk58 |= 0x100;
        }

        //Check if collection gamebit was reset
        if ((objData->gamebitCollected != NO_GAMEBIT) && 
            (main_get_bits(objData->gamebitCollected) == FALSE)
        ) {
            self->unkDC = 0;
        }
        return;
    }

    self->unkAF &= ~ARROW_FLAG_8_No_Targetting;

    //Update animation/motion
    collectable_handle_animation_and_fx(self);
    if (objData->moving) {
        collectable_handle_motion(self);
    }

    //Return early if no player interaction can happen
    player = get_player();
    if (!player || 
        objData->interactFlags & Collectable_FLAG_Interaction_Off || 
        !self->def->collectableDef
    ) {
        return;
    }

    //Handle collection when close enough 
    //(either automatically or via target arrow, depending on objectID)
    distance = vec3_distance_xz(&self->globalPosition, &player->globalPosition);
    if ((distance >= objData->interactionRadius) || objData->delayCollect) {
        objData->distanceToPlayer = distance;
        return;
    }

    switch (self->id) {
    case OBJ_EnergyEgg:
        objData->interactFlags |= Collectable_FLAG_Interaction_Off;
        gDLL_13_Expgfx->vtbl->func5(self);

        for (index = 10; index > 0; index--){
            gDLL_17_partfx->vtbl->spawn(self, 0x549, 0, 1, -1, 0);
        }

        if (main_get_bits(BIT_90E) == 0) {
            gDLL_3_Animation->vtbl->func30(collectableDef->seqObjectID, 0, 0);
            outMessage = 0;
            obj_send_mesg(
                player, 
                0x7000A, 
                self, 
                0
            );
            main_set_bits(BIT_90E, 1);
        } else {
            objData->timerDestroy = 180.0f;
            collectable_collect(self);
        }
        break;

    case OBJ_BoneDust:
    case OBJ_WM_PureMagic:
    case OBJ_beanPickup:
    case OBJ_DIMAlpineRoot2:
    case OBJ_applePickup:
        objData->interactFlags |= Collectable_FLAG_Interaction_Off;
        collectable_collect(self);
        break;

    default:
        //Check for A button press when highlighted with arrow
        if ((self->unkAF & ARROW_FLAG_1_Interacted) == FALSE) {
            break;
        }

        //@recomp: Set an animObjID if the item collection sequence should play
        if (collectable_will_tutorial_be_shown(self, objSetup) && (collectableDef->seqObjectID > 0)) {
            //TODO: make sure all relevant items have an appropriate animObjectID
            gDLL_3_Animation->vtbl->func30(collectableDef->seqObjectID, 0, 0);
        }

        //Have the player scoop up the item, and play a tutorial cutscene if needed
        messageArg = collectable_override_tutorial_gamebitID(self, objSetup); //@recomp: handle cases like the gears/gold
        obj_send_mesg(
            player,
            0x7000A,
            self,
            (void*)messageArg
        );

        //@recomp: optionally show pop-up
        collectable_handle_popup(self, objSetup, objData);        
        break;
    }

    objData->distanceToPlayer = distance;
}

/** 
  * - Adds spinning behaviour for custom `OBJ_WCTrexTooth2` collectable (originally by MusicalProgrammer).
  * - Framerate dependent behaviour fixes.
  */
RECOMP_PATCH void collectable_handle_animation_and_fx(Object* self) {
    s16 id;
    Collectable_Data_Recomp* objData;
    ObjectShadow* shadow;
    s32 opacity;
    CollectableDef* collectableDef;
    s32 temp;

    collectableDef = self->def->collectableDef;
    if (collectableDef == NULL) {
        return;
    }

    objData = self->data;

    //Handle magic (@framerate-dependent)
    if (collectableDef->type == Collectable_Type_Magic) {
        //Spin (@recomp: fix framerate dependency, assuming average N64 gUpdateRate of 2)
        self->srt.yaw   += 25*gUpdateRate; 
        self->srt.pitch += 25*gUpdateRate;
        self->srt.roll  += 25*gUpdateRate;

        //Grow
        if (self->srt.scale < 0.008f) {
            self->srt.scale += 0.0001f * gUpdateRateF; //@recomp: fix framerate dependency
        }

        //Vertical motion + shadow animation
        if (self->velocity.y < 0.0f) {
            self->srt.transl.y += self->velocity.y * gUpdateRate;   //@recomp: fix framerate dependency
            self->velocity.y += 0.015f * gUpdateRateF;              //@recomp: fix framerate dependency
            if (self->velocity.y >= 0.0f) {
                shadows_func_8004D974(1);
                objData->shadowOpacity = 0;
            }
        } else {
            shadow = self->shadow;
            if (shadow) {
                opacity = objData->shadowOpacity + (gUpdateRate * 8);
                if (opacity > OBJECT_OPACITY_MAX) {
                    opacity = OBJECT_OPACITY_MAX;
                }
                objData->shadowOpacity = opacity;
                temp = shadows_calc_opacity(self, shadow);
                shadow->opacity = (temp * (opacity + 1)) >> 8;
            }
        }

        //Sparkles
        if (rand_next(0, 80) == 0) {
            gDLL_17_partfx->vtbl->spawn(self, 0x47, 0, 4, -1, 0);
        }
    }

    id = self->id;
    switch (id) {
    case OBJ_EnergyEgg:
        //Play rattle sound when random timer runs out
        objData->soundTimer -= gUpdateRate;
        if (objData->soundTimer <= 0) {
            objData->pitchAnimate = rand_next(600, 800);
            objData->soundTimer = rand_next(180, 240);
            gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_8FC_Egg_Rattle, MAX_VOLUME, 0, 0, 0, 0);
        }

        //Rapidly oscillate rotational pitch for a basic rattle animation
        self->srt.pitch = objData->pitchAnimate;
        objData->pitchAnimate *= -0.8f; //@framerate-dependent 
                                        //(maybe intentional though, to avoid skips creating a 
                                        // run of frames where the value has the same sign?)
        if ((10 > self->srt.pitch) && (self->srt.pitch > -10)) {
            self->srt.pitch = 0;
            return;
        }
        break;
    case OBJ_DIM2PuzzleKey:
    case OBJ_DIM2GoldKey:
    case OBJ_DIM2SilverKey:
    case OBJ_DIMTruthHorn:
    case OBJ_DIMBridgeCogCol:
        self->srt.yaw += gUpdateRate * 200;
        return;
    case OBJ_SC_golden_nugge:
        if (objData->distanceToPlayer < 200.0f) {
            if (rand_next(0, 10) == 0) {
                gDLL_17_partfx->vtbl->spawn(self, 0x423, 0, 2, -1, 0);
            }
            self->srt.yaw += 182.0f * gUpdateRateF;
            return;
        }
        break;
    case OBJ_WCTrexTooth:
        if (objData->distanceToPlayer < 200.0f) {
            if (rand_next(0, 10) == 0) {
                if (self->modelInstIdx == 0) {
                    gDLL_17_partfx->vtbl->spawn(self, 0x73D, 0, 2, -1, 0);
                } else {
                    gDLL_17_partfx->vtbl->spawn(self, 0x73E, 0, 2, -1, 0);
                }
            }
            self->srt.yaw += 182.0f * gUpdateRateF;
        }
        break;
    case OBJ_WCTrexTooth2: //@recomp: new case
        self->srt.yaw += 182.0f * gUpdateRateF;
        break;
    }
}

/** Optionally show item info pop-up */
RECOMP_PATCH void collectable_collect(Object* self) {
    Collectable_Data_Recomp* objData;
    Collectable_Setup* objSetup;
    CollectableDef* collectableDef;
    Object* player;
    Object* sidekick;
    s16 id;
    LightAction lfxAction;
    Object* foodbag;
    ObjDef* objdef;

    objData = self->data;
    objSetup = (Collectable_Setup*)self->setup;
    player = get_player();
    sidekick = get_sidekick();
    collectableDef = self->def->collectableDef;

    if (collectableDef == NULL) {
        return;
    }

    //@recomp: optionally show pop-up
    collectable_handle_popup(self, objSetup, objData);

    //Set the collection gamebit (usually for making sure the object doesn't reappear)
    if (objData->gamebitCollected != NO_GAMEBIT) {
        main_set_bits(objData->gamebitCollected, TRUE);
    }

    //Increment the counter gamebit (if one is being used)
    if (objSetup->gamebitCount > 0) {
        main_increment_bits(objSetup->gamebitCount);
    }

    switch (collectableDef->type) {
    case Collectable_Type_Inventory:
        switch (self->id) {
        case OBJ_Duster: //NOTE: suggests Dusters were once inventory items, and used this DLL!
        default:
            break;
        case OBJ_DIMAlpineRoot2: 
            gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_506_Chomping_Food, MAX_VOLUME, 0, 0, 0, 0);
            main_set_bits(BIT_3E9, 1);
            self->unkDC = 1;
            objData->rootTimer = 1200;
            return;
        case OBJ_foodbagSmall:
        case OBJ_foodbagMedium:
        case OBJ_foodbagLarge:
            foodbag = ((DLL_210_Player*)player->dll)->vtbl->func66(player, 15);
            ((DLL_IFoodbag*)foodbag->dll)->vtbl->set_capacity(foodbag);
            break;
        }
        break;
    case Collectable_Type_Food:
        foodbag = ((DLL_210_Player*)player->dll)->vtbl->func66(player, 15);
        switch (self->id) {  
        case OBJ_EnergyEgg:      
            ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Dino_Egg);
            obj_free_object_type(self, OBJTYPE_5);
            return;
        case OBJ_applePickup:
            ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Red_Apple);
            obj_free_object_type(self, OBJTYPE_5);
            obj_destroy_object(self);
            return;
        case OBJ_beanPickup:
            ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Blue_Bean);
            break;
        }
        break;
    case Collectable_Type_SidekickA:
        obj_send_mesg(sidekick, 0x70004, self, (void*)(collectableDef->amountRestored + objData->sidekickArgBase));
        break;
    case Collectable_Type_SidekickB:
        obj_send_mesg(sidekick, 0x70005, self, (void*)(collectableDef->amountRestored + objData->sidekickArgBase));
        break;
    case Collectable_Type_Magic:
        ((DLL_210_Player*)player->dll)->vtbl->add_magic(player, collectableDef->amountRestored);
        gDLL_13_Expgfx->vtbl->func5(self);
        gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_8E_Magic_Chime, MAX_VOLUME, 0, 0, 0, 0);
        break;
    case Collectable_Type_Upgrade:
        obj_send_mesg(sidekick, 0x70008, self, (void*)(collectableDef->amountRestored + objData->sidekickArgBase));
        break;
    }

    //Hide shadow
    objdef = self->def;
    self->srt.scale = objdef->scale;
    self->unkDC = 1;
    if (objdef->unk87 & 0x10) {
        lfxAction.unk12.asByte = 2;
        lfxAction.unke = 0;
        lfxAction.unk1b = 0;
        lfxAction.unk10 = self->unkD6;
        gDLL_11_Newlfx->vtbl->func0(self, self, &lfxAction, 0, 0, 0);
    }
}
