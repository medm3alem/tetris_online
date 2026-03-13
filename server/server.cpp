#include "server.h"
#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>   // TCP_NODELAY
#include <algorithm>
#include <mutex>
#include <memory>
#include <map>
#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// ─────────────────────────────────────────────
//  Config
// ─────────────────────────────────────────────
static constexpr int    PORT            = 4242;
static constexpr int    MAX_CLIENTS     = 100;
static constexpr int    PING_INTERVAL_S = 10;   // envoyer PING toutes les 10s
static constexpr int    PONG_TIMEOUT_S  = 30;   // kick si pas de PONG depuis 30s
static constexpr size_t MAX_MSG_SIZE    = 4096;  // taille max d'un message

// ─────────────────────────────────────────────
//  Logs structurés
// ─────────────────────────────────────────────
static std::mutex log_mutex;

static std::string now_str() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&t), "%H:%M:%S");
    return ss.str();
}

template<typename... Args>
static void log(const std::string& level, Args&&... args) {
    std::lock_guard<std::mutex> lk(log_mutex);
    std::cout << "[" << now_str() << "] [" << level << "] ";
    (std::cout << ... << args);
    std::cout << "\n";
}

#define LOG_INFO(...)  log("INFO ", __VA_ARGS__)
#define LOG_WARN(...)  log("WARN ", __VA_ARGS__)
#define LOG_ERROR(...) log("ERROR", __VA_ARGS__)

// ─────────────────────────────────────────────
//  Room
// ─────────────────────────────────────────────
struct Room {
    int id;
    int players[2]   = {-1, -1};
    int player_count = 0;
    bool started     = false;

    bool is_full()  const { return player_count == 2; }
    bool is_empty() const { return player_count == 0; }

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
//  ClientState — suivi par socket
// ─────────────────────────────────────────────
struct ClientState {
    std::chrono::steady_clock::time_point last_pong;
    bool waiting_pong = false;

    void reset_pong() {
        last_pong     = std::chrono::steady_clock::now();
        waiting_pong  = false;
    }

    bool is_timed_out() const {
        auto age = std::chrono::steady_clock::now() - last_pong;
        return std::chrono::duration_cast<std::chrono::seconds>(age).count() > PONG_TIMEOUT_S;
    }
};

// ─────────────────────────────────────────────
//  État global
// ─────────────────────────────────────────────
std::mutex                              global_mutex;
std::map<int, std::shared_ptr<Room>>    socket_to_room;
std::map<int, ClientState>              client_states;
std::vector<std::shared_ptr<Room>>      waiting_rooms;
std::atomic<int>                        next_room_id{1};
std::atomic<int>                        client_count{0};

// ─────────────────────────────────────────────
//  Helpers réseau
// ─────────────────────────────────────────────
static void send_msg(int sock, const std::string& msg) {
    if (sock < 0) return;
    ssize_t sent = send(sock, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    if (sent < 0)
        LOG_WARN("send() failed on fd=", sock, ": ", strerror(errno));
}

static void enable_tcp_nodelay(int sock) {
    int flag = 1;
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0)
        LOG_WARN("TCP_NODELAY failed on fd=", sock);
}

// ─────────────────────────────────────────────
//  Matchmaking
// ─────────────────────────────────────────────
static std::shared_ptr<Room> join_or_create_room(int sock) {
    std::lock_guard<std::mutex> lock(global_mutex);

    for (auto it = waiting_rooms.begin(); it != waiting_rooms.end(); ++it) {
        auto room = *it;
        if (!room->is_full()) {
            room->add_player(sock);
            socket_to_room[sock] = room;
            waiting_rooms.erase(it);
            room->started = true;
            LOG_INFO("[Room ", room->id, "] Match start! ",
                     room->players[0], " vs ", room->players[1]);
            send_msg(room->players[0], "MATCH_START\n");
            send_msg(room->players[1], "MATCH_START\n");
            return room;
        }
    }

    auto room  = std::make_shared<Room>();
    room->id   = next_room_id++;
    room->add_player(sock);
    socket_to_room[sock] = room;
    waiting_rooms.push_back(room);
    LOG_INFO("[Room ", room->id, "] Created, waiting for opponent (fd=", sock, ")");
    return room;
}

static void leave_room(int sock) {
    std::lock_guard<std::mutex> lock(global_mutex);

    client_states.erase(sock);

    auto it = socket_to_room.find(sock);
    if (it == socket_to_room.end()) return;

    auto room = it->second;
    int  opp  = room->opponent_of(sock);

    room->remove_player(sock);
    socket_to_room.erase(it);

    waiting_rooms.erase(
        std::remove_if(waiting_rooms.begin(), waiting_rooms.end(),
            [](const std::shared_ptr<Room>& r){ return r->is_empty(); }),
        waiting_rooms.end());

    if (opp >= 0) {
        send_msg(opp, "OPPONENT_LEFT\n");
        LOG_INFO("[Room ", room->id, "] fd=", sock, " left, notified fd=", opp);
    }
}

// ─────────────────────────────────────────────
//  Thread watchdog ping
//  Envoie PING à tous les clients toutes les
//  PING_INTERVAL_S secondes et kick ceux qui
//  ne répondent pas dans PONG_TIMEOUT_S.
// ─────────────────────────────────────────────
static void ping_watchdog() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(PING_INTERVAL_S));

        std::vector<int> to_kick;
        {
            std::lock_guard<std::mutex> lock(global_mutex);
            for (auto& [sock, state] : client_states) {
                if (state.is_timed_out()) {
                    to_kick.push_back(sock);
                } else if (!state.waiting_pong) {
                    send_msg(sock, "PING\n");
                    state.waiting_pong = true;
                }
            }
        }

        for (int sock : to_kick) {
            LOG_WARN("Timeout: kicking fd=", sock);
            shutdown(sock, SHUT_RDWR);
            close(sock);
        }
    }
}

