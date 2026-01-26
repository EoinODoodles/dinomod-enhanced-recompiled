#include "modding.h"

#include "dlls/objects/214_animobj.h"
#include "dlls/objects/338_LFXEmitter.h"
#include "dlls/objects/453_CCfirecrystalin.h"
#include "game/objects/object.h"
#include "sys/objmsg.h"

#include "recomp/dlls/objects/452_CCfirecrystal_recomp.h"

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
    FireCrystal_State_Collectable = 0,
    FireCrystal_State_1 = 1,
    FireCrystal_State_2 = 2,
    FireCrystal_State_3 = 3
} CCfirecrystal_States;

extern void CCfirecrystal_free(Object* self, s32 arg1);

RECOMP_PATCH void CCfirecrystal_control(Object* self) {
    CCfirecrystal_Data* objData;
    CCfirecrystal_Setup* objSetup;
    u32 message;

    objSetup = (CCfirecrystal_Setup*)self->setup;
    objData = self->data;
    
    self->opacity = rand_next(0, 56) + 100;
    
    switch (objData->state) {
    case FireCrystal_State_3:
        while (obj_recv_mesg(self, &message, 0, 0)){
            switch (message) {
            case 0x7000B:
                objData->state = FireCrystal_State_1;
                CCfirecrystal_free(self, 0);
            default:
                break;
            }
        } 
        break;
    case FireCrystal_State_Collectable:
        if (func_80032538(self) != 0) {
            main_set_bits(objSetup->gamebitCollected, 1);
            main_increment_bits(BIT_CC_Fire_Crystal);
            objData->state = FireCrystal_State_3;
            self->unkAF = 8;
            self->objhitInfo->unk58 = 0x100;
            obj_send_mesg(get_player(), 0x7000A, self, (void*)0x1EA);
            // @recomp: Render Fire Crystal approximation (Golden nugget) when picking it up (original patch by MusicalProgrammer)
            gDLL_3_Animation->vtbl->func30(OBJ_CCgolden_nugget, NULL, 0);
        }
        break;
    }
}
