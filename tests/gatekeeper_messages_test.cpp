/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <gatekeeper/gatekeeper_messages.h>

using ::gatekeeper::SizedBuffer;
using ::testing::Test;
using ::gatekeeper::EnrollRequest;
using ::gatekeeper::EnrollResponse;
using ::gatekeeper::VerifyRequest;
using ::gatekeeper::VerifyResponse;
using std::cout;
using std::endl;

static const uint32_t USER_ID = 3857;

static SizedBuffer *make_buffer(size_t size) {
    SizedBuffer *result = new SizedBuffer;
    result->length = size;
    uint8_t *buffer = new uint8_t[size];
    srand(size);

    for (size_t i = 0; i < size; i++) {
        buffer[i] = rand();
    }

    result->buffer.reset(buffer);
    return result;
}

TEST(RoundTripTest, EnrollRequestNullEnrolledNullHandle) {
    const size_t password_size = 512;
    SizedBuffer *provided_password = make_buffer(password_size);
    const SizedBuffer *deserialized_password;
    // create request, serialize, deserialize, and validate
    EnrollRequest req(USER_ID, NULL, provided_password, NULL);
    uint8_t *serialized_req = req.Serialize();
    EnrollRequest deserialized_req;
    deserialized_req.Deserialize(serialized_req, serialized_req + req.GetSerializedSize());
    delete[] serialized_req;

    ASSERT_EQ(gatekeeper::gatekeeper_error_t::ERROR_NONE,
            deserialized_req.error);

    deserialized_password = &deserialized_req.provided_password;
    ASSERT_EQ(USER_ID, deserialized_req.user_id);
    ASSERT_EQ((uint32_t) password_size, deserialized_password->length);
    ASSERT_EQ(0, memcmp(req.provided_password.buffer.get(), deserialized_password->buffer.get(), password_size));
    ASSERT_EQ((size_t) 0, deserialized_req.enrolled_password.length);
    ASSERT_EQ(NULL, deserialized_req.enrolled_password.buffer.get());
    ASSERT_EQ((size_t) 0, deserialized_req.password_handle.length);
    ASSERT_EQ(NULL, deserialized_req.password_handle.buffer.get());
    delete provided_password;
}

TEST(RoundTripTest, EnrollRequestEmptyEnrolledEmptyHandle) {
    const size_t password_size = 512;
    SizedBuffer *provided_password = make_buffer(password_size);
    SizedBuffer enrolled;
    SizedBuffer handle;
    const SizedBuffer *deserialized_password;
    // create request, serialize, deserialize, and validate
    EnrollRequest req(USER_ID, &handle, provided_password, &enrolled);
    uint8_t *serialized_req = req.Serialize();
    EnrollRequest deserialized_req;
    deserialized_req.Deserialize(serialized_req, serialized_req + req.GetSerializedSize());
    delete[] serialized_req;

    ASSERT_EQ(gatekeeper::gatekeeper_error_t::ERROR_NONE,
            deserialized_req.error);

    deserialized_password = &deserialized_req.provided_password;
    ASSERT_EQ(USER_ID, deserialized_req.user_id);
    ASSERT_EQ((uint32_t) password_size, deserialized_password->length);
    ASSERT_EQ(0, memcmp(req.provided_password.buffer.get(), deserialized_password->buffer.get(), password_size));
    ASSERT_EQ((size_t) 0, deserialized_req.enrolled_password.length);
    ASSERT_EQ(NULL, deserialized_req.enrolled_password.buffer.get());
    ASSERT_EQ((size_t) 0, deserialized_req.password_handle.length);
    ASSERT_EQ(NULL, deserialized_req.password_handle.buffer.get());
    delete provided_password;
}

