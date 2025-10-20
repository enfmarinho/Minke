ifndef EXE
	EXE = minke
	EXE_NOT_SET = true
endif
ifndef EVALFILE
	EVALFILE = src/minke.bin
endif

VERSION=3.0.0

BASE_BUILD_DIR = build

SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(SOURCES)))

CXX = g++
CXXSTD = -std=c++20
CXXWARNS = -Wall
CXXFLAGS = -O3 -funroll-loops -DNDEBUG -DEVALFILE=\"$(EVALFILE)\" $(CXXSTD) $(CXXWARNS)
CXXLINKERFLAGS = -fuse-ld=lld

ARCH_NATIVE = -march=native
ARCH_AVX2 = -DUSE_AVX2 -DUSE_SIMD -mavx2 -mbmi -mfma
ARCH_BMI2 = -DUSE_AVX2 -DUSE_SIMD -mavx2 -mbmi -mbmi2 -mfma
ARCH_AVX512 = -DUSE_AVX512 -DUSE_SIMD -mavx512f -mavx512bw -mfma
ARCH_APPLE_SILICON = -DUSE_NEON -DUSE_SIMD -march=armv8.5-a
ARCHS = native avx2 bmi2 avx512 apple-silicon

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
	endif
	CXXLINKERFLAGS += -pthread
	MKDIR = mkdir -p
	RMDIR = rm -rf
	RM = rm
endif

ifeq ($(PGO), 1)
	ifeq ($(CXX), g++)
		PGO_GEN = -fprofile-generate
		PGO_USE = -fprofile-use
	else ifeq ($(CXX), clang++)
		PGO_GEN = -fprofile-instr-generate
		PGO_MERGE = llvm-profdata merge -output=minke.profdata *.profraw
		PGO_USE = -fprofile-instr-use=minke.profdata
	else
		$(warning "Minke only officially support clang++ and g++")
	endif
define build
    $(eval EXE_PATH := $(EXE)$(if $(EXE_NOT_SET),-v$(VERSION))$(if $(EXE_NOT_SET),-$1)$(SUFFIX))
	$(CXX) $(CXXFLAGS) $(CXXLINKERFLAGS) $(PGO_GEN) $(SOURCES) -MMD -MP -o $(EXE_PATH)
	./$(EXE_PATH) bench
	$(PGO_MERGE)
	$(CXX) $(CXXFLAGS) $(CXXLINKERFLAGS) $(ARCH_$(2)) $(PGO_USE) $(SOURCES) -MMD -MP -o $(EXE_PATH)
	$(RM) -f *.d *.profraw *.profdata *.gcda
endef
else
	CXXLINKERFLAGS += -flto
define build
	$(MKDIR) $(BASE_BUILD_DIR)/$1
	$(MAKE) BUILD_DIR=$(BASE_BUILD_DIR)/$1 \
			ARCH_FLAGS="$(ARCH_$2)" \
			$(EXE)$(if $(EXE_NOT_SET),-v$(VERSION))$(if $(EXE_NOT_SET),-$1)$(SUFFIX)
endef
endif

all: native

native:
	$(call build,native,NATIVE)

avx2:
	$(call build,avx2,AVX2)

bmi2:
	$(call build,bmi2,BMI2)

avx512:
	$(call build,avx512,AVX512)

apple-silicon:
	$(call build,apple-silicon,APPLE_SILICON)

$(EXE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) $(CXXLINKERFLAGS) -o $@ $^

$(EXE)-%: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) $(CXXLINKERFLAGS) -o $@ $^

# Ensure init.o is rebuilt when EVALFILE changes
$(BUILD_DIR)/init.o: src/init.cpp $(EVALFILE) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) -c $< -o $@ -MMD -MP

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) -c $< -o $@ -MMD -MP

clean:
	$(RMDIR) $(BASE_BUILD_DIR)
	$(RM) -f $(foreach a,$(ARCHS),$(EXE)-v$(VERSION)-$(a)$(SUFFIX))

-include $(OBJECTS:.o=.d)
