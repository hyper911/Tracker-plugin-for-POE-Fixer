#include "StatusEffectLogic.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "PluginSDK.h"
#include <imgui.h>
#include <algorithm>

StatusEffectLogic::StatusEffectLogic(const TrackerSettings& s, void* statusIcons, int texW, int texH)
    : Settings(s), StatusIconsPtr(statusIcons), StatusIconsWidth(texW), StatusIconsHeight(texH)
{
}

std::string StatusEffectLogic::ToTimerText(float timeLeft) {
    if (std::isinf(timeLeft) && timeLeft > 0) return "~";
    int seconds = static_cast<int>(std::floor(timeLeft));
    if (seconds < 0) seconds = 0;
    if (seconds > 5000) return "~";
    return std::to_string(seconds);
}

void StatusEffectLogic::Draw(const PluginSDK::Snapshot& snap, const PluginSDK::Context* ctx) {
    // Helper to draw for single entity
    auto drawForEntity = [&](const PluginSDK::Entity& e, const auto& buffs,
                             const std::vector<StatusEffectSettings>& effectsList,
                             int horizontalOffset, int verticalOffset, int iconGap) {
        // convert integer offsets/gap from settings to floats to avoid narrowing warnings at call sites
        float horizontalOffsetF = static_cast<float>(horizontalOffset);
        float verticalOffsetF = static_cast<float>(verticalOffset);
        float iconGapF = static_cast<float>(iconGap);
        std::vector<StatusEffectSettings> effects;
        auto toLower = [](const std::string& s){ std::string r; r.reserve(s.size()); for(char c: s) r.push_back((char)tolower((unsigned char)c)); return r; };
        for (const auto& eff : effectsList) {
            if (!eff.IsEnabled) continue;
            std::string effName = eff.Name;
            std::string effDisplay = eff.DisplayName;
            std::string effNameL = toLower(effName);
            std::string effDisplayL = toLower(effDisplay);
            // match by id or display name, case-insensitive, or substring
            for (const auto& b : buffs) {
                std::string bL = toLower(b.Name);
                bool match = false;
                if (!effName.empty() && b.Name == effName) match = true;
                if (!match && !effDisplay.empty() && b.Name == effDisplay) match = true;
                if (!match && !effNameL.empty() && bL == effNameL) match = true;
                if (!match && !effDisplayL.empty() && bL == effDisplayL) match = true;
                if (!match && !effNameL.empty() && bL.find(effNameL) != std::string::npos) match = true;
                if (!match && !effDisplayL.empty() && bL.find(effDisplayL) != std::string::npos) match = true;
                if (match) { effects.push_back(eff); break; }
            }
        }
        if (effects.empty()) return;

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        float sx = 0, sy = 0;
        if (!ctx->Render.WorldToScreen(e.WorldX, e.WorldY, e.WorldZ, sx, sy)) return;
        sx += horizontalOffsetF; sy += verticalOffsetF;

        float totalWidth = 0.0f;
        std::vector<float> scaledSizes;
        for (size_t i = 0; i < effects.size(); ++i) {
            float scale = (0.2f > effects[i].IconScale) ? 0.2f : effects[i].IconScale;
            float sz = (8.0f > 32.0f * scale) ? 8.0f : 32.0f * scale;
            scaledSizes.push_back(sz);
            totalWidth += sz;
            if (i + 1 < effects.size()) totalWidth += iconGapF;
        }

        float startX = sx - totalWidth * 0.5f;
        float currentX = startX;
        float startY = sy;

        for (size_t i = 0; i < effects.size(); ++i) {
            const auto& eff = effects[i];
            auto bIt = std::find_if(buffs.begin(), buffs.end(), [&](const auto& bb){ return bb.Name == eff.Name; });
            if (bIt == buffs.end()) continue;
            float iconSz = scaledSizes[i];
            ImVec2 pos{currentX, startY};
            ImVec2 end{currentX + iconSz, startY + iconSz};
            ImVec2 center{currentX + iconSz*0.5f, startY + iconSz*0.5f};

            // draw circular background under every icon (matches expected UI)
            float radius = iconSz * 0.5f;
            ImU32 bgCol = ImGui::ColorConvertFloat4ToU32(ImVec4(eff.BarColor.x, eff.BarColor.y, eff.BarColor.z, eff.BarColor.w));
            dl->AddCircleFilled(center, radius, bgCol);

            if (StatusIconsPtr && StatusIconsWidth > 0 && StatusIconsHeight > 0) {
                // draw icon from atlas on top of background
                const float tile = 32.0f;
                float u0 = (static_cast<float>(eff.IconColumn) * tile) / static_cast<float>(StatusIconsWidth);
                float v0 = (static_cast<float>(eff.IconRow) * tile) / static_cast<float>(StatusIconsHeight);
                float u1 = (u0 + (tile / static_cast<float>(StatusIconsWidth)));
                float v1 = (v0 + (tile / static_cast<float>(StatusIconsHeight)));
                dl->AddImage(StatusIconsPtr, pos, end, ImVec2(u0, v0), ImVec2(u1, v1));
            } else {
                // draw small inner circle as icon placeholder
                dl->AddCircleFilled(center, radius * 0.8f, ImGui::ColorConvertFloat4ToU32(ImVec4(eff.TextColor.x, eff.TextColor.y, eff.TextColor.z, eff.TextColor.w)));
            }

            // charges
            int charges = bIt->Charges;
                if (charges > 0) {
                std::string txt = std::to_string(charges);
                ImVec2 txtSz = ImGui::CalcTextSize(txt.c_str());
                    ImVec2 txtPos{center.x - txtSz.x*0.5f + static_cast<float>(Settings.ChargesXOffset), pos.y - txtSz.y - 2 + static_cast<float>(Settings.ChargesYOffset)};
                // draw shadow if configured
                ImU32 textCol = ImGui::ColorConvertFloat4ToU32(ImVec4(eff.TextColor.x, eff.TextColor.y, eff.TextColor.z, eff.TextColor.w));
                if (Settings.TextShadowSize > 0 && Settings.TextShadowAlpha > 0.0f) {
                    ImU32 shadowCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0,0,0,Settings.TextShadowAlpha));
                    for (int sx = -Settings.TextShadowSize; sx <= Settings.TextShadowSize; ++sx) {
                        for (int sy = -Settings.TextShadowSize; sy <= Settings.TextShadowSize; ++sy) {
                            if (sx == 0 && sy == 0) continue;
                            ImVec2 sp{txtPos.x + (float)sx, txtPos.y + (float)sy};
                            dl->AddText(sp, shadowCol, txt.c_str());
                        }
                    }
                }
                dl->AddText(txtPos, textCol, txt.c_str());
            }

            // timer
            if (bIt->TimeLeft > 0.f) {
                std::string timer = ToTimerText(bIt->TimeLeft);
                ImVec2 tSz = ImGui::CalcTextSize(timer.c_str());
                float barW = iconSz * 0.9f;
                float barH = tSz.y * 1.1f;
                float barX = pos.x + (iconSz - barW) * 0.5f + static_cast<float>(Settings.TimerXOffset);
                float barY = end.y + 2 + static_cast<float>(Settings.TimerYOffset);
                ImVec2 barMin{barX, barY};
                ImVec2 barMax{barX + barW, barY + barH};
                dl->AddRectFilled(barMin, barMax, ImGui::ColorConvertFloat4ToU32(Settings.StatusBarBackgroundColor));
                float total = bIt->TotalTime > 0.0001f ? bIt->TotalTime : bIt->TimeLeft;
                float proportion = 1.0f;
                if (total > 0.0001f) {
                    proportion = bIt->TimeLeft / total;
                    if (proportion < 0.0f) proportion = 0.0f;
                    if (proportion > 1.0f) proportion = 1.0f;
                }
                ImVec2 filledMax{barX + barW * proportion, barMax.y};
                dl->AddRectFilled(barMin, filledMax, ImGui::ColorConvertFloat4ToU32(ImVec4(eff.BarColor.x, eff.BarColor.y, eff.BarColor.z, eff.BarColor.w)));
                ImVec2 timerPos{barX + (barW - tSz.x)*0.5f, barY + (barH - tSz.y)*0.5f};
                ImU32 textCol2 = ImGui::ColorConvertFloat4ToU32(ImVec4(eff.TextColor.x, eff.TextColor.y, eff.TextColor.z, eff.TextColor.w));
                if (Settings.TextShadowSize > 0 && Settings.TextShadowAlpha > 0.0f) {
                    ImU32 shadowCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0,0,0,Settings.TextShadowAlpha));
                    for (int sx = -Settings.TextShadowSize; sx <= Settings.TextShadowSize; ++sx) {
                        for (int sy = -Settings.TextShadowSize; sy <= Settings.TextShadowSize; ++sy) {
                            if (sx == 0 && sy == 0) continue;
                            ImVec2 sp{timerPos.x + (float)sx, timerPos.y + (float)sy};
                            dl->AddText(sp, shadowCol, timer.c_str());
                        }
                    }
                }
                dl->AddText(timerPos, textCol2, timer.c_str());
            }

            currentX += iconSz + iconGapF;
        }
    };

    // Draw for player
    if (Settings.ShowPlayerStatusEffects && snap.Player.IsValid) {
        const auto& pe = snap.Player;
        if (pe.Components.HasBuffs()) {
            auto buffs = ctx->Components.EnumerateBuffs(pe.Components.Buffs);
            drawForEntity(pe, buffs, Settings.PlayerStatusEffects, Settings.PlayerStatusXOffset, Settings.PlayerStatusYOffset, Settings.PlayerStatusIconGap);
        }
    }

    // Draw for monsters
    for (const auto& e : snap.Entities) {
        if (!e.IsValid) continue;
        if (e.EntityType != PluginSDK::EntityType::Monster) continue;
        if (!e.Components.HasBuffs()) continue;
        auto buffs = ctx->Components.EnumerateBuffs(e.Components.Buffs);
        drawForEntity(e, buffs, Settings.StatusEffects, Settings.MonsterStatusXOffset, Settings.MonsterStatusYOffset, Settings.MonsterStatusIconGap);
    }
}

void StatusEffectLogic::UpdateIcons(void* statusIcons, int texW, int texH) {
    StatusIconsPtr = statusIcons;
    StatusIconsWidth = texW;
    StatusIconsHeight = texH;
}
