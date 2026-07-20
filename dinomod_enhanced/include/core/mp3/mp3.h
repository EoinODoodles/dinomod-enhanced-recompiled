#pragma once

#include "PR/ultratypes.h"
#include "mp3/mp3_internal.h"

extern void mp3Pause();
extern void mp3Unpause();

/* Checks whether an MP3 is currently playing. */
s32 mp3IsPlaying(void) {
    return (g_Mp3Vars.state == MP3STATE_PLAYING);
}

/* Checks whether an MP3 is currently paused. */
s32 mp3IsPaused(void) {
    return (g_Mp3Vars.state == MP3STATE_PAUSED);
}

/* Safely pauses an MP3 (avoiding a crash if no MP3 is playing). */
void mp3PauseIfPlaying(void) {
    if (mp3IsPlaying()) {
        mp3Pause();
    }
}

/* Safely unpauses an MP3. */
void mp3UnpauseIfPaused(void) {
    if (mp3IsPaused()) {
        mp3Unpause();
    }
}
