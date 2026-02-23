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
    bool scoreChanged;
    Music music;
    Sound rotate_sound;
    Sound destroy_sound;

    // Adversaire
    int opp_score;
    int opp_niveau;

    // Chat
    int max_chat;
    std::vector<std::string> chat_messages;
    std::vector<bool> chat_recu;

    // ─── Getters / Setters ────────────────────
    std::string get_msg()        const { return msg; }
    int         get_score()      const { return score; }
    int         get_niveau()     const { return niveau; }
    void set_msg(std::string m)        { msg = m; }
    void set_score(int s)              { score = s; scoreChanged = true; }
    void set_niveau(int n)             { niveau = n; }

    // ─── Constructeur ─────────────────────────
    Game() {
        grid    = object();
        objs    = get_all_objects();
        current = get_random_object();
        next    = get_random_object();
        score        = 0;
        niveau       = 0;
        msg          = "";
        justLost     = false;
        linesToSend  = 0;
        scoreChanged = false;
        opp_score    = 0;
        opp_niveau   = 0;
        max_chat     = 0;
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

    // ─── Pièces ───────────────────────────────
    std::vector<object> get_all_objects() {
        object T,O,I,J,L,S,Z;
        T.make_T(); O.make_O(); I.make_I();
        J.make_J(); L.make_L(); S.make_S(); Z.make_Z();
        return {T,O,I,J,L,S,Z};
    }

    object get_random_object() {
        if (objs.empty()) objs = get_all_objects();
        // Générateur statique : initialisé une seule fois, évite les seeds identiques
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<> dist(0, (int)objs.size()-1);
        int idx = dist(gen);
        object obj = objs[idx];
        objs.erase(objs.begin()+idx);
        return obj;
    }

    // ─── Reset ────────────────────────────────
    void reset() {
        grid    = object();
        objs    = get_all_objects();
        current = get_random_object();
        next    = get_random_object();
        score        = 0;
        niveau       = 0;
        msg          = "";
        justLost     = false;
        linesToSend  = 0;
        scoreChanged = false;
        opp_score    = 0;
        opp_niveau   = 0;
        max_chat     = 0;
        for (int i=0;i<10;i++) { chat_messages[i].clear(); chat_recu[i]=false; }
    }

    // ─── Réseau ───────────────────────────────
    bool apply_network_message(const std::string& m) {
        if (m.rfind("LINES|",0)==0) {
            int n=std::stoi(m.substr(6));
            for(int i=0;i<n;i++) add_garbage_line();
            set_msg("ATTAQUE !"); return false;
        }
        if (m=="GAMEOVER") { set_msg("VICTOIRE !"); return true; }
        if (m.rfind("CHAT|",0)==0) {
            ajouter_msg(m.substr(5), true); max_chat++; return false;
        }
        if (m.rfind("SCORE|",0)==0) {
            std::string data=m.substr(6);
            size_t sep=data.find('|');
            if(sep!=std::string::npos){
                opp_score  = std::stoi(data.substr(0,sep));
                opp_niveau = std::stoi(data.substr(sep+1));
            }
            return false;
        }
        return false;
    }

    void add_garbage_line() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, grid.line-1);
        int hole = dist(gen);
        int target_col = grid.column-1;
        for(int j=grid.column-1;j>=0;j--){
            bool empty=true;
            for(int i=0;i<grid.line;i++) if(grid.matrice[i][j]!=0){empty=false;break;}
            if(empty){target_col=j;break;}
        }
        for(int i=0;i<grid.line;i++) if(i!=hole) grid.matrice[i][target_col]=1;
    }

    // ─── Chat ─────────────────────────────────
    void ajouter_msg(const std::string& message, bool recu) {
        if(max_chat>=10){
            for(int i=0;i<10;i++){chat_messages[i].clear();chat_recu[i]=false;}
            max_chat=0;
        }
        chat_messages[max_chat]=message;
        chat_recu[max_chat]=recu;
    }

    void draw_msg() {
        for(int i=0;i<max_chat;i++){
            float dec = chat_recu[i]?0.0f:120.0f;
            Color col = chat_recu[i]?RED:BLACK;
            DrawText(chat_messages[i].c_str(), 550+(int)dec, (int)(195+28*i), 15, col);
        }
    }

    // ─── Ghost piece ──────────────────────────
    object get_ghost() {
        object ghost = current;
        // Descendre jusqu'à collision
        while (!ghost.check_collision(grid))
            ghost.translate_bas();
        return ghost;
    }

    // ─── Rendu principal ──────────────────────
    // Ordre correct : grille → ghost → pièce courante
    void dessiner() {
        // 1. Grille de fond (toujours)
        grid.dessiner();

        // Si partie perdue ou current vide : grille seule
        if (justLost) return;

        // Ne pas afficher current si elle intersecte la grille (overlap)
        if (!current.checkintersection(grid)) return;

        // 2. Ghost (contour blanc)
        object ghost = get_ghost();
        auto Pg = ghost.get_pos();
        auto Pc = current.get_pos();
        bool same = true;
        for(int i=0;i<4;i++)
            if(Pg[0][i]!=Pc[0][i]||Pg[1][i]!=Pc[1][i]){same=false;break;}
        if (!same) {
            for(int i=0;i<4;i++){
                int x = Pg[0][i]*ghost.cellsize+1;
                int y = Pg[1][i]*ghost.cellsize+1;
                DrawRectangleLines(x, y, ghost.cellsize-1, ghost.cellsize-1, WHITE);
            }
        }

        // 3. Pièce courante
        current.dessiner_piece(WHITE, false);
    }

    // ─── Next ─────────────────────────────────
    void dessiner_next() {
        auto P = next.get_pos();
        int min_x=*std::min_element(P[0].begin(),P[0].end());
        int min_y=*std::min_element(P[1].begin(),P[1].end());
        int max_x=*std::max_element(P[0].begin(),P[0].end());
        int max_y=*std::max_element(P[1].begin(),P[1].end());

        int piece_w = (max_y-min_y+1)*next.cellsize;
        int piece_h = (max_x-min_x+1)*next.cellsize;

        // Cadre Next : {320, 260, 170, 140}
        int offset_x = 320+170/2 - piece_w/2;
        int offset_y = 260+140/2 - piece_h/2;

        auto colors = next.GetCellColors();
        for(int i=0;i<4;i++){
            int col    = next.matrice[P[0][i]][P[1][i]];
            int draw_x = (P[1][i]-min_y)*next.cellsize + offset_x;
            int draw_y = (P[0][i]-min_x)*next.cellsize + offset_y;
            DrawRectangle(draw_x, draw_y, next.cellsize-2, next.cellsize-2, colors[col]);
        }
    }

    // ─── Panneau adversaire ───────────────────
    void dessiner_opponent() {
        Color panel = {80, 40, 40, 255};
        DrawRectangleRounded({320, 480, 170, 90}, 0.3f, 6, panel);
        DrawText("Adversaire", 330, 487, 18, WHITE);
        char sc[32], lvl[32];
        sprintf(sc,  "Score: %d", opp_score);
        sprintf(lvl, "Level: %d", opp_niveau);
        DrawText(sc,  330, 513, 16, ORANGE);
        DrawText(lvl, 330, 535, 16, ORANGE);
    }

    // ─── Input ────────────────────────────────
    void input() {
        int key = GetKeyPressed();
        switch(key){
            case KEY_RIGHT: if(current.check_right(grid)) current.translate_d(); break;
            case KEY_LEFT:  if(current.check_left(grid))  current.translate_g(); break;
            case KEY_DOWN:
                if(current.check_collision(grid)&&!loose()&&current.checkintersection(grid)){
                    grid.add(current); current=next; next=get_random_object();
                } else current.translate_bas();
                break;
            case KEY_UP:   if(current.check_rotate(grid)) rotate(); break;
            case KEY_ENTER: hard_drop(); break;
            default: break;
        }
    }

    // Hard drop : fait tomber la pièce instantanément + bonus de score
    void hard_drop() {
        if (justLost || !current.checkintersection(grid)) return;
        int cells_dropped = 0;
        while (!current.check_collision(grid)) {
            current.translate_bas();
            cells_dropped++;
        }
        set_score(score + cells_dropped * 2);
        grid.add(current);
        current.set_zero();
        current = next;
        next = get_random_object();
        // Détecter la perte au spawn
        if (!current.checkintersection(grid)) {
            set_msg("GAME OVER");
            justLost = true;
            current.set_zero();
        }
    }

    void rotate() { PlaySound(rotate_sound); current.rotate(); }

    // ─── Gravité ──────────────────────────────
    void move_down() {
        if (justLost) return;
        if (current.check_collision(grid)) {
            if (current.checkintersection(grid)) {
                grid.add(current);
                current.set_zero();
                current = next;
                next = get_random_object();
                // Détecter la perte au spawn : si la nouvelle pièce intersecte déjà
                if (!current.checkintersection(grid)) {
                    set_msg("GAME OVER");
                    justLost = true;
                    current.set_zero();
                }
            }
        } else {
            current.translate_bas();
        }
    }

    // ─── Défaite ──────────────────────────────
    bool loose() {
        for(int i=0;i<grid.line;i++){
            if(grid.matrice[i][0]!=0){
                if(!justLost){set_msg("GAME OVER");justLost=true;}
                return true;
            }
        }
        return false;
    }

    // ─── Score ────────────────────────────────
    int calcscore(int nb, int niv) {
        int base=0;
        if(nb==1)base=40; else if(nb==2)base=100;
        else if(nb==3)base=300; else if(nb==4)base=1200;
        return base*(niv+1);
    }

    void destroy() {
        int nb=grid.destroy();
        if(nb!=0){
            PlaySound(destroy_sound);
            int sc=score+calcscore(nb,niveau);
            set_score(sc);
            set_niveau(sc/1000);
            linesToSend+=nb;
        }
    }
};

#endif // TETRISONLINE_GAME_H
