// Tracker.cpp
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Minimal C++ port scaffold for the Tracker plugin (POEFixer SDK v6).

#include "PluginSDK.h"
#include "TrackerSettings.h"
#include "StatusEffectLogic.h"
#include "GroundEffectLogic.h"
#include "MonsterLineLogic.h"

#include "TextureLoader.h"

#include <imgui.h>

class TrackerPlugin : public PluginSDK::Plugin {
public:
    const char* GetName() const override { return "Tracker"; }
    bool WantsOverlay() const override { return true; }

    void OnEnable(bool /*isGameAttached*/) override {
        if (ctx()->ImGuiContext) {
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));
        }
        m_settings.Load(DirectoryPath());
        // Ensure icon coordinates are migrated from older 28-column layout if needed
        m_settings.MigrateIconCoordinatesTo8ColumnsIfNeeded();

        // Load atlas from settings if present (stored relative to POE Fixer root), otherwise default to plugin dir status_icons.png
        std::filesystem::path displayPath;
        if (!m_settings.IconsAtlasPath.empty()) displayPath = m_settings.IconsAtlasPath;
        else displayPath = std::filesystem::path("Resources") / "tracker" / "status_icons.png";
        // Resolve for loading (ResolveIconsAtlasPath expects POE Fixer root)
        std::filesystem::path iconPath = m_settings.ResolveIconsAtlasPath(DirectoryPath());
        if (std::filesystem::is_directory(iconPath) || iconPath.extension().empty()) iconPath /= "status_icons.png";
        // show resolved atlas path in UI for clarity
        std::string iconPathStr = iconPath.string();
        strncpy_s(m_iconPathBuf, sizeof(m_iconPathBuf), iconPathStr.c_str(), _TRUNCATE);
        int w = 0, h = 0;
        m_statusIconsPtr = LoadTextureFromFileWIC(ctx()->D3DDevice, iconPath, &w, &h);
        if (m_statusIconsPtr) {
            m_statusIconsWidth = w; m_statusIconsHeight = h;
            ctx()->Log.Info("Tracker: loaded status_icons.png");
        }

        // populate import/export UI buffers from settings (show resolved absolute paths)
        // Interpret stored relative paths as relative to POE Fixer root (host root)
        std::filesystem::path hostRoot = DirectoryPath().parent_path().parent_path();
        auto fillBufFromSetting = [&](const std::string& value, char* buf, size_t bufSize) {
            if (value.empty()) return;
            std::filesystem::path p(value);
            std::filesystem::path resolved = p;
            if (!p.is_absolute()) resolved = hostRoot / p;
            std::string s = resolved.string();
            strncpy_s(buf, bufSize, s.c_str(), _TRUNCATE);
        };
        fillBufFromSetting(m_settings.GroundImportFile, m_groundImportBuf, sizeof(m_groundImportBuf));
        fillBufFromSetting(m_settings.GroundExportFile, m_groundExportBuf, sizeof(m_groundExportBuf));
        fillBufFromSetting(m_settings.MonstersImportFile, m_monstersImportBuf, sizeof(m_monstersImportBuf));
        fillBufFromSetting(m_settings.MonstersExportFile, m_monstersExportBuf, sizeof(m_monstersExportBuf));
        fillBufFromSetting(m_settings.PlayerImportFile, m_playerImportBuf, sizeof(m_playerImportBuf));
        fillBufFromSetting(m_settings.PlayerExportFile, m_playerExportBuf, sizeof(m_playerExportBuf));

        // Construct logic instances now that settings and optional texture are ready
        m_monsterLogic = std::make_unique<MonsterLineLogic>(m_settings);
        m_groundLogic = std::make_unique<GroundEffectLogic>(m_settings);
        m_statusLogic = std::make_unique<StatusEffectLogic>(m_settings, m_statusIconsPtr, m_statusIconsWidth, m_statusIconsHeight);
        ctx()->Log.Info("Tracker C++ plugin enabled");
    }

    void OnDisable() override {
        // release texture if loaded
        if (m_statusIconsPtr) ReleaseTexture(m_statusIconsPtr);
        m_statusIconsPtr = nullptr;
        m_monsterLogic.reset();
        m_groundLogic.reset();
        m_statusLogic.reset();
        ctx()->Log.Info("Tracker C++ plugin disabled");
    }

    void DrawSettings() override {
        if (ctx()->ImGuiContext) ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));

        DrawSettingsExport();
        DrawStatusLayoutProfiles();
        DrawTextRenderingSettings();
        DrawMonsterLineSettings();
        DrawGroundEffectSettings();
        DrawStatusEffectSettings();
        DrawPlayerEffectSettings();

        // Atlas controls (kept near bottom)
        ImGui::Separator();
        ImGui::Text("Status icons atlas");
        ImGui::SetNextItemWidth(420);
        ImGui::InputText("Atlas path", m_iconPathBuf, sizeof(m_iconPathBuf));
        ImGui::SameLine();
        if (ImGui::Button("Load Atlas")) {
            std::filesystem::path p(m_iconPathBuf);
            // If the user entered a relative path in the UI, resolve relative to host root for loading
            std::filesystem::path hostRoot = DirectoryPath().parent_path().parent_path();
            std::filesystem::path pResolved = p;
            if (!p.is_absolute()) pResolved = hostRoot / p;
            if (std::filesystem::is_directory(pResolved) || pResolved.extension().empty()) pResolved /= "status_icons.png";
            int w = 0, h = 0;
            void* newTex = LoadTextureFromFileWIC(ctx()->D3DDevice, pResolved, &w, &h);
            if (newTex) {
                if (m_statusIconsPtr) ReleaseTexture(m_statusIconsPtr);
                m_statusIconsPtr = newTex;
                m_statusIconsWidth = w; m_statusIconsHeight = h;
                if (m_statusLogic) m_statusLogic->UpdateIcons(m_statusIconsPtr, m_statusIconsWidth, m_statusIconsHeight);
                // Store path relative to POE Fixer root when possible
                std::error_code relEc;
                std::filesystem::path storePath = p;
                auto rel = std::filesystem::relative(pResolved, hostRoot, relEc);
                if (!relEc && !rel.empty()) storePath = rel;
                m_settings.IconsAtlasPath = storePath.string();
                m_settings.Save(DirectoryPath());
                // Update displayed input with the stored (likely relative) path
                std::string disp = m_settings.IconsAtlasPath;
                strncpy_s(m_iconPathBuf, sizeof(m_iconPathBuf), disp.c_str(), _TRUNCATE);
                ctx()->Log.Info("Tracker: loaded atlas from path and saved setting");
            } else {
                ctx()->Log.Info("Tracker: failed to load atlas from path");
            }
        }

        // removed example-path helper to avoid confusion
        if (ImGui::Button("Save")) {
            m_settings.Save(DirectoryPath());
            ctx()->Log.Info("Tracker settings saved");
        }
    }

    // --- UI helpers ported from GH-Tracker C# ---
    void DrawSettingsExport() {
        if (!ImGui::CollapsingHeader("Settings Import/Export")) return;
        ImGui::Indent();
        // Ground Effects import/export
        ImGui::Text("Ground Effects");
        ImGui::SetNextItemWidth(420);
        ImGui::InputText("Import Ground file", m_groundImportBuf, sizeof(m_groundImportBuf)); ImGui::SameLine();
        if (ImGui::Button("Import Ground")) {
            m_lastExportStatus = m_settings.ImportFromFile(m_groundImportBuf);
            m_settings.Save(DirectoryPath());
            ReinitializeLogic();
        }
        ImGui::SameLine(); ImGui::SetNextItemWidth(180); ImGui::InputText("Export Ground file", m_groundExportBuf, sizeof(m_groundExportBuf)); ImGui::SameLine();
        if (ImGui::Button("Export Ground")) {
            m_settings.GroundExportFile = std::string(m_groundExportBuf);
            m_settings.Save(DirectoryPath());
            m_lastExportStatus = m_settings.ExportGroundToFile(m_groundExportBuf);
        }
        ImGui::Separator();
        // Monsters effects import/export
        ImGui::Text("Monsters Status Effects");
        ImGui::SetNextItemWidth(420);
        ImGui::InputText("Import Monsters file", m_monstersImportBuf, sizeof(m_monstersImportBuf)); ImGui::SameLine();
        if (ImGui::Button("Import Monsters")) {
            m_lastExportStatus = m_settings.ImportFromFile(m_monstersImportBuf);
            m_settings.Save(DirectoryPath());
            ReinitializeLogic();
        }
        ImGui::SameLine(); ImGui::SetNextItemWidth(180); ImGui::InputText("Export Monsters file", m_monstersExportBuf, sizeof(m_monstersExportBuf)); ImGui::SameLine();
        if (ImGui::Button("Export Monsters")) {
            m_settings.MonstersExportFile = std::string(m_monstersExportBuf);
            m_settings.Save(DirectoryPath());
            m_lastExportStatus = m_settings.ExportMonstersToFile(m_monstersExportBuf);
        }
        ImGui::Separator();
        // Player effects import/export
        ImGui::Text("Player Status Effects");
        ImGui::SetNextItemWidth(420);
        ImGui::InputText("Import Player file", m_playerImportBuf, sizeof(m_playerImportBuf)); ImGui::SameLine();
        if (ImGui::Button("Import Player")) {
            m_lastExportStatus = m_settings.ImportFromFile(m_playerImportBuf);
            m_settings.Save(DirectoryPath());
            ReinitializeLogic();
        }
        ImGui::SameLine(); ImGui::SetNextItemWidth(180); ImGui::InputText("Export Player file", m_playerExportBuf, sizeof(m_playerExportBuf)); ImGui::SameLine();
        if (ImGui::Button("Export Player")) {
            m_settings.PlayerExportFile = std::string(m_playerExportBuf);
            m_settings.Save(DirectoryPath());
            m_lastExportStatus = m_settings.ExportPlayerToFile(m_playerExportBuf);
        }

        if (!m_lastExportStatus.empty()) ImGui::TextWrapped(m_lastExportStatus.c_str());
        ImGui::Unindent();
    }

    void DrawTextRenderingSettings() {
        if (!ImGui::CollapsingHeader("Text Rendering")) return;
        ImGui::Indent();
        ImGui::Text("Text Shadow Alpha"); ImGui::SameLine(); ImGui::SetNextItemWidth(120);
        ImGui::SliderFloat("##GlobalTextShadowAlpha", &m_settings.TextShadowAlpha, 0.0f, 1.0f, "%.2f");
        ImGui::SameLine(); ImGui::Text("Shadow Size"); ImGui::SameLine(); ImGui::SetNextItemWidth(90);
        ImGui::SliderInt("##GlobalTextShadowSize", &m_settings.TextShadowSize, 0, 2);
        ImGui::SameLine(); ImGui::Text("StatusBar Background"); ImGui::SameLine(); ImGui::SetNextItemWidth(200);
        float col[4] = { m_settings.StatusBarBackgroundColor.x, m_settings.StatusBarBackgroundColor.y, m_settings.StatusBarBackgroundColor.z, m_settings.StatusBarBackgroundColor.w };
        if (ImGui::ColorEdit4("##StatusBarBackground", col)) {
            m_settings.StatusBarBackgroundColor = ImVec4{col[0], col[1], col[2], col[3]};
        }
        ImGui::Separator();
        ImGui::Text("Charges Offset X"); ImGui::SameLine(); ImGui::SetNextItemWidth(90);
        ImGui::SliderInt("##ChargesOffsetX", &m_settings.ChargesXOffset, -64, 64);
        ImGui::SameLine(); ImGui::Text("Charges Offset Y"); ImGui::SameLine(); ImGui::SetNextItemWidth(90);
        ImGui::SliderInt("##ChargesOffsetY", &m_settings.ChargesYOffset, -64, 64);
        ImGui::SameLine(); ImGui::Text("Timer Offset X"); ImGui::SameLine(); ImGui::SetNextItemWidth(90);
        ImGui::SliderInt("##TimerOffsetX", &m_settings.TimerXOffset, -64, 64);
        ImGui::SameLine(); ImGui::Text("Timer Offset Y"); ImGui::SameLine(); ImGui::SetNextItemWidth(90);
        ImGui::SliderInt("##TimerOffsetY", &m_settings.TimerYOffset, -64, 64);
        ImGui::Unindent();
    }

    void DrawMonsterLineSettings() {
        if (!ImGui::CollapsingHeader("Monster Line")) return;
        ImGui::Indent();
        ImGui::Checkbox("##UniqueLine", &m_settings.UniqueLine); ImGui::SameLine(); ColorSwatch("Unique monster line color", m_settings.UniqueLineColor);
        ImGui::Checkbox("##RareLine", &m_settings.RareLine); ImGui::SameLine(); ColorSwatch("Rare monster line color", m_settings.RareLineColor);
        ImGui::Checkbox("##MagicLine", &m_settings.MagicLine); ImGui::SameLine(); ColorSwatch("Magic monster line color", m_settings.MagicLineColor);
        ImGui::Unindent();
    }

    void DrawGroundEffectSettings() {
        if (!ImGui::CollapsingHeader("Ground Effects")) return;
        ImGui::Indent();
        // Ground effects are managed manually in the UI (auto-sync removed)
        ImGui::Separator();
        for (size_t i = 0; i < m_settings.GroundEffects.size(); ++i) {
            auto& ge = m_settings.GroundEffects[i];
            ImGui::Checkbox((std::string("##GroundEffectEnabled") + std::to_string(i)).c_str(), &ge.IsEnabled);
            ImGui::SameLine(); ColorSwatch((std::string("##GroundEffectColor") + std::to_string(i)).c_str(), ge.Color);
            ImGui::SameLine(); ImGui::Text("Fill"); ImGui::SameLine(); ImGui::Checkbox((std::string("##Fill") + std::to_string(i)).c_str(), &ge.Filled);
            ImGui::SameLine(); ImGui::Text("Weight"); ImGui::SameLine(); ImGui::SetNextItemWidth(50); ImGui::SliderInt((std::string("##Weight") + std::to_string(i)).c_str(), &ge.LineWeight, 1, 5);
            ImGui::SameLine(); ImGui::Text("Radius"); ImGui::SameLine(); ImGui::SetNextItemWidth(50); ImGui::SliderInt((std::string("##Radius") + std::to_string(i)).c_str(), &ge.Radius, 50, 300);
            ImGui::SameLine(); ImGui::SetNextItemWidth(GetInputWidth());
            // path edit
            char buf[512]; strncpy_s(buf, ge.Path.c_str(), _TRUNCATE);
            ImGui::InputText((std::string("##GroundEffectPath") + std::to_string(i)).c_str(), buf, sizeof(buf));
            ge.Path = buf;
            ImGui::SameLine();
            if (ImGui::Button((std::string("Delete##GroundEffectDelete") + std::to_string(i)).c_str())) { m_settings.GroundEffects.erase(m_settings.GroundEffects.begin() + i); break; }
        }
        if (ImGui::Button("Add Ground Effect")) m_settings.GroundEffects.emplace_back(true, std::string(), ImVec4{1.f,0.f,0.f,0.6f}, 100, 1, false);
        ImGui::Unindent();
    }

    void DrawStatusEffectSettings() {
        if (!ImGui::CollapsingHeader("Monster Status Effects")) return;
        ImGui::Indent();
        ImGui::Checkbox("Only show monster status effects for Rare+", &m_settings.OnlyShowMonsterStatusEffectsForRareAndAbove);
        Tooltip("When enabled, status effects are shown only on Rare (and Unique) monsters.");
        ImGui::Separator();
        ImGui::Text("Horizontal Offset"); ImGui::SameLine(); ImGui::SetNextItemWidth(100); ImGui::SliderInt("##MonsterStatusXOffset", &m_settings.MonsterStatusXOffset, -400, 400);
        ImGui::SameLine(); ImGui::Text("Vertical Offset"); ImGui::SameLine(); ImGui::SetNextItemWidth(100); ImGui::SliderInt("##MonsterStatusYOffset", &m_settings.MonsterStatusYOffset, -400, 400);
        ImGui::SameLine(); ImGui::Text("Gap"); ImGui::SameLine(); ImGui::SetNextItemWidth(90); ImGui::SliderInt("##MonsterStatusIconGap", &m_settings.MonsterStatusIconGap, 0, 50);
        ImGui::Separator();
        for (size_t i = 0; i < m_settings.StatusEffects.size(); ++i) {
            auto& s = m_settings.StatusEffects[i];
            ImGui::Checkbox((std::string("##StatusEffectEnabled") + std::to_string(i)).c_str(), &s.IsEnabled);
            Tooltip("Enable status Effect.");
            ImGui::SameLine(); ColorSwatch((std::string("##StatusEffectTextColor") + std::to_string(i)).c_str(), s.TextColor);
            ImGui::SameLine(); ColorSwatch((std::string("##StatusEffectBarColor") + std::to_string(i)).c_str(), s.BarColor);
            ImGui::SameLine(); ImGui::Text("Icon"); ImGui::SameLine(); DrawIconPreview(s, std::string("MonsterIconPreview") + std::to_string(i));
            ImGui::Text("Scale"); ImGui::SameLine(); ImGui::SetNextItemWidth(90); float scale = s.IconScale; ImGui::InputFloat((std::string("##MonsterIconScale") + std::to_string(i)).c_str(), &scale, 0.1f, 0.5f, "%.2f"); s.IconScale = std::clamp(scale, 0.2f, 10.0f);
            ImGui::SameLine(); if (ImGui::Button((std::string("Pick##MonsterPick") + std::to_string(i)).c_str())) m_iconPickerState[std::string("MonsterPick") + std::to_string(i)] = true;
            DrawIconPickerPopup(std::string("MonsterPick") + std::to_string(i), s);
            ImGui::SameLine(); ImGui::Text("Display Name"); ImGui::SameLine(); ImGui::SetNextItemWidth(120);
            char dname[256]; strncpy_s(dname, s.DisplayName.c_str(), _TRUNCATE); ImGui::InputText((std::string("##DisplayName") + std::to_string(i)).c_str(), dname, sizeof(dname)); s.DisplayName = dname;
            ImGui::SameLine(); ImGui::Text("Name"); ImGui::SameLine(); ImGui::SetNextItemWidth(GetInputWidth()); char namebuf[256]; strncpy_s(namebuf, s.Name.c_str(), _TRUNCATE); ImGui::InputText((std::string("##Name") + std::to_string(i)).c_str(), namebuf, sizeof(namebuf)); s.Name = namebuf;
            ImGui::SameLine(); if (ImGui::Button((std::string("Delete##StatusEffectDelete") + std::to_string(i)).c_str())) { m_settings.StatusEffects.erase(m_settings.StatusEffects.begin() + i); break; }
        }
        if (ImGui::Button("Add Status Effect")) m_settings.StatusEffects.emplace_back(true, "xxx", "XXX", ImVec4{1,1,1,1}, ImVec4{0.4549f,0.0314f,0.0314f,1.0f});
        ImGui::Unindent();
    }

    void DrawPlayerEffectSettings() {
        if (!ImGui::CollapsingHeader("Player Status Effects")) return;
        ImGui::Indent();
        ImGui::Checkbox("Show Player Status Effects", &m_settings.ShowPlayerStatusEffects);
        Tooltip("Toggle displaying status effects for the player.");
        ImGui::SameLine(); ImGui::Text("Vertical Offset"); ImGui::SameLine(); ImGui::SetNextItemWidth(100); ImGui::SliderInt("##PlayerStatusYOffset", &m_settings.PlayerStatusYOffset, -200, 400);
        Tooltip("Positive value draws status effects further below the player on screen.");
        ImGui::SameLine(); ImGui::Text("Horizontal Offset"); ImGui::SameLine(); ImGui::SetNextItemWidth(100); ImGui::SliderInt("##PlayerStatusXOffset", &m_settings.PlayerStatusXOffset, -400, 400);
        ImGui::SameLine(); ImGui::Text("Gap"); ImGui::SameLine(); ImGui::SetNextItemWidth(90); ImGui::SliderInt("##PlayerStatusIconGap", &m_settings.PlayerStatusIconGap, 0, 50);
        ImGui::Separator(); ImGui::Text("Player Status Effects Configuration:");
        for (size_t i = 0; i < m_settings.PlayerStatusEffects.size(); ++i) {
            auto& s = m_settings.PlayerStatusEffects[i];
            ImGui::Checkbox((std::string("##PlayerStatusEffectEnabled") + std::to_string(i)).c_str(), &s.IsEnabled);
            Tooltip("Enable status Effect."); ImGui::SameLine(); ColorSwatch((std::string("##PlayerStatusEffectTextColor") + std::to_string(i)).c_str(), s.TextColor);
            ImGui::SameLine(); ColorSwatch((std::string("##PlayerStatusEffectBarColor") + std::to_string(i)).c_str(), s.BarColor);
            ImGui::SameLine(); ImGui::Text("Icon"); ImGui::SameLine(); DrawIconPreview(s, std::string("PlayerIconPreview") + std::to_string(i));
            ImGui::Text("Scale"); ImGui::SameLine(); ImGui::SetNextItemWidth(90); float scale = s.IconScale; ImGui::InputFloat((std::string("##PlayerIconScale") + std::to_string(i)).c_str(), &scale, 0.1f, 0.5f, "%.2f"); s.IconScale = std::clamp(scale, 0.2f, 10.0f);
            ImGui::SameLine(); if (ImGui::Button((std::string("Pick##PlayerPick") + std::to_string(i)).c_str())) m_iconPickerState[std::string("PlayerPick") + std::to_string(i)] = true;
            DrawIconPickerPopup(std::string("PlayerPick") + std::to_string(i), s);
            ImGui::SameLine(); ImGui::Text("Display Name"); ImGui::SameLine(); ImGui::SetNextItemWidth(120); char dname[256]; strncpy_s(dname, s.DisplayName.c_str(), _TRUNCATE); ImGui::InputText((std::string("##PlayerDisplayName") + std::to_string(i)).c_str(), dname, sizeof(dname)); s.DisplayName = dname;
            ImGui::SameLine(); ImGui::Text("Name"); ImGui::SameLine(); ImGui::SetNextItemWidth(GetInputWidth()); char namebuf[256]; strncpy_s(namebuf, s.Name.c_str(), _TRUNCATE); ImGui::InputText((std::string("##PlayerName") + std::to_string(i)).c_str(), namebuf, sizeof(namebuf)); s.Name = namebuf;
            ImGui::SameLine(); if (ImGui::Button((std::string("Delete##PlayerStatusEffectDelete") + std::to_string(i)).c_str())) { m_settings.PlayerStatusEffects.erase(m_settings.PlayerStatusEffects.begin() + i); break; }
        }
        if (ImGui::Button("Add Player Status Effect")) m_settings.PlayerStatusEffects.emplace_back(true, "xxx", "XXX", ImVec4{1,1,1,1}, ImVec4{0.4549f,0.0314f,0.0314f,1.0f});
        ImGui::Unindent();
    }

    void DrawStatusLayoutProfiles() {
        if (!ImGui::CollapsingHeader("Status Layout Profiles")) return;
        ImGui::Indent();
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        ImGui::Text("Detected resolution: %dx%d", (int)displaySize.x, (int)displaySize.y);
        ImGui::Checkbox("Auto apply profile on startup", &m_settings.AutoApplyStatusLayoutProfile);
        ImGui::SameLine(); if (ImGui::Button("Detect")) { m_selectedResolutionProfile = GetProfileIndexByHeight(displaySize.y); }
        ImGui::SameLine(); const char* profiles[] = { "1080p", "1440p", "4K" };
        ImGui::SetNextItemWidth(120);
        ImGui::Combo("##StatusLayoutProfile", &m_selectedResolutionProfile, profiles, 3);
        ImGui::SameLine(); if (ImGui::Button("Apply")) ApplyStatusLayoutProfile(m_selectedResolutionProfile);
        ImGui::SameLine(); if (ImGui::Button("Reset to current profile defaults")) ApplyStatusLayoutProfile(m_selectedResolutionProfile);
        ImGui::Unindent();
    }

    // --- helper utilities ---
    void ColorSwatch(const char* label, ImVec4& color) {
        if (ImGui::ColorButton(label, color)) ImGui::OpenPopup(label);
        if (ImGui::BeginPopup(label)) { float c[4] = { color.x, color.y, color.z, color.w }; if (ImGui::ColorPicker4(label, c)) color = ImVec4{c[0],c[1],c[2],c[3]}; ImGui::EndPopup(); }
        if (label[0] != '#' || label[1] != '#') { ImGui::SameLine(); ImGui::Text(label); }
    }

    void Tooltip(const char* text) {
        if (ImGui::IsItemHovered()) { ImGui::BeginTooltip(); ImGui::Text(text); ImGui::EndTooltip(); }
    }

    float GetInputWidth() {
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float minWidth = 50.0f;
        ImVec2 buttonSize = ImGui::CalcTextSize("Delete");
        float btnX = buttonSize.x + ImGui::GetStyle().FramePadding.x * 2;
        return std::max(availableWidth - btnX - 10.0f, minWidth);
    }

    bool TryLoadStatusIconsTexture() {
        if (m_statusIconsPtr) return true;
        std::filesystem::path resolved = m_settings.ResolveIconsAtlasPath(DirectoryPath());
        std::filesystem::path p = resolved.empty() ? (std::filesystem::path(Directory()) / "status_icons.png") : resolved;
        if (!std::filesystem::exists(p)) return false;
        int w = 0, h = 0;
        void* ptr = LoadTextureFromFileWIC(ctx()->D3DDevice, p, &w, &h);
        if (!ptr) return false;
        m_statusIconsPtr = ptr; m_statusIconsWidth = w; m_statusIconsHeight = h; return true;
    }

    void DrawIconPreview(StatusEffectSettings& s, const std::string& id) {
        if (!TryLoadStatusIconsTexture()) { ImGui::Text("N/A"); return; }
        auto uvpair = GetIconUV(s.IconColumn, s.IconRow);
        ImVec2 uv0 = uvpair.first;
        ImVec2 uv1 = uvpair.second;
        ImVec2 previewSize{18,18};
        ImGui::Image(m_statusIconsPtr, previewSize, uv0, uv1);
        Tooltip((std::string("Col: ") + std::to_string(s.IconColumn) + ", Row: " + std::to_string(s.IconRow)).c_str());
    }

    void DrawIconPickerPopup(const std::string& key, StatusEffectSettings& s) {
        if (!m_iconPickerState[key]) return;
        if (!TryLoadStatusIconsTexture()) { m_iconPickerState[key] = false; return; }
        bool shouldStayOpen = true;
        ImGui::Begin((std::string("Icon Picker##") + key).c_str(), &shouldStayOpen, ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoSavedSettings);
        ImVec2 imageStart = ImGui::GetCursorScreenPos();
        ImVec2 imageSize{ (float)m_statusIconsWidth, (float)m_statusIconsHeight };
        ImGui::Image(m_statusIconsPtr, imageSize);
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            ImVec2 mousePos = ImGui::GetIO().MouseClickedPos[0];
            ImVec2 clickPos{ mousePos.x - imageStart.x, mousePos.y - imageStart.y };
            int maxColumns = std::max(1, (int)(m_statusIconsWidth / 32));
            int maxRows = std::max(1, (int)(m_statusIconsHeight / 32));
            int selectedColumn = (int)(clickPos.x / 32);
            int selectedRow = (int)(clickPos.y / 32);
            s.IconColumn = std::clamp(selectedColumn, 0, maxColumns - 1);
            s.IconRow = std::clamp(selectedRow, 0, maxRows - 1);
            m_iconPickerState[key] = false;
        }
        ImGui::Text("Click icon to select");
        ImGui::End();
        if (!shouldStayOpen) m_iconPickerState[key] = false;
    }

    std::pair<ImVec2, ImVec2> GetIconUV(int iconColumn, int iconRow) {
        float iconTile = 32.0f;
        float w = (float)std::max(1, m_statusIconsWidth);
        float h = (float)std::max(1, m_statusIconsHeight);
        float iconWidthUv = iconTile / w;
        float iconHeightUv = iconTile / h;
        int clampedColumn = std::max(0, iconColumn);
        int clampedRow = std::max(0, iconRow);
        ImVec2 uv0{ clampedColumn * iconWidthUv, clampedRow * iconHeightUv };
        ImVec2 uv1{ uv0.x + iconWidthUv, uv0.y + iconHeightUv };
        return { uv0, uv1 };
    }

    int GetProfileIndexByHeight(float screenHeight) {
        if (screenHeight >= 2160) return 2;
        if (screenHeight >= 1440) return 1;
        return 0;
    }

    void ApplyStatusLayoutProfile(int profileIndex) {
        switch (profileIndex) {
        case 0:
            m_settings.PlayerStatusXOffset = 0; m_settings.PlayerStatusYOffset = 55; m_settings.PlayerStatusIconGap = 2;
            m_settings.MonsterStatusXOffset = 0; m_settings.MonsterStatusYOffset = -34; m_settings.MonsterStatusIconGap = 2;
            break;
        case 2:
            m_settings.PlayerStatusXOffset = 0; m_settings.PlayerStatusYOffset = 52; m_settings.PlayerStatusIconGap = 5;
            m_settings.MonsterStatusXOffset = 0; m_settings.MonsterStatusYOffset = -58; m_settings.MonsterStatusIconGap = 5;
            break;
        default:
            m_settings.PlayerStatusXOffset = 0; m_settings.PlayerStatusYOffset = 38; m_settings.PlayerStatusIconGap = 4;
            m_settings.MonsterStatusXOffset = 0; m_settings.MonsterStatusYOffset = -42; m_settings.MonsterStatusIconGap = 4;
            break;
        }
    }

    void DrawUI() override {
        if (!ctx()->Game.IsInGame()) return;
        if (ctx()->ImGuiContext) {
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));
        }
        // emit a one-time diagnostic log to help with debugging rendering issues
        static bool s_statusDiagnosticsEmitted = false;
        if (!s_statusDiagnosticsEmitted) {
            char buf[512];
            snprintf(buf, sizeof(buf), "Tracker: status icons %s, StatusEffects=%zu, PlayerStatusEffects=%zu",
                (m_statusIconsPtr ? "loaded" : "missing"), m_settings.StatusEffects.size(), m_settings.PlayerStatusEffects.size());
            ctx()->Log.Info(buf);
            s_statusDiagnosticsEmitted = true;
        }
        auto snap = ctx()->Game.GetSnapshot();
        // Draws: monster lines, ground effects, status effects
        if (m_monsterLogic) m_monsterLogic->Draw(snap, ctx());
        if (m_groundLogic) m_groundLogic->Draw(snap, ctx());
        if (m_statusLogic) m_statusLogic->Draw(snap, ctx());
    }

    void SaveSettings() override {
        m_settings.Save(DirectoryPath());
    }

