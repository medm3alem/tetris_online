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
    bool waiting;
    int linesToSend;
    bool mode;
    bool start;
    bool fin_partie_online;
    Music music;
    Sound rotate_sound;
    Sound destroy_sound;
    int max_chat = 0;

    std::string get_msg() {
        return msg;
    }
    int get_score() {
        return score;
    }
    int get_niveau() {
        return niveau;
    }

    void set_msg(std::string m) {
        msg = m;
    }

    void set_score(int s) {
        score = s;
    }
    void set_niveau(int n) {
        niveau = n;
    }


    std::vector<object> get_all_objects() {
        object objT = object();
        objT.make_T();
        object objO = object();
        objO.make_O();
        object objI = object();
        objI.make_I();
        object objJ = object();
        objJ.make_J();
        object objL = object();
        objL.make_L();
        object objS = object();
        objS.make_S();
        object objZ = object();
        objZ.make_Z();
        return  {objT, objO, objI, objJ, objL, objS, objZ};
    }

    object get_random_object() {
        if (objs.empty()) {
            objs = get_all_objects();
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, objs.size() -1);

        int rand_num = dist(gen);
        object myobj= objs[rand_num];
        objs.erase(objs.begin() + rand_num);
        return myobj;
    }

    Game() {
        grid = object();
        objs = get_all_objects();
        current = get_random_object();
        next = get_random_object();
        set_score(0);
        set_niveau(0);
        set_msg("");
        justLost = false;
        linesToSend = 0;
        music = LoadMusicStream("sounds/cover.wav");
        PlayMusicStream(music);
        rotate_sound = LoadSound("sounds/rotate.wav");
        destroy_sound = LoadSound("sounds/destroy.wav");
        mode = false;
        start = false;
        fin_partie_online = false;
        waiting = true;
        int max_chat = 0;
    }

    ~Game() {
        UnloadMusicStream(music);
        UnloadSound(destroy_sound);
        UnloadSound(rotate_sound);
    }

    void apply_network_message(const std::string& msg) {
        if (msg.rfind("LINES|", 0) == 0) {
            int n = std::stoi(msg.substr(6));
            for (int i = 0; i < n; i++)
                add_garbage_line();
            set_msg("ATTACK !");
        }
        else if (msg == "GAMEOVER") {
            set_msg("VICTOIRE!");
            fin_partie_online = true;
            start = false;
        }
        else if (msg.rfind("CHAT|", 0) == 0) {
            std::string chat_msg = msg.substr(5);
            draw_msg(chat_msg, max_chat, true);
            max_chat +=1;
        }
    }
    void draw_msg(std::string message, int x, bool recu){
        // (530, 180, 230, 300)
        // à ajouter : faire défiler les messages si trop nombreux en fonction de max_chat
        int dec = recu ?  130 : 20;
        COLOR col = recu ? RED : DARKGRAY;
        Rectangle cadre = { 530, 70*x + dec, 80, 50 };
        DrawRectangleRounded(cadre, 0.3f, 6, WHITE);
        DrawText(message.c_str(), cadre.x + 10, cadre.y + 13, 15, col);
    }

    void add_garbage_line() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, grid.line -1);

        int rand_num = dist(gen);
        int i=grid.line-1;
        int j=grid.column-1;
        bool found = false;
        for (; j >= 0; j--) {
            i=grid.line-1;
            found = true;
            for (; i>=0; i--) {
                if (grid.matrice[i][j]!=0) found = false;
            }
            if (found) break ;
        }
        for (int i = 0; i<grid.line; i++) if (i!=rand_num) grid.matrice[i][j] = 1;
    }

    void dessiner() {
        int x = grid.line;
        int y = grid.column;
        object temp1 = object(x, y);
        object temp2 = object(x, y);
        temp1 = grid;
        temp2 = current;
        if (temp2.checkintersection(temp1)) temp1.add(temp2);
        if (!loose()) temp1.dessiner();
        else grid.dessiner();
    }

    void reset() {
        grid = object();
        objs = get_all_objects();
        current = get_random_object();
        next = get_random_object();
        set_score(0);
        set_niveau(0);
        set_msg("");
        justLost = false;
        linesToSend = 0;
        mode = false;
        start = false;
        fin_partie_online = false;
        waiting = true;
        int max_chat = 0;
    }

    void input() {
        int pressed_key = GetKeyPressed();
        if (pressed_key != 0 && loose()) {
            reset();
        }
        switch (pressed_key) {
            case KEY_RIGHT:
                if (current.check_right(grid)) {
                    current.translate_d();
                    break;
                }
                else break;

            case KEY_LEFT:
                if (current.check_left(grid)) {
                    current.translate_g();
                    break;
                }
                else break;

            case KEY_DOWN:
                if (current.check_collision(grid)==true && !loose() && current.checkintersection(grid)) {
                    grid.add(current);
                    current = next;
                    next = get_random_object();
                }
                else current.translate_bas();
                break;

            case KEY_UP:
                if (current.check_rotate(grid)) {
                    rotate();
                    break;
                }
                else break;

            case KEY_ENTER:
                current.translate_haut();
                break;
        }
    }

    void rotate() {
        PlaySound(rotate_sound);
        current.rotate();
    }

    void move_down() {
        if (!loose()) {
            if (!current.checkintersection(grid)) {
                std::vector<std::vector<int>> P = current.get_pos();
                std::set<int> s(P[1].begin(), P[1].end());
                int n = s.size();
                for (int j = 0; j<n; j++) {
                    for (int i=0; i<current.line; i++) {
                        current.matrice[i][j] = 0;
                        current.translate_haut();
                    }
                }
            }
            if (current.check_collision(grid)) {
                grid.add(current);
                if (!loose()) {
                    current = next;
                    next = get_random_object();
                }
            }
            else {
                current.translate_bas();
            }
        }
    }

    bool loose() {
        for (int i=0; i < grid.line; i++) {
            if (grid.matrice[i][0]!=0) {
                if (!justLost) {
                    set_msg("GAME OVER");
                    justLost = true;
                }
                return true;
            }
        }
        return false;
    }

    int calcscore0(int nb) {
        if (nb==1) return 40;
        if (nb==2) return 100-40;
        if (nb==3) return 300-100-40;
        if (nb==4) return 1200-200;
        return 0;
    }

    int calcscore(int nb, int niv) {
        int p = calcscore0(nb);
        return p*(niv+1);
    }

    void dessiner_next() {
        std::vector<std::vector<int>> P = next.get_pos();
        for (int i=0; i < 4; i++) {
            int ind_col = next.matrice[P[0][i]][P[1][i]];
            DrawRectangle((P[0][i])*next.cellsize +255, P[1][i]*next.cellsize +281, next.cellsize-1, next.cellsize-1, next.GetCellColors()[ind_col]);
        }
    }

    void destroy() {
        int nb = grid.destroy();
        if (nb!=0) {
            PlaySound(destroy_sound);
            int niv = get_niveau();
            int sc = get_score() + calcscore(nb, niv);
            set_score(sc) ;
            set_niveau(sc/1000);
            linesToSend = linesToSend + nb;
        }
    }
};

#endif //TETRISONLINE_GAME_H
