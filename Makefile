TARGET    := vulkan_engine
BUILD_DIR := build

CXX      ?= g++
CC       ?= gcc
CXXFLAGS := -std=c++17 -O2 -MMD -MP
CFLAGS   := -O2 -MMD -MP

INCLUDES := -Isrc \
            -Iexternal/glm-1.0.0 \
            -Iexternal/imgui-master \
            -Iexternal/imgui-master/backends \
            -Iexternal/nativefiledialog-master/src/include \
            -Iexternal/basisu/transcoder

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SOURCES := $(call rwildcard,src,*.cpp) \
           $(wildcard external/imgui-master/*.cpp) \
           external/imgui-master/backends/imgui_impl_glfw.cpp \
           external/imgui-master/backends/imgui_impl_vulkan.cpp \
           external/basisu/transcoder/basisu_transcoder.cpp \
           external/basisu/zstd/zstd.c \
           external/nativefiledialog-master/src/nfd_common.c

ifeq ($(OS),Windows_NT)
    TARGET    := $(TARGET).exe
    CC        := gcc
    GLSLC     := $(VULKAN_SDK)/Bin/glslc.exe
    SOURCES   += external/nativefiledialog-master/src/nfd_win.cpp
    INCLUDES  += -I"$(VULKAN_SDK)/Include" \
                 -Iexternal/glfw-3.3.9.bin.WIN64/include \
                 -Iexternal/KTX/include
    CXXFLAGS  += -D_CRT_SECURE_NO_WARNINGS
    LDFLAGS   := -L"$(VULKAN_SDK)/Lib" \
                 -Lexternal/glfw-3.3.9.bin.WIN64/lib-mingw-w64 \
                 -Lexternal/KTX/lib
    # link the GCC runtime statically so the exe runs outside an MSYS2 shell
    LDFLAGS   += -static-libgcc -static-libstdc++
    LIBS      := -lvulkan-1 -lglfw3 -lktx -lole32 -luuid -lcomdlg32 -lgdi32 -lshell32
    LIBS      += -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic
    # recipes run through sh (MSYS2 / Git Bash), so use POSIX commands
    MKDIR      = mkdir -p $(1)
    RMDIR      = rm -rf $(1)
    DLLS      := $(BUILD_DIR)/ktx.dll
    COPY       = cp -f $(1) $(2)
    RUN        = ./$(BUILD_DIR)/$(TARGET)
else
    GLSLC     := glslc
    SOURCES   += external/nativefiledialog-master/src/nfd_gtk.c
    INCLUDES  += $(shell pkg-config --cflags glfw3 vulkan gtk+-3.0)
    LIBS      := $(shell pkg-config --libs glfw3 vulkan gtk+-3.0) -lktx -lpthread -ldl
    MKDIR      = mkdir -p $(1)
    RMDIR      = rm -rf $(1)
    DLLS      :=
    RUN        = ./$(BUILD_DIR)/$(TARGET)
endif

OBJECTS := $(patsubst %,$(BUILD_DIR)/obj/%.o,$(SOURCES))
DEPS    := $(OBJECTS:.o=.d)

SHADERS     := $(filter-out shaders/terrainHeight.vert, $(wildcard shaders/*.vert shaders/*.frag shaders/*.comp))
SHADERS_SPV := $(SHADERS:=.spv)

.PHONY: all shaders run clean

all: $(BUILD_DIR)/$(TARGET) $(DLLS) shaders

$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@ $(LDFLAGS) $(LIBS)

$(BUILD_DIR)/%.dll: external/KTX/dll/%.dll
	@$(call MKDIR,$(@D))
	$(call COPY,$<,$@)

$(BUILD_DIR)/obj/%.cpp.o: %.cpp
	@$(call MKDIR,$(@D))
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/obj/%.c.o: %.c
	@$(call MKDIR,$(@D))
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

shaders: $(SHADERS_SPV)

shaders/%.spv: shaders/%
	"$(GLSLC)" $< -o $@ --target-env=vulkan1.2

run: all
	$(RUN)

clean:
	$(call RMDIR,$(BUILD_DIR))

-include $(DEPS)
