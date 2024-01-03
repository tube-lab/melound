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
    crow::App<web::CorsMiddleware, web::ApiTokenMiddleware> app {
        web::CorsMiddleware { web::CorsRules {
            .Origin = std::nullopt,
            .Headers = std::nullopt,
            .Methods = { "GET"_method, "POST"_method }
        }},
        web::ApiTokenMiddleware {
            config->Token
        }
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
    CROW_POST_ROUTE(app, "/<string>/activate")([&](const crow::request& req, const std::string& channel)
    {
        auto r = speaker->Activate(channel, Urgent(req.get_body_params()));
        return r ? LongPolling(r.value()) : BindError(r.error());
    });

    CROW_POST_ROUTE(app, "/<string>/deactivate")([&](const crow::request& req, const std::string& channel)
    {
        auto r = speaker->Deactivate(channel, Urgent(req.get_body_params()));
        return r ? LongPolling(r.value()) : BindError(r.error());
    });

    // Playback management
    CROW_POST_ROUTE(app, "/<string>/play")([&](const crow::request& req, const std::string& channel)
    {
        std::string raw = crow::utility::base64decode(req.body);
        std::vector<char> decoded { raw.begin(), raw.end() };

        auto track = audio::TrackLoader::FromWav(decoded);
        if (!track)
        {
            return crow::response { 400, "400 Track Not Wav" };
        }

        auto r = speaker->Enqueue(channel, *track);
        return r ? LongPolling(r.value()) : BindError(r.error());
    });

    CROW_POST_ROUTE(app, "/<string>/clear")([&](const std::string& channel)
    {
        auto r = speaker->Clear(channel);
        return r ? crow::response { "Ok" } : BindError(r.error());
    });

    // Channel state getters
    CROW_ROUTE(app, "/<string>/state")([&](const std::string& channel)
    {
        auto r = speaker->State(channel);
        return r ? BindState(r.value()) : BindError(r.error());
    });

    CROW_ROUTE(app, "/<string>/duration-left")([&](const std::string& channel)
    {
        auto r = speaker->DurationLeft(channel);
        return r ? std::to_string(r.value()) : BindError(r.error());
    });

    // Speaker state getters
    CROW_ROUTE(app, "/activation-duration")([&](const crow::request& req)
    {
        return speaker->ActivationDuration(Urgent(req.url_params));
    });

    CROW_ROUTE(app, "/deactivation-duration")([&](const crow::request& req)
    {
        return speaker->DeactivationDuration(Urgent(req.url_params));
    });

    CROW_ROUTE(app, "/duration-left")([&]()
    {
        return std::to_string(speaker->DurationLeft());
    });

    CROW_ROUTE(app, "/working")([&]()
    {
        return std::to_string(speaker->Working());
    });

    CROW_LOG_INFO << "Created the web-server. Running it on the port " << port;
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
    if (error == speaker::AE_ChannelOpened) return crow::response { 400, "400 Channel Opened" };
    if (error == speaker::AE_ChannelClosed) return crow::response { 400, "400 Channel Closed" };
    if (error == speaker::AE_ChannelInactive) return crow::response { 400, "400 Channel Inactive" };
    if (error == speaker::AE_IncompatibleTrack) return crow::response { 400, "400 Incompatible Track" };
    if (error == speaker::AE_ChannelNotFound) return crow::response { 404, "404 Channel Not Found" };
    std::unreachable();
}

auto WebServer::BindState(speaker::ChannelState state) noexcept -> crow::response
{
    if (state == speaker::CS_Closed) return crow::response { "Closed" };
    if (state == speaker::CS_Opened) return crow::response { "Opened" };
    if (state == speaker::CS_Active) return crow::response { "Active" };
    if (state == speaker::CS_PendingTermination) return crow::response { "Pending Termination" };
    if (state == speaker::CS_PendingActivation) return crow::response { "Pending Activation" };
    if (state == speaker::CS_PendingDeactivation) return crow::response { "Pending Deactivation" };
    std::unreachable();
}

auto WebServer::Urgent(const crow::query_string& str) noexcept -> bool
{
    return str.get("urgent") != nullptr;
}