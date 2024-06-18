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
  "src/evaluate.cpp"
  "src/hash.cpp"
  "src/main.cpp"
  "src/movegen.cpp"
  "src/position.cpp"
  "src/search.cpp"
  "src/thread.cpp"
  "src/tt.cpp"
  "src/uci.cpp")
target_compile_features(minke PUBLIC cxx_std_20)
