#
# Scale harness — GLFW + OpenGL3 + libpng.
#
# macOS:
#   brew install glfw libpng
# Linux:
#   apt-get install libglfw3-dev libpng-dev
#

EXE = example_scale_harness
IMGUI_DIR = ../..
SOURCES  = main.cpp widget_zoo.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

UNAME_S := $(shell uname -s)

CXXFLAGS  = -std=c++11 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -g -Wall -Wformat
LIBS =

# libpng via pkg-config (works on both macOS/Homebrew and Linux).
CXXFLAGS += $(shell pkg-config --cflags libpng)
LIBS     += $(shell pkg-config --libs   libpng)

ifeq ($(UNAME_S), Linux)
	ECHO_MESSAGE = "Linux"
	LIBS     += -lGL $(shell pkg-config --static --libs glfw3)
	CXXFLAGS += $(shell pkg-config --cflags glfw3)
endif

ifeq ($(UNAME_S), Darwin)
	ECHO_MESSAGE = "Mac OS X"
	LIBS     += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS     += -L/opt/homebrew/lib -L/usr/local/lib -lglfw
	CXXFLAGS += -I/opt/homebrew/include -I/usr/local/include
endif

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
