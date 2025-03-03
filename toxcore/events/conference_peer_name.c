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


struct Tox_Event_Conference_Peer_Name {
    uint32_t conference_number;
    uint32_t peer_number;
    uint8_t *name;
    size_t name_length;
};

non_null()
static void tox_event_conference_peer_name_construct(Tox_Event_Conference_Peer_Name *conference_peer_name)
{
    *conference_peer_name = (Tox_Event_Conference_Peer_Name) {
        0
    };
}
non_null()
static void tox_event_conference_peer_name_destruct(Tox_Event_Conference_Peer_Name *conference_peer_name)
{
    free(conference_peer_name->name);
}

non_null()
static void tox_event_conference_peer_name_set_conference_number(Tox_Event_Conference_Peer_Name *conference_peer_name,
        uint32_t conference_number)
{
    assert(conference_peer_name != nullptr);
    conference_peer_name->conference_number = conference_number;
}
uint32_t tox_event_conference_peer_name_get_conference_number(const Tox_Event_Conference_Peer_Name
        *conference_peer_name)
{
    assert(conference_peer_name != nullptr);
    return conference_peer_name->conference_number;
}

non_null()
static void tox_event_conference_peer_name_set_peer_number(Tox_Event_Conference_Peer_Name *conference_peer_name,
        uint32_t peer_number)
{
    assert(conference_peer_name != nullptr);
    conference_peer_name->peer_number = peer_number;
}
uint32_t tox_event_conference_peer_name_get_peer_number(const Tox_Event_Conference_Peer_Name *conference_peer_name)
{
    assert(conference_peer_name != nullptr);
    return conference_peer_name->peer_number;
}

non_null()
static bool tox_event_conference_peer_name_set_name(Tox_Event_Conference_Peer_Name *conference_peer_name,
        const uint8_t *name, size_t name_length)
{
    assert(conference_peer_name != nullptr);

    if (conference_peer_name->name != nullptr) {
        free(conference_peer_name->name);
        conference_peer_name->name = nullptr;
        conference_peer_name->name_length = 0;
    }

    conference_peer_name->name = (uint8_t *)malloc(name_length);

    if (conference_peer_name->name == nullptr) {
        return false;
    }

    memcpy(conference_peer_name->name, name, name_length);
    conference_peer_name->name_length = name_length;
    return true;
}
size_t tox_event_conference_peer_name_get_name_length(const Tox_Event_Conference_Peer_Name *conference_peer_name)
{
    assert(conference_peer_name != nullptr);
    return conference_peer_name->name_length;
}
const uint8_t *tox_event_conference_peer_name_get_name(const Tox_Event_Conference_Peer_Name *conference_peer_name)
{
    assert(conference_peer_name != nullptr);
    return conference_peer_name->name;
}

non_null()
static void tox_event_conference_peer_name_pack(
    const Tox_Event_Conference_Peer_Name *event, msgpack_packer *mp)
{
    assert(event != nullptr);
    msgpack_pack_array(mp, 3);
    msgpack_pack_uint32(mp, event->conference_number);
    msgpack_pack_uint32(mp, event->peer_number);
    msgpack_pack_bin(mp, event->name_length);
    msgpack_pack_bin_body(mp, event->name, event->name_length);
}

non_null()
static bool tox_event_conference_peer_name_unpack(
    Tox_Event_Conference_Peer_Name *event, const msgpack_object *obj)
{
    assert(event != nullptr);

    if (obj->type != MSGPACK_OBJECT_ARRAY || obj->via.array.size < 3) {
        return false;
    }

    return bin_unpack_u32(&event->conference_number, &obj->via.array.ptr[0])
           && bin_unpack_u32(&event->peer_number, &obj->via.array.ptr[1])
           && bin_unpack_bytes(&event->name, &event->name_length, &obj->via.array.ptr[2]);
}


/*****************************************************
 *
 * :: add/clear/get
 *
 *****************************************************/


non_null()
static Tox_Event_Conference_Peer_Name *tox_events_add_conference_peer_name(Tox_Events *events)
{
    if (events->conference_peer_name_size == UINT32_MAX) {
        return nullptr;
    }

    if (events->conference_peer_name_size == events->conference_peer_name_capacity) {
        const uint32_t new_conference_peer_name_capacity = events->conference_peer_name_capacity * 2 + 1;
        Tox_Event_Conference_Peer_Name *new_conference_peer_name = (Tox_Event_Conference_Peer_Name *)realloc(
                    events->conference_peer_name, new_conference_peer_name_capacity * sizeof(Tox_Event_Conference_Peer_Name));

        if (new_conference_peer_name == nullptr) {
            return nullptr;
        }

        events->conference_peer_name = new_conference_peer_name;
        events->conference_peer_name_capacity = new_conference_peer_name_capacity;
    }

    Tox_Event_Conference_Peer_Name *const conference_peer_name =
        &events->conference_peer_name[events->conference_peer_name_size];
    tox_event_conference_peer_name_construct(conference_peer_name);
    ++events->conference_peer_name_size;
    return conference_peer_name;
}

void tox_events_clear_conference_peer_name(Tox_Events *events)
{
    if (events == nullptr) {
        return;
    }

    for (uint32_t i = 0; i < events->conference_peer_name_size; ++i) {
        tox_event_conference_peer_name_destruct(&events->conference_peer_name[i]);
    }

    free(events->conference_peer_name);
    events->conference_peer_name = nullptr;
    events->conference_peer_name_size = 0;
    events->conference_peer_name_capacity = 0;
}

uint32_t tox_events_get_conference_peer_name_size(const Tox_Events *events)
{
    if (events == nullptr) {
        return 0;
    }

    return events->conference_peer_name_size;
}

const Tox_Event_Conference_Peer_Name *tox_events_get_conference_peer_name(const Tox_Events *events, uint32_t index)
{
    assert(index < events->conference_peer_name_size);
    assert(events->conference_peer_name != nullptr);
    return &events->conference_peer_name[index];
}

void tox_events_pack_conference_peer_name(const Tox_Events *events, msgpack_packer *mp)
{
    const uint32_t size = tox_events_get_conference_peer_name_size(events);

    msgpack_pack_array(mp, size);

    for (uint32_t i = 0; i < size; ++i) {
        tox_event_conference_peer_name_pack(tox_events_get_conference_peer_name(events, i), mp);
    }
}

bool tox_events_unpack_conference_peer_name(Tox_Events *events, const msgpack_object *obj)
{
    if (obj->type != MSGPACK_OBJECT_ARRAY) {
        return false;
    }

    for (uint32_t i = 0; i < obj->via.array.size; ++i) {
        Tox_Event_Conference_Peer_Name *event = tox_events_add_conference_peer_name(events);

        if (event == nullptr) {
            return false;
        }

        if (!tox_event_conference_peer_name_unpack(event, &obj->via.array.ptr[i])) {
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


void tox_events_handle_conference_peer_name(Tox *tox, uint32_t conference_number, uint32_t peer_number,
        const uint8_t *name, size_t length, void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    Tox_Event_Conference_Peer_Name *conference_peer_name = tox_events_add_conference_peer_name(state->events);

    if (conference_peer_name == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return;
    }

    tox_event_conference_peer_name_set_conference_number(conference_peer_name, conference_number);
    tox_event_conference_peer_name_set_peer_number(conference_peer_name, peer_number);
    tox_event_conference_peer_name_set_name(conference_peer_name, name, length);
}
