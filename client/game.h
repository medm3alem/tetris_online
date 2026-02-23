#ifndef TETRISONLINE_GAME_H
#define TETRISONLINE_GAME_H

#include <random>
#include "objets.h"
#include <vector>
#include <string>
#include <set>
#include "network.h"

class Game {

private:
    int score;
    int niveau;
    std::string msg;

public:
    object grid;
    std::vector<object> objs;
    object current;
    object next;
    bool justLost;
    int linesToSend;
    Music music;
    Sound rotate_sound;
    Sound destroy_sound;

    // Chat
    int max_chat;
    std::vector<std::string> chat_messages;
    std::vector<bool> chat_recu;

    // ─── Getters / Setters ────────────────────────
    std::string get_msg()   const { return msg; }
    int get_score()         const { return score; }
    int get_niveau()        const { return niveau; }
    void set_msg(std::string m)   { msg = m; }
    void set_score(int s)         { score = s; }
    void set_niveau(int n)        { niveau = n; }

    // ─── Constructeur / Destructeur ───────────────
    Game() {
        grid    = object();
        objs    = get_all_objects();
        current = get_random_object();
        next    = get_random_object();
        set_score(0);
        set_niveau(0);
        set_msg("");
        justLost    = false;
        linesToSend = 0;
        max_chat    = 0;
        chat_messages = std::vector<std::string>(10, "");
        chat_recu     = std::vector<bool>(10, false);

        music         = LoadMusicStream("sounds/cover.wav");
        rotate_sound  = LoadSound("sounds/rotate.wav");
        destroy_sound = LoadSound("sounds/destroy.wav");
        PlayMusicStream(music);
    }

    ~Game() {
        UnloadMusicStream(music);
        UnloadSound(destroy_sound);
        UnloadSound(rotate_sound);
    }

    // ─── Pièces ───────────────────────────────────
    std::vector<object> get_all_objects() {
        object objT, objO, objI, objJ, objL, objS, objZ;
        objT.make_T(); objO.make_O(); objI.make_I();
        objJ.make_J(); objL.make_L(); objS.make_S(); objZ.make_Z();
        return {objT, objO, objI, objJ, objL, objS, objZ};
    }

    object get_random_object() {
        if (objs.empty()) objs = get_all_objects();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, (int)objs.size() - 1);

