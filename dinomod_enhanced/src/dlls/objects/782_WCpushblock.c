#include "modding.h"
#include "recomputils.h"

#include "dlls/engine/27.h"
#include "dlls/objects/782_WCPushBlock.h"
#include "game/objects/setups/51_Generic.h"
#include "sys/dll.h"
#include "sys/objects.h"

#include "recomp/dlls/objects/782_WCPushBlock_recomp.h"

typedef struct {
/*000*/    DLL27_Data unk0;   //Unused: probably DLL27_Data
/*260*/    Object* levelCtrl; //WCLevelControl
/*264*/    f32 limitX;
/*268*/    f32 limitZ;
/*26C*/    u32 soundHandle;
/*270*/    s16 gridX;
/*272*/    s16 gridZ;
/*274*/    u8 state;
/*275*/    u8 moveDirection;
/*276*/    u8 puzzlePieceID;  //The pushblock's identifier, used by WCLevelControl to quickly store which puzzle element is in each grid cell
/*277*/    u8 collidedType;
} WCPushBlock_Data;

//Prevents crash when the Sun Blocks load in Walled City (originally by MusicalProgrammer)
RECOMP_PATCH void WCPushBlock_obj_Setup(Object* self, WCPushBlock_Setup* setup, s32 reset) {
    WCPushBlock_Data* objdata = self->data;
    ObjectHitInfo* hitInfo; //@recomp

    self->opacity = 0;
    self->modelInstIdx = setup->modelIndex;

    //@recomp
    hitInfo = self->objhitInfo;
    hitInfo->unkA0 = setup->modelIndex;

    objdata->puzzlePieceID = setup->puzzlePieceID;
}
