project(PS2)

set(CMAKE_MINIMUM_REQUIRED_VERSION 3.16.3)

set(CXX_STANDARD c++23)

find_package(glfw3 REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})
link_libraries(${GLFW_LIBRARY_DIRS})

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})


set(SOURCES src/main.cpp
            src/app/Application.cpp
            src/emu/Bus.cpp
            src/emu/cpu/EmotionEngine.cpp
            src/emu/cpu/instructions.cpp
            src/emu/gs/gif.cpp
			src/emu/gs/gs.cpp
            src/emu/cpu/ee/timers.cpp
            src/emu/cpu/ee/dmac.cpp
            src/emu/cpu/ee/vu.cpp
            src/emu/cpu/ee/vif.cpp
            src/emu/sif.cpp
            src/emu/cpu/iop/iop.cpp
            src/emu/cpu/iop/opcodes.cpp
            src/emu/cpu/iop/dma.cpp
            src/emu/cpu/iop/timers.cpp
			src/emu/cpu/iop/sio2.cpp
			src/emu/cpu/cop0.cpp
			3party/gl3w/gl3w.c)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(ps2 ${SOURCES})
set(TARGET_NAME ps2)

target_link_libraries(${TARGET_NAME} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${EXTRA_LIBS} ${SDL2_LIBRARIES})

if(MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=address -pg)
  target_link_options(${TARGET_NAME} PRIVATE -fsanitize=address -pg)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
