---
marp: true
---

## TcpFileTransmit
- C++20
- 构建工具: xmake
- 网络: asio
- 加密: openssl
- 日志: spdlog
- json: jsoncpp
- 测试: gtest
    - ui:
        - imgui
        - glew
---
## asio网络 任务调度
```cpp
std::shared_ptr<asio::io_service> io;
io = std::make_shared<asio::io_context>();

socket->asyncwrite ...
socket->asyncread ...
io.post(...)
strand.post(...)

std::thread([this]() {
    asio::thread_pool threadPool(threads);
    for (int i = 0; i < threads - 1; i++) {
        asio::post(threadPool, [this]() { io->run(); });
    }
    threadPool.join();
}).detach();
...
//imgui ui
...
```

---
## 协议
```cpp
class ProtoBuf {
    public:
    enum class [[nodiscard]] Method {
        Query,
        Get,
        Post,
        Delete,
    };
private:
    std::size_t size;
    std::size_t headsize;
    bool isDir = false;
    bool isFile = false;
    std::size_t index = 0;
    std::size_t total = 0;
    Method method;
    std::filesystem::path path;
    std::vector<char> data;

    ...
}
```
---
## 解包 二级制传输 对象串行化
```cpp
inline std::ostream &operator<<(std::ostream &os, const ProtoBuf &protoBuf) {
    os.write(reinterpret_cast<const char *>(&protoBuf.GetSize()),
             sizeof(std::size_t));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetHeadSize()),
             sizeof(std::size_t));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetIsDir()),
             sizeof(bool));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetIsFile()),
             sizeof(bool));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetIndex()),
             sizeof(std::size_t));
    os.write(reinterpret_cast<const char *>(&protoBuf.GetTotal()),
             sizeof(std::size_t));
    os << ProtoBuf::MethodToString(protoBuf.method) + " " +
              protoBuf.path.string() + " ";

    spdlog::get("logger")->debug("send {}", protoBuf.toString());
    const auto &data = protoBuf.GetData();
    os.write(data.data(), data.size());
    return os;
}
```

---

## 拆包 二级制传输 对象串行化
```cpp
inline std::istream &operator>>(std::istream &is, ProtoBuf &protoBuf) {
    std::size_t size;
    std::size_t headsize;
    bool isFile;
    bool isDir;
    std::size_t index;
    std::size_t total;
    std::string method;
    std::filesystem::path path;
    is.read(reinterpret_cast<char *>(&size), sizeof(std::size_t));
    is.read(reinterpret_cast<char *>(&headsize), sizeof(std::size_t));
    is.read(reinterpret_cast<char *>(&isDir), sizeof(bool));
    is.read(reinterpret_cast<char *>(&isFile), sizeof(bool));
    is.read(reinterpret_cast<char *>(&index), sizeof(std::size_t));
    is.read(reinterpret_cast<char *>(&total), sizeof(std::size_t));
    is >> method >> path;
    is.ignore();
    std::vector<char> data(size - headsize);
    is.read(&data[0], size - headsize);

    protoBuf.SetSize(size);
    protoBuf.SetHeadSize(headsize);
    protoBuf.SetIsDir(isDir);
    protoBuf.SetIsFile(isFile);
    protoBuf.SetIndex(index);
    protoBuf.SetTotal(total);
    protoBuf.SetMethod(ProtoBuf::StringToMethod(method));
    protoBuf.SetPath(path);
    protoBuf.SetData(data);
    spdlog::get("logger")->debug("recv {}", protoBuf.toString());
    return is;

```

---
## 分段传输
```cpp
const std::vector<std::vector<char>> File::GetFileDataSplited(const std::filesystem::path &path, const int &index, const std::size_t &slice) {
    if (path.empty() || !std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("path is empty or is not regular file");
    }
    std::ifstream ifs(path, std::ios::binary);
    ifs.seekg(index);

    std::vector<std::vector<char>> file_data;

    const auto size = std::filesystem::file_size(path);

    while (ifs.tellg() < size) {
        if (ifs.tellg() + static_cast<std::ios::pos_type>(slice) >= size) {
            const auto s = size - ifs.tellg();
            std::vector<char> data(s);
            ifs.read(data.data(), s);
            file_data.push_back(data);
            break;
        } else {
            std::vector<char> data(slice);
            ifs.read(data.data(), slice);
            file_data.push_back(data);
        }
    }
    ifs.close();
    return file_data;
```

---
## client长连接 端到端加密
```cpp
using ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;
asio::ssl::context ssl_context{asio::ssl::context::tls};

void app::TcpClient::ipConnect() {
    socketPtr = std::make_shared<ssl_socket>(*io, ssl_context);
    session = std::make_shared<ClientSession>(socketPtr, io);
    session->initClient(shared_from_this());

    logger->info("connectting");

    socketPtr->next_layer().async_connect(
        asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port),
        [self = shared_from_this()](const asio::system_error &e) {
            if (e.code()) {
                self->logger->warn("connect {}:{} failed: {}", self->ip, self->port, e.what());
                self->timer->async_wait([self](const asio::system_error &e) {
                    // NOTE:
                    // windows 20s
                    // linux 127s
                    self->ipConnect();
                });
                return;
            }
            self->logger->info("connect {}:{} success ", self->ip, self->port);
            self->socketPtr->async_handshake(asio::ssl::stream_base::client, [self](const asio::system_error &e) {
                    if (e.code()) {
                        self->logger->error("handshake failed: {}", e.what());
                        return;
                    }
                    self->logger->info("handshake success");
                    self->connectFlag = true;
                    self->session->registerQuery();
                    self->session->doWrite();
                    self->session->doRead();
                });
        });
}
```
---

## 文件异步保存
```cpp
self->fileWriteStrand->post([self, protoBuf]() {
    const std::string &filename =
        self->client->getSavePath() + "/" + protoBuf.GetPath().filename().string() + ".sw";
    File::SetFileData(filename, protoBuf.GetData());
});

```
## 多用户文件保护
- 文件在保存时候 保存为`{filename}.sw`
- 传输完毕后再重命名为`{filename}`

---

## 断点续传

## 客户端获取服务端目录下的swap文件
```cpp
const std::size_t File::GetRemoteFileSize(
    const std::filesystem::path &path,
    const std::vector<std::pair<std::string, std::size_t>> dirList) {
    std::size_t size = 0;
    for (const auto &[filename, filesize] : dirList) {
        if (std::filesystem::path(filename).filename().string() ==
            path.filename().string()) {
            size = filesize;
            spdlog::get("logger")->info("has remote swap file size = {}", size);
        }
    }
    return size;
}
```
```cpp
if (ImGui::Button("send")) {
        try {
            // NOTE: transmit file
            const auto &dirList = client->getDirList();
            const std::string &path{selectPath};
            const auto size = File::GetRemoteFileSize(path + ".sw", dirList);
            const auto filesplitsize = client->getFilesplitsize();
            const auto &splitedData = File::GetFileDataSplited(selectPath, size,filesplitsize);

            client->handlePost(sendToPath, splitedData);
        } catch (std::exception &e) {
            spdlog::get("logger")->error("{}", e.what());
        }
}
```