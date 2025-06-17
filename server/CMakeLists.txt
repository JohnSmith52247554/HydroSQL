cmake_minimum_required(VERSION 3.17)
set(CMAKE_VERBOSE_MAKEFILE ON)

project(HydroSQLServer VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include_directories(
${CMAKE_SOURCE_DIR}/engine/include
${CMAKE_SOURCE_DIR}/network/include
${CMAKE_SOURCE_DIR}
)
file(GLOB_RECURSE SRC 
"engine/src/*.cpp" 
"network/src/*.cpp" 
"./main.cpp")

add_executable(${PROJECT_NAME} ${SRC})
target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/pch.hpp)

if (MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE OFF)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/release
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/debug)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
PROJECT_PATH="${CMAKE_SOURCE_DIR}"
DATA_PATH="${CMAKE_SOURCE_DIR}/data"
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()