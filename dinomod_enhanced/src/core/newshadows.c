#include "modding.h"

#include "sys/memory.h"
#include "sys/newshadows.h"
#include "sys/shadowtex.h"

extern f32 D_800B97E0[24];
extern f32 D_800B9840[24];

extern Gfx *D_800B98A0[2];
extern Vec3f *D_800B98A8[2];
extern Vtx *D_800B98B0[2];

extern Gfx *D_800BB158[2];
extern Vtx *D_800BB160[2];
extern Vec3f *D_800BB168[2];

extern Texture* D_800BB190;

RECOMP_PATCH void shadows_init(void) {
    void *temp_v0;

    D_80092BE8 = 10;
    // @recomp: Fix allocation size. In the vanilla build, space for the second Gfx buffer was not included in the alloc size.
    temp_v0 = (void *) mmAlloc(
        ((sizeof(Gfx) * 500) * 2) + ((sizeof(Vec4f) * 500) * 2) + ((sizeof(Vtx) * 400) * 2), 
        ALLOC_TAG_SHAD_COL, 
        NULL);
    D_800B98A0[0] = (Gfx*)        temp_v0;
    D_800B98A0[1] = (Gfx*)  ((u32)temp_v0 +  (sizeof(Gfx) * 500));
    D_800B98A8[0] = (Vec3f*)((u32)temp_v0 +  (sizeof(Gfx) * 500) * 2);
    D_800B98A8[1] = (Vec3f*)((u32)temp_v0 + ((sizeof(Gfx) * 500) * 2) +  (sizeof(Vec4f) * 500));
    D_800B98B0[0] = (Vtx*)  ((u32)temp_v0 + ((sizeof(Gfx) * 500) * 2) +  (sizeof(Vec4f) * 500) * 2);
    D_800B98B0[1] = (Vtx*)  ((u32)temp_v0 + ((sizeof(Gfx) * 500) * 2) + ((sizeof(Vec4f) * 500) * 2) + (sizeof(Vtx) * 400));

    temp_v0 = (void *) mmAlloc(
        ((sizeof(Gfx) * 600) * 2) + ((sizeof(Vec4f) * 700) * 2) + ((sizeof(Vtx) * 600) * 2), 
        ALLOC_TAG_SHAD_COL, 
        NULL);
    D_800BB158[0] = (Gfx*)        temp_v0;
    D_800BB158[1] = (Gfx*)  ((u32)temp_v0 +  (sizeof(Gfx) * 600));
    D_800BB168[0] = (Vec3f*)((u32)temp_v0 +  (sizeof(Gfx) * 600) * 2);
    D_800BB168[1] = (Vec3f*)((u32)temp_v0 + ((sizeof(Gfx) * 600) * 2) +  (sizeof(Vec4f) * 700));
    D_800BB160[0] = (Vtx*)  ((u32)temp_v0 + ((sizeof(Gfx) * 600) * 2) +  (sizeof(Vec4f) * 700) * 2);
    D_800BB160[1] = (Vtx*)  ((u32)temp_v0 + ((sizeof(Gfx) * 600) * 2) + ((sizeof(Vec4f) * 700) * 2) + (sizeof(Vtx) * 600));

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
    shadowtex_init();
    D_800BB190 = queue_load_texture_proxy(0xD8);
}
