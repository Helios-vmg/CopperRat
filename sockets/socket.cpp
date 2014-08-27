#include "socket.h"
#ifdef WIN32
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#endif
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#ifdef s_addr
#undef s_addr
#endif
#include <fstream>

struct SocketStaticData{
#ifdef WIN32
    SocketStaticData(){
        WSAStartup(1, &WSADATA());
    }
    ~SocketStaticData(){
        WSACleanup();
    }
#endif
} SocketStaticData_global;

void Socket::init(){
    this->socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (this->socket == -1)
        return;
}

Socket::~Socket(){
    this->close();
}

void Socket::close(){
    if (this->socket != -1){
        closesocket(this->socket);
        this->socket = -1;
    }
}

bool SocketPipe::receive(std::vector<unsigned char> &dst){
    if (!this->receive_internal(dst, 4))
        return 0;
    boost::uint32_t size = 0;
    for (int i = 0; i < 4; i++)
        size |= (boost::uint32_t)dst[i] << (8 * i);
    return this->receive_internal(dst, size);
}

bool SocketPipe::send(std::vector<unsigned char> &src){
    std::vector<unsigned char> array(4);
    size_t size = src.size();
    for (int i = 0; i < 4; i++){
        array[i] = size & 0xFF;
        size >>= 8;
    }
    return this->send_internal(array) && this->send_internal(src);
}

boost::uint32_t array_to_ip(unsigned char ip4_address[4]){
    boost::uint32_t addr = 0;
    for (int i = 0; i < 4; i++){
        addr <<= 8;
        addr |= ip4_address[i];
    }
    return addr;
}

ServerSocket::ServerSocket(unsigned char ip4_address[4], int port){
    this->init();
    my_sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(array_to_ip(ip4_address));
    if (bind(this->socket, (const sockaddr *)&address, sizeof(address)) || listen(this->socket, 1) == -1){
        this->close();
        return;
    }
}

ServerSideClientSocket ServerSocket::accept(){
    sockaddr client_address;
    int size = sizeof(client_address);
    int client = -1;
	socket_fd_t ret = ::accept(this->socket, (sockaddr *)&client_address, &size);
    return ServerSideClientSocket(this, ret);
}

bool ServerSocket::receive_internal(std::vector<unsigned char> &buffer, size_t size, socket_fd_t client){
	buffer.resize(size);
    auto result = recv(client, (char *)&buffer[0], (int)buffer.size(), 0);
    return result == size;
}

bool ServerSocket::send_internal(std::vector<unsigned char> &dst, socket_fd_t client){
    return ::send(client, (const char *)&dst[0], (int)dst.size(), 0) != -1;
}

ClientSocket::ClientSocket(unsigned char ip4_address[4], int port, bool force){
    this->init();
    my_sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(array_to_ip(ip4_address));
    bool failed;
    do{
        failed = !!connect(this->socket, (const sockaddr *)&address, sizeof(address));
    }while (failed && force);
    if (failed)
        this->close();
}

bool ClientSocket::receive_internal(std::vector<unsigned char> &buffer, size_t size){
	buffer.resize(size);
    auto result = recv(this->socket, (char *)&buffer[0], (int)buffer.size(), 0);
    return result == size;
}

bool ClientSocket::send_internal(std::vector<unsigned char> &dst){
    return ::send(this->socket, (const char *)&dst[0], (int)dst.size(), 0) != -1;
}
