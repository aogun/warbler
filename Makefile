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
    $(SRC_DIR)/flying_widgets.cpp \
    $(SRC_DIR)/imgui_vulkan_helper.cpp \
    $(SRC_DIR)/main.cpp
SRCS := \
    $(IMGUI_SRCS)      \
    $(IMGUI_IMPL_SRCS) \
    $(MAIN_SRCS)
OBJS := $(patsubst $(SRC_DIR)%.cpp,$(OBJ_DIR)%.o,$(SRCS))

CFLAGS = -O2 -I$(SRC_DIR) -I$(SRC_DIR)/imgui -I$(SRC_DIR)/imgui_impl
LDFLAGS = -lvulkan -lglfw
TARGET = nanoplayer

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@if [ ! -d "$(shell dirname $@)" ]; then  \
		mkdir -p "$(shell dirname $@)";  \
	fi
	g++ -c $(CFLAGS) -o $@ $^

nanoplayer: $(OBJS)
	g++ -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -f $(TARGET)
	rm -rf $(OBJ_DIR)
