cmake_minimum_required(VERSION 3.10)
project(Minke)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 20)

# Sources files
file(GLOB SOURCES src/*.cpp)

# Common flags
set(NET_PATH "${CMAKE_SOURCE_DIR}/src/minke.bin")
set(COMMON_COMPILE_FLAGS "-DNET_PATH=\"${NET_PATH}\"")
set(COMMON_LINK_FLAGS "")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
elseif(CMAKE_BUILD_TYPE STREQUAL "Datagen")
  set(COMMON_COMPILE_FLAGS ${COMMON_COMPILE_FLAGS} "-DDATAGEN_BUILD")
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(COMMON_COMPILE_FLAGS
      ${COMMON_COMPILE_FLAGS}
      "-Wall"
      "-Wshadow"
      "-Wsign-compare"
      "-Wold-style-cast"
      "-Wpedantic"
      "-Wextra"
      "-Wcast-align"
      "-Wcast-qual"
      "-g")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(COMMON_COMPILE_FLAGS ${COMMON_COMPILE_FLAGS} "-O3" "-funroll-loops"
                           "-DNDEBUG")
  set(COMMON_LINK_FLAGS ${COMMON_COMPILE_FLAGS} "-fuse-ld=lld" "-flto")
endif()

if(WIN32)
  set(COMMON_COMPILE_FLAGS ${COMMON_COMPILE_FLAGS} "-static")
elseif(UNIX)
  set(COMMON_LINK_FLAGS ${COMMON_LINK_FLAGS} "-pthread")
endif()

# Make init.cpp recompiled when NET_PATH has changed
set_property(
  SOURCE ${CMAKE_SOURCE_DIR}/src/init.cpp
  APPEND
  PROPERTY OBJECT_DEPENDS ${NET_PATH})

function(add_minke_variant name flags)
  add_executable(minke-${name} ${SOURCES})
  target_compile_options(minke-${name} PRIVATE ${COMMON_COMPILE_FLAGS} ${flags})
  target_link_options(minke-${name} PRIVATE ${COMMON_LINK_FLAGS})
endfunction()

if(APPLE)
  add_minke_variant(
    apple-silicon
    "-march=armv8.5-a;-mcpu=apple-m1;-DUSE_SIMD;-DUSE_APPLE_SILICON")
else()
  add_minke_variant(native "-march=native")
  add_minke_variant(avx2 "-DUSE_AVX2;-DUSE_SIMD;-mavx2;-mbmi;-mfma")
  add_minke_variant(bmi2 "-DUSE_AVX2;-DUSE_SIMD;-mavx2;-mbmi;-mbmi2;-mfma")
  add_minke_variant(avx512 "-DUSE_AVX512;-DUSE_SIMD;-mavx512f;-mavx512bw;-mfma")
endif()
