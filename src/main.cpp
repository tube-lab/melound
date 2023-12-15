#include "hardware/audio/AudioLoader.h"
#include "hardware/audio/AudioPlayer.h"

#include <fstream>
#include <iostream>

// POST open + mode
// POST close +
// POST play +
// POST stop
// GET opening-time?mode={mode}

using namespace ml;

auto FromFile(std::string path)
{
    std::ifstream ifs { path };
    std::vector<char> data = { std::istreambuf_iterator<char> { ifs }, std::istreambuf_iterator<char> {} };
    return AudioLoader::FromWav(data);
}

auto main() -> int
{
    std::ifstream ifs { "./sound.wav" };
    std::vector<char> data = { std::istreambuf_iterator<char> { ifs }, std::istreambuf_iterator<char> {} };

    auto track = *FromFile("sound.wav");

    auto player = AudioPlayer::Create(track.Specs());
    auto f = player->Enqueue(*FromFile("melody.wav"));
    auto g = player->Enqueue(*FromFile("sound.wav"));

    f.wait();
    std::cout << "Played melody\n";

    g.wait();
    std::cout << "Played sound\n";

    return 0;
}
