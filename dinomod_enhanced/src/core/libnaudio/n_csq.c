#include "modding.h"
#include "recompevents.h"
#include "recomputils.h"

#include "PR/libaudio.h"
#include "libc/string.h"
#include "libnaudio/n_libaudio.h"
#include "game/objects/object.h"
#include "sys/asset.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "sys/map.h"
#include "sys/memory.h"
#include "sys/pi.h"
#include "sys/print.h"

// #define DEBUG_MUSIC
#define DEBUG_STOP_LOGGING_TRACK_AFTER_LOOP

#ifdef DEBUG_MUSIC

extern u8 __getTrackByte(ALCSeq *seq, u32 track);
extern u32 __readVarLen(ALCSeq *seq, u32 track);
extern u32 __n_alCSeqGetTrackEvent(ALCSeq* seq, u32 track, N_ALEvent* event, s32 arg3);

#define TRACKIDX(idx) (1 << idx)

//DEBUG OPTION MACROS

//Which musicID to log
#define DEBUG_MUSICID 0x3F       
//A bitfield of which channels to log (all are logged when < 0)
#define DEBUG_TRACK_BITFIELD (TRACKIDX(5)) 
//Optionally only start logging once the midi ticks have crossed this time threshold
#define DEBUG_START_TIME 0

static u16 DEBUG_ON = 0;                //A bitfield of the track indices to log
static ALCSeq* DEBUG_SEQ = NULL;        //A pointer to the seq struct of the musicID we're interested in
static u8 DEBUG_REACHED_TIME = FALSE;   //Whether the seq has reached the time we should start logging from
static u32 DEBUG_TRACK_TIME_FRAME = 0;  
static u32 DEBUG_TRACK_TIME_TICKS = 0;
static u32 TRACK_TIMES[] = { 
    0, 0, 0, 0, 
    0, 0, 0, 0, 
    0, 0, 0, 0, 
    0, 0, 0, 0 
};
static u32* DEBUG_SONG_HEADER = NULL;  //The header trackOffsets of the musicID we're interested in, used to identify when the seq is playing

/* Stores the header trackOffsets of the musicID we're interested in, so we can check if a ALCSeq is playing the music later */
static void debugGetSongHeader(void) {
    s32* tab = NULL;
    u8* bin = NULL;
    u32 length;
    ALSeqFile* midis = NULL;
    ALCMidiHdr* song = NULL;
    s32 i;

    if (DEBUG_SONG_HEADER != NULL) {
        return;
    }

    //Get AUDIO.tab
    assetRomLoad((void**)&tab, AUDIO_TAB);

    //Get AUDIO.bin (subfile 1, music MIDIs)
    length = tab[2] - tab[1];
    if (length != 0) {
        midis = mmAlloc(length, ALLOC_TAG_SEQ_COL, NULL);
        assetRomLoadSection((void*)midis, AUDIO_BIN, tab[1], length);
    }
    mmFree(tab);
    if (midis == NULL) {
        return;
    }

    //Get the song we're interested in
    if (DEBUG_MUSICID >= midis->seqCount) {
        return;
    }
    song = (ALCMidiHdr*)((u32)midis + (u32)midis->seqArray[DEBUG_MUSICID].offset);

    //Store its header's track offsets, so we can use this as a rough signature to check if it's playing later
    DEBUG_SONG_HEADER = recomp_alloc(sizeof(u32) * 16);
    if (DEBUG_SONG_HEADER) {
        for (i = 0; i < 16; i++) {
            DEBUG_SONG_HEADER[i] = song->trackOffset[i];
        }
    }
}

/** Compares the current ALCSeq's header against the header of the music we're interested in */
static _Bool debugIsMusicOfInterestPlaying(ALCSeq* seq) {
    if (DEBUG_SONG_HEADER == NULL) {
        debugGetSongHeader();
    }
    if (DEBUG_SONG_HEADER == NULL) {
        return FALSE;
    }

    for (u32 i = 0; i < 16; i++) {
        if (DEBUG_SONG_HEADER[i] != seq->base->trackOffset[i]) {
            return FALSE;
        }
    }

    return TRUE;
}

