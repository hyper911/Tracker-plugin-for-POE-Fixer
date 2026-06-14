#pragma once

#include "TrackerSettings.h"
#include "PluginSDK.h"

class GroundEffectLogic {
public:
    explicit GroundEffectLogic(const TrackerSettings& s);
    void Draw(const PluginSDK::Snapshot& snap, const PluginSDK::Context* ctx);

private:
    const TrackerSettings& Settings;
};