        int idx = dist(gen);
        object obj = objs[idx];
        objs.erase(objs.begin() + idx);
        return obj;
    }

    // ─── Reset ────────────────────────────────────
    // Remet le jeu à zéro sans toucher aux sons
    void reset() {
        grid    = object();
        objs    = get_all_objects();
        current = get_random_object();
        next    = get_random_object();
        set_score(0);
        set_niveau(0);
        set_msg("");
        justLost    = false;
        linesToSend = 0;
        max_chat    = 0;
        for (int i = 0; i < 10; i++) {
            chat_messages[i].clear();
            chat_recu[i] = false;
        }
    }

    // ─── Réseau ───────────────────────────────────
    // Retourne true si la partie online est terminée (victoire)
    bool apply_network_message(const std::string& msg) {
        if (msg.rfind("LINES|", 0) == 0) {
            int n = std::stoi(msg.substr(6));
            for (int i = 0; i < n; i++) add_garbage_line();
            set_msg("ATTAQUE !");
            return false;
        }
        if (msg == "GAMEOVER") {
            set_msg("VICTOIRE !");
            return true;   // signale la fin à main.cpp
        }
        if (msg.rfind("CHAT|", 0) == 0) {
            ajouter_msg(msg.substr(5), true);
            max_chat++;
            return false;
        }
        return false;
    }

    void add_garbage_line() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, grid.line - 1);
        int hole = dist(gen);

        // Trouver la première colonne vide en partant de la droite
        int target_col = grid.column - 1;
        for (int j = grid.column - 1; j >= 0; j--) {
            bool empty = true;
            for (int i = 0; i < grid.line; i++)
                if (grid.matrice[i][j] != 0) { empty = false; break; }
            if (empty) { target_col = j; break; }
        }

        for (int i = 0; i < grid.line; i++)
            if (i != hole) grid.matrice[i][target_col] = 1;
    }

    // ─── Chat ─────────────────────────────────────
    void ajouter_msg(const std::string& message, bool recu) {
        if (max_chat >= 10) {
            for (int i = 0; i < 10; i++) {
                chat_messages[i].clear();
                chat_recu[i] = false;
            }
            max_chat = 0;
        }
        chat_messages[max_chat] = message;
        chat_recu[max_chat]     = recu;
    }

    void draw_msg() {
        for (int i = 0; i < max_chat; i++) {
            float  dec = chat_recu[i] ? 0.0f : 120.0f;
            Color  col = chat_recu[i] ? RED : BLACK;
            float  y   = 195 + 28 * i;
            DrawText(chat_messages[i].c_str(), 550 + (int)dec, (int)y, 15, col);
        }
    }

    // ─── Rendu ────────────────────────────────────
    void dessiner() {
        object temp = grid;
        if (current.checkintersection(grid)) temp.add(current);
        if (!loose()) temp.dessiner();
        else          grid.dessiner();
    }

    void dessiner_next() {
        std::vector<std::vector<int>> P = next.get_pos();
        for (int i = 0; i < 4; i++) {
            int col = next.matrice[P[0][i]][P[1][i]];
            DrawRectangle(P[0][i] * next.cellsize + 255,
                          P[1][i] * next.cellsize + 281,
                          next.cellsize - 1, next.cellsize - 1,
                          next.GetCellColors()[col]);
        }
    }

    // ─── Input ────────────────────────────────────
    void input() {
        int key = GetKeyPressed();
        switch (key) {
            case KEY_RIGHT:
                if (current.check_right(grid)) current.translate_d();
                break;
            case KEY_LEFT:
                if (current.check_left(grid))  current.translate_g();
                break;
            case KEY_DOWN:
                if (current.check_collision(grid) && !loose() && current.checkintersection(grid)) {
                    grid.add(current);
                    current = next;
                    next    = get_random_object();
                } else {
                    current.translate_bas();
                }
                break;
            case KEY_UP:
                if (current.check_rotate(grid)) rotate();
                break;
            case KEY_ENTER:
                current.translate_haut();
                break;
            default:
                break;
        }
    }

    void rotate() {
        PlaySound(rotate_sound);
        current.rotate();
    }

    // ─── Gravité ──────────────────────────────────
    void move_down() {
        if (loose()) return;

        if (!current.checkintersection(grid)) {
            // Pièce en intersection : la remonter
            std::vector<std::vector<int>> P = current.get_pos();
            std::set<int> cols(P[1].begin(), P[1].end());
            for (int j = 0; j < (int)cols.size(); j++) {
                for (int i = 0; i < current.line; i++) {
                    current.matrice[i][j] = 0;
                    current.translate_haut();
                }
            }
        }

        if (current.check_collision(grid)) {
            grid.add(current);
            if (!loose()) {
                current = next;
                next    = get_random_object();
            }
        } else {
            current.translate_bas();
        }
    }

    // ─── Défaite ──────────────────────────────────
    bool loose() {
        for (int i = 0; i < grid.line; i++) {
            if (grid.matrice[i][0] != 0) {
                if (!justLost) {
                    set_msg("GAME OVER");
                    justLost = true;
                }
                return true;
            }
        }
        return false;
    }

    // ─── Score ────────────────────────────────────
    int calcscore(int nb, int niv) {
        int base = 0;
        if (nb == 1) base = 40;
        else if (nb == 2) base = 100;
        else if (nb == 3) base = 300;
        else if (nb == 4) base = 1200;
        return base * (niv + 1);
    }

    void destroy() {
        int nb = grid.destroy();
        if (nb != 0) {
            PlaySound(destroy_sound);
            int sc = get_score() + calcscore(nb, get_niveau());
            set_score(sc);
            set_niveau(sc / 1000);
            linesToSend += nb;
        }
    }
};

#endif // TETRISONLINE_GAME_H
