cmake_minimum_required(VERSION 3.5)
project(Minke)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options("-pthread")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(
    "-Wall"
    "-Wpedantic"
    "-Wsign-compare"
    "-Wold-style-cast"
    "-Wcast-align"
    "-Wcast-qual"
    "-fsanitize=undefined"
    "-fsanitize=address"
    "-g")
  add_link_options(-fsanitize=address)
  add_link_options(-fsanitize=undefined)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options("-O3" "-march=native" "-DNDEBUG")
endif()

# Add and link executable.
add_executable(
  minke
  "src/hash.cpp"
  "src/main.cpp"
  "src/move.cpp"
  "src/movegen.cpp"
  "src/nnue.cpp"
  "src/position.cpp"
  "src/pv_list.cpp"
  "src/search.cpp"
  "src/time_manager.cpp"
  "src/tt.cpp"
  "src/uci.cpp"
  "src/utils.cpp")

target_link_libraries(minke)
