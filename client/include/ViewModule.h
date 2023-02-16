#pragma once
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
    TcpClient client;
    ViewModule() = default;
    ViewModule(const ViewModule &) = delete;
    ViewModule(ViewModule &&) = delete;
    ViewModule &operator=(const ViewModule &) = delete;

    /*
     * connect client
     */
    void connect();

    /**
     * @brief render result window function
     */
    void render_resultUI(bool &);
    /**
     * @brief render query window function
     */
    void render_query_window(bool &);
    /**
     * @brief render add file window function
     */
    void render_add_file_window(bool &);
    /**
     * @brief render delete file window function
     */
    void render_delete_file_window(bool &);
};

}  // namespace app