TEST(RoundTripTest, EnrollRequestNonNullEnrolledOrHandle) {
    const size_t password_size = 512;
    SizedBuffer *provided_password = make_buffer(password_size);
    SizedBuffer *enrolled_password = make_buffer(password_size);
    SizedBuffer *password_handle = make_buffer(password_size);
    const SizedBuffer *deserialized_password;
    const SizedBuffer *deserialized_enrolled;
    const SizedBuffer *deserialized_handle;
    // create request, serialize, deserialize, and validate
    EnrollRequest req(USER_ID, password_handle, provided_password, enrolled_password);
    uint8_t *serialized_req = req.Serialize();
    EnrollRequest deserialized_req;
    deserialized_req.Deserialize(serialized_req, serialized_req + req.GetSerializedSize());
    delete[] serialized_req;

    ASSERT_EQ(gatekeeper::gatekeeper_error_t::ERROR_NONE,
            deserialized_req.error);

    deserialized_password = &deserialized_req.provided_password;
    deserialized_enrolled = &deserialized_req.enrolled_password;
    deserialized_handle = &deserialized_req.password_handle;
    ASSERT_EQ(USER_ID, deserialized_req.user_id);
    ASSERT_EQ((uint32_t) password_size, deserialized_password->length);
    ASSERT_EQ(0, memcmp(req.provided_password.buffer.get(), deserialized_password->buffer.get(), password_size));
    ASSERT_EQ((uint32_t) password_size, deserialized_enrolled->length);
    ASSERT_EQ(0, memcmp(req.enrolled_password.buffer.get(), deserialized_enrolled->buffer.get(), password_size));
    ASSERT_EQ((uint32_t) password_size, deserialized_handle->length);
    ASSERT_EQ(0, memcmp(req.password_handle.buffer.get(), deserialized_handle->buffer.get(), password_size));
    delete provided_password;
    delete enrolled_password;
    delete password_handle;
}


TEST(RoundTripTest, EnrollResponse) {
    const size_t password_size = 512;
    SizedBuffer *enrolled_password = make_buffer(password_size);
    const SizedBuffer *deserialized_password;
    // create request, serialize, deserialize, and validate
    EnrollResponse req(USER_ID, enrolled_password);
    uint8_t *serialized_req = req.Serialize();
    EnrollResponse deserialized_req;
    deserialized_req.Deserialize(serialized_req, serialized_req + req.GetSerializedSize());
    delete[] serialized_req;

    ASSERT_EQ(gatekeeper::gatekeeper_error_t::ERROR_NONE,
            deserialized_req.error);

    deserialized_password = &deserialized_req.enrolled_password_handle;
    ASSERT_EQ(USER_ID, deserialized_req.user_id);
    ASSERT_EQ((uint32_t) password_size, deserialized_password->length);
    ASSERT_EQ(0, memcmp(req.enrolled_password_handle.buffer.get(),
                deserialized_password->buffer.get(), password_size));
}

TEST(RoundTripTest, VerifyRequest) {
    const size_t password_size = 512;
    SizedBuffer *provided_password = make_buffer(password_size),
          *password_handle = make_buffer(password_size);
    const SizedBuffer *deserialized_password;
    // create request, serialize, deserialize, and validate
    VerifyRequest req(USER_ID, password_handle, provided_password);
    uint8_t *serialized_req = req.Serialize();
    VerifyRequest deserialized_req;
    deserialized_req.Deserialize(serialized_req, serialized_req + req.GetSerializedSize());

    ASSERT_EQ(gatekeeper::gatekeeper_error_t::ERROR_NONE,
            deserialized_req.error);

    ASSERT_EQ(USER_ID, deserialized_req.user_id);
    deserialized_password = &deserialized_req.password_handle;
    ASSERT_EQ((uint32_t) password_size, deserialized_password->length);
    ASSERT_EQ(0, memcmp(req.provided_password.buffer.get(), deserialized_password->buffer.get(),
                password_size));

    deserialized_password = &deserialized_req.password_handle;
    ASSERT_EQ((uint32_t) password_size, deserialized_password->length);
    ASSERT_EQ(0, memcmp(req.password_handle.buffer.get(), deserialized_password->buffer.get(),
                password_size));
}

TEST(RoundTripTest, VerifyResponse) {
    const size_t password_size = 512;
    SizedBuffer *auth_token = make_buffer(password_size);
    const SizedBuffer *deserialized_password;
    // create request, serialize, deserialize, and validate
    VerifyResponse req(USER_ID, auth_token);
    uint8_t *serialized_req = req.Serialize();
    VerifyResponse deserialized_req;
    deserialized_req.Deserialize(serialized_req, serialized_req + req.GetSerializedSize());
    delete[] serialized_req;

    ASSERT_EQ(gatekeeper::gatekeeper_error_t::ERROR_NONE,
            deserialized_req.error);

    ASSERT_EQ(USER_ID, deserialized_req.user_id);
    deserialized_password = &deserialized_req.auth_token;
    ASSERT_EQ((uint32_t) password_size, deserialized_password->length);
    ASSERT_EQ(0, memcmp(req.auth_token.buffer.get(), deserialized_password->buffer.get(),
                password_size));
}

