# Usage:
#   make [native|avx2|bmi2|avx512|apple-silicon]    # build for target arch (default: auto-detected)
#   make PGO=on bmi2                                # two-phase profile-guided build
#   make EVALFILE=net.nnue bmi2                     # use a custom network file

VERSION := 6.0.0
DEFAULT_EVALFILE := minke29

PGO ?= off

# Flags
CXXSTD := -std=c++20
CXXWARNS := -Wall
CXXFLAGS = -O3 -funroll-loops -flto=auto -DNDEBUG -DEVALFILE=\"$(NNUE_FILE)\" $(CXXSTD) $(CXXWARNS)
LDFLAGS := -flto=auto

# Arch flags
NATIVE_FLAGS := -march=native
AVX2_FLAGS := -DUSE_AVX2 -DUSE_SIMD -mavx2 -mbmi -mfma
BMI2_FLAGS := -DUSE_AVX2 -DUSE_SIMD -mavx2 -mbmi -mbmi2 -mfma
AVX512_FLAGS := -DUSE_AVX512 -DUSE_SIMD -mavx512f -mavx512bw -mfma
APPLESILICON_FLAGS := -DUSE_NEON -DUSE_SIMD -march=armv8.5-a

# Paths
BASE_BUILD_DIR := build
PGO_DIR := $(BASE_BUILD_DIR)/pgo
SRC_DIRS := src src/eval src/eval/nnue
SOURCES := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
OBJECTS := $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(SOURCES)))

ifndef EVALFILE
	EVALFILE := $(DEFAULT_EVALFILE)
	NNUE_FILE := $(EVALFILE).nnue
else
	NNUE_FILE := $(EVALFILE)
endif

ifndef EXE
	EXE := minke-v$(VERSION)
	EXE_NOT_SET := true
endif

ifeq ($(OS), Windows_NT)
	CXX ?= g++
	CXXFLAGS += -static
	SUFFIX := .exe
	MKDIR := cmd /C mkdir
	RMDIR := cmd /C rmdir /S /Q
	RM := del
else
	UNAME_S := $(shell uname -s)
	SUFFIX :=
	ifeq ($(UNAME_S), Darwin)
		CXX ?= clang++
		LDFLAGS := -flto
	endif
	CXX ?= g++
	LDFLAGS += -pthread
	MKDIR := mkdir -p
	RMDIR := rm -rf
	RM := rm
endif

ifeq ($(CXX), clang++)
	PGO_GEN_FLAGS := -fprofile-instr-generate
	PGO_USE_FLAGS := -fprofile-instr-use=$(PGO_DIR)/default.profdata
	PGO_MERGE_CMD := llvm-profdata merge -output=$(PGO_DIR)/default.profdata $(PGO_DIR)/instrumented.profraw
else
	PGO_GEN_FLAGS := -fprofile-generate -fprofile-update=atomic
	PGO_USE_FLAGS := -fprofile-use -fprofile-correction
	PGO_MERGE_CMD := true
endif

NATIVE_ARCH := $(shell echo | $(CXX) -march=native -E -dM -)
ifneq ($(findstring __AVX2__, $(NATIVE_ARCH)),)
	DEFAULT_TARGET := avx2
endif
ifneq ($(findstring __BMI2__, $(NATIVE_ARCH)),)
	DEFAULT_TARGET := bmi2
endif
ifneq ($(findstring __AVX512F__, $(NATIVE_ARCH)),)
	ifneq ($(findstring __AVX512BW__, $(NATIVE_ARCH)),)
		DEFAULT_TARGET := avx512
	endif
endif
ifndef DEFAULT_TARGET
	DEFAULT_TARGET := native
endif

ifneq ($(PGO),on)
define build
	$(MAKE) \
		ARCH_FLAGS="$($1_FLAGS)" \
		BUILD_DIR=$(BASE_BUILD_DIR)/$2 \
		EXE=$(EXE)$(if $(EXE_NOT_SET),-$2)$(SUFFIX) \
		build
endef
else
define build
	@echo "==> [PGO 1/3] instrumented build"
	$(RMDIR) $(PGO_DIR)
	$(MAKE) \
		ARCH_FLAGS="$($1_FLAGS)" \
		BUILD_DIR=$(PGO_DIR) \
		PROFILE_FLAGS="$(PGO_GEN_FLAGS)" \
		EXE=$(PGO_DIR)/instrumented$(SUFFIX) \
		build
	@echo "==> [PGO 2/3] collecting profile data"
	cd $(PGO_DIR) && LLVM_PROFILE_FILE=instrumented.profraw ./instrumented$(SUFFIX) bench 12
	$(PGO_MERGE_CMD)
	@echo "==> [PGO 3/3] Recompiling based on PGO profile"
	$(RM) -f $(PGO_DIR)/*.o $(PGO_DIR)/*.d
	$(MAKE) \
		ARCH_FLAGS="$($1_FLAGS)" \
		BUILD_DIR=$(PGO_DIR) \
		PROFILE_FLAGS="$(PGO_USE_FLAGS)" \
		EXE=$(EXE)$(if $(EXE_NOT_SET),-$2)$(SUFFIX) \
		build
	$(RMDIR) $(PGO_DIR)
endef
endif

.PHONY: all evalfile native avx2 bmi2 avx512 apple-silicon build clean
all: $(DEFAULT_TARGET)

evalfile:
	@if [ ! -f $(NNUE_FILE) ]; then \
		if [ "$(EVALFILE)" = "$(DEFAULT_EVALFILE)" ]; then \
			echo "Downloading $(NNUE_FILE)..."; \
			curl -sLO https://github.com/enfmarinho/MinkeNets/releases/download/$(DEFAULT_EVALFILE)/$(NNUE_FILE); \
		else \
			echo "Error: Network file '$(NNUE_FILE)' not found!"; \
			exit 1; \
		fi; \
	else \
		echo "Using network: $(NNUE_FILE)"; \
	fi

native:
	$(call build,NATIVE,native)

avx2:
	$(call build,AVX2,avx2)

bmi2:
	$(call build,BMI2,bmi2)

avx512:
	$(call build,AVX512,avx512)

apple-silicon:
	$(call build,APPLESILICON,apple-silicon)

build: evalfile $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) $(PROFILE_FLAGS) $(LDFLAGS) -o $(EXE) $(OBJECTS)

vpath %.cpp $(SRC_DIRS)

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) $(PROFILE_FLAGS) -c $< -o $@ -MMD -MP

$(BUILD_DIR):
	$(MKDIR) $(BUILD_DIR)

clean:
	$(RMDIR) $(BASE_BUILD_DIR)
	$(RM) -f $(EXE)*

-include $(OBJECTS:.o=.d)
