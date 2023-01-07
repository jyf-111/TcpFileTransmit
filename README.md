## TcpFileTransmit
---
### TOOLCHAIN
- C++20
- 文件操作主要依赖C++17的std::filesystem

### REQUIRE

- 构建工具: [xmake](https://github.com/xmake-io/xmake)
- 网络库: [asio](https://github.com/chriskohlhoff/asio)
- 日志库: [spdlog](https://github.com/gabime/spdlog)
- json库: [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
- 测试库: [gtest](https://github.com/google/googletest)
- ui库: 
	- 轻量级图形库: [imgui](https://github.com/ocornut/imgui)
	- opengl Extension: [glew](https://github.com/nigels-com/glew)


### TODO
- [x] asio basic transmit
- [x] templete coding
- [x] threadPoll
- [ ] continue transmit
- [ ] imgui ui
- [ ] gtest
- [ ] synch -> async


### summary
- fstream 会自动关闭
- [std::string is not available in asio::buffer when recvive, use std::array instead](https://www.codenong.com/4068249/)
