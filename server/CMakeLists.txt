cmake_minimum_required(VERSION 3.17)
set(CMAKE_VERBOSE_MAKEFILE ON)

project(HydroSQLServer VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

file(GLOB_RECURSE LIB_FILE 
engine/src/*.cpp 
engine/include/*.Hpp
)

add_library(HydroSQLEngine SHARED ${LIB_FILE})
target_compile_definitions(HydroSQLEngine PRIVATE HYDROSQL_ENGINE_EXPORTS)
target_precompile_headers(HydroSQLEngine PRIVATE ${CMAKE_SOURCE_DIR}/pch.hpp)
target_include_directories(HydroSQLEngine PUBLIC 
    ${CMAKE_SOURCE_DIR}/engine/include
)
set_target_properties(HydroSQLEngine PROPERTIES
ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/lib/debug
LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/debug
RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/debug

ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/lib/release
LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/release
RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/release)

file(GLOB_RECURSE SRC 
"network/src/*.cpp"
"parser/src/*.cpp" 
"authority/src/*.cpp"
"./main.cpp")

add_executable(${PROJECT_NAME} ${SRC})
target_precompile_headers(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/pch.hpp)
target_include_directories(${PROJECT_NAME} PRIVATE
${CMAKE_SOURCE_DIR}/engine/include
${CMAKE_SOURCE_DIR}/parser/include
${CMAKE_SOURCE_DIR}/network/include
${CMAKE_SOURCE_DIR}/authority/include
${CMAKE_SOURCE_DIR}/..
${CMAKE_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE HydroSQLEngine)

if (MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE OFF)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES 
RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/debug
RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/release)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
PROJECT_PATH="${CMAKE_SOURCE_DIR}"
DATA_PATH="${CMAKE_SOURCE_DIR}/data"
USER_CONFIG_PATH="${CMAKE_SOURCE_DIR}/UserConfig.bin"
)
target_compile_definitions(HydroSQLEngine PRIVATE 
PROJECT_PATH="${CMAKE_SOURCE_DIR}"
DATA_PATH="${CMAKE_SOURCE_DIR}/data"
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()