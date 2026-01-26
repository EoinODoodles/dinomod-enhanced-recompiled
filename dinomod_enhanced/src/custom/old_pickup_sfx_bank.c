#include "modding.h"
#include "mod_common.h"

#include "old_pickup_sfx_bank.h"

#include "PR/libaudio.h"
#include "sys/memory.h"

extern ALBankFile **__dll5_sBankFiles;

ALBank *recomp_oldPickupSfxBank;

/**
 * Create a custom sound bank containing the two variants of the old item pickup jingle.
 * Original patch by nuggs.
 */
RECOMP_HOOK_RETURN("game_init") void dinomod_init_old_pickup_sound(void) {
    // The old pickup sounds are actually instrument sounds in the music sound bank.
    // We'll be playing it from the loaded AMSEQ bank data.
    ALBank *musicBank = __dll5_sBankFiles[1]->bankArray[0];

    // SFX expects a bank with exactly one instrument and each sound to be a part of that instrument
    const s32 numSounds = 2;
    const u32 bankSize = sizeof(ALBank) + (sizeof(ALInstrument*));
    const u32 instSize = (sizeof(ALInstrument)) + (sizeof(ALSound*) * numSounds);
    const u32 soundsSize = (sizeof(ALSound) * numSounds);
    const u32 envelopSize = sizeof(ALEnvelope);
    const u32 keyMapSize = sizeof(ALKeyMap);
    const u32 allocSize = bankSize + instSize + soundsSize + envelopSize + keyMapSize;

    u8 *ptr = mmAlloc(allocSize, 0, NULL);
    bzero(ptr, allocSize);

    ALBank *bank =         (ALBank*)       ptr;
    ALInstrument *inst =   (ALInstrument*)(ptr + bankSize);
    ALSound *sounds =      (ALSound*)     (ptr + bankSize + instSize);
    ALEnvelope *envelope = (ALEnvelope*)  (ptr + bankSize + instSize + soundsSize);
    ALKeyMap *keyMap =     (ALKeyMap*)    (ptr + bankSize + instSize + soundsSize + envelopSize);

    bank->sampleRate = musicBank->sampleRate;
    bank->instCount = 1;
    bank->instArray[0] = inst;

    inst->volume = 0x7f; // full volume
    inst->pan = AL_PAN_CENTER;
    inst->priority = 5;
    inst->soundCount = numSounds;
    for (s32 i = 0; i < numSounds; i++) {
        inst->soundArray[i] = &sounds[i];
    }

    bcopy(musicBank->instArray[0x75]->soundArray[0xC], &sounds[0], sizeof(ALSound)); // variant 1
    bcopy(musicBank->instArray[0x75]->soundArray[0xD], &sounds[1], sizeof(ALSound)); // variant 2

    for (s32 i = 0; i < numSounds; i++) {
        // We need a custom envelope and key map as the SFX player interprets these differently
        // than the music sequence player
        ALSound *sound = &sounds[i];
        sound->envelope = envelope;
        sound->keyMap = keyMap;
    }

    envelope->decayTime = 6462404; // duration: 6.462404 seconds
    envelope->attackVolume = 0x7f; // full volume
    envelope->decayVolume = 0x7f; // full volume

    keyMap->keyBase = 60; // default pitch

    recomp_oldPickupSfxBank = bank;
}
