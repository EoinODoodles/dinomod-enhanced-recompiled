#include "modding.h"

#include "PR/ultratypes.h"
#include "game/objects/object.h"
#include "dlls/engine/53_movelib.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objlib.h"
#include "sys/objtype.h"
#include "sys/rand.h"
#include "macros.h"

#include "recomp/dlls/engine/53_movelib_recomp.h"

RECOMP_PATCH s32 movelib_func_18(Object* arg0, Object* arg1, s32* arg2, MoveLibData* arg3, f32* arg4, s16* arg5, Vec3f* arg6) {
    s16 sp3E;
    s16 sp3C;
    f32 sp38;
    s32* sp34;
    HeadAnimation* var_a3;

    sp34 = func_800349B0();
    if (arg1->objhitInfo != NULL) {
        if (arg1->objhitInfo->unk5A & 2) {
            sp38 = arg1->objhitInfo->unk56 * 4.0f;
        } else if (arg1->objhitInfo->unk5A & 1) {
            sp38 = arg1->objhitInfo->unk52;
        } else {
            sp38 = 30.0f;
        }
    } else {
        sp38 = 30.0f;
    }

    // @recomp: Set the objexpr lookat flip flag if this movelib data is configured to do so. This is required
    //          for the shop keeper as he only calls this function (not the normal movelib lookat func) during
    //          sequences, which does not correctly set the objexpr flag like the other does. Since we fixed
    //          the objexpr flag getting stuck, we need to make sure it's actually set when it's needed.
    if (arg3->unk4A9 & 0x10) {
        func_80034D94(0, 1);
    }

    sp3E = func_80031DD8(arg0, arg1, NULL);
    sp3C = func_800334A4(arg0, arg1, &arg3->headRefPoint, 
                         (arg3->unk4A9 & 8) ? NULL : arg3->unk1C, 
                         arg3->unk454, sp38, 8, arg3->unk4A4);
    if (!(arg3->unk4A9 & 8)) {
        arg3->unk490 = !func_800333C8(arg0, sp34, arg3->jointCount, arg3->unk1C);
    }
    arg3->unk490 = 0;
    if ((arg3->unk4A9 & 2) && (sp3C != 0)) {
        *arg2 = 0;
        return 0;
    }
    if (arg3->unk490 == 0) {
        if ((-arg3->unk4A6 < sp3E) && (sp3E < arg3->unk4A6)) {
            *arg4 = 0.005f;
            *arg2 = 0;
            STUBBED_PRINTF(" In turn Range ");
            return !sp3C;
        }
    }
    if ((*arg2 == 0) && (sp3C != 0)) {
        *arg2 = 1;
        *arg4 = 0.005f;
    } else if (*arg2 != 0) {
        if (sp3E > 0 && arg0->curModAnimId != arg5[1]) {
            func_80023D30(arg0, arg5[1], 0.0f, 0);
            func_80024D74(arg0, 30);
        }
        if (sp3E < 0 && arg0->curModAnimId != arg5[0]) {
            func_80023D30(arg0, arg5[0], 0.0f, 0);
            func_80024D74(arg0, 30);
        }
        if (sp3C == 0) {
            sp3E = sp3E > 0 ? (sp3E / 20) : (sp3E / 20);
        } else {
            sp3E = sp3E > 0 ? ((sp3E - 0x500) / 20) : ((sp3E + 0x500) / 20);
        }
        arg0->srt.yaw += sp3E;
        *arg4 = (f32) (sp3E >= 0 ? sp3E : -sp3E) / 20922.25f;
    }
    return 1;
}

