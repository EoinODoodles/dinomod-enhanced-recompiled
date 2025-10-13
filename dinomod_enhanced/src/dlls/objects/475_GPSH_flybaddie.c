#include "modding.h"

#include "game/gamebits.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/main.h"
#include "sys/memory.h"
#include "sys/objects.h"
#include "sys/rand.h"
#include "functions.h"
#include "dll.h"

#include "recomp/dlls/objects/475_GPSH_flybaddie_recomp.h"

typedef struct {
    ObjSetup base;
    u8 _unk18[12];
} UnkObjSetup;

RECOMP_PATCH void GPSH_flybaddie_func_7F8(Object* self) {
    ObjSetup* objsetup;
    Object* obj;
    Object* player;
    f32 dirVec[3];
    f32 magnitude;

    player = get_player();
    self->positionMirror.x = self->srt.transl.x;
    self->positionMirror.y = self->srt.transl.y;
    self->positionMirror.z = self->srt.transl.z;
    objsetup = mmAlloc(sizeof(UnkObjSetup), ALLOC_TAG_OBJECTS_COL, NULL);
    bzero(objsetup, sizeof(UnkObjSetup));
    // @recomp: Set obj ID to the (hopefully) intended object. In the vanilla code, the objId field
    //          is not set and happens to end up spawning OBJ_Sabre objects.
    objsetup->objId = OBJ_WGSH_projball;
    objsetup->loadParamA = 2;
    objsetup->loadParamB = 1;
    objsetup->loadDistance = 0xFF;
    objsetup->fadeDistance = 0xFF;
    objsetup->x = self->srt.transl.x;
    objsetup->y = self->srt.transl.y;
    objsetup->z = self->srt.transl.z;
    obj = obj_create(objsetup, OBJSETUP_FLAG_1, -1, -1, NULL);
    if (obj != NULL) {
        obj->srt.flags |= 0x2000;
        dirVec[0] = player->srt.transl.x - self->srt.transl.x;
        dirVec[1] = (player->srt.transl.y + 20.0f) - self->srt.transl.y;
        dirVec[2] = player->srt.transl.z - self->srt.transl.z;

        magnitude = SQ(dirVec[0]) + SQ(dirVec[1]) + SQ(dirVec[2]);
        if (magnitude != 0.0f) {
            magnitude = sqrtf(magnitude);
            dirVec[0] /= magnitude;
            dirVec[1] /= magnitude;
            dirVec[2] /= magnitude;
        }
        // @recomp: Spawn projectiles further along their path so they don't get stuck in the flybaddie.
        //          (in vanilla this is * 10.0f)
        obj->srt.transl.x += dirVec[0] * 20.0f;
        obj->srt.transl.y += dirVec[1] * 20.0f;
        obj->srt.transl.z += dirVec[2] * 20.0f;
        obj->speed.x = 2.0f * dirVec[0];
        obj->speed.y = 2.0f * dirVec[1];
        obj->speed.z = 2.0f * dirVec[2];
        obj->unk0xdc = 0xBE;
        obj->unk_0xe0 = 0;
        obj->positionMirror.x = obj->srt.transl.x;
        obj->positionMirror.y = obj->srt.transl.y;
        obj->positionMirror.z = obj->srt.transl.z;
        gDLL_6_AMSFX->vtbl->play_sound(obj, SOUND_730, 0x50, NULL, NULL, 0, NULL);
    }
}
