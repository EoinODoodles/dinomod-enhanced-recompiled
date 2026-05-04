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
#include "dlls/objects/common/collectable.h"
#include "dlls/objects/523_SCcollectables.h"

#include "recomp/dlls/objects/523_SCcollectables_recomp.h"
#include "sys/objtype.h"

#define DEBUG_COLLECTABLE TRUE

typedef struct {
    f32 distanceToPlayer;
    f32 interactionRadius;
    s16 gamebitCollected;
    s16 gamebitCount;
    s16 gamebitFall;            //(Optional) Collectable falls from tree when set
    u8 fallFlags;               //For Gold Nugget falling out of tree and bouncing on ground
    s8 delayInteractionTimer;
} SCCollectables_Data;

/** For the Gold Nugget falling out of tree and bouncing on ground */
typedef enum {
    FLAG_Fall_Start = 1,
    FLAG_Fall_Finished = 2
} SCcollectables_FallFlags;

/** Collectable's height offset from ground */
#define REST_HEIGHT 9.0f

extern void SCcollectables_handle_motion(Object* self, u8 alreadyOnGround);
extern void SCcollectables_collect(Object* self);

#define FLAG_Collected 4

/** Ensures Shiny Nuggets' item collection sequence only plays the first time one of these items is collected.
  *
  * (This can also be switched off if preferred, so it still shows the sequence each time)
  */
