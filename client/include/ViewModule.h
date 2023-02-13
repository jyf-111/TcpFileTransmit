#pragma once
#include <string_view>

#include "TcpClient.h"

/** @brief Application class */
namespace app {

#define BUF_SIZE 65536

/**
 * @brief ClientModule
 */
class ViewModule {
    char queryPath[BUF_SIZE];
    char selectPath[BUF_SIZE];
    char sendToPath[BUF_SIZE];
    char deletePath[BUF_SIZE];
    char result[BUF_SIZE];

   public:
    /**
     * @brief tcp TcpClient
     */
    TcpClient client{"127.0.0.1", 1234};
    ViewModule() = default;
    ViewModule(const ViewModule &) = delete;
    ViewModule(ViewModule &&) = delete;
    ViewModule &operator=(const ViewModule &) = delete;

    /**
     * @brief render query window function
     */
    void render_resultUI(bool &);
    void render_query_window(bool &);
    void render_add_file_window(bool &);
    void render_delete_file_window(bool &);
};

}  // namespace app
