/*
 * Copyright (C) 2020 The Android Open Source Project
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

#pragma once

#include <aidl/android/hardware/light/BnLights.h>

namespace aidl {
namespace android {
namespace hardware {
namespace light {

enum led_type {
    RED,
    GREEN,
    BLUE,
    NUM_LIGHTS,
};

struct LightParam {
    int max_single_brightness;
    int max_mixed_brightness;
};

class Lights : public BnLights {
  public:
    Lights();

    ndk::ScopedAStatus setLightState(int id, const HwLightState& state) override;
    ndk::ScopedAStatus getLights(std::vector<HwLight>* types) override;

  private:
    void setSpeakerLightLocked(const HwLightState& state);
    void handleSpeakerBatteryLocked();

    int getActualBrightness(led_type led, int br, bool is_mixed);
    bool setLedBlink(led_type led, int br, int onMS, int offMS, bool is_mixed);
    bool setLedBrightness(led_type led, uint32_t value, bool is_mixed);

    bool IsLit(uint32_t color);
    uint32_t ReadIntFromFile(const std::string& path, uint32_t defaultValue);
    bool WriteToFile(const std::string& path, uint32_t content);

    bool mWhiteLed;
    HwLightState mNotification;
    HwLightState mBattery;

    struct LightParam mLightParams[NUM_LIGHTS];
};

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
