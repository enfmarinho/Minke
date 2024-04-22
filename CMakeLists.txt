cmake_minimum_required(VERSION 3.5)
project(
  Minke
  VERSION 0.0.1
  LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# Append to existing flags.
add_compile_options(
  "-Wall"
  "-Wpedantic"
  "-Wshadow"
  "-Wsign-compare"
  "-Wold-style-cast"
  "-Wcast-align"
  "-Wcast-qual")
# add_compile_options("-g") # Flag to produce debug info.

# Add and link executable.
add_executable(
  minke
  "src/engine.cpp"
  "src/evaluation.cpp"
  "src/hashing.cpp"
  "src/main.cpp"
  "src/move_generation.cpp"
  "src/position.cpp"
  "src/thread.cpp"
  "src/transposition_table.cpp"
  "src/uci.cpp")
target_compile_features(minke PUBLIC cxx_std_17)
