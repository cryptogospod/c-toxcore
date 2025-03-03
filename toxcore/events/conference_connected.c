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


/*****************************************************
 *
 * :: struct and accessors
 *
 *****************************************************/


struct Tox_Event_Conference_Connected {
    uint32_t conference_number;
};

non_null()
static void tox_event_conference_connected_construct(Tox_Event_Conference_Connected *conference_connected)
{
    *conference_connected = (Tox_Event_Conference_Connected) {
        0
    };
}
non_null()
static void tox_event_conference_connected_destruct(Tox_Event_Conference_Connected *conference_connected)
{
    return;
}

non_null()
static void tox_event_conference_connected_set_conference_number(
    Tox_Event_Conference_Connected *conference_connected, uint32_t conference_number)
{
    assert(conference_connected != nullptr);
    conference_connected->conference_number = conference_number;
}
uint32_t tox_event_conference_connected_get_conference_number(
    const Tox_Event_Conference_Connected *conference_connected)
{
    assert(conference_connected != nullptr);
    return conference_connected->conference_number;
}

non_null()
static void tox_event_conference_connected_pack(
    const Tox_Event_Conference_Connected *event, msgpack_packer *mp)
{
    assert(event != nullptr);
    msgpack_pack_array(mp, 1);
    msgpack_pack_uint32(mp, event->conference_number);
}

non_null()
static bool tox_event_conference_connected_unpack(
    Tox_Event_Conference_Connected *event, const msgpack_object *obj)
{
    assert(event != nullptr);

    if (obj->type != MSGPACK_OBJECT_ARRAY || obj->via.array.size < 1) {
        return false;
    }

    return bin_unpack_u32(&event->conference_number, &obj->via.array.ptr[0]);
}


/*****************************************************
 *
 * :: add/clear/get
 *
 *****************************************************/


non_null()
static Tox_Event_Conference_Connected *tox_events_add_conference_connected(Tox_Events *events)
{
    if (events->conference_connected_size == UINT32_MAX) {
        return nullptr;
    }

    if (events->conference_connected_size == events->conference_connected_capacity) {
        const uint32_t new_conference_connected_capacity = events->conference_connected_capacity * 2 + 1;
        Tox_Event_Conference_Connected *new_conference_connected = (Tox_Event_Conference_Connected *)realloc(
                    events->conference_connected, new_conference_connected_capacity * sizeof(Tox_Event_Conference_Connected));

        if (new_conference_connected == nullptr) {
            return nullptr;
        }

        events->conference_connected = new_conference_connected;
        events->conference_connected_capacity = new_conference_connected_capacity;
    }

    Tox_Event_Conference_Connected *const conference_connected =
        &events->conference_connected[events->conference_connected_size];
    tox_event_conference_connected_construct(conference_connected);
    ++events->conference_connected_size;
    return conference_connected;
}

void tox_events_clear_conference_connected(Tox_Events *events)
{
    if (events == nullptr) {
        return;
    }

    for (uint32_t i = 0; i < events->conference_connected_size; ++i) {
        tox_event_conference_connected_destruct(&events->conference_connected[i]);
    }

    free(events->conference_connected);
    events->conference_connected = nullptr;
    events->conference_connected_size = 0;
    events->conference_connected_capacity = 0;
}

uint32_t tox_events_get_conference_connected_size(const Tox_Events *events)
{
    if (events == nullptr) {
        return 0;
    }

    return events->conference_connected_size;
}

const Tox_Event_Conference_Connected *tox_events_get_conference_connected(const Tox_Events *events, uint32_t index)
{
    assert(index < events->conference_connected_size);
    assert(events->conference_connected != nullptr);
    return &events->conference_connected[index];
}

void tox_events_pack_conference_connected(const Tox_Events *events, msgpack_packer *mp)
{
    const uint32_t size = tox_events_get_conference_connected_size(events);

    msgpack_pack_array(mp, size);

    for (uint32_t i = 0; i < size; ++i) {
        tox_event_conference_connected_pack(tox_events_get_conference_connected(events, i), mp);
    }
}

bool tox_events_unpack_conference_connected(Tox_Events *events, const msgpack_object *obj)
{
    if (obj->type != MSGPACK_OBJECT_ARRAY) {
        return false;
    }

    for (uint32_t i = 0; i < obj->via.array.size; ++i) {
        Tox_Event_Conference_Connected *event = tox_events_add_conference_connected(events);

        if (event == nullptr) {
            return false;
        }

        if (!tox_event_conference_connected_unpack(event, &obj->via.array.ptr[i])) {
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


void tox_events_handle_conference_connected(Tox *tox, uint32_t conference_number, void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    Tox_Event_Conference_Connected *conference_connected = tox_events_add_conference_connected(state->events);

    if (conference_connected == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return;
    }

    tox_event_conference_connected_set_conference_number(conference_connected, conference_number);
}
