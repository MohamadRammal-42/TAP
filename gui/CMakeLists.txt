cmake_minimum_required(VERSION 3.16)
project(RPGClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(RPGClient
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(RPGClient PRIVATE Qt6::Widgets Qt6::Network)