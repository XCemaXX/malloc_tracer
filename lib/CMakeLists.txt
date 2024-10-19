cmake_minimum_required(VERSION 3.0.0)
set(PROJECT_NAME malloc_tracer)
project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES C CXX)

add_library(${PROJECT_NAME} SHARED main.cpp)

set(CMAKE_C_FLAGS "-O2 -ggdb -Werror -Wall -Wextra -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "-O2 -ggdb -Werror -Wall -Wextra -fno-omit-frame-pointer")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(TURN_ON_MALLOC_COUNTERS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TURN_ON_MALLOC_COUNTERS=1)
endif()

if(DEBUG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
endif()

#cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
#cmake -B build -DCMAKE_BUILD_TYPE=Debug -DDEBUG=ON -DTURN_ON_MALLOC_COUNTERS=ON && cmake --build build