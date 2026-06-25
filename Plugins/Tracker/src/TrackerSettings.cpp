#include "TrackerSettings.h"

#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <iterator>

TrackerSettings::TrackerSettings() {
    // defaults matching the C# initializer where reasonable
    GroundEffects.emplace_back(true, "Metadata/Effects/Spells/ground_effects/VisibleServerGroundEffect",
        ImVec4{1.0f, 0.3019608f, 0.0f, 0.6f}, 100, 2, false);

    StatusEffects.emplace_back(true, "shocked", "Shocked",
        ImVec4{0.49411765f, 1.0f, 0.0f, 1.0f}, ImVec4{0.0f, 0.17327861f, 0.36683416f, 1.0f});

    PlayerStatusEffects.emplace_back(true, "sprint_heavy_stunnable", "HeavySprint",
        ImVec4{0.0f, 1.0f, 0.47236156f, 1.0f}, ImVec4{0.24120605f, 0.71402276f, 1.0f, 0.6627451f});

    // sensible defaults for files — resolve relative to host EXE directory (like C# AppContext.BaseDirectory)
    {
        wchar_t buf[MAX_PATH];
        std::wstring base;
        if (GetModuleFileNameW(NULL, buf, (DWORD)MAX_PATH) > 0) {
            std::filesystem::path p(buf);
            base = p.parent_path().wstring();
        } else {
            base = L".";
        }
        std::filesystem::path basePath(base);
        GroundImportFile = (basePath / "Resources" / "tracker" / "ground_effects_import.txt").string();
        GroundExportFile = (basePath / "Resources" / "tracker" / "ground_effects_export.txt").string();
        MonstersImportFile = (basePath / "Resources" / "tracker" / "monsters_effects_import.txt").string();
        MonstersExportFile = (basePath / "Resources" / "tracker" / "monsters_effects_export.txt").string();
        PlayerImportFile = (basePath / "Resources" / "tracker" / "player_effects_import.txt").string();
        PlayerExportFile = (basePath / "Resources" / "tracker" / "player_effects_export.txt").string();
        IconsAtlasPath = (basePath / "Resources" / "tracker" / "status_icons.png").string();
    }
}

void TrackerSettings::Save(const std::filesystem::path& directory) const {
    std::error_code ec;
    // determine host exe directory (where POE Fixer is installed) and write configs there
    wchar_t buf[MAX_PATH];
    std::wstring hostBaseW;
    if (GetModuleFileNameW(NULL, buf, (DWORD)MAX_PATH) > 0) {
        std::filesystem::path p(buf);
        hostBaseW = p.parent_path().wstring();
    } else hostBaseW = L".";
    std::filesystem::path hostRoot(hostBaseW);

    // write main plugin settings under hostRoot/Plugins/Tracker/config/settings.json
    std::filesystem::path pluginCfg = hostRoot / "Plugins" / "Tracker" / "config" / "settings.json";
    std::filesystem::create_directories(pluginCfg.parent_path(), ec);
    std::ofstream out(pluginCfg);
    if (out.is_open()) {
        out << "{\n";
        out << "  \"ShowPlayerStatusEffects\":" << (ShowPlayerStatusEffects ? "true" : "false") << ",\n";
        out << "  \"PlayerStatusYOffset\":" << PlayerStatusYOffset << ",\n";
        out << "  \"PlayerStatusXOffset\":" << PlayerStatusXOffset << ",\n";
        out << "  \"PlayerStatusIconGap\":" << PlayerStatusIconGap << ",\n";
        out << "  \"AutoApplyStatusLayoutProfile\":" << (AutoApplyStatusLayoutProfile ? "true" : "false") << ",\n";
        out << "  \"MonsterStatusXOffset\":" << MonsterStatusXOffset << ",\n";
        out << "  \"MonsterStatusYOffset\":" << MonsterStatusYOffset << ",\n";
        out << "  \"MonsterStatusIconGap\":" << MonsterStatusIconGap << ",\n";
        out << "  \"OnlyShowMonsterStatusEffectsForRareAndAbove\":" << (OnlyShowMonsterStatusEffectsForRareAndAbove ? "true" : "false") << ",\n";
        out << "  \"TextShadowAlpha\":" << TextShadowAlpha << ",\n";
        out << "  \"TextShadowSize\":" << TextShadowSize << ",\n";
        out << "  \"ChargesXOffset\":" << ChargesXOffset << ",\n";
        out << "  \"ChargesYOffset\":" << ChargesYOffset << ",\n";
        out << "  \"TimerXOffset\":" << TimerXOffset << ",\n";
        out << "  \"TimerYOffset\":" << TimerYOffset << ",\n";
        out << "  \"StatusBarBackgroundColor\": [" << StatusBarBackgroundColor.x << "," << StatusBarBackgroundColor.y << "," << StatusBarBackgroundColor.z << "," << StatusBarBackgroundColor.w << "],\n";

        // Write IconsAtlasPath relative to the host root when possible
        std::string iconsWrite;
        try {
            if (!IconsAtlasPath.empty()) {
                std::error_code relEc;
                auto rel = std::filesystem::relative(std::filesystem::path(IconsAtlasPath), hostRoot, relEc);
                if (!relEc && !rel.empty()) iconsWrite = rel.string();
                else iconsWrite = IconsAtlasPath;
            }
        } catch(...) { iconsWrite = IconsAtlasPath; }
        // Escape backslashes for JSON
        std::string iconsEscaped;
        iconsEscaped.reserve(iconsWrite.size()*2);
        for (char c : iconsWrite) {
            if (c == '\\') { iconsEscaped.push_back('\\'); iconsEscaped.push_back('\\'); }
            else if (c == '"') { iconsEscaped.push_back('\\'); iconsEscaped.push_back('"'); }
            else iconsEscaped.push_back(c);
        }
        out << "  \"IconsAtlasPath\": \"" << iconsEscaped << "\",\n";

        // Import/Export file paths
        auto storeRel = [&](const std::string& raw) -> std::string {
            if (raw.empty()) return std::string();
            try {
                std::error_code relEc;
                auto rel = std::filesystem::relative(std::filesystem::path(raw), hostRoot, relEc);
                if (!relEc && !rel.empty()) return rel.string();
            } catch(...) {}
            return raw;
        };
        auto esc = [&](const std::string& raw){ std::string r; for(char c:raw){ if(c=='\\'){ r.push_back('\\'); r.push_back('\\'); } else if(c=='"'){ r.push_back('\\'); r.push_back('"'); } else r.push_back(c);} return r; };
        out << "  \"GroundImportFile\": \"" << esc(storeRel(GroundImportFile)) << "\",\n";
        out << "  \"GroundExportFile\": \"" << esc(storeRel(GroundExportFile)) << "\",\n";
        out << "  \"MonstersImportFile\": \"" << esc(storeRel(MonstersImportFile)) << "\",\n";
        out << "  \"MonstersExportFile\": \"" << esc(storeRel(MonstersExportFile)) << "\",\n";
        out << "  \"PlayerImportFile\": \"" << esc(storeRel(PlayerImportFile)) << "\",\n";
        out << "  \"PlayerExportFile\": \"" << esc(storeRel(PlayerExportFile)) << "\",\n";
        out << "  \"ImportSettingsFile\": \"\",\n"; // keep legacy empty for compatibility
        out << "}\n";
        out.close();
    }

    // write effects to separate shared config files under Configs/tracker/
    std::filesystem::path cfgDir = hostRoot / "Configs" / "tracker";
    std::filesystem::create_directories(cfgDir, ec);

    // ground effects file
    std::filesystem::path groundCfg = cfgDir / "ground_effects_config.json";
    std::ofstream og(groundCfg);
    if (og.is_open()) {
        og << "{\n";
        og << "  \"GroundEffects\": [\n";
        for (size_t i = 0; i < GroundEffects.size(); ++i) {
            const auto& g = GroundEffects[i];
            og << "    {\n";
            og << "      \"Path\": \"" << g.Path << "\",\n";
            og << "      \"IsEnabled\": " << (g.IsEnabled ? "true" : "false") << ",\n";
            og << "      \"Color\": [" << g.Color.x << "," << g.Color.y << "," << g.Color.z << "," << g.Color.w << "],\n";
            og << "      \"Radius\": " << g.Radius << ",\n";
            og << "      \"LineWeight\": " << g.LineWeight << ",\n";
            og << "      \"Filled\": " << (g.Filled ? "true" : "false") << "\n";
            og << "    }" << (i + 1 < GroundEffects.size() ? ",\n" : "\n");
        }
        og << "  ]\n";
        og << "}\n";
        og.close();
    }

    // monsters status effects file
    std::filesystem::path monstersCfg = cfgDir / "monsters_effects_config.json";
    std::ofstream om(monstersCfg);
    if (om.is_open()) {
        om << "{\n";
        om << "  \"StatusEffects\": [\n";
        for (size_t i = 0; i < StatusEffects.size(); ++i) {
            const auto& s = StatusEffects[i];
            om << "    {\n";
            om << "      \"Name\": \"" << s.Name << "\",\n";
            om << "      \"DisplayName\": \"" << s.DisplayName << "\",\n";
            om << "      \"IsEnabled\": " << (s.IsEnabled ? "true" : "false") << ",\n";
            om << "      \"IconColumn\": " << s.IconColumn << ",\n";
            om << "      \"IconRow\": " << s.IconRow << ",\n";
            om << "      \"IconScale\": " << s.IconScale << ",\n";
            om << "      \"TextColor\": [" << s.TextColor.x << "," << s.TextColor.y << "," << s.TextColor.z << "," << s.TextColor.w << "],\n";
            om << "      \"BarColor\": [" << s.BarColor.x << "," << s.BarColor.y << "," << s.BarColor.z << "," << s.BarColor.w << "]\n";
            om << "    }" << (i + 1 < StatusEffects.size() ? ",\n" : "\n");
        }
        om << "  ]\n";
        om << "}\n";
        om.close();
    }

    // player status effects file
    std::filesystem::path playerCfg = cfgDir / "player_effects_config.json";
    std::ofstream op(playerCfg);
    if (op.is_open()) {
        op << "{\n";
        op << "  \"PlayerStatusEffects\": [\n";
        for (size_t i = 0; i < PlayerStatusEffects.size(); ++i) {
            const auto& s = PlayerStatusEffects[i];
            op << "    {\n";
            op << "      \"Name\": \"" << s.Name << "\",\n";
            op << "      \"DisplayName\": \"" << s.DisplayName << "\",\n";
            op << "      \"IsEnabled\": " << (s.IsEnabled ? "true" : "false") << ",\n";
            op << "      \"IconColumn\": " << s.IconColumn << ",\n";
            op << "      \"IconRow\": " << s.IconRow << ",\n";
            op << "      \"IconScale\": " << s.IconScale << ",\n";
            op << "      \"TextColor\": [" << s.TextColor.x << "," << s.TextColor.y << "," << s.TextColor.z << "," << s.TextColor.w << "],\n";
            op << "      \"BarColor\": [" << s.BarColor.x << "," << s.BarColor.y << "," << s.BarColor.z << "," << s.BarColor.w << "]\n";
            op << "    }" << (i + 1 < PlayerStatusEffects.size() ? ",\n" : "\n");
        }
        op << "  ]\n";
        op << "}\n";
        op.close();
    }
}

