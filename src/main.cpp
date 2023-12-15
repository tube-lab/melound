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

auto main() -> int
{
    std::ifstream ifs { "./sound.wav" };
    std::vector<char> data = { std::istreambuf_iterator<char> { ifs }, std::istreambuf_iterator<char> {} };

    auto track = *AudioLoader::FromWav(data);

    auto player = AudioPlayer::Create(track.Specs());
    auto f = player->Enqueue(track);

    f.wait();
    std::cout << "Played all\n";

    return 0;
}
