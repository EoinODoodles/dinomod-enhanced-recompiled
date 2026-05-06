#include "dlls/engine/27.h"
#include "game/objects/object.h"
#include "modding.h"

#include "PR/ultratypes.h"
#include "sys/camera.h"
#include "sys/main.h"

#include "recomp/dlls/engine/27_recomp.h"

extern Object *bss_0;

extern void dll_27_get_obj_world_matrix(Object* obj, DLL27_Data* data, MtxF* mtx);
extern Func_80057F1C_Struct* dll_27_func_C7C(Object* arg0, f32 arg1, f32 arg2, s32* arg3, s32 arg4);

/*
    @rom-patch only:
    Make search base a smaller number, prevents crashes when swimming over waterfalls (originally by MusicalProgrammer)
*/
#ifdef DINOMOD_ROM_PATCH
#define SEARCH_BASE -25000.0f
#else
#define SEARCH_BASE -100000.0f
#endif

#define SEARCH_CEILING 100000.0f

RECOMP_PATCH void dll_27_func_1E8(Object *obj, DLL27_Data *data, f32 updateRate) {
    MtxF worldMtx;
    Vec3f spE0[4];
    f32 spD0[4];
    f32 temp_fa1;
    f32 temp_ft4;
    f32 temp_fa0;
    f32 temp_fv0;
    f32 minX;
    f32 maxY;
    f32 minY;
    f32 maxZ;
    f32 minZ;
    f32 maxX;
    s32 var_s4;
    s32 i;

    if (data->mode == DLL27MODE_DISABLED) {
        return;
    }

    transform_point_by_object(
        obj->srt.transl.f[0], obj->srt.transl.f[1], obj->srt.transl.f[2], 
        &obj->globalPosition.x, &obj->globalPosition.y, &obj->globalPosition.z, 
        obj->parent);
    
    dll_27_get_obj_world_matrix(obj, data, &worldMtx);

    for (var_s4 = 0, i = 0; var_s4 < ((s32) data->numTestPoints >> 4); ) {
        vec3_transform(&worldMtx, 
                       data->unk4[var_s4].x, data->unk4[var_s4].y, data->unk4[var_s4].z, 
                       &spE0[var_s4].x, &spE0[var_s4].y, &spE0[var_s4].z);
        spD0[i] = data->unk68.unk40[i];
        spD0[i] = sqrtf((2 * spD0[i]) * spD0[i]);
        i++;
        var_s4++;
    }
    
    maxX = SEARCH_BASE;
    minX = SEARCH_CEILING;
    maxY = SEARCH_BASE;
    minY = SEARCH_CEILING;
    maxZ = SEARCH_BASE;
    minZ = SEARCH_CEILING;
    
    var_s4 = 0;
    i = 0;
    while (i < ((s32) data->numTestPoints >> 4)) {
        temp_fv0 = spE0[var_s4].x + spD0[i];
        if (maxX < temp_fv0) {
            maxX = temp_fv0;
        }
        temp_fv0 = spE0[var_s4].x - spD0[i];
        if (temp_fv0 < minX) {
            minX = temp_fv0;
        }
        temp_fv0 = spE0[var_s4].y + spD0[i];
        if (maxY < temp_fv0) {
            maxY = temp_fv0;
        }
        temp_fv0 = spE0[var_s4].y - spD0[i];
        if (temp_fv0 < minY) {
            minY = temp_fv0;
        }
        temp_fv0 = spE0[var_s4].z + spD0[i];
        if (maxZ < temp_fv0) {
            maxZ = temp_fv0;
        }
        temp_fv0 = spE0[var_s4].z - spD0[i];
        if (temp_fv0 < minZ) {
            minZ = temp_fv0;
        }

        if (((!data->unk38[var_s4].x) && (!data->unk38[var_s4].x)) && (!data->unk38[var_s4].x)){} // @fake

        temp_fa1 = data->unk38[var_s4].y;
        temp_ft4 = data->unk38[var_s4].z;

        temp_fv0 = data->unk38[var_s4].x + spD0[i];
        if (maxX < temp_fv0) {
            maxX = temp_fv0;
        }
        temp_fv0 = data->unk38[var_s4].x - spD0[i];
        if (temp_fv0 < minX) {
            minX = temp_fv0;
        }
        temp_fv0 = temp_fa1 + spD0[i];
        if (maxY < temp_fv0) {
            maxY = temp_fv0;
        }
        temp_fv0 = temp_fa1 - spD0[i];
        if (temp_fv0 < minY) {
            minY = temp_fv0;
        }
        temp_fv0 = temp_ft4 + spD0[i];
        if (maxZ < temp_fv0) {
            maxZ = temp_fv0;
        }
        temp_fv0 = temp_ft4 - spD0[i];
        if (temp_fv0 < minZ) {
            minZ = temp_fv0;
        }
        var_s4++;
        i++;
        
    }
    data->aabb.min.x = minX;
    data->aabb.max.x = maxX;
    data->aabb.min.y = (minY - data->boundsYExtension);
    data->aabb.max.y = (data->boundsYExtension + maxY);
    data->aabb.min.z = minZ;
    data->aabb.max.z = maxZ;
}

