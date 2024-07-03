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
    "-Wshadow"
    "-Wsign-compare"
    "-Wold-style-cast"
    "-Wcast-align"
    "-Wcast-qual"
    "-g")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options("-O3" "-march=native" "-DNDEBUG")
endif()

# Add and link executable.
add_executable(
  minke
  "src/evaluate.cpp"
  "src/game_state.cpp"
  "src/hash.cpp"
  "src/main.cpp"
  "src/movegen.cpp"
  "src/nnue.cpp"
  "src/position.cpp"
  "src/search.cpp"
  "src/thread.cpp"
  "src/tt.cpp"
  "src/uci.cpp")
target_link_libraries(minke)
