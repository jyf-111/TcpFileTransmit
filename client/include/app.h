#pragma once
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <cstddef>

#include "ImGuiFileDialog.h"
#include "File.h"
#include "TcpClient.h"
#include "app.h"
#include "def.h"

namespace app {
static char selectPath[BUF_SIZE] = "";
static char sendToPath[BUF_SIZE] = "";
char result[BUF_SIZE] = "";

TcpClient client("127.0.0.1", 1234);

/**
 * @brief render query window function
 */
void render_query_window(bool &show_window) {
    ImGui::Begin("Tcp File query", &show_window, ImGuiWindowFlags_MenuBar);

    ImGui::Text("query file");
    ImGui::BulletText("Enter the file path to query:");
    ImGui::SameLine();

    ImGui::InputTextWithHint("", "file path", selectPath,
                             IM_ARRAYSIZE(selectPath));
    ImGui::SameLine();
    if (ImGui::Button("query")) {
        spdlog::info("query file: {}", selectPath);
        std::memset(result, 0, BUF_SIZE);
        auto tmp = client.handleGet(selectPath);
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

	ImGui::Text("%s", selectPath);

    ImGui::SameLine();
    // NOTE: open Dialog Simple
    if (ImGui::Button("Open File Dialog"))
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseFileDlgKey", " Choose a File", "*.*", ".", "", 1, nullptr,
            ImGuiFileDialogFlags_Modal);
    // display
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
        // action if OK
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName =
                ImGuiFileDialog::Instance()->GetFilePathName();
            std::string filePath =
                ImGuiFileDialog::Instance()->GetCurrentPath();
            // action
            spdlog::info("add file: {}", filePathName);
            std::copy(filePathName.begin(), filePathName.end(), selectPath);
        }
        // close
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::BulletText("path to save on server");
    ImGui::SameLine();
    ImGui::InputTextWithHint("", "path to save on server", sendToPath,
                             IM_ARRAYSIZE(sendToPath));

	ImGui::SameLine();
    if (ImGui::Button("send file")) {
        // NOTE: transmit file
		if(strlen(selectPath) == 0 || strlen(sendToPath) == 0) {
			throw std::runtime_error("select file or path to save on server is empty");
		}
		File file(selectPath);
		const std::string fileContent = file.GetFileData();
		client.handlePost(sendToPath, fileContent);
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

    ImGui::InputTextWithHint("", "file path", selectPath,
                             IM_ARRAYSIZE(selectPath));
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
