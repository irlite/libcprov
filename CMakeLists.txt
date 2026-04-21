cmake_minimum_required(VERSION 3.16)
project(libcprov3)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ------------------ Fetch Dependencies ------------------
# simdjson (build from source, pinned to specific commit)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG e6c6db64937113029c3d6498f7744a8f1a72d8b8
)
FetchContent_MakeAvailable(simdjson)

# curl (library only, pinned to specific commit)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
set(CURL_USE_NGHTTP2 OFF CACHE BOOL "" FORCE)
set(CURL_USE_BROTLI OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBIDN2 OFF CACHE BOOL "" FORCE)
set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
set(CURL_USE_SSL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG c6ac2de5b38d065e2ff364a378c2cf8fc150bf90
)
FetchContent_MakeAvailable(curl)

# ------------------ Subdirectories ------------------
add_subdirectory(config)
add_subdirectory(receiver)
add_subdirectory(injector)
add_subdirectory(visualizer)
add_subdirectory(db_interface)
