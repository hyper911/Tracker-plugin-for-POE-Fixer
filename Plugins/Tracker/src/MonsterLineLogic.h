#pragma once

#include "TrackerSettings.h"
#include "PluginSDK.h"

class MonsterLineLogic {
public:
    explicit MonsterLineLogic(const TrackerSettings& s);
    void Draw(const PluginSDK::Snapshot& snap, const PluginSDK::Context* ctx);

private:
    const TrackerSettings& Settings;
};
