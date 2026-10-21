#include "modding.h"

#include "dlls/engine/33_BaddieControl.h"

#include "recomp/dlls/objects/217_GuardClaw_recomp.h"

static Object* _self;

RECOMP_HOOK_DLL(GuardClaw_setup) void GuardClaw_setup_hook(Object* self, Baddie_Setup* setup, s32 reset) {
    _self = self;
}

RECOMP_HOOK_RETURN_DLL(GuardClaw_setup) void GuardClaw_setup_ret_hook(void) {
    if (_self != NULL) {
        Baddie* baddie = _self->data;
        // @recomp: Prevent GuardClaw from slowly flying into the air. This flag stops DLL 27 from
        //          calculating Y velocity as the difference in Y-coord from tick-to-tick. If a GuardClaw
        //          is slightly in the ground when they load, they will pop out and gain some velocity
        //          that never goes away since they also configure their gravity as 0.
        baddie->fsa.unk4.flags |= DLL27FLAG_40000;
        
        _self = NULL;
    }
}
