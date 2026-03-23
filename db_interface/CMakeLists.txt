cmake_minimum_required(VERSION 3.16)
project(db_interface LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(db_interface
    src/querier.cpp
    src/parser.cpp
    src/build_json.cpp
    src/build_output_strings.cpp
)

target_include_directories(db_interface PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/config
)

target_link_libraries(db_interface PRIVATE
    CURL::libcurl
    simdjson::simdjson
    config_parser
)

