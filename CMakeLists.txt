cmake_minimum_required(VERSION 3.27)
project(melound)

include_directories(include)
include_directories(lib)
set(CMAKE_CXX_STANDARD 23)

add_executable(melound src/main.cpp
        include/hardware/speaker/SpeakerDriver.h
        include/hardware/speaker/SpeakerConfig.h
        include/hardware/switch/SwitchDriver.h

        include/hardware/audio/core/Track.h
        include/hardware/audio/core/TrackLoader.h
        include/hardware/audio/core/Player.h
        include/hardware/audio/core/Utils.h

        include/hardware/audio/mixer/ChannelsMixer.h

        src/hardware/switch/SwitchDriver.cpp

        src/hardware/audio/core/Track.cpp
        src/hardware/audio/core/TrackLoader.cpp
        src/hardware/audio/core/Player.cpp
        src/hardware/audio/core/Utils.cpp

        src/hardware/speaker/SpeakerDriver.cpp
        include/hardware/speaker/SpeakerSink.h
        include/utils/Time.h
        src/hardware/audio/mixer/ChannelsMixer.cpp
)

# Add SDL2 library
find_package(SDL2 REQUIRED)
target_link_libraries(melound SDL2)
