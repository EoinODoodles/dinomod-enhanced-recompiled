#include "PR/ultratypes.h"

f32 lerp_float(f32 tValue, f32 start, f32 end){
    return start + (end - start)*tValue;
}
