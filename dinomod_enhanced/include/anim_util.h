#pragma once

/** Helper functions for animations */

#include "object_util.h"
#include "recomputils.h"
#include "sys/objanim.h"

#define GET_SEQID(sequenceIdBitfield) ((sequenceIdBitfield >> 4) & 0x7FF)

int object_modanim_debugger(Object* obj);
