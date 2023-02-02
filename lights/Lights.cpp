/*
 * Copyright (C) 2019 The Android Open Source Project
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
    { .id = (int)light, .type = light, .ordinal = 0 }

// List of supported lights
const static std::vector<HwLight> kAvailableLights = {AutoHwLight(LightType::BATTERY),
                                                      AutoHwLight(LightType::NOTIFICATIONS)};

Lights::Lights() {
    for (int i = 0; i < NUM_LIGHTS; i++) {
        mLightParams[i].max_single_brightness =
                ReadIntFromFile(led_paths[i] + "max_single_brightness", 0xFF);
        mLightParams[i].max_mixed_brightness =
                ReadIntFromFile(led_paths[i] + "max_mixed_brightness", 0xFF);
    }
}

// AIDL methods
ndk::ScopedAStatus Lights::setLightState(int id, const HwLightState& state) {
    switch (id) {
        case (int)LightType::BATTERY:
            mBattery = state;
            handleSpeakerBatteryLocked();
            break;
        case (int)LightType::NOTIFICATIONS:
            mNotification = state;
            handleSpeakerBatteryLocked();
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
            break;
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight>* lights) {
    for (auto i = kAvailableLights.begin(); i != kAvailableLights.end(); i++) {
        lights->push_back(*i);
    }
    return ndk::ScopedAStatus::ok();
}


// device methods
void Lights::setSpeakerLightLocked(const HwLightState& state) {
    uint32_t red, green, blue;

    // Extract brightness from AARRGGBB
    red = (state.color >> 16) & 0xFF;
    green = (state.color >> 8) & 0xFF;
    blue = state.color & 0xFF;

    // Get the number of lit LED
    bool mixed_led = ((int)(red != 0) + (int)(green != 0) + (int)(blue != 0)) > 1;

    switch (state.flashMode) {
        case FlashMode::HARDWARE:
        case FlashMode::TIMED:
            WriteToFile(RGB_CTRL_PATH "sync_state", 1);
            setLedBlink(RED, red, state.flashOnMs, state.flashOffMs, mixed_led);
            setLedBlink(GREEN, green, state.flashOnMs, state.flashOffMs, mixed_led);
            setLedBlink(BLUE, blue, state.flashOnMs, state.flashOffMs, mixed_led);
            WriteToFile(RGB_CTRL_PATH "start_blink", 1);
            break;
        case FlashMode::NONE:
        default:
            WriteToFile(RGB_CTRL_PATH "sync_state", 0);
            setLedBrightness(RED, red, mixed_led);
            setLedBrightness(GREEN, green, mixed_led);
            setLedBrightness(BLUE, blue, mixed_led);
            break;
    }

    return;
}

void Lights::handleSpeakerBatteryLocked() {
    if (IsLit(mBattery.color))
        return setSpeakerLightLocked(mBattery);
    else
        return setSpeakerLightLocked(mNotification);
}

int Lights::getActualBrightness(led_type led, int br, bool is_mixed) {
    int max_br = is_mixed ? mLightParams[led].max_mixed_brightness
                          : mLightParams[led].max_single_brightness;
    return br * max_br / 0xFF;
}

bool Lights::setLedBlink(led_type led, int br, int onMS, int offMS, bool is_mixed) {
    bool ret;
    ret = WriteStringToFile(std::to_string(getActualBrightness(led, br, is_mixed)) + ",0",
                            led_paths[led] + "lut_pwm");
    ret &= WriteToFile(led_paths[led] + "step_duration", 0);
    ret &= WriteToFile(led_paths[led] + "pause_lo_multi", offMS);
    ret &= WriteToFile(led_paths[led] + "pause_hi_multi", onMS);
    return ret;
}

bool Lights::setLedBrightness(led_type led, uint32_t value, bool is_mixed) {
    return WriteToFile(led_paths[led] + "brightness", getActualBrightness(led, value, is_mixed));
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
