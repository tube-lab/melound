#include "hardware/amplifier/Driver.h"
#include "hardware/audio/TrackLoader.h"

#include <fstream>
#include <iostream>

// POST {sink-id}/open
// POST {sink-id}/prolong
// POST {sink-id}/grab
// POST {sink-id}/release
// POST {sink-id}/play + data
// POST {sink-id}/stop

// GET {sink-id}/buffer-duration
// GET {sink-id}/grabbing-time
// GET {sink-id}/timeout

using namespace ml;
using namespace audio;
using namespace amplifier;
using namespace std::chrono;

auto FromFile(const std::string& path)
{
    std::ifstream ifs { path };
    std::vector<char> data = { std::istreambuf_iterator<char> { ifs }, std::istreambuf_iterator<char> {} };
    return TrackLoader::FromWav(data);
}

void wtf()
{

}

void test()
{
    return wtf();
}

auto main() -> int
{
    auto driver = Driver::Create(Config {
        .WarmingDuration = 5000,
        .CoolingDuration = 1000,
        .PowerControlPort = "/usb/",
        .AudioDevice = std::nullopt,
        .Channels = 2,
    });

    std::cout << "Start\n";

    for (int i = 0; i < driver->Channels(); ++i)
    {
        auto p = driver->Activate(i, false);
        p.wait();
        std::cout << "Channel " << i << " has been enabled\n";
    }

    auto op = driver->Enqueue(1, *FromFile("./sound.wav"));
    if (!op)
    {
        std::cerr << "Failed to enqueue: " << op.error() << '\n';
        return 1;
    }

    driver->Resume(1);
    op->wait();

    std::cout << "Played music\n";
    return 0;
}
