cmake_minimum_required(VERSION 3.10)
project(altitude_hold)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)         
find_package(MAVSDK REQUIRED)           

add_executable(drone_project           
    src/main.cpp
    src/controller.cpp
    src/drone_control.cpp
    src/height_controller.cpp
)

target_include_directories(drone_project PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(drone_project
    MAVSDK::mavsdk
    pthread
)