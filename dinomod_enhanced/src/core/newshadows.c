#include "modding.h"

#include "sys/memory.h"
#include "sys/newshadows.h"

RECOMP_PATCH void func_8004D470(void) {
    void *temp_v0;

    D_80092BE8 = 0xA;
    // @recomp: Fix allocation size
    temp_v0 = (void *) mmAlloc(
        sizeof(Unk800B98A0) * 2 + sizeof(Unk800B98A8) * 2 + sizeof(Unk800B98B0) * 2, 
        ALLOC_TAG_SHAD_COL, 
        NULL);
    D_800B98A0[0] = (Gfx *)                    temp_v0;
    D_800B98A0[1] = (Gfx *)              ((u32)temp_v0 + sizeof(Unk800B98A0));
    D_800B98A8[0] = (Unk800B98A8 *)      ((u32)temp_v0 + sizeof(Unk800B98A0) * 2);
    D_800B98A8[1] = (Unk800B98A8 *)      ((u32)temp_v0 + sizeof(Unk800B98A0) * 2 + sizeof(Unk800B98A8));
    D_800B98B0[0] = (Unk8004FA58_Arg5 *) ((u32)temp_v0 + sizeof(Unk800B98A0) * 2 + sizeof(Unk800B98A8) * 2);
    D_800B98B0[1] = (Unk8004FA58_Arg5 *) ((u32)temp_v0 + sizeof(Unk800B98A0) * 2 + sizeof(Unk800B98A8) * 2 + sizeof(Unk800B98B0));

    // @recomp: Fix allocation size
    temp_v0 = (void *) mmAlloc(
        sizeof(Unk800BB158) * 2 + sizeof(Unk800BB168) * 2 + sizeof(Unk800BB160) * 2, 
        ALLOC_TAG_SHAD_COL, 
        NULL);
    D_800BB158[0] = (Gfx *)                    temp_v0;
    D_800BB158[1] = (Gfx *)              ((u32)temp_v0 + sizeof(Unk800BB158));
    D_800BB168[0] = (Unk800BB168 *)      ((u32)temp_v0 + sizeof(Unk800BB158) * 2);
    D_800BB168[1] = (Unk800BB168 *)      ((u32)temp_v0 + sizeof(Unk800BB158) * 2 + sizeof(Unk800BB168));
    D_800BB160[0] = (Unk8004FA58_Arg5 *) ((u32)temp_v0 + sizeof(Unk800BB158) * 2 + sizeof(Unk800BB168) * 2);
    D_800BB160[1] = (Unk8004FA58_Arg5 *) ((u32)temp_v0 + sizeof(Unk800BB158) * 2 + sizeof(Unk800BB168) * 2 + sizeof(Unk800BB160));

    D_800B9840[0] = -8.0f;
    D_800B9840[1] = 0.0f;
    D_800B9840[3] = -8.0f;
    D_800B9840[2] = 0.0f;
    D_800B9840[4] = 16.0f;
    D_800B9840[5] = 0.0f;
    D_800B9840[6] = 8.0f;
    D_800B9840[7] = 16.0f;
    D_800B9840[8] = 0.0f;
    D_800B9840[9] = 8.0f;
    D_800B9840[10] = 0.0f;
    D_800B9840[11] = 0.0f;

    D_800B97E0[0] = D_800B9840[0];
    D_800B97E0[1] = D_800B9840[1];
    D_800B97E0[2] = D_800B9840[2];
    D_800B97E0[3] = D_800B9840[3];
    D_800B97E0[4] = D_800B9840[4];
    D_800B97E0[5] = D_800B9840[5];
    D_800B97E0[6] = D_800B9840[6];
    D_800B97E0[7] = D_800B9840[7];
    D_800B97E0[8] = D_800B9840[8];
    D_800B97E0[9] = D_800B9840[9];
    D_800B97E0[10] = D_800B9840[10];
    D_800B97E0[11] = D_800B9840[11];

    D_800B97E0[12] = -8.0f;
    D_800B97E0[13] = 0.0f;
    D_800B97E0[14] = 100.0f;
    D_800B97E0[15] = -8.0f;
    D_800B97E0[16] = 16.0f;
    D_800B97E0[17] = 100.0f;
    D_800B97E0[18] = 8.0f;
    D_800B97E0[19] = 16.0f;
    D_800B97E0[20] = 100.0f;
    D_800B97E0[21] = 8.0f;
    D_800B97E0[22] = 0.0f;
    D_800B97E0[23] = 100.0f;

    D_800B9840[12] = -6.0f;
    D_800B9840[13] = 0.0f;
    D_800B9840[14] = 55.0f;
    D_800B9840[15] = -6.0f;
    D_800B9840[16] = 16.0f;
    D_800B9840[17] = 55.0f;
    D_800B9840[18] = 6.0f;
    D_800B9840[19] = 16.0f;
    D_800B9840[20] = 55.0f;
    D_800B9840[21] = 6.0f;
    D_800B9840[22] = 0.0f;
    D_800B9840[23] = 55.0f;
    func_8005B870();
    D_800BB190 = queue_load_texture_proxy(0xD8);
}