RECOMP_PATCH void movelib_func_4B8(Object* obj, MoveLibData* data) {
    f32 maxLookAtSearchDist;
    f32 lookatYOffset;
    f32 temp_fv1;
    s16 sp5A;
    s32* sp54;
    Object* lookat;
    f32 sp44[3];

    maxLookAtSearchDist = 1000.0f;
    lookatYOffset = 30.0f;
    sp5A = 0;
    sp54 = func_800349B0();
    get_player();
    if (data->unk499 == 0) {
        if ((data->unk4A9 & 1) && (data->unk498 != 8)) {
            data->unk498 = 8;
            if (!(data->unk4A9 & 8)) {
                func_80033224(obj, sp54, data->jointCount, data->unk1C);
                data->unk490 = 0x50;
                func_80033350(data->unk1C, data->jointCount, 0, 0);
            } else {
                func_800332A4(obj, func_800349B0(), data->jointCount);
            }
        } else if (!(data->unk4A9 & 1) && (data->unk498 == 8)) {
            data->unk498 = 0;
            if (!(data->unk4A9 & 8)) {
                func_80033224(obj, sp54, data->jointCount, data->unk1C);
                data->unk490 = 0x50;
            }
        }
        if (data->unk498 > 1) {
            if ((data->unk490 != 0) && !(data->unk4A9 & 8)) {
                data->unk490 = !func_800333C8(obj, sp54, data->jointCount, data->unk1C);
                return;
            }
            func_800332A4(obj, func_800349B0(), data->jointCount);
            return;
        }
        if (data->lookat == NULL) {
            lookat = obj_get_nearest_type_to(OBJTYPE_LookAt, obj, &maxLookAtSearchDist);
        } else {
            lookat = data->lookat;
        }
        if (NULL != lookat) {
            if (data->unk4A9 & 0x20) {
                sp44[0] = data->headRefPoint.x - lookat->srt.transl.x;
                sp44[1] = data->headRefPoint.y - lookat->srt.transl.y;
                sp44[2] = data->headRefPoint.z - lookat->srt.transl.z;
                temp_fv1 = sqrtf(SQ(sp44[0]) + SQ(sp44[2]));
                if (temp_fv1 <= 40.0f) {
                    temp_fv1 = (temp_fv1 - 10.0f) / 30.0f;
                    temp_fv1 = temp_fv1 < 0.0f
                        ? 0.0f
                        : temp_fv1 > 1.0f
                            ? 1.0f
                            : temp_fv1;
                    temp_fv1 = 1.0f - temp_fv1;
                    data->headRefPoint.x = (data->headRefPoint.x * (1.0f - temp_fv1)) + (obj->srt.transl.x * temp_fv1);
                    data->headRefPoint.z = (data->headRefPoint.z * (1.0f - temp_fv1)) + (obj->srt.transl.z * temp_fv1);
                }
            }
            if ((data->unk4AC != -1) && (lookat == data->prevLookat)) {
                if (((data->unk4B4 -= gUpdateRate) <= 0) && ((data->unk4B4 + gUpdateRate) > 0)) {
                    func_80033224(obj, sp54, data->jointCount, data->unk1C);
                    data->unk490 = 0x50;
                    func_80033350(data->unk1C, data->jointCount, 0, 0);
                    data->unk498 = 0;
                    return;
                }
                if (data->unk490 != 0) {
                    data->unk490 = !func_800333C8(obj, sp54, data->jointCount, data->unk1C);
                }
                if (data->unk4B4 < -data->unk4B0) {
                    data->unk4B4 = rand_next(data->unk4B0, data->unk4AC);
                }
                if (data->unk4B4 < 0) {
                    return;
                }
            } else {
                data->unk4B4 = data->unk4AC;
            }
            if ((lookat != data->prevLookat) && (lookat != NULL)) {
                if (lookat->objhitInfo != NULL) {
                    if (lookat->objhitInfo->unk5A & 2) {
                        lookatYOffset = (f32) lookat->objhitInfo->unk56 * 4.0f;
                    } else if (lookat->objhitInfo->unk5A & 1) {
                        lookatYOffset = (f32) lookat->objhitInfo->unk52;
                    } else {
                        lookatYOffset = 30.0f;
                    }
                } else {
                    lookatYOffset = 30.0f;
                }
            }
            if (lookat != NULL) {
                sp5A = func_80031DD8(obj, lookat, NULL);
            }
            if (data->unk4A9 & 0x10) {
                func_80034D94(0, 1);
                sp5A -= 0x8000;
            }
            if ((((sp5A >= 0) ? sp5A : -sp5A) >= 0x5555) || (lookat == NULL)) {
                if ((data->unk498 != 0) || ((lookat == NULL) && (data->prevLookat != NULL))) {
                    func_80033224(obj, sp54, data->jointCount, data->unk1C);
                    data->unk490 = 0xA;
                    func_80033350(data->unk1C, data->jointCount, 0, 0);
                    data->unk498 = 0;
                }
                // @recomp: If we're not gonna call func_800334A4, then we need to clear the objexpr flag that
                //          flips the head lookat angle (if we set it), otherwise it will leak into other calls.
                if (data->unk4A9 & 0x10) {
                    func_80034D94(0, 0);
                }
            } else {
                if ((lookat != data->prevLookat) || (data->unk498 == 0)) {
                    func_80033224(obj, sp54, data->jointCount, data->unk1C);
                    data->unk490 = 1;
                }
                if (data->unk4A9 & 8) {
                    data->unk490 = 0;
                }
                func_800334A4(obj, lookat, &data->headRefPoint, 
                              data->unk490 != 0 ? data->unk1C : NULL, 
                              data->unk454, lookatYOffset, 8, data->unk4A4);
                data->unk498 = 1;
            }
            data->prevLookat = lookat;
            data->lookat = NULL;
            if (!(data->unk4A9 & 8) && (data->unk490 != 0)) {
                data->unk490 = !func_800333C8(obj, sp54, data->jointCount, data->unk1C);
            }
        }
    }
}
