#pragma once
#include <imgui.h>
#include <spdlog/spdlog.h>

#include "TcpClient.h"
#include "app.h"
#include "def.h"

namespace app {
static char buf[256] = "";
char result[SIZE] = "";
TcpClient client("127.0.0.1", 1234);

/**
 * @brief render query window function
 */
void render_query_window(bool &show_window) {
    ImGui::Begin("Tcp File query", &show_window, ImGuiWindowFlags_MenuBar);

    ImGui::Text("query file");
    ImGui::BulletText("Enter the file path to query:");
    ImGui::SameLine();

    ImGui::InputTextWithHint("", "file path", buf, IM_ARRAYSIZE(buf));
    ImGui::SameLine();
    if (ImGui::Button("query")) {
        spdlog::info("query file: {}", buf);
        std::memset(result, 0, SIZE);
        auto tmp = client.handleGet(buf);
        std::copy(tmp.begin(), tmp.end(), result);
    }

    ImGui::BulletText("query result");

    ImGui::InputTextMultiline("##result", result, IM_ARRAYSIZE(result),
                              ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16),
                              ImGuiInputTextFlags_ReadOnly);
    ImGui::End();
}

/**
 * @brief render add file window function
 */
void render_add_file_window(bool &show_window) {
    ImGui::Begin("Tcp File Transmit", &show_window, ImGuiWindowFlags_MenuBar);

    ImGui::Text("transmit file");
    ImGui::BulletText("Enter the file path to transmit:");
    ImGui::SameLine();

    ImGui::InputTextWithHint("", "file path", buf, IM_ARRAYSIZE(buf));
    ImGui::SameLine();
    if (ImGui::Button("Send")) {
        spdlog::info("Send file:");
    }

    ImGui::BulletText("add result");

    ImGui::InputTextMultiline("##result", result, IM_ARRAYSIZE(result),
                              ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 4),
                              ImGuiInputTextFlags_ReadOnly);
    ImGui::End();
}

void render_delete_file_window(bool &show_window) {
    ImGui::Begin("Tcp File delete", &show_window, ImGuiWindowFlags_MenuBar);

    ImGui::Text("delete file");
    ImGui::BulletText("Enter the file path to delete:");
    ImGui::SameLine();

    ImGui::InputTextWithHint("", "file path", buf, IM_ARRAYSIZE(buf));
    ImGui::SameLine();
    if (ImGui::Button("delete")) {
        spdlog::info("delete file:");
    }

    ImGui::BulletText("delete result");
    ImGui::InputTextMultiline("##result", result, IM_ARRAYSIZE(result),
                              ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 4),
                              ImGuiInputTextFlags_ReadOnly);
    ImGui::End();
}
}  // namespace app
