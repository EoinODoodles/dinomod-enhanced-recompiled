/** Helper functions for working with game Objects */

#include "object_util.h"
#include "common_objsetups.h"
#include "recomputils.h"

extern s16 *gFile_OBJINDEX;

/** Convert an OBJINDEX index (decomp OBJ_* enum) into an OBJECTS index */
s32 objindex_to_object_id(s32 objIndex){
    if (!gFile_OBJINDEX){
        recomp_eprintf("Error: gFile_OBJINDEX not initialised!\n");
        return -1;
    }
    return gFile_OBJINDEX[objIndex];
}

/** Convert from Dinosaur Planet's -0x8000 to 0x8000 angle system to degrees (-180 to 180) */
f32 dp_angle_to_degrees(s16 dpAngle){
    return ((f32)dpAngle / 0x8000)*180;
}

/** Given a MAPS header file and its ObjSetup file, returns a pointer to the end of the 
  * first group of ObjSetups that aren't in an Object Group container. */
ObjSetup *maps_find_generic_group_endpoint(MapHeader *header, ObjSetup *mapsObjSetups){
    s32 i;
    ObjSetup* setup = mapsObjSetups;

    for (i = 0; i < header->objectInstanceCount; i++) {
        setup = (ObjSetup*)((u32)setup + (setup->quarterSize << 2));
        if (setup->loadFlags & OBJSETUP_LOAD_IN_MAP_OBJGROUP && setup->mapObjGroup != 0) {
            return setup;
            break;
        }
    }

    return NULL;
}

/** Given a MAPS header file and its ObjSetup file, returns a pointer to the end of a specific Object Group container. */
ObjSetup *maps_find_object_group_endpoint(MapHeader *header, ObjSetup *mapsObjSetups, u8 objectGroupID){
    s32 i;
    ObjSetup* setup = mapsObjSetups;
    ObjSetup* lastInGroup = NULL;

    for (s32 i = 0; i < header->objectInstanceCount; i++) {
        if (setup->loadFlags & OBJSETUP_LOAD_IN_MAP_OBJGROUP && setup->mapObjGroup == objectGroupID) {
            lastInGroup = setup;
        } else if (lastInGroup != NULL) {
            break;
        }

        setup = objsetup_next(setup);
    }

    return lastInGroup;
}

/** Returns a pointer to the end of an ObjSetup struct. 
  *
  * ObjSetups' lengths are object-specific, so traversing them can be a little cumbersome without a helper of this kind.
  */
ObjSetup *objsetup_next(ObjSetup* setup){
    return (ObjSetup*)((u32)setup + (setup->quarterSize << 2));
}

/** 
  * Returns a mode flag for a HitAnimator, based on a few common configurations for them. 
  *
  * `removeWhenSet` removes the line/shape when the HitAnimator's gamebit is set, rather than before it's set.
  * `isBlocksAnimator` decides whether the HitAnimator is targetting a Blocks shape rather than a Hits line.
  * `fadeBlocks` decides whether or not to fade a Blocks shape in/out when affecting its state. 
  */
u8 hitanimator_configure_mode_flags(_Bool removeWhenSet, _Bool isBlocksAnimator, _Bool fadeBlocks) {
    u8 mode = 0;

    if (removeWhenSet) {
        mode |= HitAnimator_Mode_Invert;
    }

    if (isBlocksAnimator) {
        mode |= HitAnimator_Mode_BLOCKS;
        if (!fadeBlocks) {
            mode |= HitAnimator_Mode_No_Fade;
        }
    } else {
        mode |= HitAnimator_Mode_HITS;
    }

    return mode;
}