/* Checks if a particular trackIdx should have its events/reads logged */
static _Bool debugIsTrackLogged(ALCSeq* seq, u32 trackIdx) {
    if (DEBUG_SEQ == NULL) {
        return FALSE;
    }

    if ((DEBUG_ON & TRACKIDX(trackIdx)) == 0) {
        return FALSE;
    }

    if (DEBUG_TRACK_BITFIELD < 0) {
        return TRUE;
    } else {
        return (DEBUG_TRACK_BITFIELD & (1 << trackIdx)) != 0;
    }
}

/** A wrapper for `recomp_printf`, adding a trackIdx prefix before a track log 
  * when multiple tracks are being logged, to be able to tell them apart. 
  */
static void debugLogTrack(ALCSeq* seq, u32 trackIdx, char* format, ...) {
    va_list args;
    va_start(args, format);

    if (debugIsTrackLogged(seq, trackIdx) == FALSE) {
        return;
    }
    
    if ((DEBUG_TRACK_BITFIELD & ~TRACKIDX(trackIdx)) != 0) {
        char prefixedFormat[0xFF] = { 0 };
        u8 prefixLength;
        u8 formatLength;
        recomp_sprintf(prefixedFormat, "[TRACK %2d]", trackIdx);
        prefixLength = strlen(prefixedFormat);
        formatLength = strlen(format);
        if (formatLength > 0) {
            prefixedFormat[prefixLength] = ' ';
            bcopy(format, &prefixedFormat[prefixLength + 1], formatLength);
        }
        recomp_vprintf(prefixedFormat, args);
    } else {
        recomp_vprintf(format, args);
    }

    va_end(args);
}

/* Keep track of how many frames the music's been playing for */
RECOMP_ON_GAME_TICK_START_CALLBACK void musicTimeCrossrefFrame(void) {
    if (DEBUG_SEQ) {
        DEBUG_TRACK_TIME_FRAME += gUpdateRate;
    }
}

/* Keep track of how many ticks the music's been playing for (TODO: not sure this is accurate?) */
RECOMP_HOOK("n_alCSeqNextEvent") void musicTimeCrossrefTick(void) {
    if (DEBUG_SEQ) {
        DEBUG_TRACK_TIME_TICKS += DEBUG_SEQ->lastDeltaTicks;
    }
}

/** Returns the track's current read offset, relative to the start of the MIDI file. */
static u32 getTrackOffset(ALCSeq* seq, u32 track) {
    return (u32)seq->curLoc[track] - (u32)seq->base;
}

/** Returns the back-up offset currently being re-read from, relative to the start of the MIDI file */
static u32 getBUOffset(ALCSeq* seq, u32 track) {
    return (u32)seq->curBUPtr[track] - (u32)seq->base;
}

/** Returns the offset currently being read from, relative to the start of the MIDI file. 
    If the track reader's currently "backed up" via a repeat code, the offset currently being re-read will be returned.  */
static u32 getReadOffset(ALCSeq* seq, u32 track) {
    if (seq->curBULen[track] > 0) {
        return getBUOffset(seq, track);
    } else {
        return getTrackOffset(seq, track);
    }
}

static char* getNoteName(u8 tone) {
    static char noteNames[][3] = {
        "C", "C#", "D", "D#", 
        "E", "F", "F#", "G", 
        "G#", "A", "A#", "B"
    };
    return noteNames[tone % 12];
}

