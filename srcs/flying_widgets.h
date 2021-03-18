#ifndef FLYING_WIDGETS_H
#define FLYING_WIDGETS_H

namespace ImGui {
    void MWProgressBar(int cur, int total, const ImVec2& size, const ImVec2 &pad);
#ifdef ENABLE_OBSOLETED_CODES
    bool BufferingBar(const char* label, float value,  const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col);
    bool Spinner(const char* label, float radius, int thickness, const ImU32& color);
#endif
}

#endif