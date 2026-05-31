#include "modding.h"
#include "recomputils.h"

#include "dlls/engine/18_objfsa.h"
#include "dlls/engine/53_movelib.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/newshadows.h"
#include "sys/rand.h"
#include "dll.h"

#include "recomp/dlls/objects/775_WCTrex_recomp.h"

typedef struct {
/*000*/ ObjFSA_Data fsa;
/*34C*/ MoveLibData movedata;
/*804*/ HeadAnimation headAnim;
/*828*/ UnkCurvesStruct unk828;
/*930*/ f32 playerDist;
/*934*/ f32 attackCooldown;
/*938*/ u8 flags;
// @recomp
    u8 activeHitSpheres;
} WCTrex_Data;

enum WCTrexModAnims {
    WCTREX_ANIM_0_Idle = 0,
    WCTREX_ANIM_1_Walk = 1,
    WCTREX_ANIM_2_BiteLeft_Near = 2,
    WCTREX_ANIM_3_BiteLeft_Far = 3,
    WCTREX_ANIM_4_BiteRight_Far = 4,
    WCTREX_ANIM_5_BiteLeft_Farther = 5,
    WCTREX_ANIM_6_BiteRight_Farther = 6,
    WCTREX_ANIM_7_KickLeft = 7,
    WCTREX_ANIM_8_KickRight = 8
};

RECOMP_HOOK_DLL(WCTrex_control) void WCTrex_control_hook(Object* self) {
    // Set objhit damage and type
    func_80026128(self, 0x17, 2, -1);
}

RECOMP_PATCH u32 WCTrex_get_data_size(Object *self, u32 offsetAddr) {
    // @recomp: Use custom objdata size
    return sizeof(WCTrex_Data);
}

static void reset_hit_sphere_active_state(Object* self, ObjFSA_Data* fsa) {
    WCTrex_Data* objdata = self->data;
    self->objhitInfo->unk40 = 0;
    objdata->activeHitSpheres = 0;
}

RECOMP_PATCH s32 WCTrex_func_728(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    WCTrex_Data* objdata = self->data; // @recomp
    Object* player = get_player();
    s32 angle;
    f32 xDiff, zDiff;

    if (fsa->enteredAnimState != 0) {
        fsa->animTickDelta = 0.012f;
        // @recomp: Add anim state exit callback so can reset hit sphere stuff
        fsa->animExitAction = reset_hit_sphere_active_state;
        xDiff = self->srt.transl.x - player->srt.transl.x;
        zDiff = self->srt.transl.z - player->srt.transl.z;
        angle = arctan2_f(xDiff, zDiff) - (self->srt.yaw & 0xFFFF);
        CIRCLE_WRAP(angle);
        // @recomp: Attack player!
        //recomp_printf("%d\n", angle);
        if (angle > 0) {
            if (angle > 0x5000) {
                objdata->activeHitSpheres = 2;
                func_80023D30(self, WCTREX_ANIM_7_KickLeft, 0.0f, 0);
            } else if (angle > 0x3800) {
                objdata->activeHitSpheres = 1;
                func_80023D30(self, WCTREX_ANIM_5_BiteLeft_Farther, 0.0f, 0);
            } else if (angle > 0x1200) {
                objdata->activeHitSpheres = 1;
                func_80023D30(self, WCTREX_ANIM_3_BiteLeft_Far, 0.0f, 0);
            } else {
                objdata->activeHitSpheres = 1;
                func_80023D30(self, WCTREX_ANIM_2_BiteLeft_Near, 0.0f, 0);
            }
        } else {
            if (angle < -0x5000) {
                objdata->activeHitSpheres = 4;
                func_80023D30(self, WCTREX_ANIM_8_KickRight, 0.0f, 0);
            } else if (angle < -0x3800) {
                objdata->activeHitSpheres = 1;
                func_80023D30(self, WCTREX_ANIM_6_BiteRight_Farther, 0.0f, 0);
            } else if (angle < -0x1200) {
                objdata->activeHitSpheres = 1;
                func_80023D30(self, WCTREX_ANIM_4_BiteRight_Far, 0.0f, 0);
            } else {
                if (angle < -0x300) {
                    objdata->activeHitSpheres = 1;
                    func_80023D30(self, WCTREX_ANIM_4_BiteRight_Far, 0.0f, 0); // no near right anim :(
                } else {
                    // we don't have a near bite right and the player is mostly in front, so just bite left
                    objdata->activeHitSpheres = 1;
                    func_80023D30(self, WCTREX_ANIM_2_BiteLeft_Near, 0.0f, 0);
                }
            }
        }
    }
    // @recomp: Activate relevant hit spheres
    // HACK: the correct way to activate the hit spheres would be using OBJHITS.bin (see func_8001A8EC)
    self->objhitInfo->unk40 = (objdata->activeHitSpheres << 4);
    return 0;
}

RECOMP_PATCH s32 WCTrex_func_8DC(Object* self, ObjFSA_Data* fsa, f32 updateRate) {
    WCTrex_Data* objdata = self->data;

    if (fsa->enteredLogicState != 0) {
        objdata->flags |= 1;
        gDLL_18_objfsa->vtbl->set_anim_state(self, fsa, 1);
    }
    // @recomp: Always look at player so we don't focus on Tricky while being mad at Sabre...
    ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func1(&objdata->movedata, get_player());
    objdata->attackCooldown -= gUpdateRateF;
    if (objdata->playerDist > 220.0f) {
        // @recomp: Reset lookat
        ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func1(&objdata->movedata, NULL);
        return 1 + 1;
    }
    if ((objdata->playerDist < 100.0f) && (objdata->attackCooldown <= 0.0f)) {
        objdata->attackCooldown = (f32) rand_next(120, 250);
        return 3 + 1;
    }
    return 0;
}