uint8_t msgbuf[] = {
    220, 88,  183, 255, 71,  1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   173, 0,   0,   0,   228, 174, 98,  187, 191, 135, 253, 200, 51,  230, 114, 247, 151, 109,
    237, 79,  87,  32,  94,  5,   204, 46,  154, 30,  91,  6,   103, 148, 254, 129, 65,  171, 228,
    167, 224, 163, 9,   15,  206, 90,  58,  11,  205, 55,  211, 33,  87,  178, 149, 91,  28,  236,
    218, 112, 231, 34,  82,  82,  134, 103, 137, 115, 27,  156, 102, 159, 220, 226, 89,  42,  25,
    37,  9,   84,  239, 76,  161, 198, 72,  167, 163, 39,  91,  148, 191, 17,  191, 87,  169, 179,
    136, 10,  194, 154, 4,   40,  107, 109, 61,  161, 20,  176, 247, 13,  214, 106, 229, 45,  17,
    5,   60,  189, 64,  39,  166, 208, 14,  57,  25,  140, 148, 25,  177, 246, 189, 43,  181, 88,
    204, 29,  126, 224, 100, 143, 93,  60,  57,  249, 55,  0,   87,  83,  227, 224, 166, 59,  214,
    81,  144, 129, 58,  6,   57,  46,  254, 232, 41,  220, 209, 230, 167, 138, 158, 94,  180, 125,
    247, 26,  162, 116, 238, 202, 187, 100, 65,  13,  180, 44,  245, 159, 83,  161, 176, 58,  72,
    236, 109, 105, 160, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   11,  0,   0,   0,   98,  0,   0,   0,   1,   0,   0,   32,  2,   0,   0,   0,   1,   0,
    0,   32,  3,   0,   0,   0,   2,   0,   0,   16,  1,   0,   0,   0,   3,   0,   0,   48,  0,
    1,   0,   0,   200, 0,   0,   80,  3,   0,   0,   0,   0,   0,   0,   0,   244, 1,   0,   112,
    1,   246, 1,   0,   112, 1,   189, 2,   0,   96,  144, 178, 236, 250, 255, 255, 255, 255, 145,
    1,   0,   96,  144, 226, 33,  60,  222, 2,   0,   0,   189, 2,   0,   96,  0,   0,   0,   0,
    0,   0,   0,   0,   190, 2,   0,   16,  1,   0,   0,   0,   12,  0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   110, 0,   0,   0,   0,   0,   0,   0,   11,  0,
    0,   0,   98,  0,   0,   0,   1,   0,   0,   32,  2,   0,   0,   0,   1,   0,   0,   32,  3,
    0,   0,   0,   2,   0,   0,   16,  1,   0,   0,   0,   3,   0,   0,   48,  0,   1,   0,   0,
    200, 0,   0,   80,  3,   0,   0,   0,   0,   0,   0,   0,   244, 1,   0,   112, 1,   246, 1,
    0,   112, 1,   189, 2,   0,   96,  144, 178, 236, 250, 255, 255, 255, 255, 145, 1,   0,   96,
    144, 226, 33,  60,  222, 2,   0,   0,   189, 2,   0,   96,  0,   0,   0,   0,   0,   0,   0,
    0,   190, 2,   0,   16,  1,   0,   0,   0,
};


/*
 * These tests don't have any assertions or expectations. They just try to parse garbage, to see if
 * the result will be a crash.  This is especially informative when run under Valgrind memcheck.
 */

template <typename Message> void parse_garbage() {
    Message msg;
    size_t array_length = sizeof(msgbuf) / sizeof(msgbuf[0]);
    const uint8_t* end = msgbuf + array_length;
    for (size_t i = 0; i < array_length; ++i) {
        const uint8_t* begin = msgbuf + i;
        const uint8_t* p = begin;
        msg.Deserialize(p, end);
    }
}

#define GARBAGE_TEST(Message)                                                                      \
    TEST(GarbageTest, Message) { parse_garbage<Message>(); }

GARBAGE_TEST(VerifyRequest);
GARBAGE_TEST(VerifyResponse);
GARBAGE_TEST(EnrollRequest);
GARBAGE_TEST(EnrollResponse);