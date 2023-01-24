# class TcpClient

*Defined at client\include/TcpClient.h#16*



**brief** TcpClient



## Members

io_context io

basic_endpoint ep

basic_stream_socket sock

ProtoBuf pb



## Functions

### handleGet

*public basic_string handleGet(const std::filesystem::path & path)*

*Defined at client\include/TcpClient.h#57*

### handlePost

*public basic_string handlePost(const std::filesystem::path & path, T && data)*

*Defined at client\include/TcpClient.h#78*

### handleDelete

*public basic_string handleDelete(const std::filesystem::path & path)*

*Defined at client\include/TcpClient.h#100*

### TcpClient

*public void TcpClient(basic_string ip, size_t port)*

*Defined at client\include/TcpClient.h#37*

### isConnected

*public _Bool isConnected()*

*Defined at client\include/TcpClient.h#44*

### connect

*public void connect()*

*Defined at client\include/TcpClient.h#46*

### disconnect

*public void disconnect()*

*Defined at client\include/TcpClient.h#53*



