cmake_minimum_required(VERSION 3.10)
project(
  Minke
  VERSION 4.0.0
  LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

# Sources files
file(GLOB SOURCES src/*.cpp)

add_executable(minke ${SOURCES})

string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type)
if(NOT CMAKE_BUILD_TYPE)
  set(build_type release)
elseif(build_type STREQUAL "datagen")
  target_compile_definitions(minke PRIVATE DATAGEN_BUILD)
  set(build_type release)
elseif(build_type STREQUAL "tune")
  target_compile_definitions(minke PRIVATE TUNE)
  set(build_type release)
endif()

if(build_type STREQUAL "debug")
  target_compile_options(
    minke
    PRIVATE -Wall
            -Wshadow
            -Wsign-compare
            -Wold-style-cast
            -Wpedantic
            -Wextra
            -Wcast-align
            -Wcast-qual
            -g)
elseif(build_type STREQUAL "release")
  target_compile_options(minke PRIVATE -O3 -funroll-loops -DNDEBUG)
  target_link_options(minke PRIVATE -fuse-ld=lld -flto)
endif()

if(WIN32)
  target_link_options(minke PRIVATE -static)
elseif(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(minke PRIVATE Threads::Threads)
endif()

# Define EVALFILE and make init.cpp recompiled when EVALFILE has changed
set(EVALFILE "${CMAKE_SOURCE_DIR}/beluga.nnue")
target_compile_definitions(minke PRIVATE EVALFILE=\"${EVALFILE}\")
set_property(
  SOURCE ${CMAKE_SOURCE_DIR}/src/init.cpp
  APPEND
  PROPERTY OBJECT_DEPENDS ${EVALFILE})

# Build variants
string(TOLOWER "${CMAKE_BUILD_ARCH}" arch)
if(arch STREQUAL "apple-silicon")
  target_compile_definitions(minke PRIVATE USE_SIMD USE_NEON)
  target_compile_options(minke PRIVATE -march=armv8.5-a -mcpu=apple-m1)
elseif(arch STREQUAL "avx2")
  target_compile_definitions(minke PRIVATE USE_SIMD USE_AVX2)
  target_compile_options(minke PRIVATE -mavx2 -mbmi -mfma)
elseif(arch STREQUAL "bmi2")
  target_compile_definitions(minke PRIVATE USE_AVX2 USE_SIMD)
  target_compile_options(minke PRIVATE -mavx2 -mbmi -mbmi2 -mfma)
elseif(arch STREQUAL "avx512")
  target_compile_definitions(minke PRIVATE USE_AVX512 USE_SIMD)
  target_compile_options(minke PRIVATE -mavx512f -mavx512bw -mfma)
else()
  target_compile_options(minke PRIVATE -march=native)
endif()
set_target_properties(minke PROPERTIES OUTPUT_NAME
                                       "minke-v${PROJECT_VERSION}-${arch}")