RECOMP_PATCH void dll_27_reset(Object *obj, DLL27_Data *data) {
    s32 i;

    transform_point_by_object(
        obj->srt.transl.x, obj->srt.transl.y, obj->srt.transl.z, 
        &obj->globalPosition.x, &obj->globalPosition.y, &obj->globalPosition.z, 
        obj->parent);

    for (i = 0; i < ((s32) data->numTestPoints >> 4); i++) {
        data->unk38[i].x = obj->globalPosition.x;
        data->unk38[i].y = obj->globalPosition.y + 5.0f;
        data->unk38[i].z = obj->globalPosition.z;
    }

    for (i = 0; i < (data->numTestPoints & 0xF); i++) {
        data->unk110[i].x = obj->globalPosition.x;
        data->unk110[i].y = obj->globalPosition.y + 5.0f;
        data->unk110[i].z = obj->globalPosition.z;
    }

    data->unk25C = 0;
    data->unk25B = 0;
    if (data->mode != DLL27MODE_2) {
        data->unkD4 = 0;
        data->waterY = SEARCH_BASE;
        data->unk1AC = SEARCH_CEILING;
        data->floorY = SEARCH_BASE;
        data->underwaterDist = 0;
        data->floorDist = 0;
        
        for (i = 0; i < ((s32) data->numTestPoints >> 4); i++) {
            data->waterYList[i] = SEARCH_BASE;
            data->floorYList[i] = SEARCH_BASE;
            data->unk1CC[i] = SEARCH_CEILING;
        }
    }
}

RECOMP_PATCH void dll_27_func_15C0(Object* arg0, DLL27_Data* arg1) {
    Func_80057F1C_Struct* temp_v0;
    s32 sp60;
    s32 var_t1;
    s32 var_t4;
    s8 var_t2;
    u8 var_v0;

    var_v0 = arg1->numTestPoints >> 4;
    if (!(arg1->flags & DLL27FLAG_8000000)) {
        var_v0 = 1;
    }
    for (var_t4 = 0; var_t4 < var_v0; var_t4++) { 
        if (var_v0 >= 2) {
            bss_0 = NULL;
        }

        var_t2 = 0;

        temp_v0 = dll_27_func_C7C(arg0, arg1->unk8[var_t4].x, arg1->unk8[var_t4].z, &sp60, 0);
        
        arg1->waterYList[var_t4] = SEARCH_BASE;
        arg1->floorYList[var_t4] = SEARCH_BASE;
        arg1->unk1CC[var_t4] = SEARCH_CEILING;
        arg1->underwaterDistList[var_t4] = 0;
        arg1->floorDistList[var_t4] = 0;
        arg1->waterNormalXList[var_t4] = 0.0f;
        arg1->waterNormalZList[var_t4] = 0.0f;
        arg1->waterNormalYList[var_t4] = 1.0f;

        for (var_t1 = 0; var_t1 < sp60; var_t1++) {
            if (temp_v0[var_t1].unk14 != 0xE) {
                if ((var_t2 == 0) && (temp_v0[var_t1].unk0[0] < (arg0->globalPosition.y + 5.0f)) && (temp_v0[var_t1].unk0[2] > 0.707f)) {
                    arg1->floorYList[var_t4] = temp_v0[var_t1].unk0[0];
                    var_t2 = 1;
                    arg1->floorDistList[var_t4] = (f32) (arg0->globalPosition.y - temp_v0[var_t1].unk0[0]);
                } else {
                    if (((arg0->globalPosition.y + 5.0f) <= temp_v0[var_t1].unk0[0]) && (temp_v0[var_t1].unk0[2] < 0.0f)) {
                        arg1->unk1CC[var_t4] = temp_v0[var_t1].unk0[0];
                    }
                }
            }
        }

        if (var_t2 == 0) {
            // No floor found, assume high above one
            arg1->floorDistList[var_t4] = 100.0f;
        }

        if (arg1->unk25C & (0x10 << var_t4)) {
            arg1->floorDistList[var_t4] = 0.0f;
        }

        for (var_t1 = 0; var_t1 < sp60; var_t1++) {
            if (temp_v0[var_t1].unk14 == 0xE) {
                if ((temp_v0[var_t1].unk0[0] < arg1->unk1CC[var_t4]) && (arg1->floorYList[var_t4] < temp_v0[var_t1].unk0[0])) {
                    arg1->waterYList[var_t4] = temp_v0[var_t1].unk0[0];
                    arg1->waterNormalXList[var_t4] = (f32) temp_v0[var_t1].unk0[1];
                    arg1->waterNormalYList[var_t4] = (f32) temp_v0[var_t1].unk0[2];
                    arg1->waterNormalZList[var_t4] = (f32) temp_v0[var_t1].unk0[3];
                }
            }
        }

        if (arg1->waterYList[var_t4] != SEARCH_BASE) {
            arg1->underwaterDistList[var_t4] = arg1->waterYList[var_t4] - arg0->globalPosition.y;
        }
    }
    arg1->waterY = arg1->waterYList[0];
    arg1->floorY = arg1->floorYList[0];
    arg1->unk1AC = arg1->unk1CC[0];
    arg1->underwaterDist = arg1->underwaterDistList[0];
    arg1->floorDist = arg1->floorDistList[0];
}
