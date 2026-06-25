// TrackerSettings.h
// Port of TrackerSettings.cs -> C++ settings POCO for Tracker plugin.

#pragma once

#include <Windows.h>

#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

struct GroundEffectSettings {
    std::string Path;
    ImVec4 Color{1.f,1.f,1.f,1.f};
    bool IsEnabled = false;
    int LineWeight = 1;
    bool Filled = false;
    int Radius = 100;

    GroundEffectSettings() = default;
    GroundEffectSettings(bool isEnabled, const std::string& path, const ImVec4& color, int radius, int lineWeight, bool filled)
        : Path(path), Color(color), IsEnabled(isEnabled), LineWeight(lineWeight), Filled(filled), Radius(radius) {}
};

struct StatusEffectSettings {
    std::string Name;
    std::string DisplayName;
    ImVec4 TextColor{1.f,1.f,1.f,1.f};
    ImVec4 BarColor{0.f,0.f,0.f,1.f};
    bool IsEnabled = false;
    int IconColumn = 0;
    int IconRow = 0;
    float IconScale = 1.0f;

    StatusEffectSettings() = default;
    StatusEffectSettings(bool isEnabled, const std::string& name, const std::string& displayName, const ImVec4& textColor, const ImVec4& barColor, int iconColumn = 0, int iconRow = 0, float iconScale = 1.0f)
        : Name(name), DisplayName(displayName), TextColor(textColor), BarColor(barColor), IsEnabled(isEnabled), IconColumn(iconColumn), IconRow(iconRow), IconScale(iconScale) {}
};

struct TrackerSettings {
    bool UniqueLine = false;
    bool RareLine = false;
    bool MagicLine = false;

    ImVec4 UniqueLineColor{1.0f, 0.580f, 0.0f, 0.564f};
    ImVec4 RareLineColor{1.0f, 0.988f, 0.0f, 0.490f};
    ImVec4 MagicLineColor{0.0f, 0.117f, 1.0f, 0.294f};

    std::vector<GroundEffectSettings> GroundEffects;
    std::vector<StatusEffectSettings> StatusEffects;
    std::vector<StatusEffectSettings> PlayerStatusEffects;

    // (Auto sync removed) GroundEffectsSourceFile setting removed
    // Per-category import/export resource files (usually under Resources/tracker/)
    std::string GroundImportFile;
    std::string GroundExportFile;
    std::string MonstersImportFile;
    std::string MonstersExportFile;
    std::string PlayerImportFile;
    std::string PlayerExportFile;
    std::string IconsAtlasPath;
    bool IconCoordinatesMigratedTo8Columns = false;

    bool ShowPlayerStatusEffects = true;
    int PlayerStatusYOffset = 35;
    int PlayerStatusXOffset = 0;
    int PlayerStatusIconGap = 4;
    bool AutoApplyStatusLayoutProfile = true;

    int MonsterStatusXOffset = 0;
    int MonsterStatusYOffset = -42;
    int MonsterStatusIconGap = 4;
    // when true, show monster status effects only for Rare (2) and Unique (3) monsters
    bool OnlyShowMonsterStatusEffectsForRareAndAbove = true;
    float TextShadowAlpha = 0.5f;
    int TextShadowSize = 1;

    int ChargesXOffset = 0;
    int ChargesYOffset = 0;
    int TimerXOffset = 0;
    int TimerYOffset = 0;

    ImVec4 StatusBarBackgroundColor{0,0,0,0.6666667f};
    int StatusBarMinWidth = 50;

    TrackerSettings();

    void Save(const std::filesystem::path& directory) const;
    void Load(const std::filesystem::path& directory);
    std::filesystem::path ResolveIconsAtlasPath(const std::filesystem::path& directory) const;

    // Import/export and migration helpers
    std::string ExportToFile(const std::filesystem::path& path) const;
    std::string ImportFromFile(const std::filesystem::path& path);
    std::string ExportGroundToFile(const std::filesystem::path& path) const;
    std::string ExportMonstersToFile(const std::filesystem::path& path) const;
    std::string ExportPlayerToFile(const std::filesystem::path& path) const;

    std::string RemapIconCoordinatesTo8ColumnsNow();
    std::string ForceRemapIconCoordinatesTo8ColumnsNow();
    void MigrateIconCoordinatesTo8ColumnsIfNeeded();
};