RECOMP_PATCH void n_alCSeqNew(ALCSeq* seq, u8* ptr) {
	u32 i, tmpOff, flagTmp;

	/* load the seqence pointed to by ptr   */
	seq->base = (ALCMidiHdr*)ptr;
	seq->validTracks = 0;
	seq->lastDeltaTicks = 0;
	seq->lastTicks = 0;
	seq->deltaFlag = 1;

	for (i = 0; i < 16; i++) {
		seq->lastStatus[i] = 0;
		seq->curBUPtr[i] = 0;
		seq->curBULen[i] = 0;
		tmpOff = seq->base->trackOffset[i];

		if (tmpOff) {
			flagTmp = 1 << i;
			seq->validTracks |= flagTmp;
			seq->curLoc[i] = (u8*)((uintptr_t)ptr + tmpOff);
			seq->evtDeltaTicks[i] = __readVarLen(seq,i);

            TRACK_TIMES[i] = seq->evtDeltaTicks[i];
		} else {
			seq->curLoc[i] = 0;
		}
	}

    //DEBUG: Check if the musicID we're interested in is playing 
    if (DEBUG_SEQ == NULL) {
        if (debugIsMusicOfInterestPlaying(seq)) {
            recomp_printf("MUSIC ID %d FOUND\n", DEBUG_MUSICID);
            DEBUG_SEQ = seq;
            DEBUG_ON = DEBUG_TRACK_BITFIELD;
            DEBUG_TRACK_TIME_FRAME = 0;
            DEBUG_TRACK_TIME_TICKS = 0;
        }
    }

	seq->qnpt = 1.0f / (f32)seq->base->division;
}

RECOMP_PATCH void n_alCSeqNextEvent(ALCSeq* seq, N_ALEvent* evt, s32 arg2) {
	u32 i;
	u32 firstTime = 0xffffffff;
	u32 firstTrack;
	u32 lastTicks = seq->lastDeltaTicks;

	for (i = 0; i < 16; i++) {
		if ((seq->validTracks >> i) & 1) {
			if (seq->deltaFlag) {
				seq->evtDeltaTicks[i] -= lastTicks;
			}

			if (seq->evtDeltaTicks[i] < firstTime) {
				firstTime = seq->evtDeltaTicks[i];
				firstTrack = i;
			}
		}
	}

	__n_alCSeqGetTrackEvent(seq, firstTrack, evt, arg2);

	evt->msg.midi.ticks = firstTime;
	seq->lastTicks += firstTime;
	seq->lastDeltaTicks = firstTime;

	if (evt->type != AL_TRACK_END) {
        u32 readAddr = getReadOffset(seq, firstTrack);
        u32 evtDeltaTicksPre = seq->evtDeltaTicks[firstTrack];
        u32 debugDelta;

		seq->evtDeltaTicks[firstTrack] += __readVarLen(seq, firstTrack);

        debugDelta = seq->evtDeltaTicks[firstTrack] - evtDeltaTicksPre;

        TRACK_TIMES[firstTrack] += seq->evtDeltaTicks[firstTrack] - evtDeltaTicksPre;

        debugLogTrack(seq, firstTrack, "timeOffset: 0x%x (%d) @ 0x%x\n",
            debugDelta, debugDelta, readAddr);
	}

	seq->deltaFlag = 1;

    if (debugIsTrackLogged(seq, firstTrack)) {
        if (DEBUG_REACHED_TIME == FALSE) {
            DEBUG_REACHED_TIME = TRUE;
        }
    }
}

