#pragma once
#include <memory>

#include "TcpClient.h"
#include "imgui.h"

/** @brief Application class */
namespace app {

#define BUF_SIZE 1024

/**
 * @brief ClientModule
 */
class ViewModule {
    char getPath[BUF_SIZE];
    char savePath[BUF_SIZE] = {'.'};
    char queryPath[BUF_SIZE] = {'.'};
    char selectPath[BUF_SIZE];
    char sendToPath[BUF_SIZE];
    char deletePath[BUF_SIZE];
    char result[BUF_SIZE];

    ImVec4 color = {1.0f, 1.0f, 1.0f, 1.0f};

   public:
    /**
     * @brief tcp TcpClient
     */
    std::shared_ptr<TcpClient> client = std::make_shared<TcpClient>();
    ViewModule() = default;
    ViewModule(const ViewModule &) = delete;
    ViewModule(ViewModule &&) = delete;
    ViewModule &operator=(const ViewModule &) = delete;

    /**
     * get client
     */
    [[nodiscard]] std::shared_ptr<TcpClient> getClient() const;
    /**
     * @brief render result window function
     */
    void render_resultUI(bool &);
    /**
     * @brief render query window function
     */
    void render_query_window(bool &);
    /**
     * @brief render get window function
     */
    void render_get_window(bool &);
    /**
     * @brief render add file window function
     */
    void render_add_file_window(bool &);
    /**
     * @brief render delete file window function
     */
    void render_delete_file_window(bool &);
    /**
     * @brief render setting window function
     */
    void render_setting_window(bool &);
};

}  // namespace app
