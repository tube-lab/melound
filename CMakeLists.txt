cmake_minimum_required(VERSION 3.27)
project(melound)

include_directories(include)
include_directories(lib)
set(CMAKE_CXX_STANDARD 23)

add_executable(melound src/main.cpp
        include/hardware/relay/Driver.h

        include/hardware/audio/Track.h
        include/hardware/audio/TrackLoader.h
        include/hardware/audio/Player.h
        include/hardware/audio/Utils.h
        include/hardware/audio/ChannelsMixer.h

        include/hardware/speaker/Driver.h
        include/hardware/speaker/ActionError.h

        include/utils/Time.h
        include/utils/CustomConstructor.h

        src/hardware/relay/Driver.cpp

        src/hardware/audio/Track.cpp
        src/hardware/audio/TrackLoader.cpp
        src/hardware/audio/ChannelsMixer.cpp
        src/hardware/audio/Player.cpp
        src/hardware/audio/Utils.cpp
        include/hardware/amplifier/Driver.h
        include/hardware/amplifier/ActionError.h
        src/hardware/amplifier/Driver.cpp
        include/hardware/amplifier/LampDriver.h
        src/hardware/speaker/Driver.cpp
        src/hardware/amplifier/LampDriver.cpp
        include/app/Config.h
        include/app/WebServer.h
        src/app/WebServer.cpp
        include/app/ConfigParser.h
        src/app/ConfigParser.cpp

        include/hardware/amplifier/LampConfig.h
        include/utils/TokenMiddleware.h
        src/utils/TokenMiddleware.cpp
)

# Add SDL2 library
find_package(SDL2 REQUIRED)
target_link_libraries(melound SDL2)
