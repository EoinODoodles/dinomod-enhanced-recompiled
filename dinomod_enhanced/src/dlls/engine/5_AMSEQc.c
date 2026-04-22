#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "PR/os.h"
#include "PR/ultratypes.h"
#include "PR/ultratypes.h"
#include "game/gamebits.h"
#include "game/objects/object_id.h"
#include "libnaudio/n_libaudio.h"
#include "sys/audio/amAudio.h"
#include "sys/asset_thread.h"
#include "sys/audio.h"
#include "sys/fs.h"
#include "sys/main.h"
#include "sys/memory.h"
#include "sys/menu.h"
#include "dll.h"

#include "recomp/dlls/engine/5_AMSEQ_recomp.h"

// size:0x24C
typedef struct {
/*000*/ u8 unk0;
/*004*/ N_ALCSPlayer seqp;
/*090*/ ALCSeq seq;
/*118*/ void *midiData;
/*18C*/ u8 currentSeqID;
/*18E*/ s16 bpm;
/*190*/ s16 volume;
/*192*/ u16 unk192;
/*194*/ u16 unk194; // enabled channels?
/*196*/ u16 unk196; // dirty channels?
/*198*/ u16 unk198; // ignore channels?
/*19A*/ u8 volUpRate; // volume per tick
/*19B*/ u8 volDownRate; // volume per tick
/*19C*/ s16 channelVolumes[16];
/*1BC*/ u8 _unk1BC[0x1FC - 0x1BC];
/*1FC*/ u8 nextSeqID; // current music/ambient id?
/*1FE*/ s16 nextBPM;
/*200*/ u16 unk200;
/*202*/ s16 targetVolume;
/*204*/ u16 unk204;
/*206*/ u16 unk206;
/*208*/ u16 unk208;
/*20A*/ u8 nextVolUpRate; // volume per tick
/*20B*/ u8 nextVolDownRate; // volume per tick
/*20C*/ u8 _unk20C[0x24C - 0x20C];
} AMSEQPlayer;

typedef struct {
/*0*/ s32 unk0;
/*4*/ u8 _unk4[4];
} AMSEQBss18; 

typedef struct {
/*00*/ Object *obj;
/*04*/ MusicAction action;
/*24*/ s16 actionNo;
/*26*/ s16 unk26;
/*28*/ s16 unk28;
} AMSEQHandle;

extern u8 _data_0;
extern u8 _data_4;
extern Object *sFocusObj;
extern MusicAction* sMusicAction;

extern AMSEQPlayer **sSeqPlayers; // pointer to array of 4 pointers
extern ALBankFile **sBankFiles;
extern ALSeqFile *sSeqFiles[2];
extern struct oscstate *sOscStates;
extern struct oscstate *sFreeOscStateList;
extern AMSEQBss18 *_bss_18;
extern s32 _bss_1C;
extern AMSEQHandle *sSeqHandles;
extern s32 sNumSeqHandles; // current number of sequence handles
extern u16 sVolumeGameOption; // set by game options, scales amseq volume
extern u16 sUnkVolume; // set to 256 and never changed, scales amseq volume
extern u16 sCurrGlobalVolume;

extern s32 amseq_func_1E8C(MusicAction *action, s8 a1, s16 actionNo);

RECOMP_PATCH s32 amseq_set(Object *obj, u16 actionNo, const char *filename, s32 lineNo, const char *debugStr) {
    s32 i;
    s32 temp_v1;

    //@recomp: allow music to continue in Game Select (TODO: find where this is called from and patch it instead?)
    if ((menu_get_previous() == MENU_TITLE_SCREEN) && (menu_get_current() == MENU_GAME_SELECT)) {
        if (actionNo == 0xbb || actionNo == 0xda) {
            return -1;
        }
    }

    if (!actionNo) {
        return -1;
    }
    // "music %08x,%d\n"
    queue_load_file_region_to_ptr((void** ) sMusicAction, MUSICACTIONS_BIN, (actionNo - 1) * sizeof(MusicAction), sizeof(MusicAction));
    if (obj != NULL && (sMusicAction->unk18 & sMusicAction->unk1A) != 0) {
        // "object+fade\n"
        temp_v1 = sMusicAction->unk18 & sMusicAction->unk1A;
        if (temp_v1 != 0) { // lolwat
            for (i = 0; i < sNumSeqHandles; i++) {
                if (sSeqHandles[i].obj == obj) {
                    break;
                }
            }
            if (i == sNumSeqHandles) {
                sNumSeqHandles++;
                if (sNumSeqHandles == 16) {
                    // "AUDIO: Maximum sequence handles reached.\n"
                }
                sSeqHandles[i].actionNo = actionNo;
                sSeqHandles[i].obj = obj;
                sSeqHandles[i].unk28 = -1;
                sSeqHandles[i].unk26 = sSeqHandles[i].unk28;
                bcopy(sMusicAction, &sSeqHandles[i].action, sizeof(MusicAction));
                // "registered %d,%08x\n"
            } else {
                // "already registered %d,%08x\n";
            }
            if (temp_v1 != 0xFFFF) {
                // "starting non-fade channels\n"
                return amseq_func_1E8C(sMusicAction, 0, actionNo);
            }
        }
        goto bail;
    }
    // "starting static sequence\n"
    return amseq_func_1E8C(sMusicAction, 0, actionNo);

    bail:
    return -1;
}
