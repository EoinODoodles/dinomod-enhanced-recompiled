#include "modding.h"

#include "PR/ultratypes.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/607_WL_LevelControl.h"
#include "dlls/objects/common/foodbag.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/segment_1460.h"
#include "dll.h"

#include "recomp/dlls/objects/607_WL_LevelControl_recomp.h"

/*0x14*/ extern u8 dInitSpirit4Visit;
RECOMP_PATCH void WL_LevelControl_setup5_tick(Object* self) {
    WL_LevelControl_Data* objData;
    f32 distance;
    Object* guardClaw;
    Object** objects;
    s32 count;
    s16 i;
    s16 lastUsedSpell;
    ObjSetup *someObjsetup;
    Object* player;
    
    count = 0;
    distance = 10000.0f;
    player = get_player();
    objData = self->data;

    //Set up the visit (only runs once)
    if (dInitSpirit4Visit && (main_get_bits(BIT_318) == FALSE)) {
        //Ensure the player has relevant spells
        main_set_bits(BIT_Spell_Projectile, 1);
        main_set_bits(BIT_Spell_Forcefield, 1);
        main_set_bits(BIT_Spell_Illusion, 1);

        //Restore some magic and make sure the player has the 4th Spirit
        ((DLL_210_Player*)player->dll)->vtbl->func39(player, SPIRIT_INDEX(4), TRUE);
        ((DLL_210_Player*)player->dll)->vtbl->add_magic(player, 20);

        main_set_bits(BIT_WM_Setup5_Sabre_Dock_Pushed_Crate_Onto_GuardClaw, 0);

        dInitSpirit4Visit = FALSE;
    }

    //Disable HITS line (TO-DO: find where this line is)
    if (main_get_bits(BIT_2DB)) {
        func_80059038(0x18, 0, 0);
    }

    //Delete the dock's GuardClaw after dropping a crate from above
    if (main_get_bits(BIT_WM_Setup5_Sabre_Dock_Pushed_Crate_Onto_GuardClaw)) {
        main_set_bits(BIT_CFExplodeTunnel_Trigger_31B6F, 1);
        main_set_bits(BIT_WM_Setup5_Sabre_Dock_Pushed_Crate_Onto_GuardClaw, 0);

        guardClaw = obj_get_nearest_type_to(OBJTYPE_4, self, &distance);
        if (guardClaw != NULL) {
            //@bug: may potentially delete a Skeetla instead, since they're also objType4
            obj_destroy_object(guardClaw);
        }

        objData->timer = 30;
    }

    //Search through the objects, and delete the hall's SharpClaw and GuardClaw
    if (main_get_bits(BIT_WM_Setup5_Sabre_Hall_Delete_Claws)) {
        objects = obj_get_all_of_type(OBJTYPE_4, &count);
        for (i = 0; i < count; i++) {
            someObjsetup = objects[i]->setup;
            if ((someObjsetup->uID == 0x296E) ||    //SharpClaw
                (someObjsetup->uID == 0x296F)       //GuardClaw
            ) {
                obj_destroy_object(objects[i]);
            }
        }
        main_set_bits(BIT_WM_Setup5_Sabre_Hall_Delete_Claws, 0);
    }

    //Handle Sabre entering the hall with the GuardClaw
    if (main_get_bits(BIT_WM_Setup5_Sabre_Entered_GuardClaw_Hall)) {
        lastUsedSpell = ((DLL_210_Player*)player->dll)->vtbl->func50(player);

        //Warp the player away if they're not using the Illusion or Forcefield Spells
        if ((lastUsedSpell != BIT_Spell_Illusion) && 
            (lastUsedSpell != BIT_Spell_Forcefield) && 
            (main_get_bits(BIT_WM_Setup5_Sabre_Hall_Disable_GuardClaw_Warp) == 0)) {
            // @recomp: Instead of warping, play a cutscene for the GuardClaw in Warlock Mountain, 
            //          to avoid crashing the game after you deposit the spirit. 
            //          The set bits plays the cutscene. (originally by MusicalProgrammer)
            //warpPlayer(WARP_WM_SABRE_KRAZOA_CORRIDOR, /*fadeToBlack=*/FALSE);
            main_set_bits(0x1DE, 1);
        }
        main_set_bits(BIT_WM_Setup5_Sabre_Entered_GuardClaw_Hall, 0);
    }

    /* Handle removing the GuardClaw hall's warp-away behaviour (and deleting the SharpClaw)

       NOTE: BIT_2FA intended to be set upon depositing Spirit 4?
             Doesn't seem to get set in practice.
    */
    if (main_get_bits(BIT_WM_Setup5_Sabre_Hall_GuardClaw_Gone)) {
        if (main_get_bits(BIT_WM_Setup5_Sabre_Hall_Delete_Claws) == 0) {
            main_set_bits(BIT_WM_Setup5_Sabre_Hall_Delete_Claws, 1);
        }

        objData->timer -= (s16)gUpdateRate;
        if (objData->timer <= 0) {
            objData->timer = 0;
            main_set_bits(BIT_WM_Setup5_Sabre_Hall_GuardClaw_Gone, 0);
            main_set_bits(BIT_WM_Setup5_Sabre_Hall_Disable_GuardClaw_Warp, 1);
            objData->timer = 30;
        }
    }
}

