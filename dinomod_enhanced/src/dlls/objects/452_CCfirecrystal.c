#include "recompconfig.h"
#include "recomputils.h"
#include "configs.h"
#include "modding.h"

#include "game/objects/interaction_arrow.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/gfx/animseq.h"
#include "sys/main.h"
#include "sys/objlib.h"
#include "sys/objmsg.h"
#include "dlls/objects/338_LFXEmitter.h"
#include "dlls/objects/453_CCfirecrystalin.h"

#include "recomp/dlls/objects/452_CCfirecrystal_recomp.h"

#define DEBUG_COLLECTABLE TRUE

#define GAMEBIT_INVENTORY BIT_CC_Fire_Crystal
#define GAMEBIT_TUTORIAL BIT_Tutorial_Fire_Crystal
#define GAMEBIT_TUTORIAL_ANIMOBJ OBJ_CCgolden_nugget

typedef struct {
    ObjSetup base;
    s16 gamebitCollected;
} CCfirecrystal_Setup;

typedef struct {
    u8 state;
    Object* flameObjects[4];        //"CCfirecrystalin" objects (for a flame effect) 
    LightAction* lfxEmitterSetup;
} CCfirecrystal_Data;

typedef enum {
    FireCrystal_State_0_Collectable = 0,    //Can be collected via interaction
    FireCrystal_State_1_Finished = 1,       //Has been collected, freed
    FireCrystal_State_2_Decorative = 2,     //No interaction, intended for CCbeacons
    FireCrystal_State_3_Collected = 3       //Waiting for tutorial to finish via player message
} CCfirecrystal_States;

extern void CCfirecrystal_free(Object* self, s32 arg1);

/** Check whether the item collection sequence will be shown on collecting the crystal */
static int fire_crystal_will_tutorial_be_shown() {
    if (GAMEBIT_TUTORIAL > (NO_GAMEBIT + 1)) {
        return (main_get_bits(GAMEBIT_TUTORIAL) == FALSE);
    } else {
        return TRUE;
    }
}

/** Log info about the collectable */
static void fire_crystal_log_info(Object* self, CCfirecrystal_Setup* objSetup, CCfirecrystal_Data* objData) {
    recomp_printf("\nCollecting %s!\n", self->def->name);
    recomp_printf("Inventory gamebit: %x\n", GAMEBIT_INVENTORY);
    recomp_printf("Collected gamebit: %x\n", objSetup->gamebitCollected);
    recomp_printf("Tutorial gamebit: %x\n", GAMEBIT_TUTORIAL);
    recomp_printf("Tutorial animObjID: %d\n", GAMEBIT_TUTORIAL_ANIMOBJ);
    recomp_printf("Show tutorial: %s\n", fire_crystal_will_tutorial_be_shown() ? "YES" : "NO");
    recomp_printf("Current count: %d\n", main_get_bits(BIT_CC_Fire_Crystal));
    recomp_printf("Icon gamebit: %x\n", GAMEBIT_INVENTORY);
}

/** Optionally shows an item info pop-up when collecting the item (not shown when the tutorial sequence appears instead) */
static void fire_crystal_handle_popup(Object* self, CCfirecrystal_Setup* objSetup, CCfirecrystal_Data* objData) {
    s8 count;
 
    //Log info about the collectable
    #if DEBUG_COLLECTABLE
    fire_crystal_log_info(self, objSetup, objData);
    #endif

    //Do nothing if the mod config isn't enabled
    if (recomp_get_config_u32("cmdmenu_info_popup_expand") == FALSE) {
        return;
    }

    //Don't show the pop-up if the item collection tutorial box sequence will be shown instead
    if (fire_crystal_will_tutorial_be_shown()) {
        return;
    }

    //Show the item collection pop-up
    count = main_get_bits(BIT_CC_Fire_Crystal);
    if (count) {
        gDLL_1_cmdmenu->vtbl->info_show(
            BIT_CC_Fire_Crystal,
            INFO_POPUP_DURATION,
            count
        );
    }
}

/**
  * - Set an animObj to show when collected (using a Shiny Nugget as a placeholder) (originally by MusicalProgrammer)
  * - Delete self after being collected, to avoid collision persisting.
  * - Optionally show an item info pop-up upon collection (after seeing tutorial sequence).
  */
RECOMP_PATCH void CCfirecrystal_control(Object* self) {
    CCfirecrystal_Data* objData;
    CCfirecrystal_Setup* objSetup;
    u32 message;

    objSetup = (CCfirecrystal_Setup*)self->setup;
    objData = self->data;
    
    // @recomp: Destroy self when lifetime runs out
    if (self->unkE0) {
        self->unkE0 -= gUpdateRate * 3;
        if (self->unkE0 <= 0) {
            CCfirecrystal_free(self, 0);
            obj_destroy_object(self);
            return;
        }
    }

    self->opacity = rand_next(0, 56) + 100;
    
    switch (objData->state) {
    case FireCrystal_State_3_Collected:
        while (obj_recv_mesg(self, &message, 0, 0)){
            switch (message) {
            case 0x7000B:
                objData->state = FireCrystal_State_1_Finished;
                CCfirecrystal_free(self, 0);
                //@recomp: delete via State 1
            default:
                break;
            }
        } 
        break;
    case FireCrystal_State_1_Finished: //@recomp: handle state
        //Delete after collection
        self->unkE0 = 1;
        break;
    case FireCrystal_State_0_Collectable:
        //Handle being collected via player interaction
        if (func_80032538(self) == FALSE) {
            break;
        }

        main_set_bits(objSetup->gamebitCollected, 1);
        main_increment_bits(GAMEBIT_INVENTORY);
        objData->state = FireCrystal_State_3_Collected;
        self->unkAF = ARROW_FLAG_8_No_Targetting;
        self->objhitInfo->unk58 = 0x100;

        // @recomp: Render Fire Crystal approximation (Golden nugget) when picking it up (original patch by MusicalProgrammer)
        if (fire_crystal_will_tutorial_be_shown()) {
            gDLL_3_Animation->vtbl->set_variable_obj(GAMEBIT_TUTORIAL_ANIMOBJ, NULL, 0);
        }

        //Have the player scoop up the item, and play a tutorial cutscene if needed
        obj_send_mesg(
            get_player(), 
            0x7000A, 
            self, 
            (void*)GAMEBIT_TUTORIAL
        );

        //@recomp: optionally show pop-up
        fire_crystal_handle_popup(self, objSetup, objData);
        break;
    }
}