// ─────────────────────────────────────────────
//  Thread par client
// ─────────────────────────────────────────────
static void handle_client(int client_socket) {
    enable_tcp_nodelay(client_socket);

    {
        std::lock_guard<std::mutex> lock(global_mutex);
        ClientState cs;
        cs.reset_pong();
        client_states[client_socket] = cs;
    }

    LOG_INFO("Client fd=", client_socket, " connected (total=", client_count.load(), ")");

    char        buffer[512];
    std::string accumulated;

    auto process_message = [&](const std::string& msg) -> bool {
        // Mise à jour du heartbeat à chaque message reçu
        {
            std::lock_guard<std::mutex> lock(global_mutex);
            auto it = client_states.find(client_socket);
            if (it != client_states.end())
                it->second.reset_pong();
        }

        if (msg == "READY\n") {
            join_or_create_room(client_socket);

        } else if (msg == "PONG\n") {
            // heartbeat répondu — déjà géré par reset_pong() ci-dessus

        } else if (msg == "QUIT\n") {
            return false;  // déconnexion propre

        } else {
            // Vérification taille max
            if (msg.size() > MAX_MSG_SIZE) {
                LOG_WARN("Message too large from fd=", client_socket, ", dropping");
                return true;
            }
            // Relai à l'adversaire
            std::lock_guard<std::mutex> lock(global_mutex);
            auto it = socket_to_room.find(client_socket);
            if (it != socket_to_room.end()) {
                int opp = it->second->opponent_of(client_socket);
                send_msg(opp, msg);
            }
        }
        return true;
    };

    while (true) {
        int n = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;

        buffer[n] = '\0';
        accumulated += buffer;

        // Protection contre accumulation excessive (client malicieux)
        if (accumulated.size() > MAX_MSG_SIZE * 4) {
            LOG_WARN("Buffer overflow from fd=", client_socket, ", disconnecting");
            break;
        }

        size_t pos;
        while ((pos = accumulated.find('\n')) != std::string::npos) {
            std::string msg = accumulated.substr(0, pos + 1);
            accumulated     = accumulated.substr(pos + 1);

            if (!process_message(msg))
                goto disconnect;
        }
    }

disconnect:
    LOG_INFO("Client fd=", client_socket, " disconnected");
    leave_room(client_socket);
    close(client_socket);
    --client_count;
}

// ─────────────────────────────────────────────
//  Main
// ─────────────────────────────────────────────
int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        LOG_ERROR("Socket creation failed: ", strerror(errno));
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(PORT);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        LOG_ERROR("Bind failed: ", strerror(errno));
        return 1;
    }

    listen(server_fd, 10);
    LOG_INFO("Server listening on port ", PORT,
             " (max_clients=", MAX_CLIENTS,
             ", ping=", PING_INTERVAL_S, "s",
             ", timeout=", PONG_TIMEOUT_S, "s)");

    // Thread watchdog ping (détaché)
    std::thread(ping_watchdog).detach();

    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            LOG_ERROR("Accept failed: ", strerror(errno));
            continue;
        }

        if (client_count.load() >= MAX_CLIENTS) {
            LOG_WARN("Max clients reached, refusing fd=", client_socket);
            send_msg(client_socket, "SERVER_FULL\n");
            close(client_socket);
            continue;
        }

        ++client_count;
        std::thread(handle_client, client_socket).detach();
    }

    close(server_fd);
    return 0;
}
