
# TcpFileTransmit

<img width="793" alt="pic" src="https://user-images.githubusercontent.com/77335030/222915789-58bcf5ab-f414-42d5-9b4b-a8201d4ce66c.png">

---

## TOOLCHAIN

- C++20
- 文件操作主要依赖C++17的std::filesystem

### REQUIRE

- 构建工具: [xmake](https://github.com/xmake-io/xmake)
- 网络库: [asio](https://github.com/chriskohlhoff/asio)
- 加密库: [openssl](https://github.com/openssl/openssl)
- 日志库: [spdlog](https://github.com/gabime/spdlog)
- json库: [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
- 测试库: [gtest](https://github.com/google/googletest)
- ui库:
  - 轻量级图形库:
    - [imgui](https://github.com/ocornut/imgui)
    - [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog)
  - opengl Extension: [glew](https://github.com/nigels-com/glew)

## Build from source

first get [xmake](https://github.com/xmake-io/xmake)

then

```bash
git clone --recursive https://github.com/jyf-111/TcpFileTransmit
cd TcpFileTransmit

# build
xmake -b server     # build server
xmake -b client     # build client
xmake -b test       # build test

xmake               # or just `xmake` to build all

# run
xmake run server    # run server
xmake run client    # run client
```

## config

### server config

example:

```json
{
    "ip": "0.0.0.0",
    "port": 8000,
    "level": "info",
    "filesplit": 15000,
    "threads": 0,
    "certificate": "cert/server.pem",
    "privatekey": "cert/private.key",
    "gaptime": 1000
}
```

- ip
listening address
- port
listening port
- level
log level(debug,info,warn,error,...)
- filesplit
split file into small slice to transmit
- certificate
the certificate file for tls
- privatekey
the privatekey for tls
- threads
if small than 1, it will be your machine cpu number
else will be the `threads` number
- gaptime
the gap time between each send

### client config

example:

```json
{
    "domain" : "jyfwin.japaneast.cloudapp.azure.com",
    "filesplit" : 15000,
    "font" : "font/微软雅黑.ttf",
    "ip" : "127.0.0.1",
    "level" : "debug",
    "port" : 8000,
    "threads" : 0,
    "gaptime": 3000
}
```

- domain
the domain to connect(high priority than ip)
- ip
the ip to connect
- port
listening port
- filesplit
split file into small slice to transmit
- level
log level(debug,info,warn,error,...)
- threads
if small than 1, it will be your machine cpu number
else will be the `threads` number
- gaptime
the gap time between each send

## Known Issue

- 文件名不能带空格,中文

## UML

- server
![server](https://user-images.githubusercontent.com/77335030/222911286-fe84e710-1113-4d15-bde8-2ebb88385873.jpeg)

- client
![client](https://user-images.githubusercontent.com/77335030/222911347-74025e98-8a07-41d5-89ce-f696aec4bce8.jpeg)