void TrackerSettings::Load(const std::filesystem::path& directory) {
    // determine host exe directory (POE Fixer root)
    wchar_t buf[MAX_PATH];
    std::wstring hostBaseW;
    if (GetModuleFileNameW(NULL, buf, (DWORD)MAX_PATH) > 0) {
        std::filesystem::path p(buf);
        hostBaseW = p.parent_path().wstring();
    } else hostBaseW = L".";
    std::filesystem::path hostRoot(hostBaseW);

    // Load main plugin settings from <hostRoot>/Plugins/Tracker/config/settings.json
    std::filesystem::path pluginCfg = hostRoot / "Plugins" / "Tracker" / "config" / "settings.json";
    if (std::filesystem::exists(pluginCfg)) {
        std::ifstream in(pluginCfg);
        if (in.is_open()) {
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                auto unescape = [&](const std::string& s)->std::string {
                    std::string out; out.reserve(s.size());
                    for (size_t i=0;i<s.size();++i) {
                        if (s[i]=='\\' && i+1<s.size()) {
                            char c = s[i+1];
                            if (c=='\\') { out.push_back('\\'); ++i; }
                            else if (c=='"') { out.push_back('"'); ++i; }
                            else if (c=='n') { out.push_back('\n'); ++i; }
                            else out.push_back(c); // best-effort
                        } else out.push_back(s[i]);
                    }
                    return out;
                };
            auto readBool = [&](const char* key, bool def) -> bool {
                return content.find(std::string("\"") + key + "\":true") != std::string::npos ? true : def;
            };
            auto readInt = [&](const char* key, int def) -> int {
                size_t pos = content.find(std::string("\"") + key + "\":");
                if (pos == std::string::npos) return def;
                pos += strlen(key) + 3;
                try { return std::stoi(content.substr(pos)); } catch(...) { return def; }
            };
            auto readFloat = [&](const char* key, float def) -> float {
                size_t pos = content.find(std::string("\"") + key + "\":");
                if (pos == std::string::npos) return def;
                pos += strlen(key) + 3;
                try { return std::stof(content.substr(pos)); } catch(...) { return def; }
            };
            ShowPlayerStatusEffects = readBool("ShowPlayerStatusEffects", ShowPlayerStatusEffects);
            PlayerStatusYOffset = readInt("PlayerStatusYOffset", PlayerStatusYOffset);
            PlayerStatusXOffset = readInt("PlayerStatusXOffset", PlayerStatusXOffset);
            PlayerStatusIconGap = readInt("PlayerStatusIconGap", PlayerStatusIconGap);
            AutoApplyStatusLayoutProfile = readBool("AutoApplyStatusLayoutProfile", AutoApplyStatusLayoutProfile);
            MonsterStatusXOffset = readInt("MonsterStatusXOffset", MonsterStatusXOffset);
            MonsterStatusYOffset = readInt("MonsterStatusYOffset", MonsterStatusYOffset);
            MonsterStatusIconGap = readInt("MonsterStatusIconGap", MonsterStatusIconGap);
            OnlyShowMonsterStatusEffectsForRareAndAbove = readBool("OnlyShowMonsterStatusEffectsForRareAndAbove", OnlyShowMonsterStatusEffectsForRareAndAbove);
            TextShadowAlpha = readFloat("TextShadowAlpha", TextShadowAlpha);
            TextShadowSize = readInt("TextShadowSize", TextShadowSize);
            ChargesXOffset = readInt("ChargesXOffset", ChargesXOffset);
            ChargesYOffset = readInt("ChargesYOffset", ChargesYOffset);
            TimerXOffset = readInt("TimerXOffset", TimerXOffset);
            TimerYOffset = readInt("TimerYOffset", TimerYOffset);

            // Read Icon path and import/export paths
            size_t posIcon = content.find("\"IconsAtlasPath\"");
            if (posIcon != std::string::npos) {
                size_t colon = content.find(':', posIcon);
                if (colon != std::string::npos) {
                    size_t quote = content.find('"', colon);
                    if (quote != std::string::npos) {
                        size_t quote2 = content.find('"', quote + 1);
                        if (quote2 != std::string::npos && quote2 > quote) {
                            std::string raw = content.substr(quote + 1, quote2 - quote - 1);
                            IconsAtlasPath = unescape(raw);
                        }
                    }
                }
            }
            size_t posGImp = content.find("\"GroundImportFile\"");
            if (posGImp != std::string::npos) {
                size_t colon = content.find(':', posGImp);
                if (colon != std::string::npos) {
                    size_t quote = content.find('"', colon);
                    if (quote != std::string::npos) {
                        size_t quote2 = content.find('"', quote + 1);
                        if (quote2 != std::string::npos && quote2 > quote) {
                            GroundImportFile = unescape(content.substr(quote + 1, quote2 - quote - 1));
                        }
                    }
                }
            }
            size_t posGExp = content.find("\"GroundExportFile\"");
            if (posGExp != std::string::npos) {
                size_t colon = content.find(':', posGExp);
                if (colon != std::string::npos) {
                    size_t quote = content.find('"', colon);
                    if (quote != std::string::npos) {
                        size_t quote2 = content.find('"', quote + 1);
                        if (quote2 != std::string::npos && quote2 > quote) {
                            GroundExportFile = unescape(content.substr(quote + 1, quote2 - quote - 1));
                        }
                    }
                }
            }
            size_t posMImp = content.find("\"MonstersImportFile\"");
            if (posMImp != std::string::npos) {
                size_t colon = content.find(':', posMImp);
                if (colon != std::string::npos) {
                    size_t quote = content.find('"', colon);
                    if (quote != std::string::npos) {
                        size_t quote2 = content.find('"', quote + 1);
                        if (quote2 != std::string::npos && quote2 > quote) {
                            MonstersImportFile = unescape(content.substr(quote + 1, quote2 - quote - 1));
                        }
                    }
                }
            }
            size_t posMExp = content.find("\"MonstersExportFile\"");
            if (posMExp != std::string::npos) {
                size_t colon = content.find(':', posMExp);
                if (colon != std::string::npos) {
                    size_t quote = content.find('"', colon);
                    if (quote != std::string::npos) {
                        size_t quote2 = content.find('"', quote + 1);
                        if (quote2 != std::string::npos && quote2 > quote) {
                            MonstersExportFile = unescape(content.substr(quote + 1, quote2 - quote - 1));
                        }
                    }
                }
            }
            size_t posPImp = content.find("\"PlayerImportFile\"");
            if (posPImp != std::string::npos) {
                size_t colon = content.find(':', posPImp);
                if (colon != std::string::npos) {
                    size_t quote = content.find('"', colon);
                    if (quote != std::string::npos) {
                        size_t quote2 = content.find('"', quote + 1);
                        if (quote2 != std::string::npos && quote2 > quote) {
                            PlayerImportFile = unescape(content.substr(quote + 1, quote2 - quote - 1));
                        }
                    }
                }
            }
            size_t posPExp = content.find("\"PlayerExportFile\"");
            if (posPExp != std::string::npos) {
                size_t colon = content.find(':', posPExp);
                if (colon != std::string::npos) {
                    size_t quote = content.find('"', colon);
                    if (quote != std::string::npos) {
                        size_t quote2 = content.find('"', quote + 1);
                        if (quote2 != std::string::npos && quote2 > quote) {
                            PlayerExportFile = unescape(content.substr(quote + 1, quote2 - quote - 1));
                        }
                    }
                }
            }
        }
    }

    // Load effects from separate files under <root>/Configs/tracker/
    std::filesystem::path cfgDir = hostRoot / "Configs" / "tracker";
    std::filesystem::path groundFile = cfgDir / "ground_effects_config.json";
    std::filesystem::path monstersFile = cfgDir / "monsters_effects_config.json";
    std::filesystem::path playerFile = cfgDir / "player_effects_config.json";

    // helpers: case-insensitive find and bracket matching
    auto toLower = [&](const std::string& s) {
        std::string r; r.resize(s.size());
        for (size_t i = 0; i < s.size(); ++i) r[i] = (char)std::tolower((unsigned char)s[i]);
        return r;
    };
    auto find_matching = [&](const std::string& s, size_t pos, char openC, char closeC)->size_t {
        int depth = 0; for (size_t i = pos; i < s.size(); ++i) { if (s[i] == openC) ++depth; else if (s[i] == closeC) { --depth; if (depth == 0) return i; } } return std::string::npos;
    };

    auto extractStatusListFromContent = [&](const std::string& content, const std::string& key, std::vector<StatusEffectSettings>& outList) {
        outList.clear();
        std::string contentLower = toLower(content);
        std::string kl = toLower(key);
        size_t pos = contentLower.find(kl);
        if (pos == std::string::npos) return;
        size_t arrStart = content.find('[', pos);
        if (arrStart == std::string::npos) return;
        size_t arrEnd = find_matching(content, arrStart, '[', ']');
        if (arrEnd == std::string::npos) return;
        std::string sub = content.substr(arrStart + 1, arrEnd - arrStart - 1);
        size_t i = 0;
        while (i < sub.size()) {
            size_t objStart = sub.find('{', i);
            if (objStart == std::string::npos) break;
            size_t objEnd = find_matching(sub, objStart, '{', '}');
            if (objEnd == std::string::npos) break;
            std::string obj = sub.substr(objStart + 1, objEnd - objStart - 1);
            StatusEffectSettings s;
            auto extractString = [&](const char* k) -> std::string {
                std::string klk = toLower(k);
                size_t p = toLower(obj).find(klk);
                if (p == std::string::npos) return std::string();
                size_t colon = obj.find(':', p);
                if (colon == std::string::npos) return std::string();
                size_t quote = obj.find('"', colon);
                if (quote == std::string::npos) {
                    size_t start = colon + 1; while (start < obj.size() && isspace((unsigned char)obj[start])) ++start;
                    size_t endn = start; while (endn < obj.size() && obj[endn] != ',' && obj[endn] != '\n' && obj[endn] != '}') ++endn;
                    return obj.substr(start, endn - start);
                }
                size_t quote2 = obj.find('"', quote + 1);
                if (quote2 == std::string::npos) return std::string();
                return obj.substr(quote + 1, quote2 - quote - 1);
            };
            auto extractInt = [&](const char* k, int def) -> int {
                std::string klk = toLower(k);
                size_t p = toLower(obj).find(klk);
                if (p == std::string::npos) return def;
                size_t colon = obj.find(':', p);
                if (colon == std::string::npos) return def;
                size_t start = colon + 1; while (start < obj.size() && isspace((unsigned char)obj[start])) ++start;
                size_t endn = start; while (endn < obj.size() && (obj[endn] == '-' || isdigit((unsigned char)obj[endn]))) ++endn;
                try { return std::stoi(obj.substr(start, endn - start)); } catch (...) { return def; }
            };
            auto extractFloat = [&](const char* k, float def) -> float {
                std::string klk = toLower(k);
                size_t p = toLower(obj).find(klk);
                if (p == std::string::npos) return def;
                size_t colon = obj.find(':', p);
                if (colon == std::string::npos) return def;
                size_t start = colon + 1; while (start < obj.size() && isspace((unsigned char)obj[start])) ++start;
                size_t endn = start; while (endn < obj.size() && (obj[endn] == '-' || obj[endn] == '.' || isdigit((unsigned char)obj[endn]))) ++endn;
                try { return std::stof(obj.substr(start, endn - start)); } catch (...) { return def; }
            };
            s.Name = extractString("Name");
            s.DisplayName = extractString("DisplayName");
            s.IsEnabled = (toLower(obj).find("isenabled") != std::string::npos && obj.find("true") != std::string::npos) || (toLower(obj).find("enabled") != std::string::npos && obj.find("true") != std::string::npos);
            s.IconColumn = extractInt("IconColumn", 0);
            s.IconRow = extractInt("IconRow", 0);
            s.IconScale = extractFloat("IconScale", 1.0f);
            // extract color arrays
            auto extractArray = [&](const char* k, ImVec4& out, ImVec4 def) {
                out = def; std::string klk = toLower(k); size_t p = toLower(obj).find(klk); if (p == std::string::npos) return;
                // try array form
                size_t b = obj.find('[', p);
                if (b != std::string::npos) {
                    size_t e = find_matching(obj, b, '[', ']'); if (e == std::string::npos) return; std::string subarr = obj.substr(b+1, e-b-1);
                    std::vector<float> vals; size_t idx = 0; while (idx < subarr.size()) { while (idx < subarr.size() && isspace((unsigned char)subarr[idx])) ++idx; size_t st = idx; while (idx < subarr.size() && (subarr[idx]=='-' || subarr[idx]=='.' || isdigit((unsigned char)subarr[idx]))) ++idx; if (st < idx) { try { vals.push_back(std::stof(subarr.substr(st, idx-st))); } catch(...) { vals.push_back(0.f); } } else break; while (idx < subarr.size() && (subarr[idx]==',' || isspace((unsigned char)subarr[idx]))) ++idx; } if (vals.size() >= 4) { out = ImVec4(vals[0], vals[1], vals[2], vals[3]); return; }
                }
                // try object form { "X": v, "Y": v, "Z": v, "W": v }
                size_t ob = obj.find('{', p);
                if (ob == std::string::npos) return;
                size_t oe = find_matching(obj, ob, '{', '}'); if (oe == std::string::npos) return;
                std::string subobj = obj.substr(ob+1, oe-ob-1);
                auto readVal = [&](const char* name, float def)->float {
                    std::string nl = toLower(name);
                    size_t pos = toLower(subobj).find(nl);
                    if (pos == std::string::npos) return def;
                    size_t colon = subobj.find(':', pos);
                    if (colon == std::string::npos) return def;
                    size_t start = colon + 1; while (start < subobj.size() && isspace((unsigned char)subobj[start])) ++start;
                    size_t endn = start; while (endn < subobj.size() && (subobj[endn]=='-' || subobj[endn]=='.' || isdigit((unsigned char)subobj[endn]) || subobj[endn]=='e' || subobj[endn]=='E' || subobj[endn]=='+' )) ++endn;
                    try { return std::stof(subobj.substr(start, endn-start)); } catch(...) { return def; }
                };
                float x = readVal("X", def.x);
                float y = readVal("Y", def.y);
                float z = readVal("Z", def.z);
                float w = readVal("W", def.w);
                out = ImVec4(x,y,z,w);
            };
            extractArray("TextColor", s.TextColor, s.TextColor);
            extractArray("BarColor", s.BarColor, s.BarColor);
            outList.push_back(s);
            i = objEnd + 1;
        }
    };

    auto extractGroundListFromContent = [&](const std::string& content) {
        size_t posG = toLower(content).find("groundeffects");
        if (posG == std::string::npos) return;
        GroundEffects.clear();
        size_t pos = content.find('[', posG);
        if (pos == std::string::npos) return;
        size_t end = find_matching(content, pos, '[', ']');
        if (end == std::string::npos) return;
        std::string sub = content.substr(pos+1, end-pos-1);
        size_t i = 0;
        while (i < sub.size()) {
            size_t objStart = sub.find('{', i);
            if (objStart == std::string::npos) break;
            size_t objEnd = find_matching(sub, objStart, '{', '}');
            if (objEnd == std::string::npos) break;
            std::string obj = sub.substr(objStart+1, objEnd-objStart-1);
            GroundEffectSettings g;
            auto extractStringG = [&](const char* k) -> std::string {
                size_t p = obj.find(std::string("\"") + k + "\"");
                if (p == std::string::npos) return std::string();
                size_t colon = obj.find(':', p);
                if (colon == std::string::npos) return std::string();
                size_t quote = obj.find('"', colon);
                if (quote == std::string::npos) return std::string();
                size_t quote2 = obj.find('"', quote + 1);
                if (quote2 == std::string::npos) return std::string();
                return obj.substr(quote + 1, quote2 - quote - 1);
            };
            auto extractIntG = [&](const char* k, int def) -> int {
                size_t p = obj.find(std::string("\"") + k + "\"");
                if (p == std::string::npos) return def;
                size_t colon = obj.find(':', p);
                if (colon == std::string::npos) return def;
                size_t start = colon + 1; while (start < obj.size() && isspace((unsigned char)obj[start])) ++start;
                size_t endn = start; while (endn < obj.size() && (obj[endn] == '-' || isdigit((unsigned char)obj[endn]))) ++endn;
                try { return std::stoi(obj.substr(start, endn - start)); } catch (...) { return def; }
            };
            g.Path = extractStringG("Path");
            g.IsEnabled = (obj.find("\"IsEnabled\": true") != std::string::npos);
            g.Radius = extractIntG("Radius", g.Radius);
            g.LineWeight = extractIntG("LineWeight", g.LineWeight);
            g.Filled = (obj.find("\"Filled\": true") != std::string::npos);
            // color extraction (supports array or object form)
            size_t colPos = obj.find("\"Color\"");
            if (colPos != std::string::npos) {
                size_t b = obj.find('[', colPos);
                if (b != std::string::npos) {
                    size_t e = find_matching(obj, b, '[', ']');
                    if (e != std::string::npos) {
                        std::string subarr = obj.substr(b+1, e-b-1);
                        std::vector<float> vals; size_t idx = 0; while (idx < subarr.size()) { while (idx < subarr.size() && isspace((unsigned char)subarr[idx])) ++idx; size_t st = idx; while (idx < subarr.size() && (subarr[idx]=='-' || subarr[idx]=='.' || isdigit((unsigned char)subarr[idx]) || subarr[idx]=='e' || subarr[idx]=='E' || subarr[idx]=='+' )) ++idx; if (st < idx) { try { vals.push_back(std::stof(subarr.substr(st, idx-st))); } catch(...) { vals.push_back(0.f); } } else break; while (idx < subarr.size() && (subarr[idx]==',' || isspace((unsigned char)subarr[idx]))) ++idx; } if (vals.size() >= 4) g.Color = ImVec4(vals[0], vals[1], vals[2], vals[3]);
                    }
                } else {
                    // try object form
                    size_t ob = obj.find('{', colPos);
                    if (ob != std::string::npos) {
                        size_t oe = find_matching(obj, ob, '{', '}');
                        if (oe != std::string::npos) {
                            std::string subobj = obj.substr(ob+1, oe-ob-1);
                            auto readVal = [&](const char* name, float def)->float {
                                std::string nl = toLower(name);
                                size_t pos = toLower(subobj).find(nl);
                                if (pos == std::string::npos) return def;
                                size_t colon = subobj.find(':', pos);
                                if (colon == std::string::npos) return def;
                                size_t start = colon + 1; while (start < subobj.size() && isspace((unsigned char)subobj[start])) ++start;
                                size_t endn = start; while (endn < subobj.size() && (subobj[endn]=='-' || subobj[endn]=='.' || isdigit((unsigned char)subobj[endn]) || subobj[endn]=='e' || subobj[endn]=='E' || subobj[endn]=='+' )) ++endn;
                                try { return std::stof(subobj.substr(start, endn-start)); } catch(...) { return def; }
                            };
                            float x = readVal("X", g.Color.x);
                            float y = readVal("Y", g.Color.y);
                            float z = readVal("Z", g.Color.z);
                            float w = readVal("W", g.Color.w);
                            g.Color = ImVec4(x,y,z,w);
                        }
                    }
                }
            }
            GroundEffects.push_back(g);
            i = objEnd + 1;
        }
    };

    bool foundAny = false;
    if (std::filesystem::exists(groundFile)) {
        std::ifstream inG(groundFile);
        if (inG.is_open()) {
            std::string content((std::istreambuf_iterator<char>(inG)), std::istreambuf_iterator<char>());
            extractGroundListFromContent(content);
            foundAny = true;
        }
    }
    if (std::filesystem::exists(monstersFile)) {
        std::ifstream inM(monstersFile);
        if (inM.is_open()) {
            std::string content((std::istreambuf_iterator<char>(inM)), std::istreambuf_iterator<char>());
            extractStatusListFromContent(content, "StatusEffects", StatusEffects);
            foundAny = true;
        }
    }
    if (std::filesystem::exists(playerFile)) {
        std::ifstream inP(playerFile);
        if (inP.is_open()) {
            std::string content((std::istreambuf_iterator<char>(inP)), std::istreambuf_iterator<char>());
            extractStatusListFromContent(content, "PlayerStatusEffects", PlayerStatusEffects);
            foundAny = true;
        }
    }

    // fallback: if no separate files, attempt to read legacy combined config.json
    if (!foundAny) {
        std::filesystem::path effectsCfg = cfgDir / "config.json";
        if (std::filesystem::exists(effectsCfg)) {
            std::ifstream in2(effectsCfg);
            if (in2.is_open()) {
                std::string content((std::istreambuf_iterator<char>(in2)), std::istreambuf_iterator<char>());
                extractGroundListFromContent(content);
                extractStatusListFromContent(content, "StatusEffects", StatusEffects);
                extractStatusListFromContent(content, "PlayerStatusEffects", PlayerStatusEffects);
            }
        }
    }
}

