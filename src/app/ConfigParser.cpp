// Created by Tube Lab. Part of the meloun project.
#include "app/ConfigParser.h"
using namespace ml::app;

auto ConfigParser::FromIni(const std::string& data) -> std::optional<Config>
{
    CSimpleIniA ini;
    ini.SetUnicode();

    SI_Error rc = ini.LoadData(data);
    if (rc < 0)
    {
        return std::nullopt;
    }

    // Parse "general" section
    Config cfg;
    cfg.Token = ini.GetValue("general", "token");
    cfg.WarmingDuration = ini.GetLongValue("general", "warming-duration");
    cfg.CoolingDuration = ini.GetLongValue("general", "cooling-duration");
    cfg.PowerPort = ini.GetValue("general", "power-port");
    cfg.AudioDevice = ini.GetValue("general", "audio-device");

    cfg.AudioDevice = cfg.AudioDevice->empty() ? std::nullopt : std::optional { cfg.AudioDevice };

    // Parse all the sink sections
    CSimpleIniA::TNamesDepend sections;
    ini.GetAllSections(sections);

    std::vector<std::pair<std::string, uint>> extracted;
    for (auto& entry : sections)
    {
        if (std::string {entry.pItem }.starts_with("channel-"))
        {
            extracted.emplace_back(entry.pItem + 8, ini.GetLongValue(entry.pItem, "priority"));
        }
    }

    // Sort by priority
    std::sort(extracted.begin(), extracted.end(), [](const auto& a, const auto& b)
    {
        return a.second < b.second;
    });

    // Inject the sorted value into the config
    for (const auto& [name, priority] : extracted)
    {
        cfg.Channels.push_back(name);
    }

    return cfg;
}
