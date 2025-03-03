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


struct Tox_Event_Friend_Read_Receipt {
    uint32_t friend_number;
    uint32_t message_id;
};

non_null()
static void tox_event_friend_read_receipt_construct(Tox_Event_Friend_Read_Receipt *friend_read_receipt)
{
    *friend_read_receipt = (Tox_Event_Friend_Read_Receipt) {
        0
    };
}
non_null()
static void tox_event_friend_read_receipt_destruct(Tox_Event_Friend_Read_Receipt *friend_read_receipt)
{
    return;
}

non_null()
static void tox_event_friend_read_receipt_set_friend_number(Tox_Event_Friend_Read_Receipt *friend_read_receipt,
        uint32_t friend_number)
{
    assert(friend_read_receipt != nullptr);
    friend_read_receipt->friend_number = friend_number;
}
uint32_t tox_event_friend_read_receipt_get_friend_number(const Tox_Event_Friend_Read_Receipt *friend_read_receipt)
{
    assert(friend_read_receipt != nullptr);
    return friend_read_receipt->friend_number;
}

non_null()
static void tox_event_friend_read_receipt_set_message_id(Tox_Event_Friend_Read_Receipt *friend_read_receipt,
        uint32_t message_id)
{
    assert(friend_read_receipt != nullptr);
    friend_read_receipt->message_id = message_id;
}
uint32_t tox_event_friend_read_receipt_get_message_id(const Tox_Event_Friend_Read_Receipt *friend_read_receipt)
{
    assert(friend_read_receipt != nullptr);
    return friend_read_receipt->message_id;
}

non_null()
static void tox_event_friend_read_receipt_pack(
    const Tox_Event_Friend_Read_Receipt *event, msgpack_packer *mp)
{
    assert(event != nullptr);
    msgpack_pack_array(mp, 2);
    msgpack_pack_uint32(mp, event->friend_number);
    msgpack_pack_uint32(mp, event->message_id);
}

non_null()
static bool tox_event_friend_read_receipt_unpack(
    Tox_Event_Friend_Read_Receipt *event, const msgpack_object *obj)
{
    assert(event != nullptr);

    if (obj->type != MSGPACK_OBJECT_ARRAY || obj->via.array.size < 2) {
        return false;
    }

    return bin_unpack_u32(&event->friend_number, &obj->via.array.ptr[0])
           && bin_unpack_u32(&event->message_id, &obj->via.array.ptr[1]);
}


/*****************************************************
 *
 * :: add/clear/get
 *
 *****************************************************/


non_null()
static Tox_Event_Friend_Read_Receipt *tox_events_add_friend_read_receipt(Tox_Events *events)
{
    if (events->friend_read_receipt_size == UINT32_MAX) {
        return nullptr;
    }

    if (events->friend_read_receipt_size == events->friend_read_receipt_capacity) {
        const uint32_t new_friend_read_receipt_capacity = events->friend_read_receipt_capacity * 2 + 1;
        Tox_Event_Friend_Read_Receipt *new_friend_read_receipt = (Tox_Event_Friend_Read_Receipt *)realloc(
                    events->friend_read_receipt, new_friend_read_receipt_capacity * sizeof(Tox_Event_Friend_Read_Receipt));

        if (new_friend_read_receipt == nullptr) {
            return nullptr;
        }

        events->friend_read_receipt = new_friend_read_receipt;
        events->friend_read_receipt_capacity = new_friend_read_receipt_capacity;
    }

    Tox_Event_Friend_Read_Receipt *const friend_read_receipt =
        &events->friend_read_receipt[events->friend_read_receipt_size];
    tox_event_friend_read_receipt_construct(friend_read_receipt);
    ++events->friend_read_receipt_size;
    return friend_read_receipt;
}

void tox_events_clear_friend_read_receipt(Tox_Events *events)
{
    if (events == nullptr) {
        return;
    }

    for (uint32_t i = 0; i < events->friend_read_receipt_size; ++i) {
        tox_event_friend_read_receipt_destruct(&events->friend_read_receipt[i]);
    }

    free(events->friend_read_receipt);
    events->friend_read_receipt = nullptr;
    events->friend_read_receipt_size = 0;
    events->friend_read_receipt_capacity = 0;
}

uint32_t tox_events_get_friend_read_receipt_size(const Tox_Events *events)
{
    if (events == nullptr) {
        return 0;
    }

    return events->friend_read_receipt_size;
}

const Tox_Event_Friend_Read_Receipt *tox_events_get_friend_read_receipt(const Tox_Events *events, uint32_t index)
{
    assert(index < events->friend_read_receipt_size);
    assert(events->friend_read_receipt != nullptr);
    return &events->friend_read_receipt[index];
}

void tox_events_pack_friend_read_receipt(const Tox_Events *events, msgpack_packer *mp)
{
    const uint32_t size = tox_events_get_friend_read_receipt_size(events);

    msgpack_pack_array(mp, size);

    for (uint32_t i = 0; i < size; ++i) {
        tox_event_friend_read_receipt_pack(tox_events_get_friend_read_receipt(events, i), mp);
    }
}

bool tox_events_unpack_friend_read_receipt(Tox_Events *events, const msgpack_object *obj)
{
    if (obj->type != MSGPACK_OBJECT_ARRAY) {
        return false;
    }

    for (uint32_t i = 0; i < obj->via.array.size; ++i) {
        Tox_Event_Friend_Read_Receipt *event = tox_events_add_friend_read_receipt(events);

        if (event == nullptr) {
            return false;
        }

        if (!tox_event_friend_read_receipt_unpack(event, &obj->via.array.ptr[i])) {
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


void tox_events_handle_friend_read_receipt(Tox *tox, uint32_t friend_number, uint32_t message_id, void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    Tox_Event_Friend_Read_Receipt *friend_read_receipt = tox_events_add_friend_read_receipt(state->events);

    if (friend_read_receipt == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return;
    }

    tox_event_friend_read_receipt_set_friend_number(friend_read_receipt, friend_number);
    tox_event_friend_read_receipt_set_message_id(friend_read_receipt, message_id);
}
