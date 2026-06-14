#include "MonsterLineLogic.h"
#include <imgui.h>

MonsterLineLogic::MonsterLineLogic(const TrackerSettings& s) : Settings(s) {}

void MonsterLineLogic::Draw(const PluginSDK::Snapshot& snap, const PluginSDK::Context* ctx) {
    if (!snap.Player.IsValid) return;
    const auto& player = snap.Player;
    float px = 0, py = 0;
    if (!ctx->Render.WorldToScreen(player.WorldX, player.WorldY, player.WorldZ, px, py)) return;
    ImVec2 playerPos{px, py};

    auto drawLine = [&](const PluginSDK::Entity& e, const ImVec4& color) {
        if (!e.IsValid) return;
        float ex = 0, ey = 0;
        if (!ctx->Render.WorldToScreen(e.WorldX, e.WorldY, e.WorldZ, ex, ey)) return;
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        dl->AddLine(playerPos, ImVec2(ex, ey), ImGui::ColorConvertFloat4ToU32(color), 1.0f);
    };

    for (const auto& e : snap.Entities) {
        if (!e.IsValid) continue;
        if (e.EntityType != PluginSDK::EntityType::Monster) continue;
        if (Settings.UniqueLine && e.Rarity == 3) drawLine(e, ImVec4(Settings.UniqueLineColor.x, Settings.UniqueLineColor.y, Settings.UniqueLineColor.z, Settings.UniqueLineColor.w));
        if (Settings.RareLine && e.Rarity == 2) drawLine(e, ImVec4(Settings.RareLineColor.x, Settings.RareLineColor.y, Settings.RareLineColor.z, Settings.RareLineColor.w));
        if (Settings.MagicLine && e.Rarity == 1) drawLine(e, ImVec4(Settings.MagicLineColor.x, Settings.MagicLineColor.y, Settings.MagicLineColor.z, Settings.MagicLineColor.w));
    }
}
