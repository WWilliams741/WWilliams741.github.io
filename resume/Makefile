# This Makefile assumes you have loaded emscripten's environment.
# (On Windows, you may need to execute emsdk_env.bat or encmdprompt.bat ahead)
#
# Running `make Makefile` will produce three files:
#  - web/resume.html
#  - web/resume.js
#  - web/resume.wasm

CC = emcc
CXX = em++
WEB_DIR = web
EXE = $(WEB_DIR)/resume.html
IMGUI_DIR = ../imgui
FREETYPE_DIR = ../freetype
SOURCES = resume.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp $(IMGUI_DIR)/misc/freetype/imgui_freetype.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)
CPPFLAGS = -DIMGUI_USER_CONFIG="\"my_imgui_config.h\""
LDFLAGS = -s USE_FREETYPE=1
EMS =

##---------------------------------------------------------------------
## EMSCRIPTEN OPTIONS
##---------------------------------------------------------------------

# ("EMS" options gets added to both CPPFLAGS and LDFLAGS, whereas some options are for linker only)
EMS += -s DISABLE_EXCEPTION_CATCHING=1
LDFLAGS += -s USE_GLFW=3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=0 -s ASSERTIONS=1

# Uncomment next line to fix possible rendering bugs with Emscripten version older then 1.39.0 (https://github.com/ocornut/imgui/issues/2877)
#EMS += -s BINARYEN_TRAP_MODE=clamp
#EMS += -s SAFE_HEAP=1    ## Adds overhead

# Emscripten allows preloading a file or folder to be accessible at runtime.
# The Makefile for this example project suggests embedding the misc/fonts/ folder into our application, it will then be accessible as "/fonts"
# See documentation for more details: https://emscripten.org/docs/porting/files/packaging_files.html
# (Default value is 0. Set to 1 to enable file-system and include the misc/fonts/ folder as part of the build.)
USE_FILE_SYSTEM ?= 0
ifeq ($(USE_FILE_SYSTEM), 0)
# LDFLAGS += -s NO_FILESYSTEM=1
# CPPFLAGS += -DIMGUI_DISABLE_FILE_FUNCTIONS
LDFLAGS += --no-heap-copy --preload-file fonts@/fonts
endif
ifeq ($(USE_FILE_SYSTEM), 1)
LDFLAGS += --no-heap-copy --preload-file fonts@/fonts
endif

##---------------------------------------------------------------------
## FINAL BUILD FLAGS
##---------------------------------------------------------------------

CPPFLAGS += -I. -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(IMGUI_DIR)/misc/freetype -I$(FREETYPE_DIR)/include
#CPPFLAGS += -g
CPPFLAGS += -Wall -Wformat -Os $(EMS)
LDFLAGS += --shell-file resume_shell.html
LDFLAGS += $(EMS)

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/misc/freetype/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(EXE)

$(WEB_DIR):
	mkdir $@

serve: all
	python3 -m http.server -d $(WEB_DIR)

$(EXE): $(OBJS) $(WEB_DIR)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -rf $(OBJS) $(WEB_DIR)
