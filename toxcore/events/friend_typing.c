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


struct Tox_Event_Friend_Typing {
    uint32_t friend_number;
    bool typing;
};

non_null()
static void tox_event_friend_typing_construct(Tox_Event_Friend_Typing *friend_typing)
{
    *friend_typing = (Tox_Event_Friend_Typing) {
        0
    };
}
non_null()
static void tox_event_friend_typing_destruct(Tox_Event_Friend_Typing *friend_typing)
{
    return;
}

non_null()
static void tox_event_friend_typing_set_friend_number(Tox_Event_Friend_Typing *friend_typing,
        uint32_t friend_number)
{
    assert(friend_typing != nullptr);
    friend_typing->friend_number = friend_number;
}
uint32_t tox_event_friend_typing_get_friend_number(const Tox_Event_Friend_Typing *friend_typing)
{
    assert(friend_typing != nullptr);
    return friend_typing->friend_number;
}

non_null()
static void tox_event_friend_typing_set_typing(Tox_Event_Friend_Typing *friend_typing, bool typing)
{
    assert(friend_typing != nullptr);
    friend_typing->typing = typing;
}
bool tox_event_friend_typing_get_typing(const Tox_Event_Friend_Typing *friend_typing)
{
    assert(friend_typing != nullptr);
    return friend_typing->typing;
}

non_null()
static void tox_event_friend_typing_pack(
    const Tox_Event_Friend_Typing *event, msgpack_packer *mp)
{
    assert(event != nullptr);
    msgpack_pack_array(mp, 2);
    msgpack_pack_uint32(mp, event->friend_number);

    if (event->typing) {
        msgpack_pack_true(mp);
    } else {
        msgpack_pack_false(mp);
    }
}

non_null()
static bool tox_event_friend_typing_unpack(
    Tox_Event_Friend_Typing *event, const msgpack_object *obj)
{
    assert(event != nullptr);

    if (obj->type != MSGPACK_OBJECT_ARRAY || obj->via.array.size < 2) {
        return false;
    }

    return bin_unpack_u32(&event->friend_number, &obj->via.array.ptr[0])
           && bin_unpack_bool(&event->typing, &obj->via.array.ptr[1]);
}


/*****************************************************
 *
 * :: add/clear/get
 *
 *****************************************************/


non_null()
static Tox_Event_Friend_Typing *tox_events_add_friend_typing(Tox_Events *events)
{
    if (events->friend_typing_size == UINT32_MAX) {
        return nullptr;
    }

    if (events->friend_typing_size == events->friend_typing_capacity) {
        const uint32_t new_friend_typing_capacity = events->friend_typing_capacity * 2 + 1;
        Tox_Event_Friend_Typing *new_friend_typing = (Tox_Event_Friend_Typing *)realloc(
                    events->friend_typing, new_friend_typing_capacity * sizeof(Tox_Event_Friend_Typing));

        if (new_friend_typing == nullptr) {
            return nullptr;
        }

        events->friend_typing = new_friend_typing;
        events->friend_typing_capacity = new_friend_typing_capacity;
    }

    Tox_Event_Friend_Typing *const friend_typing = &events->friend_typing[events->friend_typing_size];
    tox_event_friend_typing_construct(friend_typing);
    ++events->friend_typing_size;
    return friend_typing;
}

void tox_events_clear_friend_typing(Tox_Events *events)
{
    if (events == nullptr) {
        return;
    }

    for (uint32_t i = 0; i < events->friend_typing_size; ++i) {
        tox_event_friend_typing_destruct(&events->friend_typing[i]);
    }

    free(events->friend_typing);
    events->friend_typing = nullptr;
    events->friend_typing_size = 0;
    events->friend_typing_capacity = 0;
}

uint32_t tox_events_get_friend_typing_size(const Tox_Events *events)
{
    if (events == nullptr) {
        return 0;
    }

    return events->friend_typing_size;
}

const Tox_Event_Friend_Typing *tox_events_get_friend_typing(const Tox_Events *events, uint32_t index)
{
    assert(index < events->friend_typing_size);
    assert(events->friend_typing != nullptr);
    return &events->friend_typing[index];
}

void tox_events_pack_friend_typing(const Tox_Events *events, msgpack_packer *mp)
{
    const uint32_t size = tox_events_get_friend_typing_size(events);

    msgpack_pack_array(mp, size);

    for (uint32_t i = 0; i < size; ++i) {
        tox_event_friend_typing_pack(tox_events_get_friend_typing(events, i), mp);
    }
}

bool tox_events_unpack_friend_typing(Tox_Events *events, const msgpack_object *obj)
{
    if (obj->type != MSGPACK_OBJECT_ARRAY) {
        return false;
    }

    for (uint32_t i = 0; i < obj->via.array.size; ++i) {
        Tox_Event_Friend_Typing *event = tox_events_add_friend_typing(events);

        if (event == nullptr) {
            return false;
        }

        if (!tox_event_friend_typing_unpack(event, &obj->via.array.ptr[i])) {
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


void tox_events_handle_friend_typing(Tox *tox, uint32_t friend_number, bool typing, void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    Tox_Event_Friend_Typing *friend_typing = tox_events_add_friend_typing(state->events);

    if (friend_typing == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return;
    }

    tox_event_friend_typing_set_friend_number(friend_typing, friend_number);
    tox_event_friend_typing_set_typing(friend_typing, typing);
}
