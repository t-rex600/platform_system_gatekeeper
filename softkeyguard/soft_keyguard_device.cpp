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

#include "soft_keyguard_device.h"
#include "native_keyguard_file_io.h"

__attribute__((visibility("default")))
int softkeyguard_device_open(const hw_module_t *module, const char *name, hw_device_t **device) {
    if (device == NULL || strcmp(name, HARDWARE_KEYGUARD) != 0)
        return -EINVAL;

    keyguard::SoftKeyguardDevice *dev = new keyguard::SoftKeyguardDevice(module);
    if (dev == NULL)
        return -ENOMEM;

    *device = reinterpret_cast<hw_device_t *>(dev);
    return 0;
}


static struct hw_module_methods_t keyguard_module_methods  = {
    .open = softkeyguard_device_open,
};

__attribute__((visibility("default")))
struct keyguard_module soft_keyguard_device_module = {
    .common =
        {
         .tag = HARDWARE_MODULE_TAG,
         .module_api_version = KEYGUARD_MODULE_API_VERSION_0_1,
         .hal_api_version = HARDWARE_HAL_API_VERSION,
         .id = KEYGUARD_HARDWARE_MODULE_ID,
         .name = "Keyguard SCrypt HAL",
         .author = "The Android Open Source Project",
         .methods = &keyguard_module_methods,
         .dso = 0,
         .reserved = {},
        },
};

namespace keyguard {

SoftKeyguardDevice::SoftKeyguardDevice(const hw_module_t *module)
    : impl_(new SoftKeyguard(new NativeKeyguardFileIo())) {
#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
    static_assert(std::is_standard_layout<SoftKeyguardDevice>::value,
                  "SoftKeyguardDevice must be standard layout");
    static_assert(offsetof(SoftKeyguardDevice, device_) == 0,
                  "device_ must be the first member of KeymasterOpenSsl");
    static_assert(offsetof(SoftKeyguardDevice, device_.common) == 0,
                  "common must be the first member of keymaster_device");
#else
    assert(reinterpret_cast<keyguard_device*>(this) == &device_);
    assert(reinterpret_cast<hw_device_t*>(this) == &(device_.common));
#endif

    memset(&device_, 0, sizeof(device_));
    device_.common.tag = HARDWARE_DEVICE_TAG;
    device_.common.version = 1;
    device_.common.module = const_cast<hw_module_t *>(module);
    device_.common.close = close_device;

    device_.verify = Verify;
    device_.enroll = Enroll;
}

hw_device_t *SoftKeyguardDevice::hw_device() {
    return &device_.common;
}

static inline SoftKeyguardDevice *convert_device(const struct keyguard_device *dev) {
    return reinterpret_cast<SoftKeyguardDevice *>(const_cast<keyguard_device *>(dev));
}

/* static */
int SoftKeyguardDevice::close_device(hw_device_t* dev) {
    delete reinterpret_cast<SoftKeyguardDevice *>(dev);
    return 0;
}

int SoftKeyguardDevice::Enroll(const struct keyguard_device *dev, uint32_t uid,
            const uint8_t *current_password_handle, size_t current_password_handle_length,
            const uint8_t *current_password, size_t current_password_length,
            const uint8_t *desired_password, size_t desired_password_length,
            uint8_t **enrolled_password_handle, size_t *enrolled_password_handle_length) {

    if (dev == NULL ||
            enrolled_password_handle == NULL || enrolled_password_handle_length == NULL ||
            desired_password == NULL || desired_password_length == 0)
        return -EINVAL;

    // Current password and current password handle go together
    if (current_password_handle == NULL || current_password_handle_length == 0 ||
            current_password == NULL || current_password_length == 0) {
        current_password_handle = NULL;
        current_password_handle_length = 0;
        current_password = NULL;
        current_password_length = 0;
    }

    SizedBuffer desired_password_buffer(desired_password_length);
    memcpy(desired_password_buffer.buffer.get(), desired_password, desired_password_length);

    SizedBuffer current_password_handle_buffer(current_password_handle_length);
    if (current_password_handle) {
        memcpy(current_password_handle_buffer.buffer.get(), current_password_handle,
                current_password_handle_length);
    }

    SizedBuffer current_password_buffer(current_password_length);
    if (current_password) {
        memcpy(current_password_buffer.buffer.get(), current_password, current_password_length);
    }

    EnrollRequest request(uid, &current_password_handle_buffer, &desired_password_buffer,
            &current_password_buffer);
    EnrollResponse response;

    convert_device(dev)->impl_->Enroll(request, &response);

    if (response.error != KG_ERROR_OK)
        return -EINVAL;

    *enrolled_password_handle = response.enrolled_password_handle.buffer.release();
    *enrolled_password_handle_length = response.enrolled_password_handle.length;
    return 0;
}

int SoftKeyguardDevice::Verify(const struct keyguard_device *dev, uint32_t uid,
        const uint8_t *enrolled_password_handle, size_t enrolled_password_handle_length,
        const uint8_t *provided_password, size_t provided_password_length,
        uint8_t **auth_token, size_t *auth_token_length) {

    if (dev == NULL || enrolled_password_handle == NULL ||
            provided_password == NULL) {
        return -EINVAL;
    }

    SizedBuffer password_handle_buffer(enrolled_password_handle_length);
    memcpy(password_handle_buffer.buffer.get(), enrolled_password_handle, enrolled_password_handle_length);
    SizedBuffer provided_password_buffer(provided_password_length);
    memcpy(provided_password_buffer.buffer.get(), provided_password, provided_password_length);

    VerifyRequest request(uid, &password_handle_buffer, &provided_password_buffer);
    VerifyResponse response;

    convert_device(dev)->impl_->Verify(request, &response);

    if (response.error != KG_ERROR_OK)
       return -EINVAL;

    if (auth_token != NULL && auth_token_length != NULL) {
       *auth_token = response.auth_token.buffer.release();
       *auth_token_length = response.auth_token.length;
    }

    return 0;
}
};