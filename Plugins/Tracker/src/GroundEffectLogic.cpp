#include "GroundEffectLogic.h"
#include <imgui.h>
#include <string>

GroundEffectLogic::GroundEffectLogic(const TrackerSettings& s) : Settings(s) {}

void GroundEffectLogic::Draw(const PluginSDK::Snapshot& snap, const PluginSDK::Context* ctx) {
    for (const auto& ge : Settings.GroundEffects) {
        if (!ge.IsEnabled) continue;
        // convert path to wstring for comparison
        std::wstring wpath(ge.Path.begin(), ge.Path.end());

        for (const auto& e : snap.Entities) {
            if (!e.IsValid) continue;
            if (e.Path.rfind(wpath, 0) != 0) continue; // startswith
            // skip friendly or invalid
            if (e.Components.HasPositioned()) {
                auto pos = ctx->Components.ReadPositioned(e.Components.Positioned);
                if (pos.IsFriendly) continue;
            }

            // need render component to get world position
            if (!e.Components.HasRender()) continue;
            auto rend = ctx->Components.ReadRender(e.Components.Render);
            float sx = 0, sy = 0;
            if (!ctx->Render.WorldToScreen(rend.WorldX, rend.WorldY, rend.WorldZ, sx, sy)) continue;
            ImVec2 loc{sx, sy};
            ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(ge.Color.x, ge.Color.y, ge.Color.z, ge.Color.w));
            ImDrawList* dl = ImGui::GetBackgroundDrawList();
            if (ge.Filled) dl->AddCircleFilled(loc, static_cast<float>(ge.Radius), color);
            else dl->AddCircle(loc, static_cast<float>(ge.Radius), color, 0, static_cast<float>(ge.LineWeight));
        }
    }
}
