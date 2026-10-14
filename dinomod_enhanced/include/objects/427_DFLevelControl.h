#pragma once

#include "PR/ultratypes.h"
#include "game/objects/object.h"

enum DF_CustomObjectGroups {
    DF_ObjGroup_Tunnel_to_MMP = 18,                //For moving the tunnel's objects out of the Lower Falls objGroup and into a new one of their own, so the ropes etc. don't need to be loaded
    DF_ObjGroup_MMP_Door = 19,                     //A new objGroup just for the unused door to MMP
    DF_ObjGroup_Rope_BWC_Detached = 20,            //The original DFropenode setup near BWC, but moved into an objGroup of its own (synced with `DF_ObjGroup2_Lower_Falls` via `DFlevelcontrol`)
    DF_ObjGroup_Rope_BWC_Attached = 21,            //A duplicate of the rope setup near BWC, but already attached (used for restoring state without seq preempt messing up rope's spring dynamics)
    DF_ObjGroup_Rope_Upper_Falls_Detached = 22,    //The original upper falls DFropenode setup, but moved into an objGroup of its own (synced with `DF_ObjGroup14_Middle_and_Upper_Falls` via `DFlevelcontrol`)
    DF_ObjGroup_Rope_Upper_Falls_Attached = 23,    //A duplicate of the upper falls' rope setup, but already attached (used for restoring state without seq preempt messing up rope's spring dynamics)
    DF_ObjGroup_Turbine_Sequence_HitAnimators = 24 //Enables widescreen fixes just while the turbine sequence is active
};
