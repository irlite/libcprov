cmake_minimum_required(VERSION 3.16)
project(libcprov)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
# -------- Prov Executable --------
add_executable(prov
    src/prov.cpp
    src/parser.cpp
    src/processor.cpp
    src/json_string_builders.cpp
    src/get_job_info.cpp
    src/process_coordinator.cpp
    src/ingester.cpp
    src/xxhash.c
    src/sqlite3.c
)

target_include_directories(prov PRIVATE
    ${PROJECT_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/config
  )

target_link_libraries(prov PRIVATE
    CURL::libcurl
    pthread
    dl
    simdjson::simdjson
    config_parser
)

# -------- Injector Shared Library --------
add_library(injector SHARED
    src/injector.cpp
)

target_link_libraries(injector PRIVATE
    pthread
    dl
)
