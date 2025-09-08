EXE = minke
NET_PATH = src/minke.bin
BASE_BUILD_DIR = build

SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(SOURCES)))

CXX = clang++
CXXSTD = -std=c++20
CXXWARNS = -Wall
CXXFLAGS = -O3 -funroll-loops -DNDEBUG -DNET_PATH=\"$(NET_PATH)\" $(CXXSTD) $(CXXWARNS)
CXXLINKERFLAGS = -flto -fuse-ld=lld

NATIVEFLAGS = -march=native
AVX2FLAGS = -DUSE_AVX2 -DUSE_SIMD -mavx2 -mbmi -mfma
BMI2FLAGS = -DUSE_AVX2 -DUSE_SIMD -mavx2 -mbmi -mbmi2 -mfma
AVX512FLAGS = -DUSE_AVX512 -DUSE_SIMD -mavx512f -mavx512bw -mfma
APPLESILICONFLAGS = -march=armv8.5-a -mcpu=apple-m1 -DUSE_SIMD -DUSE_APPLE_SILICON
ARCHS = native avx2 bmi2 avx512 apple-silicon 

ifeq ($(OS), Windows_NT)
	CXXFLAGS += -static
    EXE := $(EXE).exe
    MKDIR = cmd /C mkdir
    RMDIR = cmd /C rmdir /S /Q
    RM = del
else
    CXXLINKERFLAGS += -pthread
    MKDIR = mkdir -p
    RMDIR = rm -rf
    RM = rm
endif

all: $(ARCHS)

native:
	$(MAKE) BUILD_DIR=$(BASE_BUILD_DIR)/$@ ARCH_FLAGS="$(NATIVEFLAGS)" $(EXE)-$@

avx2:
	$(MAKE) BUILD_DIR=$(BASE_BUILD_DIR)/$@ ARCH_FLAGS="$(AVX2FLAGS)" $(EXE)-$@

bmi2:
	$(MAKE) BUILD_DIR=$(BASE_BUILD_DIR)/$@ ARCH_FLAGS="$(BMI2FLAGS)" $(EXE)-$@

avx512:
	$(MAKE) BUILD_DIR=$(BASE_BUILD_DIR)/$@ ARCH_FLAGS="$(AVX512FLAGS)" $(EXE)-$@

apple-silicon:
	$(MAKE) BUILD_DIR=$(BASE_BUILD_DIR)/$@ ARCH_FLAGS="$(APPLESILICONFLAGS)" $(EXE)-$@

$(EXE)-%: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) $(CXXLINKERFLAGS) -o $@ $^

# Ensure init.o is rebuilt when NET_PATH changes
$(BUILD_DIR)/init.o: src/init.cpp $(NET_PATH) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) -c $< -o $@ -MMD -MP

# Compile all .cpp files into build/<arch>/*.o
$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) -c $< -o $@ -MMD -MP

# Ensure build dir exists
$(BUILD_DIR):
	$(MKDIR) $(BUILD_DIR)

clean:
	$(RMDIR) $(BASE_BUILD_DIR)
	$(RM) -f $(foreach v,$(ARCHS),$(EXE)-$(v))

-include $(OBJECTS:.o=.d)
