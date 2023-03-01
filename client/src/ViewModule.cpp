#include "ViewModule.h"

#include <imgui.h>
#include <json/json.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <string>

#include "File.h"
#include "ImGuiFileDialog.h"
#include "LoggerRegister.h"
#include "Properties.h"
#include "TcpClient.h"

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a
// merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char *desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

app::ViewModule::ViewModule(std::shared_ptr<TcpClient> tcpClient)
    : client(std::move(tcpClient)) {
    logger = spdlog::get("logger");
}

std::shared_ptr<app::TcpClient> app::ViewModule::getClient() const {
    return client;
}

void app::ViewModule::init() {
    const Json::Value &value = Properties::readProperties("config.json");
    const std::string &domain = value["domain"].asString();
    const std::string &ip = value["ip"].asString();
    const int port = value["port"].asInt();
    const std::string &level = value["level"].asString();
    const int filesplit = value["filesplit"].asInt();
    const int threads = value["threads"].asInt();
    const std::string &font = value["font"].asString();
    std::copy(domain.begin(), domain.end(), std::begin(this->domain));
    std::copy(ip.begin(), ip.end(), std::begin(this->ip));
    std::copy(level.begin(), level.end(), std::begin(this->level));
    std::copy(font.begin(), font.end(), std::begin(this->font));
    this->port = port;
    this->filesplit = filesplit;
    this->threads = threads;
}

void app::ViewModule::render_resultUI(bool &show_window) {
    ImGui::Begin("result", &show_window);

    const std::string &res = LoggerRegister::getLogStream().str();
    auto p = res.size();
    std::string_view s = {res.begin(), res.begin() + p};
    for (int i = 0; i < 15; i++) {
        p = s.find_last_of('\n');
        if (std::string::npos != p) {
            s = s.substr(0, p);
        } else {
            break;
        }
    }
    std::string show = {res.begin() + p + 1, res.end()};
    ImGui::Text("%s", show.c_str());
}

void app::ViewModule::render_query_window(bool &show_window) {
    ImGui::Begin("Tcp File query", &show_window);

    ImGui::BulletText("路径: ");
    ImGui::SameLine();
    ImGui::InputTextWithHint("##file path", "file path", queryPath,
                             IM_ARRAYSIZE(queryPath));
    ImGui::Separator();

    if (ImGui::Button("上层目录")) {
        std::string_view s{std::begin(queryPath),
                           std::begin(queryPath) + std::strlen(queryPath) - 1};
        if (s.size() >= 1) {
            s = s.substr(0, s.find_last_of('\\'));
            std::copy(s.begin(), s.end(), std::begin(queryPath));
            queryPath[s.size()] = '\0';
        }
    }

    const auto &res = client->getDirList();

    if (ImGui::BeginTable("split", 2)) {
        ImGui::TableNextColumn();
        ImGui::Text("文件");
        ImGui::TableNextColumn();

        const auto sizeLabel = "大小(字节)";
        auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() -
                     ImGui::CalcTextSize(sizeLabel).x - ImGui::GetScrollX() -
                     2 * ImGui::GetStyle().ItemSpacing.x);
        if (posX > ImGui::GetCursorPosX()) ImGui::SetCursorPosX(posX);
        ImGui::Text(sizeLabel);

        for (const auto &[filename, filesize] : res) {
            ImGui::TableNextColumn();
            if (ImGui::Selectable(filename.c_str())) {
                if (*filename.rbegin() == '\\') {
                    // is dir
                    std::memset(std::begin(queryPath), 0,
                                std::strlen(queryPath));
                    std::copy(filename.begin(), filename.end(), queryPath);
                } else {
                    // is file
                    std::memset(std::begin(getPath), 0, std::strlen(getPath));
                    std::memset(std::begin(deletePath), 0,
                                std::strlen(deletePath));
                    std::copy(filename.begin(), filename.end(), getPath);
                    std::copy(filename.begin(), filename.end(), deletePath);
                }
            }

            ImGui::TableNextColumn();

            auto posX =
                (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() -
                 ImGui::CalcTextSize(std::to_string(filesize).c_str()).x -
                 ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
            if (posX > ImGui::GetCursorPosX()) ImGui::SetCursorPosX(posX);
            ImGui::Text("%llu", filesize);
        }
        ImGui::EndTable();
    }

    ImGui::End();
    try {
        client->handleQuery(queryPath);
    } catch (std::exception &e) {
        spdlog::get("logger")->error("{}", e.what());
    }
}