std::filesystem::path TrackerSettings::ResolveIconsAtlasPath(const std::filesystem::path& directory) const {
    if (IconsAtlasPath.empty()) return {};
    try {
        std::filesystem::path candidate(IconsAtlasPath);
        if (candidate.is_absolute()) return candidate;
        // prefer host-root relative (user-visible stored path is intended to be relative to POE Fixer root)
        wchar_t buf[MAX_PATH];
        std::wstring hostBaseW;
        if (GetModuleFileNameW(NULL, buf, (DWORD)MAX_PATH) > 0) {
            std::filesystem::path p(buf);
            hostBaseW = p.parent_path().wstring();
        } else hostBaseW = L".";
        std::filesystem::path hostRoot(hostBaseW);
        std::filesystem::path tryHost = hostRoot / candidate;
        if (std::filesystem::exists(tryHost)) return tryHost;
        // fall back to plugin directory relative resolution
        return directory / candidate;
    } catch(...) {
        return std::filesystem::path(IconsAtlasPath);
    }
}

namespace {
    bool ListContainsOldLayoutCoordinates(const std::vector<StatusEffectSettings>& effects, int newColumns)
    {
        if (effects.empty()) return false;
        for (const auto& e : effects) {
            if (e.IconColumn >= newColumns) return true;
        }
        return false;
    }

