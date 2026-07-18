cmake_minimum_required(VERSION 3.10)
project(ApeDynasty)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB_RECURSE SRC_FILES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SRC_FILES})

find_package(SFML 2.5 COMPONENTS graphics window system network audio REQUIRED)
target_link_libraries(${PROJECT_NAME} sfml-graphics sfml-window sfml-system sfml-network sfml-audio)