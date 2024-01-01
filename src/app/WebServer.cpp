// Created by Tube Lab. Part of the meloun project.
#include "app/WebServer.h"
using namespace ml::app;

#define CROW_POST_ROUTE(app, url) CROW_ROUTE(app, url).methods("POST"_method)

auto WebServer::Run(const std::string& configPath, uint port) noexcept -> bool
{
    // Load the config
    std::ifstream stream { configPath };
    if (!stream)
    {
        CROW_LOG_CRITICAL << "Can't open the config file.";
        return false;
    }

    auto config = ConfigParser::FromIni({ std::istreambuf_iterator<char> { stream }, std::istreambuf_iterator<char> {} });
    if (!config)
    {
        CROW_LOG_CRITICAL << "Can't parse the config.";
        return false;
    }

    stream.close();

    // Create the amplifier
    auto amplifier = amplifier::LampDriver::Create(amplifier::LampConfig {
        .WarmingDuration = config->WarmingDuration,
        .CoolingDuration = config->CoolingDuration,
        .PowerPort = config->PowerPort,
        .AudioDevice = config->AudioDevice,
        .Channels = (uint)config->Channels.size()
    });

    if (!amplifier)
    {
        CROW_LOG_CRITICAL << "Can't create the amplifier driver";
        return false;
    }

    // Create the speaker driver
    auto speaker = speaker::Driver::Create(speaker::Config {
        .Amplifier = amplifier,
        .Channels = config->Channels
    });

    if (!speaker)
    {
        CROW_LOG_CRITICAL << "Can't create the speaker driver";
        return false;
    }

    // Create the server & the API
    // For docs refer to API.md
    crow::App<TokenMiddleware> app { 
        TokenMiddleware { config->Token } 
    };

    // Session management
    CROW_POST_ROUTE(app, "/<string>/open")([&](const std::string& channel)
    {
        auto r = speaker->Open(channel);
        return r ? crow::response { "Ok" } : BindError(r.error());
    });

    CROW_POST_ROUTE(app, "/<string>/prolong")([&](const std::string& channel)
    {
        auto r = speaker->Prolong(channel);
        return r ? crow::response { "Ok" } : BindError(r.error());
    });

    // Activate/deactivate channel
    CROW_POST_ROUTE(app, "/<string>/activate")([&](const std::string& channel)
    {
        char* flag = crow::request().url_params.get("urgent");
        auto r = speaker->Activate(channel, flag != nullptr);
        return r ? LongPolling(r.value()) : BindError(r.error());
    });

    CROW_POST_ROUTE(app, "/<string>/deactivate")([&](const std::string& channel)
    {
        char* flag = crow::request().url_params.get("urgent");
        auto r = speaker->Deactivate(channel, flag != nullptr);
        return r ? LongPolling(r.value()) : BindError(r.error());
    });

    // Playback management
    CROW_POST_ROUTE(app, "/<string>/play")([&](const std::string& channel)
    {
        std::string raw = crow::utility::base64decode(crow::request().body);
        std::vector<char> decoded { raw.begin(), raw.end() };

        auto track = audio::TrackLoader::FromWav(decoded);
        if (track)
        {
            return crow::response { 400, "TrackIsNotWav" };
        }

        auto r = speaker->Enqueue(channel, *track);
        return r ? LongPolling(r.value()) : BindError(r.error());
    });

    CROW_POST_ROUTE(app, "/<string>/clear")([&](const std::string& channel)
    {
        auto r = speaker->Clear(channel);
        return r ? crow::response { "Ok" } : BindError(r.error());
    });

    CROW_ROUTE(app, "/duration-left")([&]()
    {
        auto r = speaker->DurationLeft();
        return r ? crow::response { "Ok" } : BindError(r.error());
    });

    CROW_ROUTE(app, "/activation-duration")([&]()
    {
        return speaker->ActivationDuration();
    });

    CROW_ROUTE(app, "/deactivation-duration")([&]()
    {
        return speaker->DeactivationDuration();
    });

    CROW_LOG_INFO << "Created the web-server. Running it on port " << port;
    app.multithreaded().port(port);
    app.multithreaded().run();

    return true;
}

auto WebServer::LongPolling(const std::future<void>& f) noexcept -> crow::response
{
    f.wait();
    return crow::response { "Ok" };
}

auto WebServer::BindError(speaker::ActionError error) noexcept -> crow::response
{
    if (error == speaker::AE_ChannelOpened) return crow::response { 400, "ChannelOpened" };
    if (error == speaker::AE_ChannelClosed) return crow::response { 400, "ChannelClosed" };
    if (error == speaker::AE_ChannelInactive) return crow::response { 400, "ChannelInactive" };
    if (error == speaker::AE_IncompatibleTrack) return crow::response { 400, "IncompatibleTrack" };
    if (error == speaker::AE_ChannelNotFound) return crow::response {400, "ChannelNotFound" };
}