#include "modding.h"
#include "recomputils.h"

#include "dlls/objects/437_EWTrobotpatrol.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/curves.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objprint.h"
#include "sys/rand.h"
#include "dll.h"
#include "macros.h"

#include "recomp/dlls/objects/439_EWTrobotpatrolB_recomp.h"

#define MAX_ROBOS 12
#define MAX_NODES 24

typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 unk18; // unused due to, presumably, missing code. likely the number of robos to spawn 
/*19*/ u8 numNodes; // set but unused, the dll discovers this count on its own
/*1A*/ u8 roboFadeDistance;
/*1B*/ u8 maxSearchTime; // maximum search time (after aggro) divided by 64
} EWTrobotpatrolB_Setup;

typedef struct {
/*00*/ Object* robos[MAX_ROBOS];
/*30*/ CurveSetup* nodes[MAX_NODES];
/*90*/ u8 numNodes;
/*91*/ u8 engagingPlayer;
/*92*/ u8 searchingForPlayer;
/*94*/ s16 aggroCounter;
/*96*/ s16 lastAggroCounter;
/*98*/ f32 searchTimeElapsed;
/*@RECOMP*/
    s32 roboCurveUIDs[MAX_ROBOS]; // where each robo is heading
} EWTrobotpatrolB_Data;

/*0x0*/ extern CurveSetup* sPathfindVisited[MAX_NODES];
/*0x60*/ extern CurveSetup* sPathfindGoal;
/*0x64*/ extern Object* sPathfindSelf;

void EWTrobotpatrolB_spawnRobo(Object* self, CurveSetup* startNode);
void EWTrobotpatrolB_initCurveNetwork(Object* self);
CurveSetup* EWTrobotpatrolB_getPathNode(Object* self, u32 uID);
void EWTrobotpatrolB_pathRoboRandom(Object* self, Object* robo, u32 currUID);
void EWTrobotpatrolB_pathRoboToPlayer(Object* self, Object* robo, u32 currUID);
void EWTrobotpatrolB_findNodesClosestToPlayer(CurveSetup** closest, CurveSetup** nodes, Object* player);
s16 EWTrobotpatrolB_calcLinkPathDistance(s32 searchIdx, CurveSetup* currNode, s32 linkUID, s32 iteration);

RECOMP_HOOK_DLL(EWTrobotpatrolB_obj_Setup) void EWTrobotpatrolB_obj_Setup_hook(Object *self) {
    EWTrobotpatrolB_Data* objdata = self->data;
    EWTrobotpatrolB_Setup* setup = (EWTrobotpatrolB_Setup*)self->setup;

    // @recomp: Implement missing setup logic. Initialize the curve network and attempt to spawn the desired number of robots
    EWTrobotpatrolB_initCurveNetwork(self);
    
    if (objdata->numNodes > 0) {
        if (objdata->numNodes < setup->unk18) {
            recomp_eprintf("EWTrobotpatrolB 0x%X trying to spawn %d robots but the network only has %d nodes!\n",
                setup->base.uID,
                setup->unk18,
                objdata->numNodes);
        }

        u8 used[MAX_NODES];
        bzero(used, sizeof(used));
        CurveSetup* roboStarts[MAX_ROBOS];
        for (s32 roboIdx = 0; roboIdx < setup->unk18; roboIdx++) {
            s32 attempts = 0;
            s32 nodeIdx;
            do {
                nodeIdx = mathRnd(0, objdata->numNodes - 1);
            } while (used[nodeIdx] && attempts++ < 10);
            used[nodeIdx] = TRUE;
            
            roboStarts[roboIdx] = objdata->nodes[nodeIdx];
            objdata->roboCurveUIDs[roboIdx] = objdata->nodes[nodeIdx]->uID;
        }
        // Note: spawn late so that initial pathing knows about all initially in-use curves
        for (s32 roboIdx = 0; roboIdx < setup->unk18; roboIdx++) {
            EWTrobotpatrolB_spawnRobo(self, roboStarts[roboIdx]);
        }
    }
}

RECOMP_PATCH u32 EWTrobotpatrolB_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(EWTrobotpatrolB_Data); // @recomp: new size
}

static _Bool recomp_EWTrobotpatrolB_getRoboIndex(Object* self, Object* robo, s32* index) {
    EWTrobotpatrolB_Data* objdata = self->data;
    EWTrobotpatrolB_Setup* setup = (EWTrobotpatrolB_Setup*)self->setup;

    for (s32 i = 0; i < setup->unk18; i++) {
        if (objdata->robos[i] == robo) {
            *index = i;
            return TRUE;
        }
    }

    return FALSE;
}

