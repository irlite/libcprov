cmake_minimum_required(VERSION 3.16)
project(config)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CONFIG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_library(config_parser STATIC
    config_parser.cpp
)

target_include_directories(config_parser PUBLIC
    ${CONFIG_INCLUDE_DIR}
)

target_link_libraries(config_parser PUBLIC
    simdjson::simdjson
)