    void RemapIconCoordinates(std::vector<StatusEffectSettings>& effects, int oldColumns, int newColumns)
    {
        if (effects.empty()) return;
        for (auto& e : effects) {
            int rowIdx = (e.IconRow < 0) ? 0 : e.IconRow;
            int colIdx = (e.IconColumn < 0) ? 0 : e.IconColumn;
            int oldIndex = rowIdx * oldColumns + colIdx;
            e.IconColumn = oldIndex % newColumns;
            e.IconRow = oldIndex / newColumns;
        }
    }
}

std::string TrackerSettings::ExportToFile(const std::filesystem::path& path) const {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path);
    if (!out.is_open()) return "Export failed: cannot open file.";
    out << "{\n";
    out << "  \"UniqueLine\":" << (UniqueLine ? "true" : "false") << ",\n";
    out << "  \"RareLine\":" << (RareLine ? "true" : "false") << ",\n";
    out << "  \"MagicLine\":" << (MagicLine ? "true" : "false") << ",\n";
    // StatusEffects
    out << "  \"StatusEffects\": [\n";
    for (size_t i = 0; i < StatusEffects.size(); ++i) {
        const auto& s = StatusEffects[i];
        out << "    {\n";
        out << "      \"Name\": \"" << s.Name << "\",\n";
        out << "      \"DisplayName\": \"" << s.DisplayName << "\",\n";
        out << "      \"IsEnabled\": " << (s.IsEnabled ? "true" : "false") << ",\n";
        out << "      \"IconColumn\": " << s.IconColumn << ",\n";
        out << "      \"IconRow\": " << s.IconRow << ",\n";
        out << "      \"IconScale\": " << s.IconScale << ",\n";
        out << "      \"TextColor\": [" << s.TextColor.x << "," << s.TextColor.y << "," << s.TextColor.z << "," << s.TextColor.w << "],\n";
        out << "      \"BarColor\": [" << s.BarColor.x << "," << s.BarColor.y << "," << s.BarColor.z << "," << s.BarColor.w << "]\n";
        out << "    }" << (i + 1 < StatusEffects.size() ? ",\n" : "\n");
    }
    out << "  ],\n";
    // PlayerStatusEffects
    out << "  \"PlayerStatusEffects\": [\n";
    for (size_t i = 0; i < PlayerStatusEffects.size(); ++i) {
        const auto& s = PlayerStatusEffects[i];
        out << "    {\n";
        out << "      \"Name\": \"" << s.Name << "\",\n";
        out << "      \"DisplayName\": \"" << s.DisplayName << "\",\n";
        out << "      \"IsEnabled\": " << (s.IsEnabled ? "true" : "false") << ",\n";
        out << "      \"IconColumn\": " << s.IconColumn << ",\n";
        out << "      \"IconRow\": " << s.IconRow << ",\n";
        out << "      \"IconScale\": " << s.IconScale << ",\n";
        out << "      \"TextColor\": [" << s.TextColor.x << "," << s.TextColor.y << "," << s.TextColor.z << "," << s.TextColor.w << "],\n";
        out << "      \"BarColor\": [" << s.BarColor.x << "," << s.BarColor.y << "," << s.BarColor.z << "," << s.BarColor.w << "]\n";
        out << "    }" << (i + 1 < PlayerStatusEffects.size() ? ",\n" : "\n");
    }
    out << "  ]\n";
    out << "}\n";
    out.close();
    return std::string("Exported: ") + path.string();
}

