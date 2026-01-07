cmake_minimum_required(VERSION 3.16)
project(CacheModel CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cache_sim
    main.cpp
    cache_model.cpp
)

target_include_directories(cache_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
