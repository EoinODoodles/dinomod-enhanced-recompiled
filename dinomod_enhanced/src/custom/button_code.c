#include "button_code.h"
#include "macros.h"

/** 
  * Initialises the ButtonCode handler.
  *
  * buttonSequence should be a pointer to a u16 array of N64 buttonIDs (`A_BUTTON`, `B_BUTTON`, etc.)
  *
  * buttonSequenceLength should be `ARRAYCOUNT(buttonSequence)`
  */
void button_code_setup(ButtonCode* handler, u16* buttonSequence, u8 buttonSequenceLength) {
    if (!handler || !buttonSequence || buttonSequenceLength == 0) {
        return;
    }

    handler->sequence = buttonSequence;
    handler->sequenceLength = buttonSequenceLength;
    handler->initialised = TRUE;
}

/** Logs info about the ButtonCode */
void button_code_print(ButtonCode* code) {
    char string[64] = "";
    u8 i;

    if (!code) {
        return;
    }

    //Convert button code to string
    for (i = 0; (i < ARRAYCOUNT(string) - 1) && i < (code->sequenceLength); i++) {
        switch (code->sequence[i]) {
            case A_BUTTON:
                string[i] = 65;
                break;
            case B_BUTTON:
                string[i] = 66;
                break;
            case Z_TRIG:
                string[i] = 90;
                break;
            case L_TRIG:
                string[i] = 76;
                break;
            case R_TRIG:
                string[i] = 82;
                break;
            case START_BUTTON:
                string[i] = 83;
                break;
            case U_JPAD:
                string[i] = 85;
                break;
            case D_JPAD:
                string[i] = 68;
                break;
            case L_JPAD:
                string[i] = 76;
                break;
            case R_JPAD:
                string[i] = 82;
                break;
            case U_CBUTTONS:
                string[i++] = 67;
                string[i] = 117;
                break;
            case D_CBUTTONS:
                string[i++] = 67;
                string[i] = 100;
                break;
            case L_CBUTTONS:
                string[i++] = 67;
                string[i] = 108;
                break;
            case R_CBUTTONS:
                string[i++] = 67;
                string[i] = 114;
                break;
        }
    }

    #ifndef DINOMOD_ROM_PATCH
    recomp_printf("\nBUTTON CODE\n");
    if (code->sequenceLength > 0) {
        recomp_printf("Sequence: %s\n", string);
    }
    recomp_printf("Sequence length: %d\n", code->sequenceLength);
    recomp_printf("Initialised: %d\n", (code->initialised) ? "YES" : "NO");
    #else
    diPrintf("\nBUTTON CODE\n");
    if (code->sequenceLength > 0) {
        diPrintf("Sequence: %s\n", string);
    }
    diPrintf("Sequence length: %d\n", code->sequenceLength);
    diPrintf("Initialised: %d\n", (code->initialised) ? "YES" : "NO");
    #endif
}

/**
  * Checks if the button sequence was completed (if so returns TRUE),
  */
int button_code_entered(ButtonCode* code) {
    u16 buttonsPressed;

    if (!code || 
        !code->initialised || !code->sequence || code->sequenceLength == 0 || 
        code->finished
    ) {
        return 0;
    }

    //Check for button presses
    buttonsPressed = joy_get_pressed(0);
    if (buttonsPressed) {
        if (buttonsPressed & code->sequence[code->position]) {
            code->position++;
        } else {
            code->position = 0;
        }
    }

    //Button sequence entered
    if (code->position >= code->sequenceLength) {
        code->finished = TRUE;
        return 1;
    }

    return 0;
}