std::string TrackerSettings::ImportFromFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) return std::string("Import failed: file not found: ") + path.string();
    std::ifstream in(path);
    if (!in.is_open()) return std::string("Import failed: cannot open file: ") + path.string();
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    // crude parser: extract StatusEffects and PlayerStatusEffects by searching for objects
    // we'll search in a case-insensitive and format-flexible way and robustly match brackets
    auto toLower = [&](const std::string& s) {
        std::string r; r.resize(s.size());
        for (size_t i = 0; i < s.size(); ++i) r[i] = (char)std::tolower((unsigned char)s[i]);
        return r;
    };
    std::string contentLower = toLower(content);

    auto find_key_pos = [&](const std::string& key)->size_t {
        // try to find quoted key first (case-insensitive)
        std::string kl = toLower(key);
        std::string quoted = std::string("\"") + kl + std::string("\"");
        size_t p = contentLower.find(quoted);
        if (p != std::string::npos) return p;
        // accept variants with underscores/dashes and mixedCase as whole-word matches
        std::string ku = kl; for (auto& c : ku) if (c == ' ') c = '_';
        std::string kd = kl; for (auto& c : kd) if (c == ' ') c = '-';
        auto findWhole = [&](const std::string& needle)->size_t {
            size_t at = contentLower.find(needle);
            while (at != std::string::npos) {
                // ensure not inside a longer identifier: check char before and after
                bool okBefore = (at == 0) || !std::isalnum((unsigned char)contentLower[at-1]);
                size_t after = at + needle.size();
                bool okAfter = (after >= contentLower.size()) || !std::isalnum((unsigned char)contentLower[after]);
                if (okBefore && okAfter) return at;
                at = contentLower.find(needle, at + 1);
            }
            return std::string::npos;
        };
        p = findWhole(ku);
        if (p != std::string::npos) return p;
        p = findWhole(kd);
        if (p != std::string::npos) return p;
        // lastly try plain key as whole word
        p = findWhole(kl);
        return p;
    };

    auto find_matching = [&](const std::string& s, size_t pos, char openC, char closeC)->size_t {
        int depth = 0;
        for (size_t i = pos; i < s.size(); ++i) {
            if (s[i] == openC) ++depth;
            else if (s[i] == closeC) { --depth; if (depth == 0) return i; }
        }
        return std::string::npos;
    };

    auto extractList = [&](const std::string& key, std::vector<StatusEffectSettings>& outList) {
        size_t pos = find_key_pos(key);
        if (pos == std::string::npos) return;
        outList.clear();
        // find '[' after pos
        size_t arrStart = content.find('[', pos);
        if (arrStart == std::string::npos) return;
        size_t arrEnd = find_matching(content, arrStart, '[', ']');
        if (arrEnd == std::string::npos) return;
        std::string sub = content.substr(arrStart + 1, arrEnd - arrStart - 1);
        size_t i = 0;
        while (i < sub.size()) {
            size_t objStart = sub.find('{', i);
            if (objStart == std::string::npos) break;
            size_t objEnd = find_matching(sub, objStart, '{', '}');
            if (objEnd == std::string::npos) break;
            std::string obj = sub.substr(objStart + 1, objEnd - objStart - 1);
            StatusEffectSettings s;
            auto extractString = [&](const char* k) -> std::string {
                std::string kl = toLower(k);
                // search for "key" or key without quotes
                size_t p = toLower(obj).find(kl);
                if (p == std::string::npos) return std::string();
                size_t colon = obj.find(':', p);
                if (colon == std::string::npos) return std::string();
                // find first quote after colon
                size_t quote = obj.find('"', colon);
                if (quote == std::string::npos) {
                    // maybe unquoted value until comma
                    size_t start = colon + 1;
                    while (start < obj.size() && isspace((unsigned char)obj[start])) ++start;
                    size_t endn = start;
                    while (endn < obj.size() && obj[endn] != ',' && obj[endn] != '\n' && obj[endn] != '}') ++endn;
                    return obj.substr(start, endn - start);
                }
                size_t quote2 = obj.find('"', quote + 1);
                if (quote2 == std::string::npos) return std::string();
                return obj.substr(quote + 1, quote2 - quote - 1);
            };
            auto extractInt = [&](const char* k, int def) -> int {
                std::string kl = toLower(k);
                size_t p = toLower(obj).find(kl);
                if (p == std::string::npos) return def;
                size_t colon = obj.find(':', p);
                if (colon == std::string::npos) return def;
                size_t start = colon + 1;
                while (start < obj.size() && isspace((unsigned char)obj[start])) ++start;
                size_t endn = start;
                while (endn < obj.size() && (obj[endn] == '-' || isdigit((unsigned char)obj[endn]))) ++endn;
                try { return std::stoi(obj.substr(start, endn - start)); } catch (...) { return def; }
            };
            auto extractFloat = [&](const char* k, float def) -> float {
                std::string kl = toLower(k);
                size_t p = toLower(obj).find(kl);
                if (p == std::string::npos) return def;
                size_t colon = obj.find(':', p);
                if (colon == std::string::npos) return def;
                size_t start = colon + 1;
                while (start < obj.size() && isspace((unsigned char)obj[start])) ++start;
                size_t endn = start;
                while (endn < obj.size() && (obj[endn] == '-' || obj[endn] == '.' || isdigit((unsigned char)obj[endn]))) ++endn;
                try { return std::stof(obj.substr(start, endn - start)); } catch (...) { return def; }
            };

            s.Name = extractString("Name");
            s.DisplayName = extractString("DisplayName");
            s.IsEnabled = (toLower(obj).find("isenabled") != std::string::npos && obj.find("true") != std::string::npos) || (toLower(obj).find("enabled") != std::string::npos && obj.find("true") != std::string::npos);
            s.IconColumn = extractInt("IconColumn", 0);
            s.IconRow = extractInt("IconRow", 0);
            s.IconScale = extractFloat("IconScale", 1.0f);
            // extract TextColor and BarColor arrays if present (supports array or {X,Y,Z,W} object)
            auto extractArray = [&](const char* k, ImVec4& out, ImVec4 def) {
                out = def;
                std::string kl = toLower(k);
                size_t p = toLower(obj).find(kl);
                if (p == std::string::npos) return;
                // try array form
                size_t b = obj.find('[', p);
                if (b != std::string::npos) {
                    size_t e = find_matching(obj, b, '[', ']');
                    if (e != std::string::npos) {
                        std::string subarr = obj.substr(b+1, e-b-1);
                        std::vector<float> vals; size_t idx = 0;
                        while (idx < subarr.size()) {
                            while (idx < subarr.size() && isspace((unsigned char)subarr[idx])) ++idx;
                            size_t st = idx;
                            while (idx < subarr.size() && (subarr[idx]=='-' || subarr[idx]=='.' || isdigit((unsigned char)subarr[idx]) || subarr[idx]=='e' || subarr[idx]=='E' || subarr[idx]=='+' )) ++idx;
                            if (st < idx) { try { vals.push_back(std::stof(subarr.substr(st, idx-st))); } catch(...) { vals.push_back(0.f); } } else break;
                            while (idx < subarr.size() && (subarr[idx]==',' || isspace((unsigned char)subarr[idx]))) ++idx;
                        }
                        if (vals.size() >= 4) { out = ImVec4(vals[0], vals[1], vals[2], vals[3]); return; }
                    }
                }
                // try object form
                size_t ob = obj.find('{', p);
                if (ob == std::string::npos) return;
                size_t oe = find_matching(obj, ob, '{', '}'); if (oe == std::string::npos) return;
                std::string subobj = obj.substr(ob+1, oe-ob-1);
                auto readVal = [&](const char* name, float def)->float {
                    std::string nl = toLower(name);
                    size_t pos = toLower(subobj).find(nl);
                    if (pos == std::string::npos) return def;
                    size_t colon = subobj.find(':', pos);
                    if (colon == std::string::npos) return def;
                    size_t start = colon + 1; while (start < subobj.size() && isspace((unsigned char)subobj[start])) ++start;
                    size_t endn = start; while (endn < subobj.size() && (subobj[endn]=='-' || subobj[endn]=='.' || isdigit((unsigned char)subobj[endn]) || subobj[endn]=='e' || subobj[endn]=='E' || subobj[endn]=='+' )) ++endn;
                    try { return std::stof(subobj.substr(start, endn-start)); } catch(...) { return def; }
                };
                float x = readVal("X", def.x);
                float y = readVal("Y", def.y);
                float z = readVal("Z", def.z);
                float w = readVal("W", def.w);
                out = ImVec4(x,y,z,w);
            };
            extractArray("TextColor", s.TextColor, s.TextColor);
            extractArray("BarColor", s.BarColor, s.BarColor);
            outList.push_back(s);
            i = objEnd + 1;
        }
    };

    extractList("StatusEffects", StatusEffects);
    extractList("PlayerStatusEffects", PlayerStatusEffects);
    // also attempt to import GroundEffects if present in the file
    auto extractGroundFromImport = [&]() {
        std::string contentLower2 = toLower(content);
        size_t posG = contentLower2.find("groundeffects");
        if (posG == std::string::npos) return;
        size_t arrStart = content.find('[', posG);
        if (arrStart == std::string::npos) return;
        size_t arrEnd = find_matching(content, arrStart, '[', ']');
        if (arrEnd == std::string::npos) return;
        std::string sub = content.substr(arrStart + 1, arrEnd - arrStart - 1);
        size_t i = 0;
        std::vector<GroundEffectSettings> gList;
        while (i < sub.size()) {
            size_t objStart = sub.find('{', i);
            if (objStart == std::string::npos) break;
            size_t objEnd = find_matching(sub, objStart, '{', '}');
            if (objEnd == std::string::npos) break;
            std::string obj = sub.substr(objStart + 1, objEnd - objStart - 1);
            GroundEffectSettings g;
            auto extractStringG = [&](const char* k) -> std::string {
                std::string kl = toLower(k);
                size_t p = toLower(obj).find(kl);
                if (p == std::string::npos) return std::string();
                size_t colon = obj.find(':', p);
                if (colon == std::string::npos) return std::string();
                size_t quote = obj.find('"', colon);
                if (quote == std::string::npos) return std::string();
                size_t quote2 = obj.find('"', quote + 1);
                if (quote2 == std::string::npos) return std::string();
                return obj.substr(quote + 1, quote2 - quote - 1);
            };
            auto extractIntG = [&](const char* k, int def) -> int {
                std::string kl = toLower(k);
                size_t p = toLower(obj).find(kl);
                if (p == std::string::npos) return def;
                size_t colon = obj.find(':', p);
                if (colon == std::string::npos) return def;
                size_t start = colon + 1; while (start < obj.size() && isspace((unsigned char)obj[start])) ++start;
                size_t endn = start; while (endn < obj.size() && (obj[endn] == '-' || isdigit((unsigned char)obj[endn]))) ++endn;
                try { return std::stoi(obj.substr(start, endn - start)); } catch (...) { return def; }
            };
            g.Path = extractStringG("Path");
            g.IsEnabled = (toLower(obj).find("isenabled") != std::string::npos && obj.find("true") != std::string::npos);
            g.Radius = extractIntG("Radius", g.Radius);
            g.LineWeight = extractIntG("LineWeight", g.LineWeight);
            g.Filled = (toLower(obj).find("filled") != std::string::npos && obj.find("true") != std::string::npos);
            // color (support array or object {"X":..})
            size_t colPos = toLower(obj).find("color");
            if (colPos != std::string::npos) {
                size_t b = obj.find('[', colPos);
                if (b != std::string::npos) {
                    size_t e = find_matching(obj, b, '[', ']');
                    if (e != std::string::npos) {
                        std::string subarr = obj.substr(b+1, e-b-1);
                        std::vector<float> vals; size_t idx = 0; while (idx < subarr.size()) { while (idx < subarr.size() && isspace((unsigned char)subarr[idx])) ++idx; size_t st = idx; while (idx < subarr.size() && (subarr[idx]=='-' || subarr[idx]=='.' || isdigit((unsigned char)subarr[idx]) || subarr[idx]=='e' || subarr[idx]=='E' || subarr[idx]=='+' )) ++idx; if (st < idx) { try { vals.push_back(std::stof(subarr.substr(st, idx-st))); } catch(...) { vals.push_back(0.f); } } else break; while (idx < subarr.size() && (subarr[idx]==',' || isspace((unsigned char)subarr[idx]))) ++idx; } if (vals.size() >= 4) g.Color = ImVec4(vals[0], vals[1], vals[2], vals[3]);
                    }
                } else {
                    // try object form
                    size_t ob = obj.find('{', colPos);
                    if (ob != std::string::npos) {
                        size_t oe = find_matching(obj, ob, '{', '}');
                        if (oe != std::string::npos) {
                            std::string subobj = obj.substr(ob+1, oe-ob-1);
                            auto readVal = [&](const char* name, float def)->float {
                                std::string nl = toLower(name);
                                size_t pos = toLower(subobj).find(nl);
                                if (pos == std::string::npos) return def;
                                size_t colon = subobj.find(':', pos);
                                if (colon == std::string::npos) return def;
                                size_t start = colon + 1; while (start < subobj.size() && isspace((unsigned char)subobj[start])) ++start;
                                size_t endn = start; while (endn < subobj.size() && (subobj[endn]=='-' || subobj[endn]=='.' || isdigit((unsigned char)subobj[endn]) || subobj[endn]=='e' || subobj[endn]=='E' || subobj[endn]=='+' )) ++endn;
                                try { return std::stof(subobj.substr(start, endn-start)); } catch(...) { return def; }
                            };
                            float x = readVal("X", g.Color.x);
                            float y = readVal("Y", g.Color.y);
                            float z = readVal("Z", g.Color.z);
                            float w = readVal("W", g.Color.w);
                            g.Color = ImVec4(x,y,z,w);
                        }
                    }
                }
            }
            gList.push_back(g);
            i = objEnd + 1;
        }
        if (!gList.empty()) {
            GroundEffects = gList;
        }
    };
    extractGroundFromImport();
    // note: we don't fully populate colors from import here — keep defaults if missing
    Save(std::filesystem::path("."));
    // return counts for diagnostics
    char buf[256];
    snprintf(buf, sizeof(buf), "Imported: %s. StatusEffects=%zu, PlayerStatusEffects=%zu", path.string().c_str(), StatusEffects.size(), PlayerStatusEffects.size());
    return std::string(buf);
}

