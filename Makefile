SRC_DIR := srcs
OBJ_DIR := objs
IMGUI_SRCS := \
    $(SRC_DIR)/imgui/imgui.cpp \
    $(SRC_DIR)/imgui/imgui_draw.cpp \
    $(SRC_DIR)/imgui/imgui_widgets.cpp
IMGUI_IMPL_SRCS := \
    $(SRC_DIR)/imgui_impl/imgui_impl_glfw.cpp \
    $(SRC_DIR)/imgui_impl/imgui_impl_vulkan.cpp
MAIN_SRCS := \
    $(SRC_DIR)/imgui_vulkan_helper.cpp \
    $(SRC_DIR)/main.cpp
SRCS := \
    $(IMGUI_SRCS)      \
    $(IMGUI_IMPL_SRCS) \
    $(MAIN_SRCS)
OBJS := $(patsubst $(SRC_DIR)%.cpp,$(OBJ_DIR)%.o,$(SRCS))

CC = g++
CFLAGS_DBG = -Wall -DVSYNC -DDEBUG -g -I$(SRC_DIR) -I$(SRC_DIR)/imgui -I$(SRC_DIR)/imgui_impl
CFLAGS_REL = -DVSYNC -O2 -I$(SRC_DIR) -I$(SRC_DIR)/imgui -I$(SRC_DIR)/imgui_impl
ifeq ($(build), debug)
    CFLAGS := $(CFLAGS_DBG)
endif
ifeq ($(build), release)
    CFLAGS := $(CFLAGS_REL)
endif
CFLAGS ?= $(CFLAGS_DBG)
LDFLAGS = -lvulkan -lglfw
TARGET = warbler

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@if [ ! -d "$(shell dirname $@)" ]; then  \
		mkdir -p "$(shell dirname $@)";  \
	fi
	$(CC) -c $(CFLAGS) -o $@ $^

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -f $(TARGET)
	rm -rf $(OBJ_DIR)
