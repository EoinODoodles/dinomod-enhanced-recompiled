#include "modding.h"
#include "recomputils.h"

#include "sys/dll.h"
#include "sys/objects.h"

#include "recomp/dlls/objects/782_WCpushblock_recomp.h"

typedef struct {
/*000*/    s8 unk0[0x260 - 0];
/*260*/    Object* unk260; //WCLevelControl
/*264*/    f32 unk264;
/*268*/    f32 unk268;
/*26C*/    u32 unk26C;
/*270*/    s16 unk270;
/*272*/    s16 unk272;
/*274*/    u8 unk274;
/*275*/    u8 unk275;
/*276*/    u8 unk276;
/*277*/    u8 unk277;
} WCPushBlock_Data;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 unk18;
/*19*/ s8 modelIndex;
/*1A*/ s16 unk1A;
} WCPushBlock_Setup;

//Prevents crash when the Sun Blocks loads in Walled City (originally by MusicalProgrammer)
RECOMP_PATCH void dll_782_setup(Object* self, WCPushBlock_Setup* setup, s32 arg2) {
    WCPushBlock_Data* objdata = self->data;
    ObjectHitInfo* hitInfo; //@recomp
    
    self->unk_0x36 = 0;
    self->modelInstIdx = setup->modelIndex;

    //@recomp
    hitInfo = self->objhitInfo;
    hitInfo->unk_0xa0 = setup->modelIndex;

    objdata->unk276 = setup->unk1A;
}
