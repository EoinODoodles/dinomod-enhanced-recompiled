#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "sys/dll.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"

#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/210_player.h"
#include "recomp/dlls/objects/297_scarab_recomp.h"

typedef struct {
s32 soundHandle;
s8 unk4[0x10 - 0x4];
f32 unk10; //y
s16 unk14;
s8 unk16[0x18 - 0x16];
s16 unk18;
s16 unk1A;
s8 unk1C[0x20 - 0x1C];
s16 unk20;
s16 unk22;
s16 unk24;
u8 unk26;
s8 unk27[0x29 - 0x27];
u8 unk29; //scarabTypeIndex
} ScarabState;

typedef struct {
ObjCreateInfo base;
s16 unk18;
s16 unk1A;
} ScarabCreateInfo;

typedef union {
    u8 bytes[4];
    u32 word;
} ScarabValues;

/*0x20*/ extern ScarabValues _data_20;

// Enables the Scarab UI counter upon collection
RECOMP_PATCH void dll_297_func_1284(Object* self, Object* player, ScarabState* state) {
  ScarabValues values = _data_20;

  //@recomp: enable Scarab counter UI
  main_set_bits(0x919, 1);
    
  ((DLL_Unknown*)player->dll)->vtbl->func[19].withTwoArgs((s32) player, values.bytes[state->unk29]);
    
  state->unk14 = 0x50;
  state->unk18 = 0;
}