std::string TrackerSettings::RemapIconCoordinatesTo8ColumnsNow()
{
    const int oldColumns = 28;
    const int newColumns = 8;

    bool hasOld = ListContainsOldLayoutCoordinates(StatusEffects, newColumns) || ListContainsOldLayoutCoordinates(PlayerStatusEffects, newColumns);
    if (!hasOld) return "Remap skipped: no icon coordinates from 28-column layout were found.";
    RemapIconCoordinates(StatusEffects, oldColumns, newColumns);
    RemapIconCoordinates(PlayerStatusEffects, oldColumns, newColumns);
    IconCoordinatesMigratedTo8Columns = true;
    Save(std::filesystem::path("."));
    return "Icon coordinates remapped from 28 to 8 columns.";
}

std::string TrackerSettings::ForceRemapIconCoordinatesTo8ColumnsNow()
{
    const int oldColumns = 28;
    const int newColumns = 8;
    RemapIconCoordinates(StatusEffects, oldColumns, newColumns);
    RemapIconCoordinates(PlayerStatusEffects, oldColumns, newColumns);
    IconCoordinatesMigratedTo8Columns = true;
    Save(std::filesystem::path("."));
    return "Force remap completed: icon coordinates recalculated from 28 to 8 columns.";
}

void TrackerSettings::MigrateIconCoordinatesTo8ColumnsIfNeeded()
{
    const int oldColumns = 28;
    const int newColumns = 8;
    if (IconCoordinatesMigratedTo8Columns) return;
    bool looksLikeOld = ListContainsOldLayoutCoordinates(StatusEffects, newColumns) || ListContainsOldLayoutCoordinates(PlayerStatusEffects, newColumns);
    if (!looksLikeOld) {
        IconCoordinatesMigratedTo8Columns = true;
        return;
    }
    RemapIconCoordinates(StatusEffects, oldColumns, newColumns);
    RemapIconCoordinates(PlayerStatusEffects, oldColumns, newColumns);
    IconCoordinatesMigratedTo8Columns = true;
    Save(std::filesystem::path("."));
}

