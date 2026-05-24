#include "SHbarrel.h"
#include "custom_objsetups.h"

#include "dlls/engine/27.h"
#include "dlls/engine/54_pickup.h"
#include "dlls/objects/210_player.h"
#include "game/objects/interaction_arrow.h"
#include "game/objects/object.h"
#include "player_util.h"
#include "recomputils.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "dll.h"
#include "sys/print.h"

typedef struct {
    Pickup pickup;
    DLL27_Data collider;
} SHbarrel_Data;

void SHbarrel_ctor(void *dll) { }
void SHbarrel_dtor(void *dll) { }

static void SHbarrel_setup(Object* self, SHBarrel_Setup* setup, s32 reset) {
    static Vec3f colliderPoints[] = {VEC3F(0.0f, 0.0f, 0.0f)}; // must be static
    f32 colliderRadii[] = {self->objhitInfo->unk52};
    u8 colliderArg[] = {1};

    SHbarrel_Data* objdata = self->data;

    obj_add_object_type(self, OBJTYPE_Barrel);

    self->srt.yaw = setup->yaw << 8;

    gDLL_54_pickup->vtbl->setup(self, (Pickup*)self->data, 33);
    gDLL_54_pickup->vtbl->set_dont_save(self->data, TRUE);

    gDLL_27->vtbl->init(&objdata->collider, DLL27FLAG_1 | DLL27FLAG_2 | DLL27FLAG_4 | DLL27FLAG_20 | DLL27FLAG_40000, 0, DLL27MODE_1);
    gDLL_27->vtbl->setup_terrain_collider(&objdata->collider, 1, colliderPoints, colliderRadii, colliderArg);
}

static void SHbarrel_drop_if_player_underwater(Object* self) {
    Object* player = get_player();
    if (player == NULL || player->data == NULL) {
        return;
    }

    Player_Data* playerData = player->data;
    if (playerData->unk0.unk4.underwaterDist > 25.0f) {
        playerUtil_stop_carrying(player);
    }
}

static void SHbarrel_not_held_control(Object* self, SHbarrel_Data* objdata) {
    diPrintf("water: %f", &objdata->collider.underwaterDist);
    if (objdata->collider.underwaterDist > 0.0f && objdata->collider.waterY != -100000.0f) {
        Object* player = get_player();
        if (player != NULL) {
            Vec3f vec;
            VECTOR_SUBTRACT(self->globalPosition, player->globalPosition, vec);
            vec.y = 0.0f;
            f32 playerDist = VECTOR_MAGNITUDE(vec);

            const f32 pushRange = 22.0f;
            const f32 pushForce = 2.0f;

            if (playerDist <= pushRange && playerDist != 0.0f) {
                VECTOR_DIVIDE_BY_SCALAR(vec, playerDist);
                VECTOR_MULTIPLY_BY_SCALAR(vec, (1.0f - (playerDist / pushRange)) * pushForce * gUpdateRateF);

                self->velocity.x += vec.x;
                self->velocity.z += vec.z;
            }
        }

        f32 targetY = objdata->collider.waterY - 7.0f;

        self->velocity.y += (targetY - self->globalPosition.y) * 0.01f * gUpdateRateF;
        self->velocity.y = CLAMP_EXPR(self->velocity.y, -20.0f, 20.f);

        self->velocity.x *= 0.85f;
        self->velocity.y *= 0.95f;
        self->velocity.z *= 0.85f;

        self->unkAF |= ARROW_FLAG_8_No_Targetting;

        objdata->collider.mode = DLL27MODE_1;
        gDLL_54_pickup->vtbl->set_gravity(self->data, FALSE);

        obj_move(self, 
            self->velocity.x * gUpdateRateF, 
            self->velocity.y * gUpdateRateF, 
            self->velocity.z * gUpdateRateF);
    } else {
        // self->velocity.x *= 0.80f;
        // self->velocity.y -= 0.1f * gUpdateRateF;
        // self->velocity.z *= 0.80f;
        self->velocity.x = 0.0f;
        //self->velocity.y = 0.0f;
        self->velocity.z = 0.0f;
        gDLL_54_pickup->vtbl->set_gravity(self->data, TRUE);

        self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
        objdata->collider.mode = DLL27MODE_2;
    }

    
}

static void SHbarrel_control(Object* self) {
    SHbarrel_Data* objdata = self->data;

    s32 pickupState = gDLL_54_pickup->vtbl->get_state(self->data);
    if (pickupState == PICKUP_Held) {
        SHbarrel_drop_if_player_underwater(self);
    }

    pickupState = gDLL_54_pickup->vtbl->control(self);
    switch (pickupState) {
        case PICKUP_NotHeld:
            SHbarrel_not_held_control(self, objdata);
            break;
        case PICKUP_Held:
        default:
            objdata->collider.mode = DLL27MODE_DISABLED;
            break;
    }

    gDLL_27->vtbl->func_1E8(self, &objdata->collider, gUpdateRateF); // update aabb
    gDLL_27->vtbl->func_5A8(self, &objdata->collider); // do track intersect
    gDLL_27->vtbl->func_624(self, &objdata->collider, gUpdateRateF); // collision resolution
}

static void SHbarrel_update(Object* self) {}

static void SHbarrel_print(Object* self, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    if (gDLL_54_pickup->vtbl->should_print(self, visibility)) {
        draw_object(self, gdl, mtxs, vtxs, pols, 1.0f);
    }
}

static void SHbarrel_free(Object* self, s32 onlySelf) {
    obj_free_object_type(self, OBJTYPE_Barrel);
    gDLL_54_pickup->vtbl->free(self);
}

static u32 SHbarrel_get_model_flags(Object *self) {
    return MODFLAGS_NONE;
}

static u32 SHbarrel_get_data_size(Object *self, u32 offsetAddr) {
    return sizeof(SHbarrel_Data);
}

DLL_IObject_Vtbl DLL_SHbarrel_vtbl = {
    .setup = (void*)SHbarrel_setup,
    .control = SHbarrel_control,
    .update = SHbarrel_update,
    .print = SHbarrel_print,
    .free = SHbarrel_free,
    .get_model_flags = SHbarrel_get_model_flags,
    .get_data_size = SHbarrel_get_data_size
};
