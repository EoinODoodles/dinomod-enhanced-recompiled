#pragma once

#include "recomputils.h"
#include "PR/ultratypes.h"
#include "common.h"

/** A handler for checking whether the player entered a specific sequence of button presses */
typedef struct {
    u16* sequence;      //A pointer to a u16 array of N64 buttonIDs (`A_BUTTON`, `B_BUTTON`, etc.)
    u8 sequenceLength;  //The length of `sequence` (the buttonID array)
    u8 position;        //How far the player has progressed through the sequence of presses
    u8 finished;        //Sequence complete, stops reading
    u8 initialised;     //Handler ready to use (see `button_code_setup`)
} ButtonCode;

void button_code_setup(ButtonCode* handler, u16* buttonSequence, u8 buttonSequenceLength);
void button_code_print(ButtonCode* code);
int button_code_entered(ButtonCode* code);
