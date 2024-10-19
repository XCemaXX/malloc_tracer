cmake_minimum_required(VERSION 3.0.0)

project(project_root VERSION 1.0.0 LANGUAGES C CXX)

option(TURN_ON_MALLOC_COUNTERS "Enable malloc counters in malloc_tracer" OFF)
option(DEBUG "Enable print debug info in malloc_tracer" OFF)
option(BUILD_HELLO_WORLD "Build hello_world example" OFF)

add_subdirectory(lib)

if(BUILD_HELLO_WORLD)
    add_subdirectory(example)
    install(TARGETS hello_world
        RUNTIME DESTINATION .
    )
endif()

install(TARGETS malloc_tracer
    RUNTIME DESTINATION .
    LIBRARY DESTINATION .
    ARCHIVE DESTINATION .
)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "Cleaning the entire build directory."
)

#cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
#cmake -B build -DCMAKE_BUILD_TYPE=Release -DDEBUG=ON -DTURN_ON_MALLOC_COUNTERS=ON -DBUILD_HELLO_WORLD=ON && cmake --build build
#cmake -B build -DCMAKE_BUILD_TYPE=Debug -DDEBUG=ON -DBUILD_HELLO_WORLD=ON && cmake --build build
#cmake --install build --prefix ./output
#cmake --build build --target clean-all