void app::ViewModule::render_get_window(bool &show_window) {
    ImGui::Begin("Tcp File get", &show_window);

    ImGui::Text("获取文件");

    ImGui::BulletText("保存文件本地路径:");
    ImGui::InputText("##save path", savePath, IM_ARRAYSIZE(savePath));
    ImGui::SameLine();
    if (ImGui::Button("open explorer", ImVec2(160, 0))) {
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseDirDlgKey", "Choose File", nullptr, ".", 1, nullptr,
            ImGuiFileDialogFlags_Modal);
    }
    if (ImGuiFileDialog::Instance()->Display("ChooseDirDlgKey")) {
        // action if OK
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName =
                ImGuiFileDialog::Instance()->GetFilePathName();
            std::string filePath =
                ImGuiFileDialog::Instance()->GetCurrentPath();
            // action
            std::memset(savePath, 0, sizeof savePath);
            std::copy(filePath.begin(), filePath.end(), savePath);
            client->setSavePath(savePath);
            logger->debug("select dir: {}", filePathName);
        }
        // close
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::Separator();
    ImGui::BulletText("本地文件保存路径");
    ImGui::InputTextWithHint("##get file", "file path", getPath,
                             IM_ARRAYSIZE(getPath));
    ImGui::SameLine();
    if (ImGui::Button("get file", ImVec2(160, 0))) {
        logger->info("get file: {}", getPath);
        try {
            client->handleGet(getPath, savePath);
        } catch (const std::exception &e) {
            spdlog::get("logger")->error("{}", e.what());
        }
    }
    ImGui::End();
}

void app::ViewModule::render_add_file_window(bool &show_window) {
    ImGui::Begin("Tcp File Transmit", &show_window);

    ImGui::Text("传输文件");
    ImGui::Separator();
    ImGui::BulletText("传送的文件路径:");

    ImGui::SameLine();
    // NOTE: open Dialog Simple
    if (ImGui::Button("Open File Dialog"))
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseFileDlgKey", " Choose a File", "*.*", ".", "", 1, nullptr,
            ImGuiFileDialogFlags_Modal);

    ImGui::Text("%s", selectPath);
    // display
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
        // action if OK
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName =
                ImGuiFileDialog::Instance()->GetFilePathName();
            std::string filePath =
                ImGuiFileDialog::Instance()->GetCurrentPath();
            // action
            logger->debug("add file: {}", filePathName);
            std::memset(selectPath, 0, sizeof selectPath);
            std::copy(filePathName.begin(), filePathName.end(), selectPath);
            // get file name
            std::filesystem::path path(selectPath);
            std::string fileName = path.filename().string();
            std::memset(sendToPath, 0, sizeof sendToPath);
            std::copy(fileName.begin(), fileName.end(), sendToPath);
        }
        // close
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::Separator();
    ImGui::BulletText("远程保存路径");
    ImGui::InputTextWithHint("##path to save on server",
                             "path to save on server", sendToPath,
                             IM_ARRAYSIZE(sendToPath));

    ImGui::SameLine();
    if (ImGui::Button("send")) {
        try {
            // NOTE: transmit file
            File file(selectPath);

            const auto &dirList = client->getDirList();
            const std::string &path{selectPath};
            const auto size = File::GetRemoteFileSize(path + ".sw", dirList);
            const auto filesplitsize = client->getFilesplitsize();
            const auto &splitedData =
                file.GetFileDataSplited(size, filesplitsize);

            client->handlePost(sendToPath, splitedData);
        } catch (std::exception &e) {
            spdlog::get("logger")->error("{}", e.what());
        }
    }

    ImGui::End();
}

void app::ViewModule::render_delete_file_window(bool &show_window) {
    ImGui::Begin("Tcp File delete", &show_window);

    ImGui::Text("删除文件");
    ImGui::Separator();
    ImGui::BulletText("远程文件删除路径:");

    ImGui::InputTextWithHint("##file path", "file path", deletePath,
                             IM_ARRAYSIZE(deletePath));

    ImGui::SameLine();
    if (ImGui::Button("delete")) {
        logger->info("delete file: {}", deletePath);
        try {
            client->handleDelete(deletePath);
        } catch (std::exception &e) {
            spdlog::get("logger")->error("{}", e.what());
        }
    }

    ImGui::End();
}

