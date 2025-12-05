/** Helper functions for working with game Objects */

#include "object_util.h"

/** Convert from Dinosaur Planet's -0x8000 to 0x8000 angle system to degrees (-180 to 180) */
static f32 dp_angle_to_degrees(s16 dpAngle){
    return ((f32)dpAngle / 0x8000)*180;
}