std::string TrackerSettings::ExportGroundToFile(const std::filesystem::path& path) const {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path);
    if (!out.is_open()) return std::string("Export failed: cannot open file.");
    out << "{\n";
    out << "  \"GroundEffects\": [\n";
    for (size_t i = 0; i < GroundEffects.size(); ++i) {
        const auto& g = GroundEffects[i];
        out << "    {\n";
        out << "      \"Path\": \"" << g.Path << "\",\n";
        out << "      \"IsEnabled\": " << (g.IsEnabled ? "true" : "false") << ",\n";
        out << "      \"Color\": [" << g.Color.x << "," << g.Color.y << "," << g.Color.z << "," << g.Color.w << "],\n";
        out << "      \"Radius\": " << g.Radius << ",\n";
        out << "      \"LineWeight\": " << g.LineWeight << ",\n";
        out << "      \"Filled\": " << (g.Filled ? "true" : "false") << "\n";
        out << "    }" << (i + 1 < GroundEffects.size() ? ",\n" : "\n");
    }
    out << "  ]\n";
    out << "}\n";
    out.close();
    return std::string("Exported: ") + path.string();
}

std::string TrackerSettings::ExportMonstersToFile(const std::filesystem::path& path) const {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path);
    if (!out.is_open()) return std::string("Export failed: cannot open file.");
    out << "{\n";
    out << "  \"StatusEffects\": [\n";
    for (size_t i = 0; i < StatusEffects.size(); ++i) {
        const auto& s = StatusEffects[i];
        out << "    {\n";
        out << "      \"Name\": \"" << s.Name << "\",\n";
        out << "      \"DisplayName\": \"" << s.DisplayName << "\",\n";
        out << "      \"IsEnabled\": " << (s.IsEnabled ? "true" : "false") << ",\n";
        out << "      \"IconColumn\": " << s.IconColumn << ",\n";
        out << "      \"IconRow\": " << s.IconRow << ",\n";
        out << "      \"IconScale\": " << s.IconScale << ",\n";
        out << "      \"TextColor\": [" << s.TextColor.x << "," << s.TextColor.y << "," << s.TextColor.z << "," << s.TextColor.w << "],\n";
        out << "      \"BarColor\": [" << s.BarColor.x << "," << s.BarColor.y << "," << s.BarColor.z << "," << s.BarColor.w << "]\n";
        out << "    }" << (i + 1 < StatusEffects.size() ? ",\n" : "\n");
    }
    out << "  ]\n";
    out << "}\n";
    out.close();
    return std::string("Exported: ") + path.string();
}

