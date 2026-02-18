cmake_minimum_required(VERSION 3.10)
project(ComputationalNodeSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(simulator_example
    src/main.cpp
    src/Simulator.cpp
    src/EventQueue.cpp
    src/Distribution.cpp
    src/RandomGenerator.cpp
)