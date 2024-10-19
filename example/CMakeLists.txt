cmake_minimum_required(VERSION 3.0.0)
set(PROJECT_NAME hello_world)
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES C CXX)

add_executable(${PROJECT_NAME} main.cpp)

set(CMAKE_C_FLAGS "-O2 -ggdb -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "-O2 -ggdb -fno-omit-frame-pointer")

#cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build