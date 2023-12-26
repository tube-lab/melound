cmake_minimum_required(VERSION 3.27)
project(melound)

include_directories(include)
include_directories(lib)
set(CMAKE_CXX_STANDARD 23)

add_executable(melound src/main.cpp
        include/hardware/amplifier/Config.h
        include/hardware/amplifier/Driver.h
        include/hardware/amplifier/DriverError.h
        include/hardware/amplifier/ChannelState.h

        include/hardware/relay/Driver.h

        include/hardware/audio/Track.h
        include/hardware/audio/TrackLoader.h
        include/hardware/audio/Player.h
        include/hardware/audio/Utils.h
        include/hardware/audio/ChannelsMixer.h

        #include/hardware/speaker/Config.h
        #include/hardware/speaker/Driver.h
        #include/hardware/speaker/DriverError.h

        include/utils/Time.h

        src/hardware/relay/Driver.cpp

        src/hardware/audio/Track.cpp
        src/hardware/audio/TrackLoader.cpp
        src/hardware/audio/ChannelsMixer.cpp
        src/hardware/audio/Player.cpp
        src/hardware/audio/Utils.cpp

        #src/hardware/speaker/Driver.cpp

        include/utils/CustomConstructor.h
        src/hardware/amplifier/Driver.cpp
)

# Add SDL2 library
find_package(SDL2 REQUIRED)
target_link_libraries(melound SDL2)
