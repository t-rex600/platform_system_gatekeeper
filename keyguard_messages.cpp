/*
 * Copyright 2015 The Android Open Source Project
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
 *
 */

#include <keyguard/keyguard_messages.h>

#include <string.h>


namespace keyguard {

/**
 * Methods for serializing/deserializing SizedBuffers
 */

static inline size_t serialized_buffer_size(const SizedBuffer &buf) {
    return sizeof(uint32_t) + buf.length;
}

static inline void append_to_buffer(uint8_t **buffer, const SizedBuffer *to_append) {
    memcpy(*buffer, &to_append->length, sizeof(to_append->length));
    *buffer += sizeof(to_append->length);
    memcpy(*buffer, to_append->buffer.get(), to_append->length);
    *buffer += to_append->length;
}

static inline keyguard_error_t read_from_buffer(const uint8_t **buffer, const uint8_t *end,
        SizedBuffer *target) {
    if (*buffer + sizeof(target->length) >= end) return KG_ERROR_INVALID;

    memcpy(&target->length, *buffer, sizeof(target->length));
    *buffer += sizeof(target->length);
    const uint8_t *buffer_end = *buffer + target->length;
    if (buffer_end > end || buffer_end <= *buffer) return KG_ERROR_INVALID;

    target->buffer.reset(new uint8_t[target->length]);
    memcpy(target->buffer.get(), *buffer, target->length);
    *buffer += target->length;
    return KG_ERROR_OK;
}


size_t KeyguardMessage::GetSerializedSize() const {
    if (error_ == KG_ERROR_OK) {
        return 2 * sizeof(uint32_t) + nonErrorSerializedSize();
    } else {
        return sizeof(uint32_t);
    }
}

uint8_t *KeyguardMessage::Serialize() const {
    if (error_ != KG_ERROR_OK) {
        uint32_t *error_buf = new uint32_t;
        *error_buf = static_cast<uint32_t>(error_);
        return reinterpret_cast<uint8_t *>(error_buf);
    } else {
        uint8_t *buf = new uint8_t[2*sizeof(uint32_t) + nonErrorSerializedSize()];
        uint32_t error_value = static_cast<uint32_t>(error_);
        memcpy(buf, &error_value, sizeof(uint32_t));
        memcpy(buf + sizeof(uint32_t), &user_id_, sizeof(user_id_));
        nonErrorSerialize(buf + 2*sizeof(uint32_t));
        return buf;
    }
}

keyguard_error_t KeyguardMessage::Deserialize(const uint8_t *payload, const uint8_t *end) {
    uint32_t error_value;
    if (payload + sizeof(uint32_t) > end) return KG_ERROR_INVALID;
    memcpy(&error_value, payload, sizeof(uint32_t));
    error_ = static_cast<keyguard_error_t>(error_value);
    payload += sizeof(uint32_t);
    if (error_ == KG_ERROR_OK) {
        if (payload == end) return KG_ERROR_INVALID;
        user_id_ = *((uint32_t *) payload);
        error_ = nonErrorDeserialize(payload + sizeof(uint32_t), end);
    }

    return error_;
}


VerifyRequest::VerifyRequest(uint32_t user_id, SizedBuffer *enrolled_password_handle,
        SizedBuffer *provided_password_payload) {
    user_id_ = user_id;
    password_handle_.buffer = std::move(enrolled_password_handle->buffer);
    password_handle_.length = enrolled_password_handle->length;
    provided_password_.buffer = std::move(provided_password_payload->buffer);
    provided_password_.length = provided_password_payload->length;
}

VerifyRequest::VerifyRequest() {
    memset_s(&password_handle_, 0, sizeof(password_handle_));
    memset_s(&provided_password_, 0, sizeof(provided_password_));
}

VerifyRequest::~VerifyRequest() {
    if (password_handle_.buffer) {
        password_handle_.buffer.reset();
    }

    if (provided_password_.buffer) {
        memset_s(provided_password_.buffer.get(), 0, provided_password_.length);
        provided_password_.buffer.reset();
    }
}

size_t VerifyRequest::nonErrorSerializedSize() const {
    return serialized_buffer_size(password_handle_) + serialized_buffer_size(provided_password_);
}

void VerifyRequest::nonErrorSerialize(uint8_t *buffer) const {
    append_to_buffer(&buffer, &password_handle_);
    append_to_buffer(&buffer, &provided_password_);
}

keyguard_error_t VerifyRequest::nonErrorDeserialize(const uint8_t *payload, const uint8_t *end) {
    keyguard_error_t error = KG_ERROR_OK;

    if (password_handle_.buffer) {
        password_handle_.buffer.reset();
    }

    if (provided_password_.buffer) {
        memset_s(provided_password_.buffer.get(), 0, provided_password_.length);
        provided_password_.buffer.reset();
    }

    error = read_from_buffer(&payload, end, &password_handle_);
    if (error != KG_ERROR_OK) return error;

    return read_from_buffer(&payload, end, &provided_password_);

}

VerifyResponse::VerifyResponse(uint32_t user_id, SizedBuffer *verification_token) {
    user_id_ = user_id;
    verification_token_.buffer = std::move(verification_token->buffer);
    verification_token_.length = verification_token->length;
}

VerifyResponse::VerifyResponse() {
    memset_s(&verification_token_, 0, sizeof(verification_token_));
};

VerifyResponse::~VerifyResponse() {
    if (verification_token_.length > 0) {
        verification_token_.buffer.reset();
    }
}

void VerifyResponse::SetVerificationToken(SizedBuffer *verification_token) {
    verification_token_.buffer = std::move(verification_token->buffer);
    verification_token_.length = verification_token->length;
}

size_t VerifyResponse::nonErrorSerializedSize() const {
    return serialized_buffer_size(verification_token_);
}

void VerifyResponse::nonErrorSerialize(uint8_t *buffer) const {
    append_to_buffer(&buffer, &verification_token_);
}

keyguard_error_t VerifyResponse::nonErrorDeserialize(const uint8_t *payload, const uint8_t *end) {
    if (verification_token_.buffer) {
        verification_token_.buffer.reset();
    }

    return read_from_buffer(&payload, end, &verification_token_);
}

EnrollRequest::EnrollRequest(uint32_t user_id, SizedBuffer *provided_password) {
    user_id_ = user_id;
    provided_password_.buffer = std::move(provided_password->buffer);
    provided_password_.length = provided_password->length;
}

EnrollRequest::EnrollRequest() {
    memset_s(&provided_password_, 0, sizeof(provided_password_));
}

EnrollRequest::~EnrollRequest() {
    if (provided_password_.buffer) {
        memset_s(provided_password_.buffer.get(), 0, provided_password_.length);
        provided_password_.buffer.reset();
    }
}

size_t EnrollRequest::nonErrorSerializedSize() const {
   return serialized_buffer_size(provided_password_);
}

void EnrollRequest::nonErrorSerialize(uint8_t *buffer) const {
    append_to_buffer(&buffer, &provided_password_);
}

keyguard_error_t EnrollRequest::nonErrorDeserialize(const uint8_t *payload, const uint8_t *end) {
    if (provided_password_.buffer) {
        memset_s(provided_password_.buffer.get(), 0, provided_password_.length);
        provided_password_.buffer.reset();
    }

    return read_from_buffer(&payload, end, &provided_password_);
}

EnrollResponse::EnrollResponse(uint32_t user_id, SizedBuffer *enrolled_password_handle) {
    user_id_ = user_id;
    enrolled_password_handle_.buffer = std::move(enrolled_password_handle->buffer);
    enrolled_password_handle_.length = enrolled_password_handle->length;
}

EnrollResponse::EnrollResponse() {
    memset_s(&enrolled_password_handle_, 0, sizeof(enrolled_password_handle_));
}

EnrollResponse::~EnrollResponse() {
    if (enrolled_password_handle_.buffer) {
        enrolled_password_handle_.buffer.reset();
    }
}

void EnrollResponse::SetEnrolledPasswordHandle(SizedBuffer *enrolled_password_handle) {
    enrolled_password_handle_.buffer = std::move(enrolled_password_handle->buffer);
    enrolled_password_handle_.length = enrolled_password_handle->length;
}

size_t EnrollResponse::nonErrorSerializedSize() const {
    return serialized_buffer_size(enrolled_password_handle_);
}

void EnrollResponse::nonErrorSerialize(uint8_t *buffer) const {
    append_to_buffer(&buffer, &enrolled_password_handle_);
}

keyguard_error_t EnrollResponse::nonErrorDeserialize(const uint8_t *payload, const uint8_t *end) {
    if (enrolled_password_handle_.buffer) {
        enrolled_password_handle_.buffer.reset();
    }

    return read_from_buffer(&payload, end, &enrolled_password_handle_);
}

};