std::string TrackerSettings::ExportPlayerToFile(const std::filesystem::path& path) const {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path);
    if (!out.is_open()) return std::string("Export failed: cannot open file.");
    out << "{\n";
    out << "  \"PlayerStatusEffects\": [\n";
    for (size_t i = 0; i < PlayerStatusEffects.size(); ++i) {
        const auto& s = PlayerStatusEffects[i];
        out << "    {\n";
        out << "      \"Name\": \"" << s.Name << "\",\n";
        out << "      \"DisplayName\": \"" << s.DisplayName << "\",\n";
        out << "      \"IsEnabled\": " << (s.IsEnabled ? "true" : "false") << ",\n";
        out << "      \"IconColumn\": " << s.IconColumn << ",\n";
        out << "      \"IconRow\": " << s.IconRow << ",\n";
        out << "      \"IconScale\": " << s.IconScale << ",\n";
        out << "      \"TextColor\": [" << s.TextColor.x << "," << s.TextColor.y << "," << s.TextColor.z << "," << s.TextColor.w << "],\n";
        out << "      \"BarColor\": [" << s.BarColor.x << "," << s.BarColor.y << "," << s.BarColor.z << "," << s.BarColor.w << "]\n";
        out << "    }" << (i + 1 < PlayerStatusEffects.size() ? ",\n" : "\n");
    }
    out << "  ]\n";
    out << "}\n";
    out.close();
    return std::string("Exported: ") + path.string();
}
