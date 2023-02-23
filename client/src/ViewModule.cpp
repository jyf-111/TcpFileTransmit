#include "ViewModule.h"

#include <imgui.h>
#include <spdlog/spdlog.h>

#include <string>

#include "File.h"
#include "ImGuiFileDialog.h"
#include "TcpClient.h"


using namespace spdlog;

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
    : client(std::move(tcpClient)) {}

std::shared_ptr<app::TcpClient> app::ViewModule::getClient() const {
    return client;
}

void app::ViewModule::render_resultUI(bool &show_window) {
    ImGui::Begin("result", &show_window);

    std::string res = client->getResult();
    char *s = const_cast<char *>(res.c_str());
    ImGui::InputTextMultiline("##result", s, res.size(),
                              ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 20),
                              ImGuiInputTextFlags_ReadOnly);
    ImGui::End();
}

void app::ViewModule::render_query_window(bool &show_window) {
    ImGui::Begin("Tcp File query", &show_window);

    ImGui::Text("query file");
    ImGui::BulletText("path:");
    ImGui::SameLine();
    ImGui::InputTextWithHint("", "file path", queryPath,
                             IM_ARRAYSIZE(queryPath));
    client->handleQuery(queryPath);

    std::string res = client->getDir();
    char *s = const_cast<char *>(res.c_str());
    ImGui::InputTextMultiline("##result", s, res.size(),
                              ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 50),
                              ImGuiInputTextFlags_ReadOnly);
    ImGui::End();
}

void app::ViewModule::render_get_window(bool &show_window) {
    ImGui::Begin("Tcp File get", &show_window, ImGuiWindowFlags_MenuBar);

    ImGui::Text("get file");

    ImGui::BulletText("Enter the place to save file:");
    ImGui::InputText("save path", savePath, IM_ARRAYSIZE(savePath));
    ImGui::SameLine();
    if (ImGui::Button("open explorer")) {
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
            debug("select dir: {}", filePathName);
        }
        // close
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::BulletText("Enter the file path to get on server:");
    ImGui::InputTextWithHint(" get file", "file path", getPath,
                             IM_ARRAYSIZE(getPath));
    ImGui::SameLine();
    if (ImGui::Button("get")) {
        info("get file: {}", getPath);
        client->handleGet(getPath);
    }
    ImGui::End();
}

void app::ViewModule::render_add_file_window(bool &show_window) {
    ImGui::Begin("Tcp File Transmit", &show_window, ImGuiWindowFlags_MenuBar);

    ImGui::Text("transmit file");
    ImGui::BulletText("Enter the file path to transmit:");

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
            debug("add file: {}", filePathName);
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

    ImGui::BulletText("path to save on server");
    ImGui::InputTextWithHint("", "path to save on server", sendToPath,
                             IM_ARRAYSIZE(sendToPath));

    ImGui::SameLine();
    if (ImGui::Button("send file")) {
        try {
            // NOTE: transmit file
            File file(selectPath);
            client->handlePost(sendToPath, file.GetFileDataSplited(
                                               client->getFilesplitsize()));
        } catch (std::exception &e) {
            error("{}", e.what());
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
        info("delete file: {}", deletePath);
        try {
            client->handleDelete(deletePath);
        } catch (std::exception &e) {
            error("{}", e.what());
        }
    }

    ImGui::End();
}

void app::ViewModule::render_setting_window(bool &show_window) {
    ImGui::Begin("setting", &show_window);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::Separator();

    ImGuiIO &io = ImGui::GetIO();

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
            "Enable gamepad controls. Require backend to set io.BackendFlags "
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
            "Instruct backend to not alter mouse cursor shape and visibility.");
        ImGui::Checkbox("io.ConfigInputTrickleEventQueue",
                        &io.ConfigInputTrickleEventQueue);
        ImGui::SameLine();
        HelpMarker(
            "Enable input queue trickling: some types of events submitted "
            "during the same frame (e.g. button down + up) will be spread over "
            "multiple frames, improving interactions with low framerates.");
        ImGui::Checkbox("io.MouseDrawCursor", &io.MouseDrawCursor);
        ImGui::SameLine();
        HelpMarker(
            "Instruct Dear ImGui to render a mouse cursor itself. Note that a "
            "mouse cursor rendered via your application GPU rendering path "
            "will feel more laggy than hardware cursor, but will be more in "
            "sync with your other visuals.\n\nSome desktop applications may "
            "use both kinds of cursors (e.g. enable software cursor only when "
            "resizing/dragging something).");

        ImGui::Text("Widgets");
        ImGui::Checkbox("io.ConfigInputTextCursorBlink",
                        &io.ConfigInputTextCursorBlink);
        ImGui::SameLine();
        HelpMarker(
            "Enable blinking cursor (optional as some users consider it to be "
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
            "ImGuiBackendFlags_HasMouseCursors) because it needs mouse cursor "
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
        // FIXME: We don't use BeginDisabled() to keep label bright, maybe we
        // need a BeginReadonly() equivalent..
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
            "The same contents can be accessed in 'Tools->Style Editor' or by "
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
            "a window or a block. Tree nodes can be automatically expanded.\n"
            "Try opening any of the contents below in this window and then "
            "click one of the \"Log To\" button.");
        ImGui::LogButtons();

        HelpMarker(
            "You can also call ImGui::LogText() to output directly to the log "
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
