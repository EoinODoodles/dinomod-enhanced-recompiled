#include "modding.h"

#include "dlls/objects/210_player.h"
#include "game/objects/object_id.h"
#include "common.h"

#include "recomp/dlls/engine/32_recomp.h"

extern s32 _data_1864;

extern s32 dll_32_func_1D3C(f32* a0, f32* a1, f32* a2);

// @recomp: Fix Sabre's glowing spirit eyes/pendant effect (original patch by MusicalProgrammer)
// TODO: replace with cleaner decomp, eventually
RECOMP_PATCH void dll_32_func_1314(Gfx** gdl, Mtx** mtxs, Object* a2) {
    s32 temp_t0;
    s32 temp_v0_3;
    SRT spD8;
    s32 var_s2;
    s32 var_v1;
    f32 normalized;
    f32 var_fs0;
    ModelFacebatch* temp_s0;
    ModelFacebatch* pad2;
    //s32 pad;
    ModelInstance* temp_s3;
    f32 spB4;
    Vec3f spA8 = { -1.0f, 0.0f, 0.0f }; // _data_184C
    Vec3f sp9C = { 0.0f, 0.0f, 1.0f }; // _data_1858
    Camera* camera;
    ModelInstance_0x4* temp_a3;
    s32 sp90;
    s32 sp8C;
    Object* player;
    s16 temp_a2;
    MtxF *temp;

    // @recomp: Pull face ID from Sabre/Krystal ObjDef
    //sp90 = 0x15;\
    //sp8C = 8;
    sp90 = *(s8*)((u32)a2->def + 0x68);
    sp8C = *(s8*)((u32)a2->def + 0x69);
    player = get_player();
    if (a2->modelInstIdx != 0) {
        return;
    }
    temp_s3 = a2->modelInsts[a2->modelInstIdx];
    spD8.yaw = 0;
    spD8.roll = 0;
    spD8.pitch = 0;
    spD8.scale = 1.0f;
    camera = get_camera();
    if (camera != NULL) {
        spA8.x = camera->srt.transl.x - a2->srt.transl.x;
        spA8.y = camera->srt.transl.y - a2->srt.transl.y;
        spA8.z = camera->srt.transl.z - a2->srt.transl.z;
        var_fs0 = SQ(spA8.f[0]) + SQ(spA8.f[1]) + SQ(spA8.f[2]);
        if (var_fs0 != 0) {
            var_fs0 = sqrtf(var_fs0);
        }
        spD8.yaw = 0;
        spA8.x /= var_fs0;
        spA8.y /= var_fs0;
        spA8.z /= var_fs0;
        spD8.yaw = a2->srt.yaw;
        spD8.transl.x = 0.0f;
        spD8.transl.y = 0.0f;
        spD8.transl.z = 0.0f;
        rotate_vec3(&spD8, sp9C.f);
        spD8.yaw = 0;
    }
    temp_v0_3 = dll_32_func_1D3C(spA8.f, sp9C.f, &spB4);
    if (temp_v0_3 != -1) {
        return;
    }

    if (((DLL_210_Player*)player->dll)->vtbl->func44(player) == 0) {
        if (temp_v0_3 == -1) {
            // @recomp: Pull face ID from Sabre/Krystal ObjDef
            var_s2 = *(s8*)((u32)a2->def + 0x67);
            for (; var_s2 < sp90; var_s2++) {
                if ((a2->id != 0) || (var_s2 != 0x2A)) {
                    temp_s0 = &temp_s3->model->faces[var_s2];
                    temp_a2 = temp_s0->baseVertexID;
                    temp_a3 = &temp_s3->unk4[0][temp_a2];
                    temp_t0 = temp_s3->model->faces[var_s2 + 1].baseVertexID - temp_a2;
                    spD8.transl.x = 0.0f;
                    spD8.transl.y = -26.0f;
                    spD8.transl.z = 0.0f;

                    for (var_v1 = 0; var_v1 < temp_t0; var_v1++) {
                        spD8.transl.x += temp_a3[var_v1].unk0[0];
                        spD8.transl.y += temp_a3[var_v1].unk0[1];
                        spD8.transl.z += temp_a3[var_v1].unk0[2];
                    }

                    spD8.scale = 1.0f;
                    spD8.transl.x /= temp_t0;
                    spD8.transl.y /= temp_t0;
                    spD8.transl.z /= temp_t0;
                    //pad = temp_s0->jointID_A;
                    temp = (MtxF *) ((f32**)(temp_s3->matrices)[(temp_s3->unk34 & 1)] + (temp_s0->jointID_A << 4));
                    vec3_transform(temp,
                                   spD8.transl.x, spD8.transl.y, spD8.transl.z, 
                                   &spD8.transl.x, &spD8.transl.y, &spD8.transl.z);
                    spD8.transl.x += gWorldX;
                    spD8.transl.z += gWorldZ;
                    normalized = SQ(a2->speed.x) + SQ(a2->speed.y) + SQ(a2->speed.z);
                    if (normalized < 0.1f) {
                        gDLL_17_partfx->vtbl->spawn(a2, 0x19A, &spD8, 0x200000, -1, NULL);
                    } else {
                        gDLL_17_partfx->vtbl->spawn(a2, 0x19B, &spD8, 0x200000, -1, NULL);
                    }
                    gDLL_17_partfx->vtbl->spawn(a2, 0x199, &spD8, 0x200000, -1, NULL);
                }
            }
        }
    }
    for (var_s2 = sp8C; var_s2 < (sp8C + 1); var_s2++) {
        temp_s0 = &temp_s3->model->faces[var_s2];
        temp_a2 = temp_s0->baseVertexID;
        temp_a3 = &temp_s3->unk4[0][temp_a2];
        _data_1864 = 4;
        spD8.transl.x = 0.0f;
        spD8.transl.y = 10.0f;
        spD8.transl.z = -40.0f;
        // @recomp: Adjust pendant fx position for Sabre
        if (a2->id == OBJ_Sabre) {
            spD8.transl.x = temp_a3[4].unk0[0] + 25.0f;
            spD8.transl.y = temp_a3[4].unk0[1] + 0.0f;
            spD8.transl.z = temp_a3[4].unk0[2] + 0.0f;
        } else {
            spD8.transl.x = temp_a3[4].unk0[0];
            spD8.transl.y = temp_a3[4].unk0[1] + 2.0f;
            spD8.transl.z = temp_a3[4].unk0[2] + 10.0f;
        }
        spD8.transl.x += rand_next(-5, 5);
        spD8.transl.y += rand_next(-5, 5);
        spD8.transl.z += rand_next(-5, 5);
        spD8.scale = 1.0f;
        //pad = temp_s0->jointID_A;
        temp = (MtxF *) &(((f32 **)temp_s3->matrices)[(temp_s3->unk34 & 1)][temp_s0->jointID_A << 4]);
        vec3_transform(temp, 
                       spD8.transl.x, spD8.transl.y, spD8.transl.z, 
                       &spD8.transl.x, &spD8.transl.y, &spD8.transl.z);
        spD8.transl.x += gWorldX;
        spD8.transl.z += gWorldZ;
        // @recomp: Don't factor in object speed, the partfx is already relative to the player
        // spD8.transl.x += (2.0f * a2->speed.x);
        // spD8.transl.y += (2.0f * a2->speed.y);
        // spD8.transl.z += (2.0f * a2->speed.z);
        normalized = SQ(a2->speed.x) + SQ(a2->speed.y) + SQ(a2->speed.z);
        if (normalized < 0.01f) {
            gDLL_17_partfx->vtbl->spawn(a2, 0x19D, &spD8, 0x200000, -1, NULL);
        }
        gDLL_17_partfx->vtbl->spawn(a2, 0x19C, &spD8, 0x200000, -1, NULL);
    }
}
