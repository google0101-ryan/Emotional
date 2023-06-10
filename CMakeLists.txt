project(PS2)

set(CMAKE_MINIMUM_REQUIRED_VERSION 3.16.3)

set(CXX_STANDARD c++23)

set(SOURCES src/main.cpp
            src/app/Application.cpp
            src/emu/memory/Bus.cpp
            src/emu/System.cpp
			src/emu/cpu/ee/EmotionEngine.cpp
			src/emu/cpu/ee/dmac.cpp
			src/emu/cpu/ee/vu.cpp
			src/emu/cpu/ee/vif.cpp
			src/emu/cpu/ee/x64/reg_alloc.cpp
			src/emu/cpu/ee/x64/emit.cpp
			src/emu/cpu/iop/cpu.cpp
			src/emu/cpu/iop/opcodes.cpp
			src/emu/cpu/iop/dma.cpp
			src/emu/sched/scheduler.cpp
			src/emu/gpu/gif.cpp
			src/emu/gpu/gs.cpp
			src/emu/dev/sif.cpp
			src/emu/dev/cdvd.cpp
			src/emu/dev/sio2.cpp
			src/emu/cpu/ee/ee_interpret.cpp
			src/emu/cpu/ee/ee_opcode.cpp)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(ps2 ${SOURCES})
set(TARGET_NAME ps2)

find_package(SDL2 REQUIRED)
include_directories(ps2 ${SDL2_INCLUDE_DIRS})

target_link_libraries(ps2 ${SDL2_LIBRARIES})

if(MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${TARGET_NAME} PRIVATE -pg -O0 -mincoming-stack-boundary=3)
  target_link_options(${TARGET_NAME} PRIVATE -pg)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
