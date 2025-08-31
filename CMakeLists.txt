cmake_minimum_required(VERSION 3.10)
project(Minke)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
elseif(CMAKE_BUILD_TYPE STREQUAL "Datagen")
  add_compile_options("-DDATAGEN_BUILD")
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

# Path to the neural net
set(NET_PATH "${CMAKE_SOURCE_DIR}/src/minke.bin")

# Make init.cpp recompiled when NET_PATH has changed
set_property(
  SOURCE ${CMAKE_SOURCE_DIR}/src/init.cpp
  APPEND
  PROPERTY OBJECT_DEPENDS ${NET_PATH})

# Define NET_PATH macro
add_compile_options("-DNET_PATH=\"${NET_PATH}\"")

# Add and link executable.
add_executable(
  minke
  "src/attacks.cpp"
  "src/history.cpp"
  "src/init.cpp"
  "src/main.cpp"
  "src/move.cpp"
  "src/movegen.cpp"
  "src/movepicker.cpp"
  "src/nnue.cpp"
  "src/position.cpp"
  "src/pv_list.cpp"
  "src/search.cpp"
  "src/time_manager.cpp"
  "src/tt.cpp"
  "src/uci.cpp")

target_link_libraries(minke)
