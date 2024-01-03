#include "app/WebServer.h"

// POST {sink-id}/open
// POST {sink-id}/prolong
// POST {sink-id}/activate
// POST {sink-id}/deactivate
// POST {sink-id}/play + data
// POST {sink-id}/clear

// GET {sink-id}/state
// GET {sink-id}/duration-left
// GET {sink-id}/activation-duration
// GET {sink-id}/deactivation-duration

using namespace ml;
using namespace audio;
using namespace ml::amplifier;

auto main() -> int
{
    auto result = ml::app::WebServer::Run("./speaker.cfg", 8080);
    return result;
}