/*0x18*/ extern u8 dInitSpirit6Visit;
RECOMP_PATCH void WL_LevelControl_setup6_tick(Object* self) {
    Object* player;
    Object* foodbag;

    player = get_player();

    //Set up the visit (only runs once)
    if (dInitSpirit6Visit && (main_get_bits(BIT_Play_Seq_020D) == FALSE)) {
        //Add 10 green apples, and 1 red and brown apple to the food bag (@debug code related to Randorn?)
        foodbag = ((DLL_210_Player*)player->dll)->vtbl->func66(player, 15);
        ((DLL_IFoodbag*)foodbag->dll)->vtbl->set_capacity(foodbag);
        ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Green_Apple);
        ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Green_Apple);
        ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Green_Apple);
        ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Green_Apple);
        ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Green_Apple);
        ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Green_Apple);
        ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Green_Apple);
        ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Green_Apple);
        ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Green_Apple);
        ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Green_Apple);
        ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Red_Apple);
        ((DLL_IFoodbag*)foodbag->dll)->vtbl->collect_food(foodbag, FOOD_Brown_Apple);

        //Ensure the player has relevant spells
        main_set_bits(BIT_Spell_Projectile, 1);
        main_set_bits(BIT_Spell_Forcefield, 1);
        // @recomp: Stop Warlock Mountain from giving you back one of the Spirits. 
        //          Possibly unwanted debug code? (originally by MusicalProgrammer)
        //((DLL_210_Player*)player->dll)->vtbl->func39(player, SPIRIT_INDEX(6), TRUE);

        dInitSpirit6Visit = FALSE;
    }
}

/*0x1C*/ extern u8 dInitSpirit7Visit;
RECOMP_PATCH void WL_LevelControl_setup7_tick(Object* self) {
    WL_LevelControl_Data* objData;
    Object* player;

    get_player();
    objData = (WL_LevelControl_Data*)self->data;

    //Set up the visit (only runs once)
    if (dInitSpirit7Visit && (main_get_bits(BIT_Play_Seq_020D) == FALSE)) {
        //Ensure the player has relevant spells
        main_set_bits(BIT_Spell_Projectile, 1);
        main_set_bits(BIT_Spell_Forcefield, 1);

        player = get_player();
        // @recomp: Stop Warlock Mountain from giving you back one of the Spirits.
        //          Possibly unwanted debug code? (originally by MusicalProgrammer)
        //((DLL_210_Player*)player->dll)->vtbl->func39(player, SPIRIT_INDEX(7), TRUE);
        ((DLL_210_Player*)player->dll)->vtbl->add_magic(player, 20);

        dInitSpirit7Visit = FALSE;

        objData->timer = 1;

        //Use envFxActions
        func_80000860(self, self, 0x32, 0);
        func_80000860(self, self, 0x33, 0);
        
        main_set_bits(BIT_221, 1);
    }

    if (main_get_bits(BIT_WM_Setup5_Interval_Behaviour)) {
        /* Over 11.666 seconds, set BIT_36D at rapid intervals:
           starting with period of 0.5s, and getting one frame more frequent each time.

           TO-DO: what's this gamebit used for?
        */
        if (objData->intervalBehaviourTimer > 0) {
            objData->intervalBehaviourTimer -= (s16)gUpdateRate;

            if (objData->timer != 0) {
                objData->timer -= (s16)gUpdateRate;
                if (objData->timer <= 0) {
                    main_set_bits(BIT_36D, 1);
                    if (objData->interval > 10) {
                        objData->interval -= 1;
                    }
                    objData->timer = objData->interval;
                }
            }
        }
    }

    //Open/close the door to Randorn's hall erratically
    //NOTE: Randorn's door isn't set up to appear in setup 7, so this behaviour can't be seen
    if (rand_next(0, 30) == 0) {
        main_set_bits(BIT_WM_Randorn_Door_OpenClose, 1);
    }
    if (rand_next(0, 10) == 0) {
        main_set_bits(BIT_WM_Randorn_Door_OpenClose, 0);
    }
}
