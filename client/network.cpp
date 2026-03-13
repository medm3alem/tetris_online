#include "network.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <thread>
#include <queue>
#include <mutex>
#ifndef _WIN32
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include <iostream>
#include <cstring>
#include <errno.h>
bool network_alive = false;

int sock = -1;
std::queue<std::string> messages;
std::mutex msg_mutex;
std::thread connection_thread;

void network_connect(const char* server_ip){
// lancer la connexiion dans un thread séparé
    connection_thread = std::thread([server_ip](){
        sock = socket(AF_INET, SOCK_STREAM, 0);  //crée une socket TCP IPv4
        if (sock < 0) {
            std::cerr << "ERROR: Socket creation failed: " << strerror(errno) << std::endl;
            return;
        }
        std::cout << "Socket created successfully (fd=" << sock << ")" << std::endl;

        sockaddr_in server{};
        server.sin_family = AF_INET;
        server.sin_port = htons(4242); // port serveura
        //const char* server_ip = "10.90.234.220";
        //const char* server_ip = "10.31.30.16";
        // Résoudre le hostname (DNS) ou IP directe
        struct addrinfo hints{}, *res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(server_ip, "4242", &hints, &res) != 0) {
            std::cerr << "ERROR: Cannot resolve host: " << server_ip << std::endl;
            #ifdef _WIN32
            closesocket(sock);
            #else
            close(sock);
            #endif
            sock = -1;
            return;
        }
        server = *(sockaddr_in*)res->ai_addr;
        freeaddrinfo(res);
        if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0) {
            std::cerr << "ERROR: Connection failed: " << strerror(errno) << std::endl;
            std::cerr << "Make sure the server is running on " << server_ip << ":4242" << std::endl;
            #ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
            sock = -1;
            return;
        }
        network_alive = true;  // Seulement ici après succès
        std::cout << "Connected to server successfully!" << std::endl;
    });
    connection_thread.detach();


}

void disconnect() {
    network_send("QUIT\n");
    if (sock >= 0) {
        #ifdef _WIN32
        shutdown(sock, SD_BOTH);
#else
        shutdown(sock, SHUT_RDWR);
#endif
        #ifdef _WIN32
            closesocket(sock);
#else
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

    int sent = send(sock, msg.c_str(), msg.size(), 0); //envoie message
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
            int n = recv(sock, buffer, sizeof(buffer)-1, 0);
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

            // Traiter tous les messages complets (terminés par \n)
            size_t pos;
            while ((pos = accumulated.find('\n')) != std::string::npos) {
                std::string msg = accumulated.substr(0, pos);
                accumulated = accumulated.substr(pos + 1);

                if (!msg.empty()) {
                    std::cout << "Received: " << msg << std::endl;
                    std::lock_guard<std::mutex> lock(msg_mutex);
                    messages.push(msg); // message mis en file
                }
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