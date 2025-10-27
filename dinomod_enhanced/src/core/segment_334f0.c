#include "PR/os.h"
#include "modding.h"

#include "PR/ultratypes.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/rand.h"

#include "segment_334F0.h"

enum SnowHornAnims {
    MODANIM_SnowHorn_Idle = 0,
    MODANIM_SnowHorn_Talk = 2,
    MODANIM_SnowHorn_Walk = 3,
    MODANIM_SnowHorn_Sleep_Intro = 4,
    MODANIM_SnowHorn_Sleep = 5,
    MODANIM_SnowHorn_Wake_Up = 6,
    MODANIM_SnowHorn_Hit_React = 47
};

/* 
    These patches address behaviour issues while SnowHorn are asleep.
    The ROM patch version of Dinomod Enhanced patches the SnowHorn DLL's update function directly, but
    a different approach is taken in recomp for the moment since the matched SnowHorn function is producing
    slightly different object behaviour. Hopefully this core patch can be replaced with a direct SnowHorn DLL
    function edit once the SnowHorn DLL's decomp is more complete! It currently accomplishes the same thing, in any case.
*/

//Prevent head turn while asleep
RECOMP_PATCH void func_80033B68(Object* obj, Unk80032CF8* arg1, f32 arg2) {
    s16* temp_v0;
    s32 var_v0;

    //@recomp: checks if the object is a SnowHorn, and returns early if the SnowHorn is asleep
    if (obj->group == 40){
        if (obj->curModAnimId == MODANIM_SnowHorn_Sleep_Intro ||
            obj->curModAnimId == MODANIM_SnowHorn_Sleep){
            return;
        }
    }

    temp_v0 = func_80034804(obj, 0);
    if (temp_v0 == NULL) {
        return;
    }

    if (temp_v0[0] != 0) {
        temp_v0[0] = (temp_v0[0] * 3) / 4;
    }
    if (arg2 < 0.0f) {
        arg2 = -arg2;
    }
    if (arg2 <= 0.1f) {
        func_80033C54(obj, arg1, arg2, temp_v0);
    } else {
        func_80033FD8(obj, arg1, arg2, temp_v0);
    }
    var_v0 = arg2 > 0.1f ? 1 : 0;
    arg1->unk1A = (var_v0 << 8) | (arg1->unk1A & 0xFF);
}

//Prevent SnowHorn from blinking while asleep
RECOMP_PATCH void func_80032A08(Object* obj, Unk80032CF8* arg1) {
    s32* sp1C;
    s32 temp_v1;
    s32* temp_v0;

    //@recomp: checks if the object is a SnowHorn, and returns early if the SnowHorn is asleep
    if (obj->group == 40){
        if (obj->curModAnimId == MODANIM_SnowHorn_Sleep_Intro ||
            obj->curModAnimId == MODANIM_SnowHorn_Sleep){
            return;
        }
    }

    sp1C = func_800348A0(obj, 5, 0);
    temp_v0 = func_800348A0(obj, 4, 0);
    if ((sp1C == NULL)  || (temp_v0 == NULL)) {
        return;
    }

    temp_v1 = *temp_v0;
    switch (arg1->unk1E & 0xF) {
    case 0:
        if (arg1->unk1F > 0) {
            arg1->unk1F -= gUpdateRate;
        } else if (rand_next(0, 0x3E8) >= 0x3DA) {
            arg1->unk1E = 1;
            arg1->unk1F = 0;
        }
        break;
    case 1:
        if (arg1->unk1E & 0x80) {
            temp_v1 -= 0x100;
            if (temp_v1 < 0) {
                temp_v1 = 0;
                arg1->unk1E = 0;
                arg1->unk1F = 0;
            }
        } else {
            temp_v1 += 0x100;
            if (temp_v1 >= 0x201) {
                temp_v1 -= 0x200;
                if (temp_v1 < 0) {
                    temp_v1 = 0;
                    arg1->unk1E = 0;
                } else {
                    arg1->unk1E = -0x7F;
                }
                arg1->unk1F = 0;
            }
        }
        *sp1C = temp_v1;
        *temp_v0 = temp_v1;
        break;
    }

    func_80034678(obj, arg1, 0);
}
