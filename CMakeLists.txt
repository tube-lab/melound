cmake_minimum_required(VERSION 3.27)
project(melound)

include_directories(include)
include_directories(lib)
set(CMAKE_CXX_STANDARD 23)

add_executable(melound src/main.cpp
        include/hardware/bell/BellDriver.h
        include/hardware/bell/BellConfig.h
        include/hardware/switch/SwitchDriver.h

        include/hardware/audio/Audio.h
        include/hardware/audio/AudioLoader.h
        include/hardware/audio/AudioPlayer.h

        src/hardware/switch/SwitchDriver.cpp
        src/hardware/audio/Audio.cpp
        src/hardware/audio/AudioLoader.cpp
        src/hardware/audio/AudioPlayer.cpp
        src/hardware/bell/BellDriver.cpp
        include/utils/Mask.h
        include/hardware/bell/BellSink.h
)

# Add SDL2 library
find_package(SDL2 REQUIRED)
target_link_libraries(melound SDL2)
