---
marp: true
theme: default
paginate: true
---
<!-- 
class: lead
-->

## [TcpFileTransmit](https://github.com/jyf-111/TcpFileTransmit)
- C++20
- 构建工具: [xmake](https://github.com/xmake-io/xmake)
- 网络: [asio](https://github.com/chriskohlhoff/asio)
- 加密: [openssl](https://github.com/openssl/openssl)
- 日志: [spdlog](https://github.com/gabime/spdlog)
- json: [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
- 测试: [gtest](https://github.com/google/googletest)
    - ui:
        - [imgui](https://github.com/ocornut/imgui/)
        - [glew](https://github.com/nigels-com/glew)

![bg right:60% 90%](https://user-images.githubusercontent.com/77335030/222915789-58bcf5ab-f414-42d5-9b4b-a8201d4ce66c.png)

---
## 结构
```bash
TcpFileTransmit/
│  xmake.lua
├─client
│  │  xmake.lua
│  ├─3rdparty
│  │  └─ImGuiFileDialog
│  ├─include
│  │      TcpClient.h
│  │      ...
│  └─src
│         TcpClient.cpp
│         ...
├─server
│  │  xmake.lua
│  ├─include
│  │      TcpServer.h
│  │      ...
│  └─src
│         TcpServer.cpp
│         ...
└─test
        test.cpp
        xmake.lua
```

---
![](https://gitcode.net/jyf_111/imgbed/-/raw/master/pictures/2023/03/4_23_15_14_client.jpeg)

---
![](https://gitcode.net/jyf_111/imgbed/-/raw/master/pictures/2023/03/4_23_15_14_server.jpeg)

---

## asio网络 任务调度
```cpp
auto io = std::make_shared<asio::io_context>();
auto fileWriteStrand = std::make_shared<asio::io_context::strand>(*io);

socket->asyncwrite ...
socket->asyncread ...
io->post(...)
strand->post(...)

std::thread([this]() {
    asio::thread_pool threadPool(threads);
    for (int i = 0; i < threads - 1; i++) {
        asio::post(threadPool, [this]() { io->run(); });
    }
    threadPool.join();
}).detach();
...
// imgui ui
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
    return is;

```

---
## doWrite
```cpp
std::queue<ProtoBuf> writeQueue;

void ClientSession::doWrite() {
    if (queryIsEmpty()) {
        timer->async_wait(
            [self = shared_from_this()](const asio::error_code &e) {
                if (e) self->logger->error("async_wait: {}", e.message());
                self->timer->expires_after(std::chrono::milliseconds(self->gaptime));
                self->doWrite();
            });
        return;
    }
    auto buf = std::make_shared<asio::streambuf>();
    std::ostream os{buf.get()};
    os << popQueryFront();
    asio::async_write(*socketPtr, *buf,
                      [self = shared_from_this()](const asio::error_code &e,
                                                  std::size_t size) {
                          if (e) self->logger->error("async_write: {}", e.message());
                          self->doWrite();
                      });
}
```
---

## doRead

```cpp
void ClientSession::doRead() {
    auto resultBuf = std::make_shared<asio::streambuf>();
    auto streambuf = std::make_shared<asio::streambuf>();
    auto peek = std::make_shared<std::array<char, sizeof(std::size_t)>>();

    asio::async_read(*socketPtr, *streambuf, [peek, streambuf, self = shared_from_this()](
        const asio::system_error &e, std::size_t size) -> std::size_t {
            if (e.code()) return 0;
            if (size == sizeof(std::size_t)) {
                std::memcpy(peek.get(), streambuf.get()->data().data(), sizeof(std::size_t));
            }
            if (size > sizeof(std::size_t) && size == *reinterpret_cast<std::size_t *>(peek.get())) {
                return 0;
            } else {
                return 1;
            }
        },
        [streambuf, self = shared_from_this()](const asio::error_code &e, std::size_t size) {
            if (e) {
                self->client->disconnect();
                self->client->ipConnect();
                return;
            }
            self->doRead();
                ...
                // NOTE hanle read ,do file action ,and push message to queue
                ...
        });
}
```

---
## client长连接 openssl tls端到端加密  断线重连
```cpp
using ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;
asio::ssl::context ssl_context{asio::ssl::context::tls};

void app::TcpClient::ipConnect() {
    socketPtr = std::make_shared<ssl_socket>(*io, ssl_context);
    session = std::make_shared<ClientSession>(socketPtr, io);
    session->initClient(shared_from_this());
    socketPtr->next_layer().async_connect(
        asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port),
        [self = shared_from_this()](const asio::system_error &e) {
            if (e.code()) {
                self->timer->async_wait([self](const asio::system_error &e) {
                    // NOTE: windows 20s linux 127s
                    self->ipConnect();
                });
                return;
            }
            self->socketPtr->async_handshake(asio::ssl::stream_base::client, [self](const asio::system_error &e) {
                    if (e.code()) return;
                    self->connectFlag = true;
                    self->session->registerQuery();
                    self->session->doWrite();
                    self->session->doRead();
                });
        });
}
```
---
## 分段传输
```cpp
const std::vector<std::vector<char>> File::GetFileDataSplited(const std::filesystem::path &path, 
                                                    const int &index, const std::size_t &slice) {
    if (path.empty() || !std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("path is empty or is not regular file");
    }
    std::ifstream ifs(path, std::ios::binary);
    ifs.seekg(index);
    std::vector<std::vector<char>> file_data;
    const auto size = File::GetFileSize(path);

    while (ifs.tellg() < size) {
        if (ifs.tellg() + static_cast<std::ios::pos_type>(slice) >= size) {
            const auto s = size - ifs.tellg();
            std::vector<char> data(s);
            ifs.read(data.data(), s);
            file_data.push_back(std::move(data));
            break;
        } else {
            std::vector<char> data(slice);
            ifs.read(data.data(), slice);
            file_data.push_back(std::move(data));
        }
    }
    ifs.close();
    return file_data;
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
## 文件异步读取
```cpp
io->post([selectPath, sendtoPath, self = shared_from_this()]() {
    const auto &size = File::GetRemoteFileSize(selectPath.string() + ".sw", self->dirList);
    const auto &data = File::GetFileDataSplited(selectPath, size, self->filesplit);

    const auto &lenth = data.size();
    for (int i = 0; i < lenth; ++i) {
        ProtoBuf protobuf{ProtoBuf::Method::Post, sendtoPath, data.at(i)};
        protobuf.SetIndex(i);
        protobuf.SetTotal(lenth - 1);
        self->session->enqueue(protobuf);
    }
    });
```
---

## 断点续传

```cpp
const std::size_t File::GetRemoteFileSize(
    const std::filesystem::path &path,
    const std::vector<std::pair<std::string, std::size_t>> dirList) {
    std::size_t size = 0;
    for (const auto &[filename, filesize] : dirList) {
        if (std::filesystem::path(filename).filename().string() ==
            path.filename().string()) {
            size = filesize;
        }
    }
    return size;
}

void app::TcpClient::handleGet(const std::filesystem::path &path, const std::filesystem::path &savepath) {
    ProtoBuf protoBuf{ProtoBuf::Method::Get, path, std::vector<char>{'n', 'u', 'l', 'l'}};
    const auto tmpFile = savepath.string() + "/" + path.filename().string() + ".sw";
    if (File::FileIsExist(tmpFile)) {
        protoBuf.SetIndex(File::GetFileSize(tmpFile));
    }
    session->enqueue(protoBuf);
}
```

---

## SO_RCVBUF SO_SNDBUF
### [linux](https://www.cnblogs.com/x_wukong/p/8444557.html)
```bash
# man setsockopt
cat /proc/sys/net/core/rmem_default
212992  # 26KB
cat /proc/sys/net/core/wmem_default
212992
#新建socket从这两个文件读取
cat /proc/sys/net/ipv4/tcp_rmem
4096    131072  6291456   # min default max
cat /proc/sys/net/ipv4/tcp_wmem
4096    16384   4194304
```
### [windows](https://learn.microsoft.com/zh-cn/troubleshoot/windows-server/networking/description-tcp-features)
默认`65536~ 4*65536`, 使用了缩放因子. $ (65536)*2^S, S∈[0,14] $
TCP 窗口刻度是用于将最大窗口大小从 `65535` 字节增加到 1 千兆字节的选项。
