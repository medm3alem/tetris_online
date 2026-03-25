#include "network.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#endif
#include <thread>
#include <queue>
#include <mutex>
#include <iostream>
#include <cstring>
#include <string>
#include <errno.h>

bool network_alive = false;
int sock = -1;
std::queue<std::string> messages;
std::mutex msg_mutex;
std::thread connection_thread;

#ifdef _WIN32
static bool wsa_initialized = false;
static void init_wsa() {
    if (!wsa_initialized) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
        wsa_initialized = true;
    }
}
#endif

static void enable_tcp_nodelay(int s) {
    int flag = 1;
#ifdef _WIN32
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag));
#else
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
#endif
}

void network_connect(const char* server_ip) {
#ifdef _WIN32
    init_wsa();
#endif
    connection_thread = std::thread([server_ip](){

        // ── Parser "host:port" ou "host" seul ────────────────
        std::string host(server_ip);
        std::string port = "4242";
        size_t colon = host.rfind(':');
        if (colon != std::string::npos) {
            port = host.substr(colon + 1);
            host = host.substr(0, colon);
        }
        std::cout << "Connecting to " << host << ":" << port << std::endl;

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cerr << "ERROR: Socket creation failed: " << strerror(errno) << std::endl;
            return;
        }
        std::cout << "Socket created successfully (fd=" << sock << ")" << std::endl;

        struct addrinfo hints{}, *res;
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0) {
            std::cerr << "ERROR: Cannot resolve host: " << host << std::endl;
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
            sock = -1;
            return;
        }
        sockaddr_in server = *(sockaddr_in*)res->ai_addr;
        freeaddrinfo(res);

        if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0) {
            std::cerr << "ERROR: Connection failed: " << strerror(errno) << std::endl;
            std::cerr << "Make sure the server is running on " << host << ":" << port << std::endl;
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
            sock = -1;
            return;
        }

        enable_tcp_nodelay(sock);
        network_alive = true;
        std::cout << "Connected to server successfully!" << std::endl;
    });
    connection_thread.detach();
}

void disconnect() {
    network_send("QUIT\n");
    if (sock >= 0) {
#ifdef _WIN32
        shutdown(sock, SD_BOTH);
        closesocket(sock);
#else
        shutdown(sock, SHUT_RDWR);
        close(sock);
#endif
        sock = -1;
    }
    network_alive = false;
}

void network_send(const std::string& msg) {
    if (!network_alive || sock < 0) {
        std::cerr << "ERROR: Network not alive, cannot send (network_alive="
                  << network_alive << ", sock=" << sock << ")" << std::endl;
        return;
    }
#ifdef _WIN32
    int sent = send(sock, msg.c_str(), (int)msg.size(), 0);
#else
    int sent = send(sock, msg.c_str(), msg.size(), MSG_NOSIGNAL);
#endif
    if (sent < 0) {
        std::cerr << "ERROR: Send failed: " << strerror(errno) << std::endl;
        network_alive = false;
    } else {
        std::cout << "Sent: " << msg;
    }
}

void network_start_listener() {
    std::thread([](){
        char buffer[256];
        std::string accumulated;
        std::cout << "Listener thread started" << std::endl;

        while (network_alive && sock >= 0) {
            int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (n < 0) {
                std::cerr << "ERROR: Recv failed: " << strerror(errno) << std::endl;
                network_alive = false;
                break;
            }
            if (n == 0) {
                std::cout << "Connection closed by server" << std::endl;
                network_alive = false;
                break;
            }

            buffer[n] = '\0';
            accumulated += std::string(buffer);

            size_t pos;
            while ((pos = accumulated.find('\n')) != std::string::npos) {
                std::string msg = accumulated.substr(0, pos);
                accumulated     = accumulated.substr(pos + 1);
                if (msg.empty()) continue;

                if (msg == "PING") {
                    const char* pong = "PONG\n";
#ifdef _WIN32
                    send(sock, pong, 5, 0);
#else
                    send(sock, pong, 5, MSG_NOSIGNAL);
#endif
                    continue;
                }

                std::cout << "Received: " << msg << std::endl;
                std::lock_guard<std::mutex> lock(msg_mutex);
                messages.push(msg);
            }
        }

        if (sock >= 0) {
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
            sock = -1;
        }
        std::cout << "Listener thread stopped" << std::endl;
    }).detach();
}

bool network_has_message() {
    std::lock_guard<std::mutex> lock(msg_mutex);
    return !messages.empty();
}

std::string network_pop_message() {
    std::lock_guard<std::mutex> lock(msg_mutex);
    if (messages.empty()) return "";
    std::string msg = messages.front();
    messages.pop();
    return msg;
}

bool is_connected() {
    return network_alive;
}
