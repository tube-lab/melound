// Created by Tube Lab. Part of the meloun project.
#include "app/WebServer.h"
using namespace ml::app;

auto WebServer::Run(const std::string& configPath) noexcept -> bool
{
    // Load the config
    std::ifstream stream { configPath };
    if (!stream)
    {
        std::cerr << "Can't open the config file.";
        return false;
    }

    auto config = ConfigParser::FromIni({ std::istreambuf_iterator<char> { stream }, std::istreambuf_iterator<char> {} });
    if (!config)
    {
        std::cerr << "Can't parse the config.";
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
        std::cerr << "Can't create the amplifier driver. Check audio-device and power-power validity.\n";
        return false;
    }

    // Create the speaker driver
    auto speaker = speaker::Driver::Create(speaker::Config {
        .Amplifier = amplifier,
        .Channels = config->Channels
    });

    if (!speaker)
    {
        std::cerr << "Can't create the speaker driver.\n";
        return false;
    }

    std::cout << "Connected to the audio device: " << config->AudioDevice.value_or("default") << '\n';
    std::cout << "Connected to the relay: " << config->PowerPort << '\n';

    // Create the server & the API
    // For docs refer to API.md
    httplib::Server app;
    app.new_task_queue = [] { return new httplib::ThreadPool(8096); };

    // Enable CORS
    app.set_cors(R"(.*)")
        .allow_credentials();

    // Create authorization by token and logging
    app.set_pre_routing_handler([&](const auto& req, auto& res)
    {
        std::cout << httplib::HttpMethod::to_string(req.method) << " request to " << req.path
                  << " from " << req.remote_addr << std::endl;

        if (req.get_header_value("Authorization") != config->Token)
        {
            res = Response(401, "401 Unauthorized");
            return httplib::Server::HandlerResponse::Handled;
        }

        return httplib::Server::HandlerResponse::Unhandled;
    });

    // Session management
    app.Post("/:channel/open", [&](const httplib::Request& req, httplib::Response& res)
    {
        auto r = speaker->Open(req.path_params.at("channel"));
        res = r ? Response(200, "Ok") : BindError(r.error());
    });

    app.Post("/:channel/prolong", [&](const httplib::Request& req, httplib::Response& res)
    {
        auto r = speaker->Prolong(req.path_params.at("channel"));
        res = r ? Response(200, "Ok") : BindError(r.error());
    });

    // Activate/deactivate channel
    app.Post("/:channel/activate", [&](const httplib::Request& req, httplib::Response& res)
    {
        auto r = speaker->Activate(req.path_params.at("channel"), req.has_param("urgently"));
        res = r ? LongPolling(r.value()) : BindError(r.error());
    });

    app.Post("/:channel/deactivate", [&](const httplib::Request& req, httplib::Response& res)
    {
        auto r = speaker->Deactivate(req.path_params.at("channel"), req.has_param("urgently"));
        res = r ? LongPolling(r.value()) : BindError(r.error());
    });

    // Playback management
    app.Post("/:channel/play", [&](const httplib::Request& req, httplib::Response& res)
    {
        std::string raw = req.body;
        std::vector<char> decoded { raw.begin(), raw.end() };

        auto track = audio::TrackLoader::FromWav(decoded);
        if (!track)
        {
            res = Response(400, "400 Track Not Wav");
            return;
        }

        auto r = speaker->Enqueue(req.path_params.at("channel"), *track);
        res = r ? LongPolling(r.value()) : BindError(r.error());
    });

    app.Post("/:channel/skip", [&](const httplib::Request& req, httplib::Response& res)
    {
        auto r = speaker->Skip(req.path_params.at("channel"));
        res = r ? Response(200, "Ok") : BindError(r.error());
    });

    app.Post("/:channel/clear", [&](const httplib::Request& req, httplib::Response& res)
    {
        auto r = speaker->Clear(req.path_params.at("channel"));
        res = r ? Response(200, "Ok") : BindError(r.error());
    });

    // Channel state getters
    app.Get("/:channel/state", [&](const httplib::Request& req, httplib::Response& res)
    {
        auto r = speaker->State(req.path_params.at("channel"));
        res = r ? BindState(r.value()) : BindError(r.error());
    });

    app.Get("/:channel/duration-left", [&](const httplib::Request& req, httplib::Response& res)
    {
        auto r = speaker->DurationLeft(req.path_params.at("channel"));
        res = r ? Response(200, std::to_string(r.value())) : BindError(r.error());
    });

    // Speaker state getters
    app.Get("/activation-duration", [&](const httplib::Request& req, httplib::Response& res)
    {
        res = Response(200, std::to_string(speaker->ActivationDuration(req.has_param("urgently"))));
    });

    app.Get("/deactivation-duration", [&](const httplib::Request& req, httplib::Response& res)
    {
        res = Response(200, std::to_string(speaker->DeactivationDuration(req.has_param("urgently"))));
    });

    app.Get("/duration-left", [&](const httplib::Request& req, httplib::Response& res)
    {
        res = Response(200, std::to_string(speaker->DurationLeft()));
    });

    app.Get("/working", [&](const httplib::Request& req, httplib::Response& res)
    {
        res = Response(200, std::to_string(speaker->Working()));
    });

    std::cout << "Created the web-server. Running it on the port " << config->Port << std::endl;
    app.listen("127.0.0.1", config->Port);

    return true;
}

auto WebServer::Response(int status, const std::string &text) noexcept -> httplib::Response 
{
    auto r = httplib::Response {};
    r.status = status;
    r.set_content(text, "text/plain");
    return r;
}

auto WebServer::LongPolling(const std::future<void>& f) noexcept -> httplib::Response
{
    f.wait();
    return Response(200, "Ok");
}

auto WebServer::BindError(speaker::ActionError error) noexcept -> httplib::Response
{
    if (error == speaker::AE_ChannelOpened) return Response(400, "400 Channel Opened");
    if (error == speaker::AE_ChannelClosed) return Response(400, "400 Channel Closed");
    if (error == speaker::AE_ChannelInactive) return Response(400, "400 Channel Inactive");
    if (error == speaker::AE_IncompatibleTrack) return Response(400, "400 Incompatible Track");
    if (error == speaker::AE_ChannelNotFound) return Response(404, "404 Channel Not Found");
    std::unreachable();
}

auto WebServer::BindState(speaker::ChannelState state) noexcept -> httplib::Response
{
    if (state == speaker::CS_Closed) return Response(200, "Closed");
    if (state == speaker::CS_Opened) return Response(200, "Opened");
    if (state == speaker::CS_Active) return Response(200, "Active");
    if (state == speaker::CS_PendingTermination) return Response(200, "Pending Termination");
    if (state == speaker::CS_PendingActivation) return Response(200, "Pending Activation");
    if (state == speaker::CS_PendingDeactivation) return Response(200, "Pending Deactivation");
    std::unreachable();
}
