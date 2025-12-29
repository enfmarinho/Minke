VERSION := 4.0.0

DEFAULT_EVALFILE := beluga
ifndef EVALFILE
	EVALFILE := $(DEFAULT_EVALFILE)
	NNUE_FILE := $(EVALFILE).nnue
else
	NNUE_FILE := $(EVALFILE)
endif


BASE_BUILD_DIR := build

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(SOURCES)))

CXXSTD := -std=c++20
CXXWARNS := -Wall
CXXFLAGS = -O3 -funroll-loops -DNDEBUG -DEVALFILE=\"$(NNUE_FILE)\" $(CXXSTD) $(CXXWARNS)
LDFLAGS := -flto -fuse-ld=lld

NATIVE_FLAGS := -march=native
AVX2_FLAGS := -DUSE_AVX2 -DUSE_SIMD -mavx2 -mbmi -mfma
BMI2_FLAGS := -DUSE_AVX2 -DUSE_SIMD -mavx2 -mbmi -mbmi2 -mfma
AVX512_FLAGS := -DUSE_AVX512 -DUSE_SIMD -mavx512f -mavx512bw -mfma
APPLESILICON_FLAGS := -DUSE_NEON -DUSE_SIMD -march=armv8.5-a

ifndef EXE
	EXE := minke
	EXE_NOT_SET := true
endif

ifeq ($(OS), Windows_NT)
	CXX := g++
	CXXFLAGS += -static
	SUFFIX := .exe
	MKDIR := cmd /C mkdir
	RMDIR := cmd /C rmdir /S /Q
	RM := del
else
	CXX := g++
	UNAME_S := $(shell uname -s)
	SUFFIX :=
	ifeq ($(UNAME_S), Darwin)
		CXX := clang++
		LDFLAGS := -flto
	endif
	LDFLAGS += -pthread
	MKDIR := mkdir -p
	RMDIR := rm -rf
	RM := rm
endif

define build
	$(MAKE) \
		ARCH_FLAGS="$($1_FLAGS)" \
		BUILD_DIR=$(BASE_BUILD_DIR)/$2 \
		EXE=$(EXE)$(if $(EXE_NOT_SET),-v$(VERSION))$(if $(EXE_NOT_SET),-$2)$(SUFFIX) \
		build
endef

.PHONY: all clean native avx2 bmi2 avx512 apple-silicon evalfile
all: native

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
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) $(LDFLAGS) -o $(EXE) $(OBJECTS)

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(ARCH_FLAGS) -c $< -o $@ -MMD -MP

$(BUILD_DIR):
	$(MKDIR) $(BUILD_DIR)

clean:
	$(RMDIR) $(BASE_BUILD_DIR)
	$(RM) -f $(EXE)$(if $(EXE_NOT_SET),-v$(VERSION))*

-include $(OBJECTS:.o=.d)
