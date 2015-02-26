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
 */
#ifndef KEYGUARD_MESSAGES_H_
#define KEYGUARD_MESSAGES_H_

#include <memory>
#include <stdint.h>

#include "google_keyguard_utils.h"
/**
 * Message serialization objects for communicating with the hardware keyguard.
 */
namespace keyguard {

typedef enum {
    KG_ERROR_OK = 0,
    KG_ERROR_INVALID = 1,
} keyguard_error_t;

typedef struct {
    std::unique_ptr<uint8_t> buffer;
    size_t length;
} SizedBuffer;

/*
 * Abstract base class of all message objects. Handles serialization of common
 * elements like the error and user ID. Delegates specialized serialization
 * to protected pure virtual functions implemented by subclasses.
 */
class KeyguardMessage {
public:
    KeyguardMessage() : error_(KG_ERROR_OK) {}
    KeyguardMessage(keyguard_error_t error) : error_(error) {}
    virtual ~KeyguardMessage() {}

    /**
     * Returns serialized size in bytes of the current state of the
     * object.
     */
    size_t GetSerializedSize() const;
    /**
     * Converts the object into its serialized representation.
     * The returned buffer's ownwership is tranferred to the caller.
     * TODO: make this return a unique_ptr so that this is clear
     */
    uint8_t *Serialize() const;

    /**
     * Inflates the object from its serial representation.
     */
    keyguard_error_t Deserialize(const uint8_t *payload, const uint8_t *end);

    keyguard_error_t GetError() const { return error_; }
    void SetError(const keyguard_error_t error) { error_ = error; }
    uint32_t GetUserId() const { return user_id_; }

protected:
    /**
     * The following methods are intended to be implemented by subclasses.
     * They are hooks to serialize the elements specific to each particular
     * specialization.
     */

    /**
     * Returns the size of serializing only the elements specific to the
     * current sublclass.
     */
    virtual size_t nonErrorSerializedSize() const { return 0; } ;
    /**
     * Takes a pointer to a buffer prepared by Serialize and writes
     * the subclass specific data into it. The size of the buffer is exaclty
     * that returned by nonErrorSerializedSize() in bytes.
     */
    virtual void nonErrorSerialize(uint8_t *buffer) const { }

    /**
     * Deserializes subclass specific data from payload without reading past end.
     */
    virtual keyguard_error_t nonErrorDeserialize(const uint8_t *payload, const uint8_t *end) {
        return KG_ERROR_OK;
    }

    keyguard_error_t error_;
    uint32_t user_id_;
};

class VerifyRequest : public KeyguardMessage {
public:
    VerifyRequest(
            uint32_t user_id,
            SizedBuffer *enrolled_password_handle,
            SizedBuffer *provided_password_payload);
    VerifyRequest();
    ~VerifyRequest();

    /**
     * The currently enrolled password handle returned by Enroll.
     */
    const SizedBuffer *GetPasswordHandle() const { return &password_handle_; }

    /**
     * The password provided by the user to be verified against the password handle
     * above.
     */
    const SizedBuffer *GetProvidedPassword() const { return &provided_password_; }

protected:
    virtual size_t nonErrorSerializedSize() const;
    virtual void nonErrorSerialize(uint8_t *buffer) const;
    virtual keyguard_error_t nonErrorDeserialize(const uint8_t *payload, const uint8_t *end);

private:
    SizedBuffer password_handle_;
    SizedBuffer provided_password_;
};

class VerifyResponse : public KeyguardMessage {
public:
    VerifyResponse(uint32_t user_id, SizedBuffer *verification_token);
    VerifyResponse();
    ~VerifyResponse();

    void SetVerificationToken(SizedBuffer *verification_token);
    const SizedBuffer *GetVerificationToken() const { return &verification_token_; }

protected:
    virtual size_t nonErrorSerializedSize() const;
    virtual void nonErrorSerialize(uint8_t *buffer) const;
    virtual keyguard_error_t nonErrorDeserialize(const uint8_t *payload, const uint8_t *end);

private:
    SizedBuffer verification_token_;
};

class EnrollRequest : public KeyguardMessage {
public:
    EnrollRequest(uint32_t user_id, SizedBuffer *provided_password);
    EnrollRequest();
    ~EnrollRequest();


    const SizedBuffer *GetProvidedPassword() const { return &provided_password_; }

protected:
    virtual size_t nonErrorSerializedSize() const;
    virtual void nonErrorSerialize(uint8_t *buffer) const;
    virtual keyguard_error_t nonErrorDeserialize(const uint8_t *payload, const uint8_t *end);
private:
    SizedBuffer provided_password_;
};

class EnrollResponse : public KeyguardMessage {
public:
    EnrollResponse(uint32_t user_id, SizedBuffer *enrolled_password_handle);
    EnrollResponse();
    ~EnrollResponse();

    void SetEnrolledPasswordHandle(SizedBuffer *password_handle);
    const SizedBuffer *GetEnrolledPasswordHandle() const { return &enrolled_password_handle_; }

protected:
    virtual size_t nonErrorSerializedSize() const;
    virtual void nonErrorSerialize(uint8_t *buffer) const;
    virtual keyguard_error_t nonErrorDeserialize(const uint8_t *payload, const uint8_t *end);

private:
   SizedBuffer enrolled_password_handle_;
};
}

#endif // KEYGUARD_MESSAGES_H_