#include "server.h"
#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <netinet/in.h>
#include <algorithm>
#include <mutex>
#include <sstream>
/*
std::vector<int> clients;
std::mutex clients_mutex;
//int ready_players = 0;
std::string start_msg = "MATCH_START\n";
bool start_match = false;

void handle_client(int client_socket) {
    char buffer[256];
    std::string accumulated;

    while (true) {
        int n = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            std::cout << "Client " << client_socket << " disconnected" << std::endl;
            break;
        }

        buffer[n] = '\0';
        accumulated += std::string(buffer);

        // Traiter tous les messages complets (terminés par \n)
        size_t pos;
        while ((pos = accumulated.find('\n')) != std::string::npos) {
            std::string msg = accumulated.substr(0, pos + 1);
            accumulated = accumulated.substr(pos + 1);

            std::cout << "Received from " << client_socket << ": " << msg;
            if (msg=="READY\n"){
                std::lock_guard<std::mutex> lock(clients_mutex);
                //ready_players++;
                    if (clients.size() == 2) {
                        start_match = true;
                        //ready_players = 0;
                    }
            }

            if (msg == "QUIT\n") {
                std::cout << "Client " << client_socket << " sent QUIT command" << std::endl;
                close(client_socket);
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
                    std::cout << "Remaining clients: " << clients.size() << std::endl;
                }

                return;
            }

            // Broadcast à tous les autres clients
            std::vector<int> clients_copy;
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                clients_copy = clients;
            }

            for (int c : clients_copy) {

                if (start_match) {
                    send(c,start_msg.c_str(), start_msg.size(), 0);
                    start_match = false;
                }


                if (c != client_socket) {
                    int sent = send(c, msg.c_str(), msg.size(), 0);
                    if (sent < 0) {
                        std::cout << "Failed to send to client " << c << std::endl;
                    }
                }
                
            }
        }
    }

    close(client_socket); // Fermer la socket du client

    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end()); //
    std::cout << "Remaining clients: " << clients.size() << std::endl;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // Création de la socket

    // Permettre la réutilisation du port
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(4242);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return 1;
    }

    listen(server_fd, 5);

    std::cout << "Server listening on port 4242" << std::endl;

    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        std::cout << "New client connected: " << client_socket << std::endl;

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(client_socket);
            std::cout << "Total clients: " << clients.size() << std::endl;
        }

        std::thread(handle_client, client_socket).detach();
    }

    close(server_fd);
    return 0;
}*/



std::vector<int> clients;
std::mutex clients_mutex;

int ready_players = 0; // nombre de joueurs READY
std::string start_msg = "MATCH_START\n";

void handle_client(int client_socket) {
    char buffer[256];
    std::string accumulated;

    while (true) {
        int n = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;

        buffer[n] = '\0';
        accumulated += buffer;

        size_t pos;
        while ((pos = accumulated.find('\n')) != std::string::npos) {
            std::string msg = accumulated.substr(0, pos + 1);
            accumulated = accumulated.substr(pos + 1);

            if (msg == "READY\n") {
                bool match_start_now = false;

                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    ready_players++;
                    if (ready_players == 2) {
                        match_start_now = true; // On est prêt
                        ready_players = 0; // réinitialiser pour la prochaine partie
                    }
                }

                if (match_start_now) {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    for (int c : clients) {
                        send(c, start_msg.c_str(), start_msg.size(), 0);
                    }
                    std::cout << "MATCH_START sent to all clients\n";
                }
            }

            // Broadcast normal
            if (msg != "READY\n") {
                std::lock_guard<std::mutex> lock(clients_mutex);
                for (int c : clients) {
                    if (c != client_socket) send(c, msg.c_str(), msg.size(), 0);
                }
            }
        }
    }

    // Client déconnecté
    close(client_socket);
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
    }
}



int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // Création de la socket

    // Permettre la réutilisation du port
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(4242);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return 1;
    }

    listen(server_fd, 5);

    std::cout << "Server listening on port 4242" << std::endl;

    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        std::cout << "New client connected: " << client_socket << std::endl;

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(client_socket);
            std::cout << "Total clients: " << clients.size() << std::endl;
        }

        std::thread(handle_client, client_socket).detach();
    }

    close(server_fd);
    return 0;

}