static void recomp_EWTrobotpatrolB_updateRoboCurve(Object* self, Object* robo, s32 currUID) {
    EWTrobotpatrolB_Data* objdata = self->data;
    
    s32 roboIndex;
    if (recomp_EWTrobotpatrolB_getRoboIndex(self, robo, &roboIndex)) {
        objdata->roboCurveUIDs[roboIndex] = currUID;
    }
}

static _Bool recomp_EWTrobotpatrolB_isCurveValid(Object* self, s32 uid) {
    EWTrobotpatrolB_Data* objdata = self->data;
    EWTrobotpatrolB_Setup* setup = (EWTrobotpatrolB_Setup*)self->setup;
    
    for (s32 i = 0; i < setup->unk18; i++) {
        if (objdata->roboCurveUIDs[i] == uid) {
            return FALSE;
        }
    }

    return TRUE;
}

RECOMP_PATCH void EWTrobotpatrolB_pathRoboRandom(Object* self, Object* robo, u32 currUID) {
    EWTrobotpatrolB_Data* objdata = self->data;
    CurveSetup* node;
    u8 count;
    s32 validLinks[4];
    s32 i;

    // @recomp: full rewrite to ignore in-use curves
    count = 0;
    node = EWTrobotpatrolB_getPathNode(self, currUID);
    for (i = 0; i < 4; i++) {
        s32 link = node->links[i];
        if (link != -1 && recomp_EWTrobotpatrolB_isCurveValid(self, link)) {
            validLinks[count++] = link;
        }
    }

    if (count > 0) {
        s32 moveToLink = validLinks[mathRnd(0, count - 1)];

        recomp_EWTrobotpatrolB_updateRoboCurve(self, robo, moveToLink);
        ((DLL_437_EWTrobotpatrol*)robo->dll)->vtbl->MoveTo(robo, moveToLink);
    } else {
        // Note: tell the robo to move to where they already are to clear their curve search debounce.
        //       otherwise, they'll pause for a long time if this case is hit.
        ((DLL_437_EWTrobotpatrol*)robo->dll)->vtbl->MoveTo(robo, currUID);
    }
}

RECOMP_PATCH void EWTrobotpatrolB_pathRoboToPlayer(Object* self, Object* robo, u32 currUID) {
    EWTrobotpatrolB_Data* objdata = self->data;
    CurveSetup* currNode;
    CurveSetup* closestNodes[2];
    s16 linkPathDists[4];
    s16 shortestDist;
    s32 bestLinkIdx;
    s32 i;

    // Calculate distance from current node to node closest to player via each link
    currNode = EWTrobotpatrolB_getPathNode(self, currUID);
    EWTrobotpatrolB_findNodesClosestToPlayer(closestNodes, objdata->nodes, objGetPlayer());
    sPathfindVisited[0] = currNode;
    sPathfindSelf = self;
    if (currNode == closestNodes[0]) {
        sPathfindGoal = closestNodes[1];
    } else {
        sPathfindGoal = closestNodes[0];
    }
    for (i = 0; i < 4; i++) {
        if (currNode->links[i] != -1) {
            linkPathDists[i] = EWTrobotpatrolB_calcLinkPathDistance(1, currNode, currNode->links[i], 1);
        } else {
            linkPathDists[i] = -1;
        }
    }
    // Choose the link with the shortest path
    // @recomp: Ignore curves that are in-use by other robos
    shortestDist = -1;
    bestLinkIdx = -1;
    i = 0;
    while (i < 4) {
        if (recomp_EWTrobotpatrolB_isCurveValid(self, currNode->links[i])) {
            if ((linkPathDists[i] != -1) && ((shortestDist == -1) || (linkPathDists[i] < shortestDist))) {
                shortestDist = linkPathDists[i];
                bestLinkIdx = i;
            }
        }
        i += 1;
    }
    if (bestLinkIdx != -1) {
        s32 moveToLink = currNode->links[bestLinkIdx];

        recomp_EWTrobotpatrolB_updateRoboCurve(self, robo, moveToLink);
        ((DLL_437_EWTrobotpatrol*)robo->dll)->vtbl->MoveTo(robo, moveToLink);
    } else {
        // @recomp: fallback
        EWTrobotpatrolB_pathRoboRandom(self, robo, currUID);
    }
}
