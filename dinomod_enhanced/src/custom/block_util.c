#include "PR/ultratypes.h"
#include "sys/dll.h"
#include "sys/map.h"
#include "block_util.h"

/** 
  * The same as `blockSetupDLGroups`, but applied to a specific shape instead of all of a Block's shapes.
  * This is useful when animating between different render modes at runtime: for example toggling RENDER_SEMI_TRANSPARENT
  * and RENDER_DECAL_SIMPLE so AlphaAnimators can turn a surface truly opaque when its vertices are at max opacity.
  */
void blockRedoShapeDLGroups(Block* block, s32 shapeIdx) {
    BlockShape* shape;
    Texture* texture;
    s32 texFlags = 0;
    s32 aa;
    s32 flags;
    Gfx* mygdl;

    if (shapeIdx >= block->shapeCount || shapeIdx < 0) {
        return;
    }

    block->shapes[shapeIdx].texScrollerID = 0xff;
    
    shape = &block->shapes[shapeIdx];
    flags = shape->flags;
    aa = flags & RENDER_ANTI_ALIASING;

    if (shape->materialIndex == 0xff) {
        texture = NULL;
    } else {
        texture = block->materials[shape->materialIndex].texture;
        if (texture != NULL) {
            texFlags = texture->flags;
        }
    }

    if ((flags & RENDER_DECAL_SIMPLE) == 0) {
        if ((flags & RENDER_UNK1000000) != 0) {
            flags |= (RENDER_Z_COMPARE | RENDER_FOG_ACTIVE | RENDER_UNK10);
        }
    }

    if (texFlags & RENDER_CUTOUT) {
        flags |= RENDER_Z_COMPARE;
    }

    if (aa != 0) {
        flags &= ~RENDER_ANTI_ALIASING;
    } else {
        flags |= RENDER_ANTI_ALIASING;
    }

    flags |= RENDER_Z_COMPARE;

    if (flags & RENDER_DECAL_SIMPLE) {
        if (flags & RENDER_SUBSURFACE) {
            flags &= ~RENDER_SUBSURFACE;
            texDisableModes(RENDER_SEMI_TRANSPARENT);
        } else {
            if ((flags & RENDER_UNK2000) || (flags & RENDER_SEMI_TRANSPARENT) || (flags & RENDER_DECAL)) {
                flags |= RENDER_SEMI_TRANSPARENT;
            } else {
                texDisableModes(RENDER_SEMI_TRANSPARENT);
            }
        }
    }

    mygdl = &block->gdlGroups[shapeIdx * 3];
    // only set modes, don't set texture!
    texDPTextureSimple(&mygdl, texture, 
        flags | RENDER_NO_CULL, 
        TEX_FRAME(0),
        TRUE, // force
        TEXOPT_INVISIBLE | TEXOPT_SET_MODES | TEXOPT_SKIP_MODE_CACHE);

    if ((flags & RENDER_UNK2000) && texture != NULL && (texture->flags & (RENDER_COMPOSITE_BASE | RENDER_COMPOSITE_OVERLAY))) {
        mygdl = &block->gdlGroups[shapeIdx * 3];
        gSPLoadGeometryMode(mygdl++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
        if (texture->flags & (RENDER_COMPOSITE_BASE | RENDER_COMPOSITE_OVERLAY)) {
            gDPSetCombineMode(
                mygdl++, 
                G_CC_DINO_BLENDTEX_ENV,
                G_CC_DINO_LERP_FROM_SHADE2
            );
        } else {
            gDPSetCombineMode(
                mygdl++, 
                G_CC_DINO_BLEND_TEX_SHADE_ENVA,
                G_CC_DINO_LERP_FROM_SHADE_INVA2
            );
        }
        gDPSetOtherMode(
            mygdl++,
            G_AD_PATTERN | G_CD_MAGICSQ | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_PERSP | G_CYC_2CYCLE | G_PM_NPRIMITIVE,
            G_AC_NONE | G_ZS_PIXEL | G_RM_NOOP | G_RM_AA_ZB_XLU_SURF2
        );
    }

    if (flags & RENDER_DECAL_SIMPLE) {
        texEnableModes(RENDER_SEMI_TRANSPARENT);
    }
}
