#include "hardware/audio/core/TrackLoader.h"
#include "hardware/audio/core/Player.h"
#include "hardware/audio/mixer/Mixer.h"

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

auto FromFile(std::string path)
{
    std::ifstream ifs { path };
    std::vector<char> data = { std::istreambuf_iterator<char> { ifs }, std::istreambuf_iterator<char> {} };
    return TrackLoader::FromWav(data);
}

auto main() -> int
{
    std::ifstream ifs { "./melody.wav" };
    std::vector<char> data = { std::istreambuf_iterator<char> { ifs }, std::istreambuf_iterator<char> {} };

    auto track = *FromFile("sound.wav");

    auto player = Mixer::Create(2, Transition {}, track.Specs());
    auto f = player->Enqueue(1, *FromFile("melody.wav"));
    auto g = player->Enqueue(0, *FromFile("sound.wav"));

    player->Resume(0);
    player->Resume(1);

    f.wait();
    std::cout << "Played melody\n";

    g.wait();
    std::cout << "Played sound\n";

    return 0;
}
