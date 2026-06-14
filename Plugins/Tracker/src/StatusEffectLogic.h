#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "TrackerSettings.h"

// Forward-declare a minimal subset of PluginSDK types to avoid pulling
// the full PluginSDK header into this header (prevents macro/parse issues).
namespace PluginSDK {
    struct Snapshot;
    struct Context;
    struct Entity;
    struct Buff;
    enum class EntityType : int;
}

class StatusEffectLogic {
public:
    explicit StatusEffectLogic(const TrackerSettings& s, void* statusIcons = nullptr, int texW = 0, int texH = 0);
    void Draw(const PluginSDK::Snapshot& snap, const PluginSDK::Context* ctx);
    void UpdateIcons(void* statusIcons, int texW, int texH);

private:
    const TrackerSettings& Settings;
    void* StatusIconsPtr = nullptr;
    int StatusIconsWidth = 0;
    int StatusIconsHeight = 0;
    static std::string ToTimerText(float timeLeft);
};
