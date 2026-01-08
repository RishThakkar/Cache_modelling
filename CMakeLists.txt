cmake_minimum_required(VERSION 3.16)

project(cache_modelling LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optional: helpful warnings
if (MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---- Library target ----
add_library(cache_model
  src/cache_model.cpp
  src/replacement_policy.cpp
)

target_include_directories(cache_model
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# If you later want this as a shared lib:
# add_library(cache_model SHARED ...)

# ---- Apps / executables ----

# Pick ONE "main" executable 
add_executable(run_experiments apps/run_experiments.cpp)
target_link_libraries(run_experiments PRIVATE cache_model)

