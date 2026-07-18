#include "modding.h"

#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/thread.h"
#include "macros.h"

#include "core/joypad.h"

#define THREAD_STACK_CONTROLLER 1024
#define MAX_BUFFERED_CONT_SNAPSHOTS 4

/**
 * Represents a single snapshot of each controller.
 *
 * In addition to raw OSContPad snapshots, includes bitfields for
 * buttons being pressed and released.
 */
typedef struct _ControllersSnapshot {
    /**
     * A raw controller pad snapshot for each controller.
     */
    OSContPad pads[MAXCONTROLLERS];

    /**
     * For each controller, a button bitfield where set button bits indicate that
     * the button was just pressed down.
     */
    u16 buttonPresses[MAXCONTROLLERS];

    /**
     * For each controller, a button bitfield where set button bits indicate that
     * the button was just released.
     */
    u16 buttonReleases[MAXCONTROLLERS];
} ControllersSnapshot;

extern ControllersSnapshot *gContSnapshots[2];
extern ControllersSnapshot gContSnapshotsBuffer0[MAX_BUFFERED_CONT_SNAPSHOTS];
extern ControllersSnapshot gContSnapshotsBuffer1[MAX_BUFFERED_CONT_SNAPSHOTS];
extern u8 gNumBufContSnapshots[2];
extern u8 gPrevContSnapshotsI;
extern OSMesgQueue gContThreadMesgQueue;
extern OSMesgQueue gContInterruptQueue;
extern s32 gNoControllers;
extern u8 gApplyContInputs;
extern OSContPad gContPads[MAXCONTROLLERS];
extern u16 gButtonPresses[MAXCONTROLLERS];
extern u16 gButtonReleases[MAXCONTROLLERS];
extern s8 gLastJoyX[MAXCONTROLLERS];
extern s8 gLastJoyY[MAXCONTROLLERS];
extern s8 gMenuJoyXHoldTimer[MAXCONTROLLERS];
extern s8 gMenuJoyYHoldTimer[MAXCONTROLLERS];
extern s8 gMenuJoyXSign[MAXCONTROLLERS];
extern s8 gMenuJoyYSign[MAXCONTROLLERS];
extern s8 gMenuJoystickDelay;
extern u16 gButtonMask[MAXCONTROLLERS];
extern u8 gIgnoreJoystick;
extern OSMesgQueue gContThreadInputsAppliedQueue;
extern OSMesg gContInterruptBuffer[2];
extern OSMesg gContThreadMesgQueueBuffer[8];
extern OSMesg gContThreadInputsAppliedQueueBuffer[1];
extern s16 gContQueue2Message;
extern u8 gVirtualContPortMap[MAXCONTROLLERS];
extern u16 D_8008C8A4;

