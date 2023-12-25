cmake_minimum_required(VERSION 3.27)
project(melound)

include_directories(include)
include_directories(lib)
set(CMAKE_CXX_STANDARD 23)

add_executable(melound src/main.cpp
        include/hardware/speaker/Driver.h
        include/hardware/speaker/Config.h
        include/hardware/relay/Driver.h

        include/hardware/audio/Track.h
        include/hardware/audio/TrackLoader.h
        include/hardware/audio/Player.h
        include/hardware/audio/Utils.h
        include/hardware/audio/ChannelsMixer.h

        include/hardware/amplifier/Config.h
        include/hardware/amplifier/Driver.h

        src/hardware/relay/Driver.cpp

        src/hardware/audio/Track.cpp
        src/hardware/audio/TrackLoader.cpp
        src/hardware/audio/Player.cpp
        src/hardware/audio/Utils.cpp

        src/hardware/speaker/SpeakerDriver.cpp
        include/hardware/speaker/Sink.h
        include/utils/Time.h
        src/hardware/audio/ChannelsMixer.cpp
        src/hardware/amplifier/Driver.cpp
        include/hardware/amplifier/DriverErrors.h
)

# Add SDL2 library
find_package(SDL2 REQUIRED)
target_link_libraries(melound SDL2)
