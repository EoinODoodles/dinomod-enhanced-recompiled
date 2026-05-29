#pragma once

#include "game/objects/object.h"

void SHbarrel_ctor(void *dll);
void SHbarrel_dtor(void *dll);

extern DLL_IObject_Vtbl DLL_SHbarrel_vtbl;
