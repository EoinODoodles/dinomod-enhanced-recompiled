#include "modding.h"
#include "recomputils.h"

#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/print.h"
#include "dlls/objects/290_magicplant.h"
#include "dlls/objects/291_magicdust.h"

#include "recomp/dlls/objects/291_MagicDust_recomp.h"

/**
  * @recomp: Fix a potential crash in the MagicPlant object's print function,
  * by clearing its references to this MagicDust object when freed.
  */
RECOMP_PATCH void MagicDust_free(Object *self, s32 a1) {
	Object* parent;
	MagicPlant_Data* magicPlantData;

	//Check if the MagicDust is parented
	if (!self || !self->unkC4){
		return;
	}
	parent = self->unkC4;

	//Do nothing if the gem's parent object is deleted
	if (parent->unkB0 & 0x40){
		return;
	}

	//If the gem's parent is a MagicPlant, check if it's still referencing this gem
	if (parent->id == OBJ_MagicPlant){
		magicPlantData = parent->data;
		if (magicPlantData && magicPlantData->magic == self){
			//Clear the reference to avoid crashes in the MagicPlant's print function
			magicPlantData->magic = NULL;
		}
	}
}
