cmake_minimum_required(VERSION 3.16)
project(csopesy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

# Every module under src/ except main.cpp goes into a core library so tests can link it.
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM CORE_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)

add_library(csopesy_core STATIC ${CORE_SOURCES})
target_include_directories(csopesy_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(csopesy_core PUBLIC Threads::Threads)

add_executable(csopesy src/main.cpp)
target_link_libraries(csopesy PRIVATE csopesy_core)

add_executable(csopesy_tests tests/ProcessTests.cpp)
target_link_libraries(csopesy_tests PRIVATE csopesy_core)

if(MINGW)
    # Static runtime so the exe runs outside the MSYS2 shell during the quiz.
    target_link_options(csopesy PRIVATE -static)
    target_link_options(csopesy_tests PRIVATE -static)
endif()

if(MSVC)
    target_compile_options(csopesy_core PRIVATE /W4)
else()
    target_compile_options(csopesy_core PRIVATE -Wall -Wextra)
endif()

# config.txt is read from the working directory; keep a copy next to the exe.
add_custom_command(TARGET csopesy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/config.txt $<TARGET_FILE_DIR:csopesy>/config.txt)