private:
    TrackerSettings m_settings;
    std::unique_ptr<StatusEffectLogic> m_statusLogic;
    std::unique_ptr<GroundEffectLogic> m_groundLogic;
    std::unique_ptr<MonsterLineLogic> m_monsterLogic;
    void* m_statusIconsPtr = nullptr;
    int m_statusIconsWidth = 0;
    int m_statusIconsHeight = 0;
    char m_importBuf[512] = {};
    char m_exportBuf[512] = {};
    char m_groundImportBuf[512] = {};
    char m_groundExportBuf[512] = {};
    char m_monstersImportBuf[512] = {};
    char m_monstersExportBuf[512] = {};
    char m_playerImportBuf[512] = {};
    char m_playerExportBuf[512] = {};
    char m_iconPathBuf[512] = {};
    std::string m_lastExportStatus;
    std::unordered_map<std::string, bool> m_iconPickerState;
    int m_selectedResolutionProfile = 1;

    void ReinitializeLogic() {
        m_monsterLogic = std::make_unique<MonsterLineLogic>(m_settings);
        m_groundLogic = std::make_unique<GroundEffectLogic>(m_settings);
        m_statusLogic = std::make_unique<StatusEffectLogic>(m_settings, m_statusIconsPtr, m_statusIconsWidth, m_statusIconsHeight);
    }
};

extern "C" PLUGIN_API PluginSDK::Plugin* CreatePlugin() {
    return new TrackerPlugin();
}

extern "C" PLUGIN_API void DestroyPlugin(PluginSDK::Plugin* p) {
    delete p;
}
