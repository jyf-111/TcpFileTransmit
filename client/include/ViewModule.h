#pragma once

#include <memory>

#include "TcpClient.h"
#include "imgui.h"

namespace app {

#define BUF_SIZE 1024

class ViewModule {
    std::shared_ptr<spdlog::logger> logger;
    char getPath[BUF_SIZE] = {};
    char savePath[BUF_SIZE] = {'.'};
    char queryPath[BUF_SIZE] = {'.'};
    char selectPath[BUF_SIZE] = {};
    char sendToPath[BUF_SIZE] = {};
    char deletePath[BUF_SIZE] = {};

    char domain[BUF_SIZE] = {};
    char ip[BUF_SIZE] = {};
    int port;
    char level[BUF_SIZE] = {};
    int filesplit;
    int threads;

    ImVec4 color = {1.0f, 1.0f, 1.0f, 1.0f};

   public:
    std::shared_ptr<TcpClient> client;
    ViewModule(std::shared_ptr<TcpClient>);
    ViewModule(const ViewModule &) = delete;
    ViewModule(ViewModule &&) = delete;
    ViewModule &operator=(const ViewModule &) = delete;

    [[nodiscard]] std::shared_ptr<TcpClient> getClient() const;
    void init();
    void render_resultUI(bool &);
    void render_query_window(bool &);
    void render_get_window(bool &);
    void render_add_file_window(bool &);
    void render_delete_file_window(bool &);
    void render_setting_window(bool &);
};

}  // namespace app