// @recomp: Make menu movements framerate independent
RECOMP_PATCH void joyControllerThreadEntry(void* arg) {
    ControllersSnapshot* currSnap;
    ControllersSnapshot* compSnap;
    s16* message;
    s32 var_a2_2;
    s32 var_a3;
    s32 contIdx;
    s32 snapIdx;
    s32 stickX;
    s32 stickY;

    gContSnapshots[0] = gContSnapshotsBuffer0;
    gContSnapshots[1] = gContSnapshotsBuffer1;
    gNumBufContSnapshots[0] = 0;
    gNumBufContSnapshots[1] = 0;
    message = NULL;
    gPrevContSnapshotsI = 0;
    while (TRUE) {
        osRecvMesg(&gContThreadMesgQueue, (OSMesg *) &message, OS_MESG_BLOCK);
        switch (*message) {
            case 1:
                if (gNumBufContSnapshots[gPrevContSnapshotsI] < MAX_BUFFERED_CONT_SNAPSHOTS) {
                    compSnap = gContSnapshots[gPrevContSnapshotsI ^ 1];
                    compSnap = &compSnap[gNumBufContSnapshots[gPrevContSnapshotsI ^ 1] - 1];
                    currSnap = &gContSnapshots[gPrevContSnapshotsI][gNumBufContSnapshots[gPrevContSnapshotsI]];
                    gNumBufContSnapshots[gPrevContSnapshotsI]++;
                    if ((currSnap - 1) >= gContSnapshots[gPrevContSnapshotsI]) {
                        compSnap = (currSnap - 1);
                    }
                    if (osRecvMesg(&gContInterruptQueue, NULL, 0) == 0) {
                        osContGetReadData(currSnap->pads);
                        osContStartReadData(&gContInterruptQueue);
                    } else {
                        bcopy(compSnap, currSnap, sizeof(ControllersSnapshot));
                    }

                    for (contIdx = 0; contIdx < MAXCONTROLLERS; contIdx++) {
                        if (gNoControllers != 0) {
                            currSnap->pads[contIdx].button = 0;
                        }
                        currSnap->buttonPresses[contIdx] = (currSnap->pads[contIdx].button ^ compSnap->pads[contIdx].button) & currSnap->pads[contIdx].button & D_8008C8A4;
                        currSnap->buttonReleases[contIdx] = (currSnap->pads[contIdx].button ^ compSnap->pads[contIdx].button) & compSnap->pads[contIdx].button & D_8008C8A4;
                    }
                }
                if (gApplyContInputs != 0) {
                    bzero(gContPads, sizeof(gContPads));
                    for (contIdx = 0; contIdx < 4; contIdx++) {
                        var_a2_2 = 0;
                        var_a3 = 0;
                        gButtonPresses[contIdx] = 0;
                        gButtonReleases[contIdx] = 0;
                        
                        currSnap = gContSnapshots[gPrevContSnapshotsI];
                        for (snapIdx = 0; snapIdx < gNumBufContSnapshots[gPrevContSnapshotsI]; snapIdx++) {
                            gButtonPresses[contIdx] |= currSnap->buttonPresses[contIdx];
                            gButtonReleases[contIdx] |= currSnap->buttonReleases[contIdx];
                            gContPads[contIdx].button |= currSnap->pads[contIdx].button;
                            var_a2_2 += currSnap->pads[contIdx].stick_x;
                            var_a3 += currSnap->pads[contIdx].stick_y;
                            currSnap++;
                        }
                        gContPads[contIdx].stick_x = var_a2_2 / gNumBufContSnapshots[gPrevContSnapshotsI];
                        gContPads[contIdx].stick_y = var_a3 / gNumBufContSnapshots[gPrevContSnapshotsI];
                        stickX = gContPads[contIdx].stick_x;
                        stickY = gContPads[contIdx].stick_y;
                        gMenuJoyXSign[contIdx] = 0;
                        gMenuJoyYSign[contIdx] = 0;
                        
                        if ((stickX < -35) && (gLastJoyX[contIdx] >= -35)) {
                            gMenuJoyXSign[contIdx] = -1;
                            gMenuJoyXHoldTimer[contIdx] = 0;
                        }
                        if ((stickX > 35) && (gLastJoyX[contIdx] <= 35)) {
                            gMenuJoyXSign[contIdx] = 1;
                            gMenuJoyXHoldTimer[contIdx] = 0;
                        }
                        if ((stickY < -35) && (gLastJoyY[contIdx] >= -35)) {
                            gMenuJoyYSign[contIdx] = -1;
                            gMenuJoyYHoldTimer[contIdx] = 0;
                        }
                        if ((stickY > 35) && (gLastJoyY[contIdx] <= 35)) {
                            gMenuJoyYSign[contIdx] = 1;
                            gMenuJoyYHoldTimer[contIdx] = 0;
                        }
                        gLastJoyY[contIdx] = stickY;
                        if (gLastJoyY[contIdx] < -35) {
                            gMenuJoyYHoldTimer[contIdx] += gUpdateRate; // @recomp
                        } else if (gLastJoyY[contIdx] > 35) {
                            gMenuJoyYHoldTimer[contIdx] += gUpdateRate; // @recomp
                        } else {
                            gMenuJoyYHoldTimer[contIdx] = 0;
                        }
                        if ((gMenuJoystickDelay * 2) < gMenuJoyYHoldTimer[contIdx]) {  // @recomp
                            gLastJoyY[contIdx] = 0;
                            gMenuJoyYHoldTimer[contIdx] = 0;
                        }
                        gLastJoyX[contIdx] = stickX;
                        if (gLastJoyX[contIdx] < -35) {
                            gMenuJoyXHoldTimer[contIdx] += gUpdateRate; // @recomp
                        } else if (gLastJoyX[contIdx] > 35) {
                            gMenuJoyXHoldTimer[contIdx] += gUpdateRate; // @recomp
                        } else {
                            gMenuJoyXHoldTimer[contIdx] = 0;
                        }
                        if ((gMenuJoystickDelay * 2) < gMenuJoyXHoldTimer[contIdx]) {  // @recomp
                            gLastJoyX[contIdx] = 0;
                            gMenuJoyXHoldTimer[contIdx] = 0;
                        }
                        gButtonMask[contIdx] = 0xFFFF;
                    }
                    gIgnoreJoystick = 0;
                    gApplyContInputs = 0;
                    gPrevContSnapshotsI ^= 1;
                    gNumBufContSnapshots[gPrevContSnapshotsI] = 0;
                    osSendMesg(&gContThreadInputsAppliedQueue, &gContQueue2Message, 0);
                }
                break;
            case 0xA:
            default:
                gApplyContInputs = 1;
                break;
        }
    }
}

/* Ignore joystick inputs */
void joy_disable_stick(int port) {
    gMenuJoyXSign[gVirtualContPortMap[port]] = 0;
    gMenuJoyYSign[gVirtualContPortMap[port]] = 0;
}
