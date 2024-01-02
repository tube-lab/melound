cmake_minimum_required(VERSION 3.27)
project(melound)

include_directories(include)
include_directories(lib)
set(CMAKE_CXX_STANDARD 23)

add_executable(melound src/main.cpp
        include/app/Config.h
        include/app/WebServer.h
        include/app/ConfigParser.h

        include/hardware/amplifier/Driver.h
        include/hardware/amplifier/Config.h
        include/hardware/amplifier/ActionError.h
        include/hardware/amplifier/LampDriver.h
        include/hardware/amplifier/LampConfig.h

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
        include/utils/TokenMiddleware.h

        #src/app/WebServer.cpp
        #src/app/ConfigParser.cpp

        src/hardware/amplifier/Driver.cpp
        src/hardware/amplifier/LampDriver.cpp

        src/hardware/relay/Driver.cpp

        src/hardware/audio/Track.cpp
        src/hardware/audio/TrackLoader.cpp
        src/hardware/audio/ChannelsMixer.cpp
        src/hardware/audio/Player.cpp
        src/hardware/audio/Utils.cpp

        #src/hardware/speaker/Driver.cpp

        src/utils/TokenMiddleware.cpp
)

# Add SDL2 library
find_package(SDL2 REQUIRED)
target_link_libraries(melound SDL2)