RECOMP_PATCH u32 __n_alCSeqGetTrackEvent(ALCSeq* seq, u32 track, N_ALEvent* event, s32 arg3) {
	u32 offset;
	u8 status, loopCt, curLpCt, *tmpPtr;

	status = __getTrackByte(seq, track);

	if (status == AL_MIDI_Meta) {
        debugLogTrack(seq, track, "META EVENT\n");

		u8 type = __getTrackByte(seq, track);

        debugLogTrack(seq, track, "Meta type: 0x%x\n", type);

		if (type == AL_MIDI_META_TEMPO) {
			event->type = AL_TEMPO_EVT;
			event->msg.tempo.status = status;
			event->msg.tempo.type = type;
			event->msg.tempo.byte1 = __getTrackByte(seq, track);
			event->msg.tempo.byte2 = __getTrackByte(seq, track);
			event->msg.tempo.byte3 = __getTrackByte(seq, track);
			seq->lastStatus[track] = 0;  /* lastStatus not supported after meta */
		} else if (type == AL_MIDI_META_EOT) {
			u32 flagMask;

			flagMask = 1 << track;
			seq->validTracks = seq->validTracks ^ flagMask;

			if (seq->validTracks) { /* there is music left don't end */
				event->type = AL_TRACK_END;
			} else {       /* no more music send AL_SEQ_END_EVT msg */
				event->type = AL_SEQ_END_EVT;
			}
		} else if (type == AL_CMIDI_LOOPSTART_CODE) {
            debugLogTrack(seq, track, "LOOP START @ 0x%x\n", getReadOffset(seq, track) - 1);

			status = __getTrackByte(seq, track);
			event->msg.loop.count = status << 8;

			status = __getTrackByte(seq, track);
			event->msg.loop.count += status;

			seq->lastStatus[track] = 0;
			event->type = AL_CSP_LOOPSTART;
		} else if (type == AL_CMIDI_LOOPEND_CODE) {
            debugLogTrack(seq, track, "LOOP START @ 0x%x\n", getReadOffset(seq, track) - 1);

            tmpPtr = seq->curLoc[track];
			loopCt = *tmpPtr++;
			curLpCt = *tmpPtr;

			if (curLpCt == 0 || !arg3) {
				*tmpPtr = loopCt; /* reset current loop count */
				seq->curLoc[track] = tmpPtr + 5; /* move pointer to end of event */
			} else {
				if (curLpCt != 0xff) { /* not a loop forever */
					*tmpPtr = curLpCt - 1;   /* decrement current loop count */
				}

				tmpPtr++;                    /* get offset from end of event */
				offset = (*tmpPtr++) << 24;
				offset += (*tmpPtr++) << 16;
				offset += (*tmpPtr++) << 8;
				offset += *tmpPtr++;
				seq->curLoc[track] = tmpPtr - offset;

                debugLogTrack(seq, track, 
                    "Looping back to offset 0x%x\n", 
                    getReadOffset(seq, track));
			}

			seq->lastStatus[track] = 0;
			event->type = AL_CSP_LOOPEND;

#ifdef DEBUG_STOP_LOGGING_TRACK_AFTER_LOOP
            if (debugIsTrackLogged(seq, track)) {
                DEBUG_ON &= ~TRACKIDX(track);
            }
#endif
		}

        if (debugIsTrackLogged(seq, track)) {
            switch (type) {
            case AL_MIDI_META_TEMPO:
                debugLogTrack(seq, track, 
                    "%d) SetTempo, ticksPerQuarterNote: %d\n", 
                    TRACK_TIMES[track],
                    (event->msg.tempo.byte1 << 16) | (event->msg.tempo.byte2 << 8) | (event->msg.tempo.byte3) 
                );
                break;
            case AL_MIDI_META_EOT:
                debugLogTrack(seq, track, 
                    "%d) EndOfTrack\n", 
                    TRACK_TIMES[track]);
                break;
            case AL_CMIDI_LOOPSTART_CODE:
                debugLogTrack(seq, track, 
                    "%d) StartLoop: loopNumber %d, endLoop %d\n", 
                    TRACK_TIMES[track], (event->msg.loop.count >> 8) & 0xFF, event->msg.loop.count & 0xFF);
                break;
            case AL_CMIDI_LOOPEND_CODE:
                debugLogTrack(seq, track, 
                    "%d) EndLoop: loopCount %d, currentLoopCount %d, offsetToBeginningLoop: 0x%x\n", 
                    TRACK_TIMES[track], loopCt, curLpCt, offset);
                break;
            }
        }

	} else {
		event->type = AL_SEQ_MIDI_EVT;

		if (status & 0x80) {
			event->msg.midi.status = (status & 0xf0) | track;
			event->msg.midi.byte1 = __getTrackByte(seq,track);
			seq->lastStatus[track] = event->msg.midi.status;
		} else {    /* running status */
			event->msg.midi.status = seq->lastStatus[track];
			event->msg.midi.byte1 = status;
		}

		if ((event->msg.midi.status & 0xf0) != AL_MIDI_ProgramChange
				&& (event->msg.midi.status & 0xf0) != AL_MIDI_ChannelPressure) {
			event->msg.midi.byte2 = __getTrackByte(seq,track);

			if ((event->msg.midi.status & 0xf0) == AL_MIDI_NoteOn) {
				event->msg.midi.duration = __readVarLen(seq,track);
			}
		} else {
			event->msg.midi.byte2 = 0;
		}

        if (debugIsTrackLogged(seq, track)) {
            switch (event->msg.midi.status & 0xf0) {
            case AL_MIDI_NoteOn:
                debugLogTrack(seq, track, 
                    "%d) Note: tone: 0x%x (%s%d), velocity: 0x%x, duration: %d\n", 
                    TRACK_TIMES[track], 
                    event->msg.midi.byte1, getNoteName(event->msg.midi.byte1), event->msg.midi.byte1/12,
                    event->msg.midi.byte2, 
                    event->msg.midi.duration
                );
                break;
            case AL_MIDI_ProgramChange:
                debugLogTrack(seq, track, 
                    "%d) InstrumentChange: instrument: %d\n", 
                    TRACK_TIMES[track], event->msg.midi.byte1);
                break;
            case AL_MIDI_ChannelPressure:
                debugLogTrack(seq, track, 
                    "%d) AfterTouch: amount: %d\n", 
                    TRACK_TIMES[track], event->msg.midi.byte1);
                break;
            case AL_MIDI_PitchBendChange:
                debugLogTrack(seq, track, 
                    "%d) PitchBend: lsb: 0x%x, msb: 0x%x\n", 
                    TRACK_TIMES[track], event->msg.midi.byte1, event->msg.midi.byte2);
                break;
            case AL_MIDI_ControlChange:
                debugLogTrack(seq, track, 
                    "%d) ControllerChange: controller: 0x%x, value: 0x%x\n", 
                    TRACK_TIMES[track], event->msg.midi.byte1, event->msg.midi.byte2);
                break;
            }
        }
	}

	return TRUE;
}

