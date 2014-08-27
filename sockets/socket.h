#ifndef SOCKET_H
#define SOCKET_H

#include <boost/cstdint.hpp>
#include <vector>
#include <string>

struct my_in_addr {
    boost::uint32_t s_addr;
};

struct my_sockaddr_in {
    boost::int16_t sin_family;
    boost::uint16_t sin_port;
    my_in_addr sin_addr;
    char sin_zero[8];
};

#ifdef WIN32
typedef uintptr_t socket_fd_t;
#else
typedef int socket_fd_t;
#endif

class Socket{
protected:
    socket_fd_t socket;
    void init();
public:
    Socket(): socket(-1){}
    virtual ~Socket();
    void close();
    operator bool(){
        return this->socket != -1;
    }
};

class ServerSocket : public Socket{
    friend class ServerSideClientSocket;
protected:
    bool receive_internal(std::vector<unsigned char> &dst, size_t, socket_fd_t client);
    bool send_internal(std::vector<unsigned char> &dst, socket_fd_t client);
public:
    ServerSocket(unsigned char ip4_address[4], int port);
    ServerSideClientSocket accept();
};

class SocketPipe{
protected:
    virtual bool receive_internal(std::vector<unsigned char> &dst, size_t) = 0;
    virtual bool send_internal(std::vector<unsigned char> &src) = 0;
public:
    bool receive(std::vector<unsigned char> &dst);
    bool send(std::vector<unsigned char> &src);
	bool send(const char *src){
		return send(std::vector<unsigned char>(src, src + strlen(src)));
	}
	bool send(std::string &src){
		return send(std::vector<unsigned char>(src.begin(), src.end()));
	}
};

class ServerSideClientSocket : public SocketPipe{
protected:
    ServerSocket *socket;
    socket_fd_t client;
    bool receive_internal(std::vector<unsigned char> &dst, size_t size){
        return this->socket->receive_internal(dst, size, this->client);
    }
    bool send_internal(std::vector<unsigned char> &dst){
        return this->socket->send_internal(dst, this->client);
    }
public:
    ServerSideClientSocket(ServerSocket *socket, socket_fd_t client): socket(socket), client(client){}
    operator bool() const{
        return this->client != -1;
    }
};

class ClientSocket : public Socket, public SocketPipe{
    friend class ServerSideClientSocket;
protected:
    bool receive_internal(std::vector<unsigned char> &dst, size_t);
    bool send_internal(std::vector<unsigned char> &dst);
public:
    ClientSocket(unsigned char ip4_address[4], int port, bool force = 1);
};

#endif // SOCKET_H
