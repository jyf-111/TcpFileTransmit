#include "ViewModule.h"

#include <imgui.h>
#include <spdlog/spdlog.h>

#include "File.h"
#include "ImGuiFileDialog.h"

using namespace spdlog;

void app::ViewModule::connect() {
    client.readProperties();
    client.run();
    client.connect();
}

void app::ViewModule::render_resultUI(bool &show_window) {
    ImGui::Begin("result", &show_window);

    std::string res = client.getResult();
    char *s = const_cast<char *>(res.c_str());
    ImGui::InputTextMultiline("##result", s, res.size(),
                              ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 20),
                              ImGuiInputTextFlags_ReadOnly);
    ImGui::End();
}

void app::ViewModule::render_query_window(bool &show_window) {
    ImGui::Begin("Tcp File query", &show_window, ImGuiWindowFlags_MenuBar);

    ImGui::Text("query file");
    ImGui::BulletText("Enter the file path to query:");

    ImGui::InputTextWithHint("", "file path", queryPath,
                             IM_ARRAYSIZE(queryPath));
    ImGui::SameLine();
    if (ImGui::Button("query")) {
        info("query file: {}", queryPath);
        client.handleGet(queryPath);
    }

    ImGui::End();
}

/**
 * @brief render add file window function
 */
void app::ViewModule::render_add_file_window(bool &show_window) {
    ImGui::Begin("Tcp File Transmit", &show_window, ImGuiWindowFlags_MenuBar);

    ImGui::Text("transmit file");
    ImGui::BulletText("Enter the file path to transmit:");

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
            info("add file: {}", filePathName);
            std::copy(filePathName.begin(), filePathName.end(), selectPath);
            // get file name
            std::filesystem::path path(selectPath);
            std::string fileName = path.filename().string();
            std::copy(fileName.begin(), fileName.end(), sendToPath);
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
        try {
            // NOTE: transmit file
            File file(selectPath);
            client.handlePost(sendToPath, file.GetFileData());
        } catch (std::exception &e) {
            spdlog::error("{}", e.what());
        }
    }

    ImGui::End();
}

void app::ViewModule::render_delete_file_window(bool &show_window) {
    ImGui::Begin("Tcp File delete", &show_window, ImGuiWindowFlags_MenuBar);

    ImGui::Text("delete file");
    ImGui::BulletText("Enter the file path to delete:");

    ImGui::InputTextWithHint("", "file path", deletePath,
                             IM_ARRAYSIZE(deletePath));

    ImGui::SameLine();
    if (ImGui::Button("delete")) {
        info("delete file:");
        try {
            client.handleDelete(deletePath);
        } catch (std::exception &e) {
            spdlog::error("{}", e.what());
        }
    }

    ImGui::End();
}
