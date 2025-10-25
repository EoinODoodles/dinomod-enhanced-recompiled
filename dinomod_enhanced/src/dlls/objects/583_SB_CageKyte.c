#include "PR/ultratypes.h"
#include "functions.h"
#include "modding.h"

#include "game/objects/object.h"
#include "game/gamebits.h"
#include "recomputils.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "game/gamebits.h"
#include "dll.h"

#include "recomp/dlls/objects/583_SB_CageKyte_recomp.h"

// Removes a flag check which could prevent the "Scales Escapes with Kyte" sequence from playing 
// just before you teleport away to SwapStone Circle (originally by MusicalProgrammer)
RECOMP_PATCH void SBCageKyte_print(Object* self, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    if (visibility && 
        // !main_get_bits(BIT_WM_Played_Randorn_First_Meeting) && @recomp: removed
        !self->unkDC) {
        draw_object(self, gdl, mtxs, vtxs, pols, 1.0f);
    }
}