void app::ViewModule::render_setting_window(bool &show_window) {
    ImGui::Begin("setting", &show_window);
    ImGui::Text("设置");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::Separator();

    ImGuiIO &io = ImGui::GetIO();

    if (ImGui::TreeNode("config.json")) {
        ImGui::InputTextWithHint("domain", "domain", domain,
                                 IM_ARRAYSIZE(domain));

        ImGui::InputTextWithHint("ip", "ip", ip, IM_ARRAYSIZE(ip));

        ImGui::InputInt("port", &port);

        ImGui::InputTextWithHint("level", "level", level, IM_ARRAYSIZE(level));

        ImGui::InputInt("filesplit", &filesplit);

        ImGui::InputInt("threads", &threads);

        ImGui::InputTextWithHint("font", "font", font, IM_ARRAYSIZE(font));

        if (ImGui::Button("save")) {
            try {
                Json::Value value;
                value["domain"] = domain;
                value["ip"] = ip;
                value["port"] = port;
                value["level"] = level;
                value["filesplit"] = filesplit;
                value["threads"] = threads;
                value["font"] = font;
                Properties::writeProperties("config.json", value);
                spdlog::get("logger")->info("config save success");
            } catch (std::exception &e) {
                spdlog::get("logger")->error("config save error:{}", e.what());
            }
        }
    }

    if (ImGui::TreeNode("Configuration##2")) {
        ImGui::Text("General");
        ImGui::CheckboxFlags("io.ConfigFlags: NavEnableKeyboard",
                             &io.ConfigFlags,
                             ImGuiConfigFlags_NavEnableKeyboard);
        ImGui::SameLine();
        HelpMarker("Enable keyboard controls.");
        ImGui::CheckboxFlags("io.ConfigFlags: NavEnableGamepad",
                             &io.ConfigFlags,
                             ImGuiConfigFlags_NavEnableGamepad);
        ImGui::SameLine();
        HelpMarker(
            "Enable gamepad controls. Require backend to set "
            "io.BackendFlags "
            "|= ImGuiBackendFlags_HasGamepad.\n\nRead instructions in "
            "imgui.cpp for details.");
        ImGui::CheckboxFlags("io.ConfigFlags: NavEnableSetMousePos",
                             &io.ConfigFlags,
                             ImGuiConfigFlags_NavEnableSetMousePos);
        ImGui::SameLine();
        HelpMarker(
            "Instruct navigation to move the mouse cursor. See comment for "
            "ImGuiConfigFlags_NavEnableSetMousePos.");
        ImGui::CheckboxFlags("io.ConfigFlags: NoMouse", &io.ConfigFlags,
                             ImGuiConfigFlags_NoMouse);
        if (io.ConfigFlags & ImGuiConfigFlags_NoMouse) {
            // The "NoMouse" option can get us stuck with a disabled mouse!
            // Let's provide an alternative way to fix it:
            if (fmodf((float)ImGui::GetTime(), 0.40f) < 0.20f) {
                ImGui::SameLine();
                ImGui::Text("<<PRESS SPACE TO DISABLE>>");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Space))
                io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        }
        ImGui::CheckboxFlags("io.ConfigFlags: NoMouseCursorChange",
                             &io.ConfigFlags,
                             ImGuiConfigFlags_NoMouseCursorChange);
        ImGui::SameLine();
        HelpMarker(
            "Instruct backend to not alter mouse cursor shape and "
            "visibility.");
        ImGui::Checkbox("io.ConfigInputTrickleEventQueue",
                        &io.ConfigInputTrickleEventQueue);
        ImGui::SameLine();
        HelpMarker(
            "Enable input queue trickling: some types of events submitted "
            "during the same frame (e.g. button down + up) will be spread "
            "over "
            "multiple frames, improving interactions with low framerates.");
        ImGui::Checkbox("io.MouseDrawCursor", &io.MouseDrawCursor);
        ImGui::SameLine();
        HelpMarker(
            "Instruct Dear ImGui to render a mouse cursor itself. Note "
            "that a "
            "mouse cursor rendered via your application GPU rendering path "
            "will feel more laggy than hardware cursor, but will be more "
            "in "
            "sync with your other visuals.\n\nSome desktop applications "
            "may "
            "use both kinds of cursors (e.g. enable software cursor only "
            "when "
            "resizing/dragging something).");

        ImGui::Text("Widgets");
        ImGui::Checkbox("io.ConfigInputTextCursorBlink",
                        &io.ConfigInputTextCursorBlink);
        ImGui::SameLine();
        HelpMarker(
            "Enable blinking cursor (optional as some users consider it to "
            "be "
            "distracting).");
        ImGui::Checkbox("io.ConfigInputTextEnterKeepActive",
                        &io.ConfigInputTextEnterKeepActive);
        ImGui::SameLine();
        HelpMarker(
            "Pressing Enter will keep item active and select contents "
            "(single-line only).");
        ImGui::Checkbox("io.ConfigDragClickToInputText",
                        &io.ConfigDragClickToInputText);
        ImGui::SameLine();
        HelpMarker(
            "Enable turning DragXXX widgets into text input with a simple "
            "mouse click-release (without moving).");
        ImGui::Checkbox("io.ConfigWindowsResizeFromEdges",
                        &io.ConfigWindowsResizeFromEdges);
        ImGui::SameLine();
        HelpMarker(
            "Enable resizing of windows from their edges and from the "
            "lower-left corner.\nThis requires (io.BackendFlags & "
            "ImGuiBackendFlags_HasMouseCursors) because it needs mouse "
            "cursor "
            "feedback.");
        ImGui::Checkbox("io.ConfigWindowsMoveFromTitleBarOnly",
                        &io.ConfigWindowsMoveFromTitleBarOnly);
        ImGui::Checkbox("io.ConfigMacOSXBehaviors", &io.ConfigMacOSXBehaviors);
        ImGui::Text("Also see Style->Rendering for rendering options.");
        ImGui::TreePop();
        ImGui::Spacing();
    }

    // IMGUI_DEMO_MARKER("Configuration/Backend Flags");
    if (ImGui::TreeNode("Backend Flags")) {
        HelpMarker(
            "Those flags are set by the backends (imgui_impl_xxx files) to "
            "specify their capabilities.\n"
            "Here we expose them as read-only fields to avoid breaking "
            "interactions with your backend.");

        // Make a local copy to avoid modifying actual backend flags.
        // FIXME: We don't use BeginDisabled() to keep label bright, maybe
        // we need a BeginReadonly() equivalent..
        ImGuiBackendFlags backend_flags = io.BackendFlags;
        ImGui::CheckboxFlags("io.BackendFlags: HasGamepad", &backend_flags,
                             ImGuiBackendFlags_HasGamepad);
        ImGui::CheckboxFlags("io.BackendFlags: HasMouseCursors", &backend_flags,
                             ImGuiBackendFlags_HasMouseCursors);
        ImGui::CheckboxFlags("io.BackendFlags: HasSetMousePos", &backend_flags,
                             ImGuiBackendFlags_HasSetMousePos);
        ImGui::CheckboxFlags("io.BackendFlags: RendererHasVtxOffset",
                             &backend_flags,
                             ImGuiBackendFlags_RendererHasVtxOffset);
        ImGui::TreePop();
        ImGui::Spacing();
    }

    // IMGUI_DEMO_MARKER("Configuration/Style");
    if (ImGui::TreeNode("Style")) {
        HelpMarker(
            "The same contents can be accessed in 'Tools->Style Editor' or "
            "by "
            "calling the ShowStyleEditor() function.");

        ImGui::ShowStyleEditor();
        ImGui::TreePop();
        ImGui::Spacing();
    }

    // IMGUI_DEMO_MARKER("Configuration/Capture, Logging");
    if (ImGui::TreeNode("Capture/Logging")) {
        HelpMarker(
            "The logging API redirects all text output so you can easily "
            "capture the content of "
            "a window or a block. Tree nodes can be automatically "
            "expanded.\n"
            "Try opening any of the contents below in this window and then "
            "click one of the \"Log To\" button.");
        ImGui::LogButtons();

        HelpMarker(
            "You can also call ImGui::LogText() to output directly to the "
            "log "
            "without a visual output.");
        if (ImGui::Button("Copy \"Hello, world!\" to clipboard")) {
            ImGui::LogToClipboard();
            ImGui::LogText("Hello, world!");
            ImGui::LogFinish();
        }
        ImGui::TreePop();
    }
    ImGui::End();
}