static s16 SCcollectables_override_tutorial_gamebitID(Object* self, Collectable_Setup* objSetup) {
    s16 defaultGamebit;
    
    if (self->def && self->def->collectableDef) {
        defaultGamebit = self->def->collectableDef->collectMessage << 0x10;
    } else {
        defaultGamebit = NO_GAMEBIT;
    }

    //Check if repeat collections should be overridden
    if (recomp_get_config_u32("cmdmenu_info_popup_expand") < POPUP_CONFIG_OVERRIDE_TUTORIAL_ON_REPEAT) {
        return defaultGamebit;
    }

    switch (self->id) {
    case OBJ_SC_golden_nugge:
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
static int SCcollectables_will_tutorial_be_shown(Object* self, Collectable_Setup* objSetup) {
    s16 tutorialGamebit;

    tutorialGamebit = SCcollectables_override_tutorial_gamebitID(self, objSetup);
    if (tutorialGamebit > (NO_GAMEBIT + 1)) {
        return (main_get_bits(tutorialGamebit) == FALSE);
    } else {
        return TRUE;
    }
}

/** For the item info pop-up: figures out what count to show when collecting an item */
static s8 SCcollectables_get_cmdmenu_popup_item_count(Object* self, Collectable_Setup* objSetup, SCCollectables_Data* objData) {
    s8 count = 0;

    switch (self->id) {
    case OBJ_SC_golden_nugge:
        //Show how many have been found in total
        count += main_get_bits(BIT_Gold_Nugget_GP);
        count += main_get_bits(BIT_Gold_Nugget_LFV);
        count += main_get_bits(BIT_Gold_Nugget_CC);
        return count + 1;
    }

    //NOTE: `count + 1` is used because the item that's currently being collected hasn't had its gamebit set/incremented yet

    return 0;
}

/** For the item info popup: returns the gamebitID to use for finding the item's icon texture */
static s16 SCcollectables_get_popup_icon_gamebit(Object* self, Collectable_Setup* objSetup) {
    u8 stackableGold;

    //Handle special cases
    switch (self->id) {
    case OBJ_SC_golden_nugge:
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
static void SCcollectables_log_info(Object* self, Collectable_Setup* objSetup, SCCollectables_Data* objData) {
    recomp_printf("\nCollecting %s!\n", self->def->name);
    recomp_printf("Inventory gamebit: %x\n", objSetup->gamebitCount);
    recomp_printf("Collected gamebit: %x\n", objData->gamebitCollected);
    recomp_printf("Tutorial gamebit: %x\n", objSetup->animMessage);
    recomp_printf("Tutorial animObjID: %d\n", self->def->collectableDef->seqObjectID);
    recomp_printf("Show tutorial: %s\n", SCcollectables_will_tutorial_be_shown(self, objSetup) ? "YES" : "NO");
    recomp_printf("Current count: %d\n", SCcollectables_get_cmdmenu_popup_item_count(self, objSetup, objData));
    recomp_printf("Icon gamebit: %x\n", SCcollectables_get_popup_icon_gamebit(self, objSetup));
}

/** Optionally shows an item info pop-up when collecting the item (not shown when the tutorial sequence appears instead) */
static void SCcollectables_handle_popup(Object* self, Collectable_Setup* objSetup, SCCollectables_Data* objData) {
    s8 count;
    s16 iconGamebit;
 
    //Log info about the collectable
    #if DEBUG_COLLECTABLE
    SCcollectables_log_info(self, objSetup, objData);
    #endif

    //Do nothing if the mod config isn't enabled
    if (recomp_get_config_u32("cmdmenu_info_popup_expand") == FALSE) {
        return;
    }

    //Don't show the pop-up if the item collection tutorial box sequence will be shown instead
    if (SCcollectables_will_tutorial_be_shown(self, objSetup)) {
        return;
    }

    //Show the item collection pop-up
    count = SCcollectables_get_cmdmenu_popup_item_count(self, objSetup, objData);
    iconGamebit = SCcollectables_get_popup_icon_gamebit(self, objSetup);
    if (count && (iconGamebit > 0)) {
        gDLL_1_cmdmenu->vtbl->info_show(
            iconGamebit,
            INFO_POPUP_DURATION,
            count
        );
    }
}

RECOMP_PATCH void SCcollectables_setup(Object* self, Collectable_Setup* objsetup, UNK_TYPE_32 arg2) {
    SCCollectables_Data* objdata;

    objdata = self->data;

    obj_add_object_type(self, OBJTYPE_5);
    obj_init_mesg_queue(self, 2);

    self->srt.yaw = objsetup->yaw << 8;
    self->srt.pitch = objsetup->pitch << 8;
    self->srt.roll = objsetup->roll << 8;
    self->srt.scale = self->def->scale;
    self->modelInstIdx = objsetup->modelIdx;
    self->velocity.y = 0.0f;
    
    if (self->objhitInfo) {
        self->objhitInfo->unk52 = objsetup->objHitsValue;
    }
    
    objdata->gamebitCount = objsetup->gamebitCount;
    objdata->delayInteractionTimer = 60;
    objdata->gamebitFall = objsetup->gamebitSecondary;

    //Check if already knocked out of tree
    if (objdata->gamebitFall != NO_GAMEBIT) {
        if (main_get_bits(objdata->gamebitFall)) {
            SCcollectables_handle_motion(self, TRUE);
        }
    } else {
        SCcollectables_handle_motion(self, TRUE);
    }
    
    //Check if already collected
    objdata->gamebitCollected = objsetup->gamebitCollected;
    if (objdata->gamebitCollected != NO_GAMEBIT) {
        self->unkDC = main_get_bits(objdata->gamebitCollected);
    } else {
        self->unkDC = 0;
    }
    
    //@recomp: handle deletion when already collected
    if (self->unkDC) {
        self->unkE0 = 1;
        return;
    }
    
    //Set interaction radius based on objDef collectable data, or objDef lock-on data
    if (self->def->collectableDef) {
        objdata->interactionRadius = self->def->collectableDef->interactionRadius;
    } else {
        objdata->interactionRadius = 50.0f;
    }
    if (self->def->lockdata) {
        objdata->interactionRadius = self->def->lockdata->interactRadius * 4;
    }

    //Set shadow flags
    if (self->shadow) {
        self->shadow->flags |= OBJ_SHADOW_FLAG_TOP_DOWN | OBJ_SHADOW_FLAG_CUSTOM_DIR;
    }
}

RECOMP_PATCH void SCcollectables_control(Object* self) {
    SCCollectables_Data* objData;
    s32 hitSphereID;
    s32 hitDamage;
    Object* messageSender;
    Object* player;
    Object* hitBy;
    CollectableDef* collectableDef;
    f32 distance;
    u32 messageArg;
    s32 index;
    /* RECOMP */
    Collectable_Setup* objSetup;

    objData = self->data;
    
    self->unkAF |= ARROW_FLAG_8_No_Targetting;

    //Destroy self when lifetime runs out
    if (self->unkE0) {
        self->unkE0 -= gUpdateRate * 3;
        if (self->unkE0 <= 0) {
            obj_destroy_object(self);
            return;
        }
    }

    //Check if should fall from tree
    if (objData->gamebitFall != NO_GAMEBIT && main_get_bits(objData->gamebitFall)) {
        objData->fallFlags |= FLAG_Fall_Start;
    }
    
    //Decrement interaction delay
    objData->delayInteractionTimer -= gUpdateRate;
    if (objData->delayInteractionTimer < 0) {
        objData->delayInteractionTimer = 0;
    }
    
    //Check for collection message (set gamebits and hide self)
    if (objData->fallFlags & FLAG_Collected) {
        while (obj_recv_mesg(self, &messageArg, &messageSender, 0)){
            if (messageArg == 0x7000B) {
                SCcollectables_collect(self);
                recomp_printf("\nMessage received, collected!\n");
                self->unkE0 = 1;
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
    
    //Disable interaction until fall completed
    if (objData->fallFlags & FLAG_Fall_Finished) {
        self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
    } else {
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
    }

    //Handle falling/bouncing behaviour (Gold Nugget falling from tree)
    if ((objData->fallFlags & FLAG_Fall_Start) && !(objData->fallFlags & FLAG_Fall_Finished)) {
        SCcollectables_handle_motion(self, 0);
    }
    
    //Handle projectile collisions (bounce sound and sparkles fly out)
    if (func_80025F40(self, &hitBy, &hitSphereID, &hitDamage)) {
        for (index = 20; index > 0; index--){
            gDLL_17_partfx->vtbl->spawn(self, 0x424, 0, 2, -1, 0);
        }
        gDLL_6_AMSFX->vtbl->play(self, SOUND_613_Gold_Bounce, MAX_VOLUME, 0, 0, 0, 0);
    }

    //@recomp: grey out the arrow if it can't be collected yet
    if (objData->delayInteractionTimer > 0) {
        self->unkAF |= ARROW_FLAG_10_Greyed_Out;
    } else {
        self->unkAF &= ~ARROW_FLAG_10_Greyed_Out;
    }

    //@recomp: Handle player pressing A when arrow over collectable
    if (self->unkAF & ARROW_FLAG_1_Interacted) {
        collectableDef = self->def->collectableDef;
        if (!collectableDef) {
            return;
        }

        objSetup = (Collectable_Setup*)self->setup;
        if (!objSetup) {
            return;
        }

        player = get_player();
        if (!player) {
            return;
        }
        
        //Make sure the player is nearby
        distance = vec3_distance(&self->globalPosition, &player->globalPosition);
        objData->distanceToPlayer = distance;
        if (distance >= objData->interactionRadius) {
            return;
        }

        //@recomp: Set an animObjID if the item collection sequence should play
        if (SCcollectables_will_tutorial_be_shown(self, objSetup) && (collectableDef->seqObjectID > 0)) {
            gDLL_3_Animation->vtbl->func30(collectableDef->seqObjectID, 0, 0);
        }

        //Have the player scoop up the item, and play a tutorial cutscene if needed
        // outMessage = collectableDef->collectMessage << 0x10;
        messageArg = SCcollectables_override_tutorial_gamebitID(self, objSetup); //@recomp
        obj_send_mesg(player, 
            0x7000A, 
            self, 
            (void*)messageArg
        );

        //@recomp: optionally show pop-up
        SCcollectables_handle_popup(self, objSetup, objData);
        
        //@recomp: track having been collected
        objData->fallFlags |= FLAG_Collected;
    }
}

RECOMP_PATCH void SCcollectables_collect(Object* self) {
    SCCollectables_Data* objData = self->data;
    Collectable_Setup* objSetup;

    objSetup = (Collectable_Setup*)self->setup;
    if (!self->def->collectableDef) {
        return;
    }
    
    //Set collection gamebit (if it's in use)
    if (objData->gamebitCollected != NO_GAMEBIT) {
        main_set_bits(objData->gamebitCollected, 1);
    }

    //Increment counter gamebit (if it's in use)
    if (objSetup->gamebitCount > 0) {
        main_increment_bits(objSetup->gamebitCount);
    }
    
    //Hide self
    self->srt.scale = self->def->scale;
    self->unkDC = 1;
}
