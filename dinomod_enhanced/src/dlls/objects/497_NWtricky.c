#include "PR/os.h"
#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "common.h"
#include "dlls/objects/common/sidekick.h"

#include "recomp/dlls/objects/497_NWtricky_recomp.h"

typedef struct {
    u8 state;
    u8 doneDemo;
    u8 demoState;
    f32 timer;
    SidekickStats *sidekickStats;
} NWtricky_Data;

typedef enum {
    STATE2_0,
    STATE2_1,
    STATE2_2
} NWtricky_State2;

/** Allow more controller inputs during tutorial (for inventory's optional D-pad controls/new controls) */
RECOMP_PATCH int NWtricky_anim_callback(Object *self, Object *animObj, AnimObj_Data *animObjData, s8 arg3) {
    NWtricky_Data *objdata;
    Object *tricky;
    s32 i;
    s32 buttonMask;
    u16 pressedButtons; //@recomp

    objdata = self->data;
    buttonMask = 0;

    if (!objdata->doneDemo) {
        tricky = get_sidekick();
        ((DLL_ISidekick*)tricky->dll)->vtbl->func14(tricky, 1);
        ((DLL_ISidekick*)tricky->dll)->vtbl->func14(tricky, 2);
        ((DLL_ISidekick*)tricky->dll)->vtbl->func14(tricky, 3);
        ((DLL_ISidekick*)tricky->dll)->vtbl->func14(tricky, 4);
        ((DLL_ISidekick*)tricky->dll)->vtbl->func14(tricky, 5);
        ((DLL_ISidekick*)tricky->dll)->vtbl->func14(tricky, 0);
        switch (objdata->demoState) {
        case STATE2_0:
            for (i = 0; i < animObjData->unk98; i++) {
                if (animObjData->unk8E[i] == 3)
                    objdata->demoState = STATE2_1;
            }
            break;
        case STATE2_1:
            pressedButtons = joy_get_pressed_raw(0); //@recomp
            for (i = 0; i < animObjData->unk98; i++) {
                if (animObjData->unk8E[i] == 4) {
                    objdata->demoState = STATE2_2;
                    break;
                } else if (animObjData->unk8E[i] == 1)
                    buttonMask = D_CBUTTONS; // simulate C-Down press
            }

            //@recomp: allow up/down on C-button/D-pad
            buttonMask |= pressedButtons & (U_CBUTTONS | U_JPAD);
            buttonMask |= pressedButtons & (D_CBUTTONS | D_JPAD);
            break;
        case STATE2_2:
            for (i = 0; i < animObjData->unk98; i++) {
                if (animObjData->unk8E[i] == 2)
                    buttonMask = A_BUTTON; // simulate A press
            }

            buttonMask |= joy_get_pressed_raw(0) & A_BUTTON;

            if (buttonMask & A_BUTTON) {
                objdata->doneDemo = TRUE;
            }
            break;
        }
        gDLL_1_cmdmenu->vtbl->set_buttons_override(buttonMask);
    } else {
        gDLL_1_cmdmenu->vtbl->set_buttons_override(CMDMENU_CLEAR_BUTTONS_OVERRIDE);
    }

    return 0;
}
