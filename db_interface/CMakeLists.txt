cmake_minimum_required(VERSION 3.16)
project(db_interface LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(simdjson REQUIRED)

add_library(common
    src/common/common.cpp
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/config
)

target_link_libraries(common PUBLIC
    config_parser
)

add_library(querier
    src/querier/querier.cpp
    src/querier/querier_build_json.cpp
    src/querier/querier_build_output_strings.cpp
    src/querier/querier_parser.cpp
)

target_include_directories(querier PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/config
)

target_link_libraries(querier PUBLIC
    common
    CURL::libcurl
    simdjson::simdjson
    config_parser
)

add_library(retriever
    src/retriever/retriever.cpp
    src/retriever/retriever_build_json.cpp
    src/retriever/retriever_parser.cpp
    src/retriever/retriever_retrieve_data.cpp
)

target_include_directories(retriever PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/config
)

target_link_libraries(retriever PUBLIC
    common
    CURL::libcurl
    simdjson::simdjson
    config_parser
)

add_executable(db_interface
    src/main.cpp
)

target_include_directories(db_interface PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/config
)

target_link_libraries(db_interface PRIVATE
    common
    querier
    retriever
)
