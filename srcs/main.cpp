#include <stdio.h>
#include <stdlib.h>
#include "imgui_vulkan_helper.h"

#define APP_NAME            "NAS Backup"
#define APP_VERSION         VK_MAKE_VERSION(0, 1, 0)
#define WIDTH               1920
#define HEIGHT              1080
#define PATH_BUF_LEN        4096
#define SPACING             10
#define FONT_NORMAL         22
#define FONT_LARGE          28
#define BTN_FILL_WIDTH      10

#define FONT                "fonts/SourceHanSansCN/SourceHanSansCN-Medium.otf"
#define TEX_YESNO           "textures/yes-no-01.png"
#define TEX_YES_UL_X        670.0f
#define TEX_YES_UL_Y        508.0f
#define TEX_YES_RB_X        885.0f
#define TEX_YES_RB_Y        724.0f
#define TEX_NO_UL_X         943.0f
#define TEX_NO_UL_Y         508.0f
#define TEX_NO_RB_X         1158.0f
#define TEX_NO_RB_Y         724.0f

int main(int, char**)
{
    ImguiVulkanHelper gui_helper;

    if (!gui_helper.initWindow(WIDTH, HEIGHT, APP_NAME))
        return EXIT_FAILURE;
    if (!gui_helper.initVulkan(APP_NAME, APP_VERSION))
        return EXIT_FAILURE;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForVulkan(gui_helper.getWindow(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    gui_helper.fillImguiVulkanInitInfo(&init_info);
    ImGui_ImplVulkan_Init(&init_info, gui_helper.getRenderPass());

    ImGuiIO& io = ImGui::GetIO();
    ImFont *font_normal = io.Fonts->AddFontFromFileTTF(FONT,
            FONT_NORMAL, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    ImFont *font_large = io.Fonts->AddFontFromFileTTF(FONT,
            FONT_LARGE, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    // upload fonts
    if (!gui_helper.initializeFontTexture()) {
        fprintf(stderr, "Initialize font texture failed.\n");
        return EXIT_FAILURE;
    }

    ImTextureID tex_yesno;
    int tex_yesno_width, tex_yesno_height;
    tex_yesno = gui_helper.loadImage(TEX_YESNO, &tex_yesno_width, &tex_yesno_height);
    if (tex_yesno == NULL) {
        fprintf(stderr, "Load image: %s failed.\n", TEX_YESNO);
        return EXIT_FAILURE;
    } else {
        fprintf(stdout, "Load image: %s succeeded. Width: %d, height: %d\n",
                TEX_YESNO, tex_yesno_width, tex_yesno_height);
    }

    ImGuiStyle& style = ImGui::GetStyle();
    style.ItemSpacing.x = SPACING;
    style.ItemSpacing.y = SPACING;
    style.ItemInnerSpacing.x = SPACING;
    char photo_hash_file[PATH_BUF_LEN];
    char video_hash_file[PATH_BUF_LEN];
    char import_dir[PATH_BUF_LEN];
    char output_dir[PATH_BUF_LEN];
    memset(photo_hash_file, 0, sizeof(photo_hash_file));
    memset(video_hash_file, 0, sizeof(video_hash_file));
    memset(import_dir, 0, sizeof(import_dir));
    memset(output_dir, 0, sizeof(output_dir));
    bool photo_hash_valid = false;
    bool video_hash_valid = false;
    bool import_dir_valid = false;
    bool output_dir_valid = false;

    while (!glfwWindowShouldClose(gui_helper.getWindow())) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Backup photos and videos to the NAS");
        // GetWindowWidth/Height should be called within a Begin/End block
        float window_width = ImGui::GetWindowWidth();
        float window_height = ImGui::GetWindowHeight();
        ImVec2 input_text_dimension = ImGui::CalcTextSize("Import directory");
        ImVec2 yesno_dimension = ImVec2(input_text_dimension.y * 1.3f, input_text_dimension.y * 1.3f);
        float yesno_pos = window_width - yesno_dimension.x - style.ItemInnerSpacing.x;
        float push_input_width = input_text_dimension.x+ yesno_dimension.x + style.ItemInnerSpacing.x * 4;

        ImGui::PushItemWidth(-push_input_width);
        ImGui::InputText("Photo hash file", photo_hash_file, sizeof(photo_hash_file));
        ImGui::SameLine();
        ImGui::SetCursorPosX(yesno_pos);
        if (photo_hash_valid)
            ImGui::Image(tex_yesno, yesno_dimension,
                        ImVec2(TEX_YES_UL_X / tex_yesno_width, TEX_YES_UL_Y / tex_yesno_height),
                        ImVec2(TEX_YES_RB_X / tex_yesno_width, TEX_YES_RB_Y / tex_yesno_height));
        else
            ImGui::Image(tex_yesno, yesno_dimension,
                        ImVec2(TEX_NO_UL_X / tex_yesno_width, TEX_NO_UL_Y / tex_yesno_height),
                        ImVec2(TEX_NO_RB_X / tex_yesno_width, TEX_NO_RB_Y / tex_yesno_height));

        ImGui::InputText("Video hash file", video_hash_file, sizeof(video_hash_file));
        ImGui::SameLine();
        ImGui::SetCursorPosX(yesno_pos);
        if (video_hash_valid)
            ImGui::Image(tex_yesno, yesno_dimension,
                        ImVec2(TEX_YES_UL_X / tex_yesno_width, TEX_YES_UL_Y / tex_yesno_height),
                        ImVec2(TEX_YES_RB_X / tex_yesno_width, TEX_YES_RB_Y / tex_yesno_height));
        else
            ImGui::Image(tex_yesno, yesno_dimension,
                        ImVec2(TEX_NO_UL_X / tex_yesno_width, TEX_NO_UL_Y / tex_yesno_height),
                        ImVec2(TEX_NO_RB_X / tex_yesno_width, TEX_NO_RB_Y / tex_yesno_height));

        ImGui::InputText("Import directory", import_dir, sizeof(import_dir));
        ImGui::SameLine();
        ImGui::SetCursorPosX(yesno_pos);
        if (import_dir_valid)
            ImGui::Image(tex_yesno, yesno_dimension,
                         ImVec2(TEX_YES_UL_X / tex_yesno_width, TEX_YES_UL_Y / tex_yesno_height),
                         ImVec2(TEX_YES_RB_X / tex_yesno_width, TEX_YES_RB_Y / tex_yesno_height));
        else
            ImGui::Image(tex_yesno, yesno_dimension,
                         ImVec2(TEX_NO_UL_X / tex_yesno_width, TEX_NO_UL_Y / tex_yesno_height),
                         ImVec2(TEX_NO_RB_X / tex_yesno_width, TEX_NO_RB_Y / tex_yesno_height));

        ImGui::InputText("Output directory", output_dir, sizeof(output_dir));
        ImGui::SameLine();
        ImGui::SetCursorPosX(yesno_pos);
        if (output_dir_valid)
            ImGui::Image(tex_yesno, yesno_dimension,
                        ImVec2(TEX_YES_UL_X / tex_yesno_width, TEX_YES_UL_Y / tex_yesno_height),
                        ImVec2(TEX_YES_RB_X / tex_yesno_width, TEX_YES_RB_Y / tex_yesno_height));
        else
            ImGui::Image(tex_yesno, yesno_dimension,
                        ImVec2(TEX_NO_UL_X / tex_yesno_width, TEX_NO_UL_Y / tex_yesno_height),
                        ImVec2(TEX_NO_RB_X / tex_yesno_width, TEX_NO_RB_Y / tex_yesno_height));
        ImGui::PopItemWidth();

        ImGui::PushFont(font_large);
        float start_btn_pos = (window_width - (ImGui::CalcTextSize("Start").x + BTN_FILL_WIDTH * 4)) * 0.5f;
        ImGui::SetCursorPosX(start_btn_pos);
        ImGui::Button("Start", ImVec2(ImGui::CalcTextSize("Start").x + BTN_FILL_WIDTH * 4,
                               ImGui::CalcTextSize("Start").y + BTN_FILL_WIDTH));
        ImGui::PopFont();
        ImGui::End();

        // Rendering
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        gui_helper.drawFrame(draw_data);
    }
    vkDeviceWaitIdle(gui_helper.getDevice());

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}
