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


struct Tox_Event_Conference_Peer_List_Changed {
    uint32_t conference_number;
};

non_null()
static void tox_event_conference_peer_list_changed_construct(Tox_Event_Conference_Peer_List_Changed
        *conference_peer_list_changed)
{
    *conference_peer_list_changed = (Tox_Event_Conference_Peer_List_Changed) {
        0
    };
}
non_null()
static void tox_event_conference_peer_list_changed_destruct(Tox_Event_Conference_Peer_List_Changed
        *conference_peer_list_changed)
{
    return;
}

non_null()
static void tox_event_conference_peer_list_changed_set_conference_number(Tox_Event_Conference_Peer_List_Changed
        *conference_peer_list_changed, uint32_t conference_number)
{
    assert(conference_peer_list_changed != nullptr);
    conference_peer_list_changed->conference_number = conference_number;
}
uint32_t tox_event_conference_peer_list_changed_get_conference_number(const Tox_Event_Conference_Peer_List_Changed
        *conference_peer_list_changed)
{
    assert(conference_peer_list_changed != nullptr);
    return conference_peer_list_changed->conference_number;
}

non_null()
static void tox_event_conference_peer_list_changed_pack(
    const Tox_Event_Conference_Peer_List_Changed *event, msgpack_packer *mp)
{
    assert(event != nullptr);
    msgpack_pack_array(mp, 1);
    msgpack_pack_uint32(mp, event->conference_number);
}

non_null()
static bool tox_event_conference_peer_list_changed_unpack(
    Tox_Event_Conference_Peer_List_Changed *event, const msgpack_object *obj)
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
static Tox_Event_Conference_Peer_List_Changed *tox_events_add_conference_peer_list_changed(Tox_Events *events)
{
    if (events->conference_peer_list_changed_size == UINT32_MAX) {
        return nullptr;
    }

    if (events->conference_peer_list_changed_size == events->conference_peer_list_changed_capacity) {
        const uint32_t new_conference_peer_list_changed_capacity = events->conference_peer_list_changed_capacity * 2 + 1;
        Tox_Event_Conference_Peer_List_Changed *new_conference_peer_list_changed = (Tox_Event_Conference_Peer_List_Changed *)
                realloc(
                    events->conference_peer_list_changed,
                    new_conference_peer_list_changed_capacity * sizeof(Tox_Event_Conference_Peer_List_Changed));

        if (new_conference_peer_list_changed == nullptr) {
            return nullptr;
        }

        events->conference_peer_list_changed = new_conference_peer_list_changed;
        events->conference_peer_list_changed_capacity = new_conference_peer_list_changed_capacity;
    }

    Tox_Event_Conference_Peer_List_Changed *const conference_peer_list_changed =
        &events->conference_peer_list_changed[events->conference_peer_list_changed_size];
    tox_event_conference_peer_list_changed_construct(conference_peer_list_changed);
    ++events->conference_peer_list_changed_size;
    return conference_peer_list_changed;
}

void tox_events_clear_conference_peer_list_changed(Tox_Events *events)
{
    if (events == nullptr) {
        return;
    }

    for (uint32_t i = 0; i < events->conference_peer_list_changed_size; ++i) {
        tox_event_conference_peer_list_changed_destruct(&events->conference_peer_list_changed[i]);
    }

    free(events->conference_peer_list_changed);
    events->conference_peer_list_changed = nullptr;
    events->conference_peer_list_changed_size = 0;
    events->conference_peer_list_changed_capacity = 0;
}

uint32_t tox_events_get_conference_peer_list_changed_size(const Tox_Events *events)
{
    if (events == nullptr) {
        return 0;
    }

    return events->conference_peer_list_changed_size;
}

const Tox_Event_Conference_Peer_List_Changed *tox_events_get_conference_peer_list_changed(const Tox_Events *events,
        uint32_t index)
{
    assert(index < events->conference_peer_list_changed_size);
    assert(events->conference_peer_list_changed != nullptr);
    return &events->conference_peer_list_changed[index];
}

void tox_events_pack_conference_peer_list_changed(const Tox_Events *events, msgpack_packer *mp)
{
    const uint32_t size = tox_events_get_conference_peer_list_changed_size(events);

    msgpack_pack_array(mp, size);

    for (uint32_t i = 0; i < size; ++i) {
        tox_event_conference_peer_list_changed_pack(tox_events_get_conference_peer_list_changed(events, i), mp);
    }
}

bool tox_events_unpack_conference_peer_list_changed(Tox_Events *events, const msgpack_object *obj)
{
    if (obj->type != MSGPACK_OBJECT_ARRAY) {
        return false;
    }

    for (uint32_t i = 0; i < obj->via.array.size; ++i) {
        Tox_Event_Conference_Peer_List_Changed *event = tox_events_add_conference_peer_list_changed(events);

        if (event == nullptr) {
            return false;
        }

        if (!tox_event_conference_peer_list_changed_unpack(event, &obj->via.array.ptr[i])) {
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


void tox_events_handle_conference_peer_list_changed(Tox *tox, uint32_t conference_number, void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    Tox_Event_Conference_Peer_List_Changed *conference_peer_list_changed = tox_events_add_conference_peer_list_changed(
                state->events);

    if (conference_peer_list_changed == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return;
    }

    tox_event_conference_peer_list_changed_set_conference_number(conference_peer_list_changed, conference_number);
}
