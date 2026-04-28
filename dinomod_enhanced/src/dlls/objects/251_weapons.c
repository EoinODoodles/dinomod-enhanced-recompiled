#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "common.h"
#include "game/gamebits.h"
#include "game/objects/object_id.h"
#include "sys/gfx/modgfx.h"
#include "sys/main.h"
#include "sys/objects.h"

#include "recomp/dlls/_asm/251_recomp.h"

/** 
  * Change collision bits for weapons, allowing them to phase through
  * light beams instead of striking them (originally by MusicalProgrammer)
  *
  * TODO: patch the constructor function instead once it's decomped?
  */
RECOMP_HOOK_DLL(dll_251_control) void weapons_phase_through_transparent_shapes(Object* self) {
    if ((self->objhitInfo) && (self->objhitInfo->unkA1 != 1)) {
        self->objhitInfo->unkA1 = 1;
    }
}
