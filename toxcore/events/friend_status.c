/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2022 The TokTok team.
 */

#include "events_alloc.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "../bin_unpack.h"

#include "../ccompat.h"
#include "../tox.h"
#include "../tox_events.h"
#include "../tox_unpack.h"


/*****************************************************
 *
 * :: struct and accessors
 *
 *****************************************************/


struct Tox_Event_Friend_Status {
    uint32_t friend_number;
    Tox_User_Status connection_status;
};

non_null()
static void tox_event_friend_status_construct(Tox_Event_Friend_Status *friend_status)
{
    *friend_status = (Tox_Event_Friend_Status) {
        0
    };
}
non_null()
static void tox_event_friend_status_destruct(Tox_Event_Friend_Status *friend_status)
{
    return;
}

non_null()
static void tox_event_friend_status_set_friend_number(Tox_Event_Friend_Status *friend_status,
        uint32_t friend_number)
{
    assert(friend_status != nullptr);
    friend_status->friend_number = friend_number;
}
uint32_t tox_event_friend_status_get_friend_number(const Tox_Event_Friend_Status *friend_status)
{
    assert(friend_status != nullptr);
    return friend_status->friend_number;
}

non_null()
static void tox_event_friend_status_set_connection_status(Tox_Event_Friend_Status *friend_status,
        Tox_User_Status connection_status)
{
    assert(friend_status != nullptr);
    friend_status->connection_status = connection_status;
}
Tox_User_Status tox_event_friend_status_get_connection_status(const Tox_Event_Friend_Status *friend_status)
{
    assert(friend_status != nullptr);
    return friend_status->connection_status;
}

non_null()
static void tox_event_friend_status_pack(
    const Tox_Event_Friend_Status *event, msgpack_packer *mp)
{
    assert(event != nullptr);
    msgpack_pack_array(mp, 2);
    msgpack_pack_uint32(mp, event->friend_number);
    msgpack_pack_uint32(mp, event->connection_status);
}

non_null()
static bool tox_event_friend_status_unpack(
    Tox_Event_Friend_Status *event, const msgpack_object *obj)
{
    assert(event != nullptr);

    if (obj->type != MSGPACK_OBJECT_ARRAY || obj->via.array.size < 2) {
        return false;
    }

    return bin_unpack_u32(&event->friend_number, &obj->via.array.ptr[0])
           && tox_unpack_user_status(&event->connection_status, &obj->via.array.ptr[1]);
}


/*****************************************************
 *
 * :: add/clear/get
 *
 *****************************************************/


non_null()
static Tox_Event_Friend_Status *tox_events_add_friend_status(Tox_Events *events)
{
    if (events->friend_status_size == UINT32_MAX) {
        return nullptr;
    }

    if (events->friend_status_size == events->friend_status_capacity) {
        const uint32_t new_friend_status_capacity = events->friend_status_capacity * 2 + 1;
        Tox_Event_Friend_Status *new_friend_status = (Tox_Event_Friend_Status *)realloc(
                    events->friend_status, new_friend_status_capacity * sizeof(Tox_Event_Friend_Status));

        if (new_friend_status == nullptr) {
            return nullptr;
        }

        events->friend_status = new_friend_status;
        events->friend_status_capacity = new_friend_status_capacity;
    }

    Tox_Event_Friend_Status *const friend_status = &events->friend_status[events->friend_status_size];
    tox_event_friend_status_construct(friend_status);
    ++events->friend_status_size;
    return friend_status;
}

void tox_events_clear_friend_status(Tox_Events *events)
{
    if (events == nullptr) {
        return;
    }

    for (uint32_t i = 0; i < events->friend_status_size; ++i) {
        tox_event_friend_status_destruct(&events->friend_status[i]);
    }

    free(events->friend_status);
    events->friend_status = nullptr;
    events->friend_status_size = 0;
    events->friend_status_capacity = 0;
}

uint32_t tox_events_get_friend_status_size(const Tox_Events *events)
{
    if (events == nullptr) {
        return 0;
    }

    return events->friend_status_size;
}

const Tox_Event_Friend_Status *tox_events_get_friend_status(const Tox_Events *events, uint32_t index)
{
    assert(index < events->friend_status_size);
    assert(events->friend_status != nullptr);
    return &events->friend_status[index];
}

void tox_events_pack_friend_status(const Tox_Events *events, msgpack_packer *mp)
{
    const uint32_t size = tox_events_get_friend_status_size(events);

    msgpack_pack_array(mp, size);

    for (uint32_t i = 0; i < size; ++i) {
        tox_event_friend_status_pack(tox_events_get_friend_status(events, i), mp);
    }
}

bool tox_events_unpack_friend_status(Tox_Events *events, const msgpack_object *obj)
{
    if (obj->type != MSGPACK_OBJECT_ARRAY) {
        return false;
    }

    for (uint32_t i = 0; i < obj->via.array.size; ++i) {
        Tox_Event_Friend_Status *event = tox_events_add_friend_status(events);

        if (event == nullptr) {
            return false;
        }

        if (!tox_event_friend_status_unpack(event, &obj->via.array.ptr[i])) {
            return false;
        }
    }

    return true;
}


/*****************************************************
 *
 * :: event handler
 *
 *****************************************************/


void tox_events_handle_friend_status(Tox *tox, uint32_t friend_number, Tox_User_Status connection_status,
                                     void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    Tox_Event_Friend_Status *friend_status = tox_events_add_friend_status(state->events);

    if (friend_status == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return;
    }

    tox_event_friend_status_set_friend_number(friend_status, friend_number);
    tox_event_friend_status_set_connection_status(friend_status, connection_status);
}
