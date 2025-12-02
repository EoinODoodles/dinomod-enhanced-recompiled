/** Helper functions for working with game Objects */

#include "object_util.h"

Object* find_object_by_uID(s32 uID){
    Object** objects;
    s32 numObjects;
    s32 i;

    objects = get_world_objects(&i, &numObjects);
    for (i = 0; i < numObjects; i++){
        if (objects[i]->setup->uID == uID){
            return objects[i];
        }
    }

    return NULL;
}
