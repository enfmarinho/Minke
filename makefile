ifndef EXE
	EXE = minke
	EXE_NOT_SET = true
endif
ifndef EVALFILE
	EVALFILE = src/minke.bin
endif

BASE_BUILD_DIR = build

SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(SOURCES)))

CXX = g++
CXXSTD = -std=c++20
CXXWARNS = -Wall
CXXFLAGS = -O3 -funroll-loops -DNDEBUG -DEVALFILE=\"$(EVALFILE)\" $(CXXSTD) $(CXXWARNS)
CXXLINKERFLAGS = -flto -fuse-ld=lld

NATIVEFLAGS = -march=native
AVX2FLAGS = -DUSE_AVX2 -DUSE_SIMD -mavx2 -mbmi -mfma
BMI2FLAGS = -DUSE_AVX2 -DUSE_SIMD -mavx2 -mbmi -mbmi2 -mfma
AVX512FLAGS = -DUSE_AVX512 -DUSE_SIMD -mavx512f -mavx512bw -mfma
APPLESILICONFLAGS = -march=armv8.5-a -mcpu=apple-m1 -DUSE_APPLE_SILICON
ARCHS = native avx2 bmi2 avx512

ifeq ($(OS), Windows_NT)
	CXXFLAGS += -static
	SUFFIX := .exe
	MKDIR = cmd /C mkdir
	RMDIR = cmd /C rmdir /S /Q
	RM = del
else
	UNAME_S := $(shell uname -s)
	SUFFIX :=
	ifeq ($(UNAME_S), Darwin)
		CXX = clang++
		ARCHS = apple-silicon
	endif
	CXXLINKERFLAGS += -pthread
	MKDIR = mkdir -p
	RMDIR = rm -rf
	RM = rm
endif

ifeq ($(CXX), clang++)
	CXXLINKERFLAGS += -fuse-ld=lld
endif

all: $(ARCHS)

native:
	$(MAKE) BUILD_DIR=$(BASE_BUILD_DIR)/$@ ARCH_FLAGS="$(NATIVEFLAGS)" \
		$(EXE)$(if $(EXE_NOT_SET),-$@)$(SUFFIX)

avx2:
	$(MAKE) BUILD_DIR=$(BASE_BUILD_DIR)/$@ ARCH_FLAGS="$(AVX2FLAGS)" \
		$(EXE)$(if $(EXE_NOT_SET),-$@)$(SUFFIX)

bmi2:
	$(MAKE) BUILD_DIR=$(BASE_BUILD_DIR)/$@ ARCH_FLAGS="$(BMI2FLAGS)" \
		$(EXE)$(if $(EXE_NOT_SET),-$@)$(SUFFIX)

avx512:
	$(MAKE) BUILD_DIR=$(BASE_BUILD_DIR)/$@ ARCH_FLAGS="$(AVX512FLAGS)" \
		$(EXE)$(if $(EXE_NOT_SET),-$@)$(SUFFIX)

apple-silicon:
	$(MAKE) BUILD_DIR=$(BASE_BUILD_DIR)/$@ ARCH_FLAGS="$(APPLESILICONFLAGS)" \
		$(EXE)$(if $(EXE_NOT_SET),-$@)$(SUFFIX)

$(EXE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) $(CXXLINKERFLAGS) -o $@ $^

$(EXE)-%: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) $(CXXLINKERFLAGS) -o $@ $^

# Ensure init.o is rebuilt when EVALFILE changes
$(BUILD_DIR)/init.o: src/init.cpp $(EVALFILE) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) -c $< -o $@ -MMD -MP

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) -c $< -o $@ -MMD -MP

# Ensure build dir exists
$(BUILD_DIR):
	$(MKDIR) $(BUILD_DIR)

clean:
	$(RMDIR) $(BASE_BUILD_DIR)
	$(RM) -f $(foreach v,$(ARCHS),$(EXE)-$(v)) apple-silicon

-include $(OBJECTS:.o=.d)
