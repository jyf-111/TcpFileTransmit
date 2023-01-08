#pragma once
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <string_view>

#include "File.h"
#include "ImGuiFileDialog.h"
#include "TcpClient.h"
#include "app.h"
#include "def.h"

/** @brief Application class */
namespace app {

/** @brief tcp client */
TcpClient client("127.0.0.1", 1234);

/** @brief UIModule */
class UIModule {
    char selectPath[BUF_SIZE];
    char sendToPath[BUF_SIZE];
    char result[BUF_SIZE];

    /** @brief result handle*/
    void resultHandle(std::string_view, char *);

   public:
    UIModule() = default;
    UIModule(const UIModule &) = delete;
    UIModule(UIModule &&) = delete;
    UIModule &operator=(const UIModule &) = delete;

    /**
     * @brief render query window function
     */
    void render_ConnectUI();
    void render_resultUI(bool &);
    void render_query_window(bool &);
    void render_add_file_window(bool &);
    void render_delete_file_window(bool &);
};

}  // namespace app

void app::UIModule::resultHandle(std::string_view result, char *log) {
    std::memset(log, 0, BUF_SIZE);

    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);

    std::string tmp;
    tmp += "[";
    tmp += std::to_string(now->tm_hour);
    tmp += ":";
    tmp += std::to_string(now->tm_min);
    tmp += ":";
    tmp += std::to_string(now->tm_sec);
    tmp += "]\n";
    tmp.append(result);
    tmp.append("\n");
    std::copy(tmp.begin(), tmp.end(), log);
}
void app::UIModule::render_ConnectUI() {
    ImGui::Text("connect status: ");
	ImGui::SameLine();
    if (client.isConnected()) {
        ImGui::Text("Connected");
    } else {
        ImGui::Text("Disconnected");
    }
    if (ImGui::Button("Connect")) {
        try {
            app::client.connect();
            resultHandle("Connect success", result);
        } catch (const std::exception &e) {
            spdlog::error(e.what());
            resultHandle(e.what(), result);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Disconnect")) {
        try {
            app::client.disconnect();
            resultHandle("Disconnect", result);
        } catch (const std::exception &e) {
            spdlog::error(e.what());
            resultHandle(e.what(), result);
        }
    }
}

void app::UIModule::render_resultUI(bool &show_window) {
    ImGui::Begin("result", &show_window);

    ImGui::InputTextMultiline("##result", result, IM_ARRAYSIZE(result),
                              ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 20),
                              ImGuiInputTextFlags_ReadOnly);
    ImGui::End();
}

void app::UIModule::render_query_window(bool &show_window) {
    ImGui::Begin("Tcp File query", &show_window, ImGuiWindowFlags_MenuBar);
    render_ConnectUI();

    ImGui::Text("query file");
    ImGui::BulletText("Enter the file path to query:");

    ImGui::InputTextWithHint("", "file path", selectPath,
                             IM_ARRAYSIZE(selectPath));
    ImGui::SameLine();
    if (ImGui::Button("query")) {
        spdlog::info("query file: {}", selectPath);
        std::memset(result, 0, BUF_SIZE);
        try {
            auto tmp = client.handleGet(selectPath);
            resultHandle(tmp, result);
        } catch (std::exception &e) {
            spdlog::error("query file error: {}", e.what());
            resultHandle(e.what(), result);
        }
    }

    ImGui::End();
}

/**
 * @brief render add file window function
 */
void app::UIModule::render_add_file_window(bool &show_window) {
    ImGui::Begin("Tcp File Transmit", &show_window, ImGuiWindowFlags_MenuBar);
    render_ConnectUI();

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
            spdlog::info("add file: {}", filePathName);
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
            if (strlen(selectPath) == 0 || strlen(sendToPath) == 0) {
                throw std::runtime_error(
                    "select file or path to save on server is empty");
            }
            File file(selectPath);
            const std::string fileContent = file.GetFileData();
            auto ret = client.handlePost(sendToPath, fileContent);
            resultHandle(ret, result);
        } catch (std::exception &e) {
            spdlog::error("{}", e.what());
            resultHandle(e.what(), result);
        }
    }

    ImGui::End();
}

void app::UIModule::render_delete_file_window(bool &show_window) {
    ImGui::Begin("Tcp File delete", &show_window, ImGuiWindowFlags_MenuBar);
    render_ConnectUI();

    ImGui::Text("delete file");
    ImGui::BulletText("Enter the file path to delete:");

    ImGui::InputTextWithHint("", "file path", sendToPath,
                             IM_ARRAYSIZE(sendToPath));

    ImGui::SameLine();
    if (ImGui::Button("delete")) {
        spdlog::info("delete file:");
        try {
            auto ret = client.handleDelete(sendToPath);
            resultHandle(ret, result);
        } catch (std::exception &e) {
            spdlog::error("{}", e.what());
            resultHandle(e.what(), result);
        }
    }

    ImGui::End();
}
