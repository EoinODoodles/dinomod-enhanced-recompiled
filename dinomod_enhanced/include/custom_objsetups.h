#pragma once

#include "game/objects/object.h"

typedef struct {
    ObjSetup base;
    u8 yaw;
} SHBarrel_Setup;

typedef struct {
    ObjSetup base;
    u8 yaw;
    u8 searchDistance;  //Creates a barrel if none are found inside this radius (stored divided by 4)
    s16 gamebitStop;    //Stops creating barrels if this gamebit is set
} SHBarrelcreator_Setup;