RECOMP_PATCH u8 __getTrackByte(ALCSeq* seq, u32 track) {
    u8 theByte;
    u8 loBackUp;
    u8 hiBackUp;
    u8 theLen;
    u8 nextByte;
    u32 backup;

    u8 initialBULen = seq->curBULen[track];

    u32 initialLoc = (u32)seq->curLoc[track];

    if (seq->curBULen[track] != 0) {
        theByte = *seq->curBUPtr[track];
        seq->curBUPtr[track] += 1;
        seq->curBULen[track] -= 1;

        debugLogTrack(seq, track, 
            "Read BACKUP byte 0x%x @ 0x%x\n", 
            theByte, getBUOffset(seq, track) - 1);
    } else {
        theByte = *seq->curLoc[track];
        seq->curLoc[track] += 1;

        debugLogTrack(seq, track, 
            "Read byte 0x%x @ 0x%x\n", 
            theByte, getTrackOffset(seq, track) - 1);

        if (theByte == AL_CMIDI_BLOCK_CODE) {
            nextByte = *seq->curLoc[track];
            seq->curLoc[track] += 1;
            
            debugLogTrack(seq, track, 
                "Read byte 0x%x @ 0x%x\n", 
                nextByte, getTrackOffset(seq, track) - 1);

            if (nextByte != AL_CMIDI_BLOCK_CODE) {

                hiBackUp = nextByte;
                loBackUp = *seq->curLoc[track];

                seq->curLoc[track] += 1;

                theLen = *seq->curLoc[track];

                seq->curLoc[track] += 1;

                backup = hiBackUp;
                backup <<= 8;
                backup += loBackUp;

                seq->curBUPtr[track] = seq->curLoc[track] - (backup + 4);
                seq->curBULen[track] = theLen;

                debugLogTrack(seq, track, 
                    "REPEAT CODE @ 0x%x\n", 
                    getTrackOffset(seq, track) - 4);
                debugLogTrack(seq, track, 
                    "NOW RE-READING SECTION: 0x%x to 0x%x\n", 
                    getBUOffset(seq, track), getBUOffset(seq, track) + theLen);

                theByte = *seq->curBUPtr[track];

                seq->curBUPtr[track] += 1;
                seq->curBULen[track] -= 1;

                debugLogTrack(seq, track, 
                    "Read BACKUP byte 0x%x @ 0x%x\n", 
                    theByte, getBUOffset(seq, track) - 1);
            }
        }
    }

    if (initialBULen > 0 && seq->curBULen[track] == 0) {
        debugLogTrack(seq, track, 
            "Finished reading BACKUP range, continuing from 0x%x\n", 
            getReadOffset(seq, track));
    }

    return theByte;
}
#endif
