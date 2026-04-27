#include "modding.h"
#include "recompconfig.h"
#include "dll_util.h"

#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "PR/os.h"
#include "types.h"
#include "game/objects/object.h"
#include "dlls/engine/56_putdown.h"
#include "dlls/objects/common/foodbag.h"

#include "engine/56_putdown.h"

#include "recomp/dlls/engine/56_putdown_recomp.h"

extern u32 putdown_get_foodID_from_foodType(s32 arg0);

/** Checks whether a particular food item can be added to the food bag (without actually adding it) */
int putdown_can_food_be_added(s32 foodType, s32 capacity, FoodbagContents* bagSlots, FoodbagItem* foodDefs) {
    s8 emptySlotIndex;
    s8 placingIndex;
    u8 slotsNeeded;
    u32 ID;

    emptySlotIndex = 0;
    
    ID = putdown_get_foodID_from_foodType(foodType);    
    
    //Iterate over bag's slots and return FALSE if all slots occupied
    while (bagSlots->foodType[emptySlotIndex]) {
        emptySlotIndex++; 
        if (emptySlotIndex == capacity){
            return FALSE;
        }
    }
    
    //Get the number of slots occupied by the food (usually 1, but 2 for fish)
    slotsNeeded = foodDefs[ID].slotsUsed;
    if (slotsNeeded > (s8) (capacity - emptySlotIndex)) {
        return FALSE;
    }

    return TRUE;
}
