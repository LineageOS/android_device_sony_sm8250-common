/*
 * Copyright (C) 2019 The Android Open Source Project
 * Copyright (C) 2023 The LineageOS Project
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

#include "Lights.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <fcntl.h>

using ::android::base::WriteStringToFile;

namespace aidl {
namespace android {
namespace hardware {
namespace light {

#define LED_PATH(led) "/sys/class/leds/" led "/"
#define RGB_CTRL_PATH LED_PATH("rgb")

static const std::string led_paths[]{
        [RED] = LED_PATH("red"),
        [GREEN] = LED_PATH("green"),
        [BLUE] = LED_PATH("blue"),
};

#define AutoHwLight(light) \
    { .id = (int32_t)light, .type = light, .ordinal = 0 }

// List of supported lights
const static std::vector<HwLight> kAvailableLights = {AutoHwLight(LightType::BATTERY),
                                                      AutoHwLight(LightType::NOTIFICATIONS)};

Lights::Lights() {
    for (int i = 0; i < NUM_LIGHTS; i++)
        mMaxBrightness[i] = ReadIntFromFile(led_paths[i] + "max_brightness", 0xFF);
}

// AIDL methods
ndk::ScopedAStatus Lights::setLightState(int32_t id, const HwLightState& state) {
    LightType type = static_cast<LightType>(id);
    switch (type) {
        case LightType::BATTERY:
            mBattery = state;
            break;
        case LightType::NOTIFICATIONS:
            mNotification = state;
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
            break;
    }

    if (IsLit(mBattery.color))
        setSpeakerLightLocked(mBattery);
    else
        setSpeakerLightLocked(mNotification);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight>* lights) {
    for (auto& light : kAvailableLights) lights->push_back(light);

    return ndk::ScopedAStatus::ok();
}

// device methods
void Lights::setSpeakerLightLocked(const HwLightState& state) {
    uint32_t red, green, blue;

    // Extract brightness from RRGGBB
    red = (state.color >> 16) & 0xFF;
    green = (state.color >> 8) & 0xFF;
    blue = state.color & 0xFF;

    switch (state.flashMode) {
        case FlashMode::HARDWARE:
        case FlashMode::TIMED:
            WriteToFile(RGB_CTRL_PATH "sync_state", 1);  // CONFIGURE_TO_BLINK
            setLedBlink(RED, red, state.flashOnMs, state.flashOffMs);
            setLedBlink(GREEN, green, state.flashOnMs, state.flashOffMs);
            setLedBlink(BLUE, blue, state.flashOnMs, state.flashOffMs);
            WriteToFile(RGB_CTRL_PATH "start_blink", 1);
            break;
        case FlashMode::NONE:
        default:
            WriteToFile(RGB_CTRL_PATH "sync_state", 0);  // NOT_BLINK
            setLedBrightness(RED, red);
            setLedBrightness(GREEN, green);
            setLedBrightness(BLUE, blue);
            break;
    }

    return;
}

uint32_t Lights::getActualBrightness(led_type led, uint32_t value) {
    return value * mMaxBrightness[led] / 0xFF;
}

bool Lights::setLedBlink(led_type led, uint32_t value, uint32_t onMs, uint32_t offMs) {
    bool ret = true;
    ret = WriteStringToFile(std::to_string(getActualBrightness(led, value)) + ",0",
                            led_paths[led] + "lut_pwm");
    ret &= WriteToFile(led_paths[led] + "step_duration", 0);
    ret &= WriteToFile(led_paths[led] + "pause_lo_multi", offMs);
    ret &= WriteToFile(led_paths[led] + "pause_hi_multi", onMs);
    return ret;
}

bool Lights::setLedBrightness(led_type led, uint32_t value) {
    return WriteToFile(led_paths[led] + "brightness", getActualBrightness(led, value));
}

// Utils
bool Lights::IsLit(uint32_t color) {
    return color & 0x00ffffff;
}

uint32_t Lights::ReadIntFromFile(const std::string& path, uint32_t defaultValue) {
    std::string buf;

    if (::android::base::ReadFileToString(path, &buf)) {
        return std::stoi(buf);
    }
    return defaultValue;
}

// Write value to path and close file.
bool Lights::WriteToFile(const std::string& path, uint32_t content) {
    return WriteStringToFile(std::to_string(content), path);
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
