#include "modding.h"

#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/map.h"

extern s32 D_800B4A50;

extern void func_8002F164(ModelInstance_0x14_0x14* arg0, ModelInstance_0x14_0x14* arg1);

RECOMP_HOOK("func_80027934") void galleon_staff_collision_hack(Object* obj, Object* otherObj) {
    // @recomp: Turn off collision between Krystal's staff and the Galleon (original patch by MusicalProgrammer)
    if (otherObj->id == OBJ_staff && D_800B4A50 == MAP_FRONT_END) {
        // Note: we technically only need to set this once, maybe there's a better way to do this?
        otherObj->objhitInfo->unk5A = 0;
    }
}

/** Fix a crash in the skeletal collision system, which could occur during the Rolling Demo (patch by Shinx) */
RECOMP_PATCH void func_8002F498(Vec3f* arg0, ModelInstance_0x14* arg1, Model* model, ModelInstance_0x14_0x14* arg3, ModelInstance_0x14_0x14* arg4) {
    s32 jointIdx;
    ModelInstance_0x14_0x14* var_s2;
    s32 var_a2;
    s32 rootIdx;
    ModelInstance_0x14_0x14 *sp50[100];
    s32 var_a3;

    arg3->unk30 = 0; // @recomp

    bzero(sp50, model->jointCount * 4);
    arg4->unk18.x = arg0->x;
    arg4->unk18.y = arg0->y;
    arg4->unk18.z = arg0->z;
    arg4->unk24.x = arg0->x;
    arg4->unk24.y = arg0->y;
    arg4->unk24.z = arg0->z;
    rootIdx = -1;
    jointIdx = 0;
    while (jointIdx < model->jointCount) {
        if (model->collisionB[jointIdx] != 0.0f) {
            arg3->unk0[0] = jointIdx;
            var_a2 = jointIdx;
            arg3->unk18.x = arg0[var_a2].x;
            arg3->unk18.y = arg0[var_a2].y;
            arg3->unk18.z = arg0[var_a2].z;
            arg3->unk24.x = arg0[var_a2].x;
            arg3->unk24.y = arg0[var_a2].y;
            arg3->unk24.z = arg0[var_a2].z;
            arg3->unk18.x += arg1->unk4[var_a2];
            arg3->unk18.y += arg1->unk4[var_a2];
            arg3->unk18.z += arg1->unk4[var_a2];
            arg3->unk24.x -= arg1->unk4[var_a2];
            arg3->unk24.y -= arg1->unk4[var_a2];
            arg3->unk24.z -= arg1->unk4[var_a2];
            sp50[var_a2] = arg3;
            var_a3 = 0;
            var_s2 = NULL;
            while(rootIdx < model->joints[var_a2].parentJointID && var_s2 == NULL) {
                var_a2 = model->joints[var_a2].parentJointID;
                var_s2 = sp50[var_a2];
                sp50[var_a2] = arg3;
                var_a3++;
                if (arg3->unk18.x < (arg0[var_a2].x + arg1->unk4[var_a2])) {
                    arg3->unk18.x = arg0[var_a2].x + arg1->unk4[var_a2];
                }
                if (arg3->unk18.y < (arg0[var_a2].y + arg1->unk4[var_a2])) {
                    arg3->unk18.y = arg0[var_a2].y + arg1->unk4[var_a2];
                }
                if (arg3->unk18.z < (arg0[var_a2].z + arg1->unk4[var_a2])) {
                    arg3->unk18.z = arg0[var_a2].z + arg1->unk4[var_a2];
                }
                if (arg0[var_a2].x < (arg1->unk4[var_a2] + arg3->unk24.x)) {
                    arg3->unk24.x = arg0[var_a2].x - arg1->unk4[var_a2];
                }
                if (arg0[var_a2].y < (arg1->unk4[var_a2] + arg3->unk24.y)) {
                    arg3->unk24.y = arg0[var_a2].y - arg1->unk4[var_a2];
                }
                if (arg0[var_a2].z < (arg1->unk4[var_a2] + arg3->unk24.z)) {
                    arg3->unk24.z = arg0[var_a2].z - arg1->unk4[var_a2];
                }
            }
            if (arg4->unk18.x < arg3->unk18.x) {
                arg4->unk18.x = arg3->unk18.x;
            }
            if (arg4->unk18.y < arg3->unk18.y) {
                arg4->unk18.y = arg3->unk18.y;
            }
            if (arg4->unk18.z < arg3->unk18.z) {
                arg4->unk18.z = arg3->unk18.z;
            }
            if (arg3->unk24.x < arg4->unk24.x) {
                arg4->unk24.x = arg3->unk24.x;
            }
            if (arg3->unk24.y < arg4->unk24.y) {
                arg4->unk24.y = arg3->unk24.y;
            }
            if (arg3->unk24.z < arg4->unk24.z) {
                arg4->unk24.z = arg3->unk24.z;
            }
            if (rootIdx == 0 && var_s2 == NULL) {
                var_s2 = sp50[0];
            }
            if (var_a3 < 3 && var_s2 != NULL && var_s2->unk30 < 3) {
                sp50[var_a2] = var_s2; // @recomp
                func_8002F164(arg3, var_s2);
                var_s2->unkC[var_s2->unk30] = model->joints[var_a2].parentJointID;
                var_s2->unk0[var_s2->unk30] = jointIdx;
                var_s2->unk30++;
            } else {
                if ((var_s2 != NULL) && (var_s2->unk30 >= 3)) {
                    // @recomp
                    // sp50[var_a2] = arg3;
                    sp50[var_a2] = var_s2;
                }
                arg3->unkC[0] = model->joints[var_a2].parentJointID;
                arg3->unk30 = 1;
                arg3++;
                // @recomp
                // if (var_a2 == 0) {
                //     rootIdx = 0;
                // }
            }
        }
        jointIdx++;
    }
    arg3->unk0[0] = -1;
}
