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


struct Tox_Event_Friend_Request {
    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
    uint8_t *message;
    size_t message_length;
};

non_null()
static void tox_event_friend_request_construct(Tox_Event_Friend_Request *friend_request)
{
    *friend_request = (Tox_Event_Friend_Request) {
        0
    };
}
non_null()
static void tox_event_friend_request_destruct(Tox_Event_Friend_Request *friend_request)
{
    free(friend_request->message);
}

non_null()
static bool tox_event_friend_request_set_public_key(Tox_Event_Friend_Request *friend_request, const uint8_t *public_key)
{
    assert(friend_request != nullptr);

    memcpy(friend_request->public_key, public_key, TOX_PUBLIC_KEY_SIZE);
    return true;
}
const uint8_t *tox_event_friend_request_get_public_key(const Tox_Event_Friend_Request *friend_request)
{
    assert(friend_request != nullptr);
    return friend_request->public_key;
}

non_null()
static bool tox_event_friend_request_set_message(Tox_Event_Friend_Request *friend_request, const uint8_t *message,
        size_t message_length)
{
    assert(friend_request != nullptr);

    if (friend_request->message != nullptr) {
        free(friend_request->message);
        friend_request->message = nullptr;
        friend_request->message_length = 0;
    }

    friend_request->message = (uint8_t *)malloc(message_length);

    if (friend_request->message == nullptr) {
        return false;
    }

    memcpy(friend_request->message, message, message_length);
    friend_request->message_length = message_length;
    return true;
}
size_t tox_event_friend_request_get_message_length(const Tox_Event_Friend_Request *friend_request)
{
    assert(friend_request != nullptr);
    return friend_request->message_length;
}
const uint8_t *tox_event_friend_request_get_message(const Tox_Event_Friend_Request *friend_request)
{
    assert(friend_request != nullptr);
    return friend_request->message;
}

non_null()
static void tox_event_friend_request_pack(
    const Tox_Event_Friend_Request *event, msgpack_packer *mp)
{
    assert(event != nullptr);
    msgpack_pack_array(mp, 2);
    msgpack_pack_bin(mp, TOX_PUBLIC_KEY_SIZE);
    msgpack_pack_bin_body(mp, event->public_key, TOX_PUBLIC_KEY_SIZE);
    msgpack_pack_bin(mp, event->message_length);
    msgpack_pack_bin_body(mp, event->message, event->message_length);
}

non_null()
static bool tox_event_friend_request_unpack(
    Tox_Event_Friend_Request *event, const msgpack_object *obj)
{
    assert(event != nullptr);

    if (obj->type != MSGPACK_OBJECT_ARRAY || obj->via.array.size < 2) {
        return false;
    }

    return bin_unpack_bytes_fixed(event->public_key, TOX_PUBLIC_KEY_SIZE, &obj->via.array.ptr[0])
           && bin_unpack_bytes(&event->message, &event->message_length, &obj->via.array.ptr[1]);
}


/*****************************************************
 *
 * :: add/clear/get
 *
 *****************************************************/


non_null()
static Tox_Event_Friend_Request *tox_events_add_friend_request(Tox_Events *events)
{
    if (events->friend_request_size == UINT32_MAX) {
        return nullptr;
    }

    if (events->friend_request_size == events->friend_request_capacity) {
        const uint32_t new_friend_request_capacity = events->friend_request_capacity * 2 + 1;
        Tox_Event_Friend_Request *new_friend_request = (Tox_Event_Friend_Request *)realloc(
                    events->friend_request, new_friend_request_capacity * sizeof(Tox_Event_Friend_Request));

        if (new_friend_request == nullptr) {
            return nullptr;
        }

        events->friend_request = new_friend_request;
        events->friend_request_capacity = new_friend_request_capacity;
    }

    Tox_Event_Friend_Request *const friend_request = &events->friend_request[events->friend_request_size];
    tox_event_friend_request_construct(friend_request);
    ++events->friend_request_size;
    return friend_request;
}

void tox_events_clear_friend_request(Tox_Events *events)
{
    if (events == nullptr) {
        return;
    }

    for (uint32_t i = 0; i < events->friend_request_size; ++i) {
        tox_event_friend_request_destruct(&events->friend_request[i]);
    }

    free(events->friend_request);
    events->friend_request = nullptr;
    events->friend_request_size = 0;
    events->friend_request_capacity = 0;
}

uint32_t tox_events_get_friend_request_size(const Tox_Events *events)
{
    if (events == nullptr) {
        return 0;
    }

    return events->friend_request_size;
}

const Tox_Event_Friend_Request *tox_events_get_friend_request(const Tox_Events *events, uint32_t index)
{
    assert(index < events->friend_request_size);
    assert(events->friend_request != nullptr);
    return &events->friend_request[index];
}

void tox_events_pack_friend_request(const Tox_Events *events, msgpack_packer *mp)
{
    const uint32_t size = tox_events_get_friend_request_size(events);

    msgpack_pack_array(mp, size);

    for (uint32_t i = 0; i < size; ++i) {
        tox_event_friend_request_pack(tox_events_get_friend_request(events, i), mp);
    }
}

bool tox_events_unpack_friend_request(Tox_Events *events, const msgpack_object *obj)
{
    if (obj->type != MSGPACK_OBJECT_ARRAY) {
        return false;
    }

    for (uint32_t i = 0; i < obj->via.array.size; ++i) {
        Tox_Event_Friend_Request *event = tox_events_add_friend_request(events);

        if (event == nullptr) {
            return false;
        }

        if (!tox_event_friend_request_unpack(event, &obj->via.array.ptr[i])) {
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


void tox_events_handle_friend_request(Tox *tox, const uint8_t *public_key, const uint8_t *message, size_t length,
                                      void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    Tox_Event_Friend_Request *friend_request = tox_events_add_friend_request(state->events);

    if (friend_request == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return;
    }

    tox_event_friend_request_set_public_key(friend_request, public_key);
    tox_event_friend_request_set_message(friend_request, message, length);
}
