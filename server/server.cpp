#include "server.h"
#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <netinet/in.h>
#include <algorithm>
#include <mutex>
#include <memory>
#include <map>
#include <atomic>

// ─────────────────────────────────────────────
//  Room : une partie isolée entre 2 joueurs
// ─────────────────────────────────────────────
struct Room {
    int id;
    int players[2] = {-1, -1};   // sockets des deux joueurs
    int player_count = 0;
    bool started = false;

    bool is_full()  const { return player_count == 2; }
    bool is_empty() const { return player_count == 0; }

    // Retourne le socket de l'adversaire
    int opponent_of(int sock) const {
        if (players[0] == sock) return players[1];
        if (players[1] == sock) return players[0];
        return -1;
    }

    void add_player(int sock) {
        players[player_count++] = sock;
    }

    void remove_player(int sock) {
        for (int i = 0; i < 2; i++) {
            if (players[i] == sock) {
                players[i] = -1;
                player_count--;
                return;
            }
        }
    }
};

// ─────────────────────────────────────────────
//  État global
// ─────────────────────────────────────────────
std::mutex global_mutex;
std::map<int, std::shared_ptr<Room>> socket_to_room;   // socket -> room
std::vector<std::shared_ptr<Room>> waiting_rooms;      // rooms avec 1 seul joueur
std::atomic<int> next_room_id{1};

// ─────────────────────────────────────────────
//  Helpers réseau
// ─────────────────────────────────────────────
void send_msg(int sock, const std::string& msg) {
    if (sock < 0) return;
    send(sock, msg.c_str(), msg.size(), 0);
}

// ─────────────────────────────────────────────
//  Rejoindre ou créer une room
// ─────────────────────────────────────────────
std::shared_ptr<Room> join_or_create_room(int sock) {
    std::lock_guard<std::mutex> lock(global_mutex);

    // Chercher une room qui attend un joueur
    for (auto it = waiting_rooms.begin(); it != waiting_rooms.end(); ++it) {
        auto room = *it;
        if (!room->is_full()) {
            room->add_player(sock);
            socket_to_room[sock] = room;
            waiting_rooms.erase(it);   // room pleine, on la retire de la liste d'attente

            // Lancer la partie !
            room->started = true;
            std::cout << "[Room " << room->id << "] Match start! Players: "
                      << room->players[0] << " vs " << room->players[1] << std::endl;
            send_msg(room->players[0], "MATCH_START\n");
            send_msg(room->players[1], "MATCH_START\n");
            return room;
        }
    }

    // Aucune room disponible : en créer une nouvelle
    auto room = std::make_shared<Room>();
    room->id = next_room_id++;
    room->add_player(sock);
    socket_to_room[sock] = room;
    waiting_rooms.push_back(room);
    std::cout << "[Room " << room->id << "] Created, waiting for opponent..." << std::endl;
    return room;
}

void leave_room(int sock) {
    std::lock_guard<std::mutex> lock(global_mutex);

    auto it = socket_to_room.find(sock);
    if (it == socket_to_room.end()) return;

    auto room = it->second;
    int opp = room->opponent_of(sock);

    room->remove_player(sock);
    socket_to_room.erase(it);

    // Retirer des rooms en attente si vide
    waiting_rooms.erase(
        std::remove_if(waiting_rooms.begin(), waiting_rooms.end(),
            [&](const std::shared_ptr<Room>& r){ return r->is_empty(); }),
        waiting_rooms.end()
    );

    // Prévenir l'adversaire s'il reste connecté
    if (opp >= 0) {
        send_msg(opp, "OPPONENT_LEFT\n");
        std::cout << "[Room " << room->id << "] Player " << sock
                  << " left, opponent " << opp << " notified." << std::endl;
    }
}

// ─────────────────────────────────────────────
//  Thread par client
// ─────────────────────────────────────────────
void handle_client(int client_socket) {
    std::cout << "Client " << client_socket << " connected." << std::endl;

    char buffer[512];
    std::string accumulated;
    std::shared_ptr<Room> room;

    while (true) {
        int n = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;

        buffer[n] = '\0';
        accumulated += buffer;

        size_t pos;
        while ((pos = accumulated.find('\n')) != std::string::npos) {
            std::string msg = accumulated.substr(0, pos + 1);
            accumulated = accumulated.substr(pos + 1);
            std::string msg_clean = msg.substr(0, msg.size() - 1); // sans \n pour les logs

            std::cout << "[Client " << client_socket << "] Received: " << msg_clean << std::endl;

            if (msg == "READY\n") {
                // Le client cherche une partie
                room = join_or_create_room(client_socket);

            } else if (msg == "QUIT\n") {
                goto disconnect;

            } else {
                // Relayer le message à l'adversaire uniquement
                std::lock_guard<std::mutex> lock(global_mutex);
                auto it = socket_to_room.find(client_socket);
                if (it != socket_to_room.end()) {
                    int opp = it->second->opponent_of(client_socket);
                    send_msg(opp, msg);
                }
            }
        }
    }

disconnect:
    std::cout << "Client " << client_socket << " disconnected." << std::endl;
    leave_room(client_socket);
    close(client_socket);
}

// ─────────────────────────────────────────────
//  Main
// ─────────────────────────────────────────────
int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(4242);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return 1;
    }

    listen(server_fd, 10);
    std::cout << "Server listening on port 4242 (multi-room mode)" << std::endl;

    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        std::thread(handle_client, client_socket).detach();
    }

    close(server_fd);
    return 0;
}