EXE = minke
NET_PATH = src/minke.bin
BUILD_DIR = build

SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(SOURCES)))

CXX = g++
CXXSTD = -std=c++20
CXXWARNS = -Wall
CXXFLAGS = -O3 -march=native -DNDEBUG -DNET_PATH=\"$(NET_PATH)\" $(CXXSTD) $(CXXWARNS) 

ifeq ($(OS), Windows_NT)
    EXE := $(EXE).exe
    MKDIR = cmd /C mkdir
    RMDIR = cmd /C rmdir /S /Q
    RM = del
else
    LINKER_FLAGS += -pthread
    MKDIR = mkdir -p
    RMDIR = rm -rf
    RM = rm
endif

all: $(EXE)

$(EXE): $(OBJECTS)
	$(CXX) -o $(EXE) $^ $(CXXFLAGS) 

# This assures that init.o is recompiled not only when init.cpp changes
# but also when $(NET_PATH) changes
$(BUILD_DIR)/init.o: src/init.cpp $(NET_PATH) | $(BUILD_DIR)
	$(CXX) -c $< $(CXXFLAGS) -o $@ -MMD -MP

# Compile other .cpp files
$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) -c $< $(CXXFLAGS) -o $@ -MMD -MP

$(BUILD_DIR):
	$(MKDIR) $(BUILD_DIR)

clean:
	$(RMDIR) $(BUILD_DIR)
	$(RM) $(EXE)

-include $(OBJECTS:.o=.d)
