project(PS2)

set(CMAKE_MINIMUM_REQUIRED_VERSION 3.16.3)

set(SOURCES src/main.cpp
            src/app/Application.cpp
            src/emu/Bus.cpp
            src/emu/cpu/EmotionEngine.cpp
            src/emu/cpu/instructions.cpp)

set(CMAKE_BUILD_TYPE DEBUG)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(ps2 ${SOURCES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/boost)