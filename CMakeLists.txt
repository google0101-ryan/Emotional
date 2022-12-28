project(PS2)

set(CMAKE_MINIMUM_REQUIRED_VERSION 3.16.3)

set(SOURCES src/main.cpp
            src/app/Application.cpp
            src/emu/Bus.cpp
            src/emu/cpu/EmotionEngine.cpp
            src/emu/cpu/instructions.cpp
            src/emu/gs/gif.cpp
            src/emu/cpu/ee/timers.cpp
            src/emu/cpu/ee/dmac.cpp
            src/emu/cpu/ee/vu.cpp
            src/emu/cpu/ee/vif.cpp
            src/emu/sif.cpp
            src/emu/cpu/iop/iop.cpp
            src/emu/cpu/iop/opcodes.cpp
            src/emu/cpu/iop/dma.cpp
            src/emu/cpu/iop/timers.cpp
	    src/emu/cpu/cop0.cpp)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(ps2 ${SOURCES})
set(TARGET_NAME ps2)

if(MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/boost)
