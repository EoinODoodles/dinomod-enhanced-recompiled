#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "dlls/objects/210_player.h"

#include "recomp/dlls/objects/305_PortalSpellDoor_recomp.h"
#include "sys/print.h"

int func_80058F50(void);

typedef struct {
    ObjSetup base;
    s8 yaw;
    s8 hitsAnimatorID;      //animatorID of the HITS line(s) that should be removed when door is open
    s16 scale;
    s16 pitch;
    s16 gamebitActivated;   //gamebitID set when door is open
} PortalSpellDoor_Setup;

typedef struct {
    Object* portalDoorAnim; //secondary object used during door transformation sequence
    f32 scale;              //the object's scale multiplied by 32.0
    s32 timer;              //countdown to door transformation after player uses Portal Spell
    u32 sequencePlayed : 1; //set at beginning of door transformation sequence
} PortalSpellDoor_Data;

extern Object* PortalSpellDoor_create_anim_obj(Object* self);

// Stop Portal from disabling all spells after you activate it (Originally by MusicalProgrammer)
RECOMP_PATCH void PortalSpellDoor_control(Object* self) {
    Object* player;
    PortalSpellDoor_Data* objData;
    PortalSpellDoor_Setup* objSetup;

    player = get_player();
    objData = self->data;
    objSetup = (PortalSpellDoor_Setup*)self->setup;
    
    //Check if the door transformation sequence has played, otherwise wait for player to use spell
    if (objData->sequencePlayed) {
        self->srt.flags |= OBJFLAG_INVISIBLE;
        
        //Destroy secondary door object
        if (objData->portalDoorAnim != NULL) {
            obj_destroy_object(objData->portalDoorAnim);
            objData->portalDoorAnim = NULL;
        }
        
        objData->sequencePlayed = FALSE; //@recomp: stop this branch from repeating on subsequent ticks

        ((DLL_210_Player*)player->dll)->vtbl->func51(player, -1);

        main_set_bits(objSetup->gamebitActivated, TRUE);

        //Remove HITS line
        if (objSetup->hitsAnimatorID && !func_80058F50()) {
            func_80059038(objSetup->hitsAnimatorID, self->parent, 0);
        }
    } else if ((((DLL_210_Player*)player->dll)->vtbl->func50(player) == BIT_Spell_Portal) && (objData->timer == -1)) {
        //Start countdown to door transformation
        objData->timer = 157;
    }

    //Handle countdown timer
    if (objData->timer != -1) {
        objData->timer -= gUpdateRate;
        STUBBED_PRINTF(" t %i ");
        //Play door transformation sequence
        if (objData->timer < 0) {
            self->unkAF |= 8;
            self->def->scale *= 0.5f;
            objData->portalDoorAnim = PortalSpellDoor_create_anim_obj(self);
            gDLL_3_Animation->vtbl->func31(fsin16_precise(self->srt.yaw) * objData->scale, 0.0f, fcos16_precise(self->srt.yaw) * objData->scale);
            gDLL_3_Animation->vtbl->start_obj_sequence(0, self, -1);
            objData->sequencePlayed = TRUE;
            objData->timer = -1;
        }
    }
}
