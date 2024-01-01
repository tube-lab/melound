#include "app/WebServer.h"

#include <fstream>
#include <iostream>

// POST {sink-id}/open
// POST {sink-id}/prolong
// POST {sink-id}/activate
// POST {sink-id}/deactivate
// POST {sink-id}/play + data
// POST {sink-id}/clear

// GET {sink-id}/duration-left
// GET {sink-id}/activation-duration
// GET {sink-id}/deactivation-duration

using namespace ml;
using namespace audio;

auto FromFile(const std::string& path)
{
    std::ifstream ifs { path };
    std::vector<char> data = { std::istreambuf_iterator<char> { ifs }, std::istreambuf_iterator<char> {} };
    return TrackLoader::FromWav(data);
}

auto main() -> int
{
    auto result = ml::app::WebServer::Run("./config.ini", 8080);
    return result;
}
