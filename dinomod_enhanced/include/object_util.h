#pragma once

/** Helper functions for working with game Objects */

#include "common.h"

#define MAP_ACT(actIndex) (1 << (actIndex - 1))

#define FSA_NEXTSTATE_SYNC(state) (state + 1)
#define FSA_NEXTSTATE_ASYNC(state) (-state)

f32 dp_angle_to_degrees(s16 dpAngle);
s32 objindex_to_object_id(s32 objIndex);
ObjSetup *maps_find_generic_group_endpoint(MapHeader *header, ObjSetup *mapsObjSetups);
ObjSetup *maps_find_object_group_endpoint(MapHeader *header, ObjSetup *mapsObjSetups, u8 objectGroupID);
ObjSetup *objsetup_next(ObjSetup* setup);

u8 hitanimator_configure_mode_flags(_Bool removeWhenSet, _Bool isBlocksAnimator, _Bool fadeBlocks);
