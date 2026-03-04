#ifndef TETRISONLINE_GAME_H
#define TETRISONLINE_GAME_H

#include <random>
#include <string>
#include <vector>
#include <algorithm>
#include "objets.h"
#include "network.h"

// ═══════════════════════════════════════════════════════════════
//  LAYOUT — coordonnées pixel partagées avec main.cpp
//  Fenêtre      : 780 × 800
//  Grille       : ox=20, oy=60  → 300×600px (10×20 cellules × 30px)
//  Panneau droit: x=340, y=60, w=432
//    Score      : x=340, y= 60, w=200, h=50
//    Level      : x=550, y= 60, w=222, h=50
//    Next       : x=340, y=120, w=180, h=140
//    Controles  : x=530, y=124
//    Message    : y=272
//    Bouton     : y=295, h=42    → fin=337
//    Adversaire : y=347, h=70    → fin=417  (online)
//    Chat btns  : y=427, h=100   → fin=527  (online)
//    Messages   : y=535, h=210   → fin=745  (online)
//    Volume     : y=755
// ═══════════════════════════════════════════════════════════════

static const int   GX        = 20;
static const int   GY        = 60;
static const int   NX        = 340;
static const int   NY        = 120;
static const int   NW        = 180;
static const int   NH        = 140;
static const int   CHAT_BUF  = 20;
static const int   CHAT_SHOW =  8;
static const float FLASH_DUR = 0.35f;

// ═══════════════════════════════════════════════════════════════
class Game {

    // ───────────────────────────────────────────────────────────
    //  PRIVÉ — jamais accédé directement depuis main.cpp
    // ───────────────────────────────────────────────────────────

    int         score_;
    int         level_;
    std::string msg_;

    bool  flashing_;     // animation de flash en cours
    float flashTimer_;   // temps restant avant destruction

    bool scoreChanged_;  // score à broadcaster (online)
    int  linesToSend_;   // lignes de pénalité à envoyer (online)

    std::vector<Piece> bag_; // 7-bag randomizer

    int  chat_total_;
    std::vector<std::string> chat_buf_;
    std::vector<bool>        chat_received_;

    // ─── 7-bag : mélange de toutes les pièces ─────────────────
    static std::vector<Piece> make_bag() {
        Piece T,O,I,J,L,S,Z;
        T.make_T(); O.make_O(); I.make_I();
        J.make_J(); L.make_L(); S.make_S(); Z.make_Z();
        std::vector<Piece> b={T,O,I,J,L,S,Z};
        static std::mt19937 rng(std::random_device{}());
        for(int i=6;i>0;i--){
            std::uniform_int_distribution<int> d(0,i);
            std::swap(b[i],b[d(rng)]);
        }
        return b;
    }

    Piece pull() {
        if(bag_.empty()) bag_=make_bag();
        Piece p=bag_.back(); bag_.pop_back(); return p;
    }

    // ─── Score (table Tetris standard) ────────────────────────
    void add_score(int lines) {
        static const int BASE[5]={0,40,100,300,1200};
        score_ += BASE[std::min(lines,4)] * (level_+1);
        level_  = score_/1000;
        scoreChanged_=true;
    }

    // ─── Poser la pièce : fusion + tirage suivant ─────────────
    void place() {
        grid.merge(current);
        current.clear();
        current = next;
        next    = pull();
        if(!current.no_overlap(grid)){
            msg_="GAME OVER"; justLost=true; current.clear();
        }
    }

    // ─── Ligne de pénalité avec un trou aléatoire ─────────────
    void add_garbage() {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> d(0,grid.W-1);
        int hole=d(rng), target=grid.H-1;
        for(int j=grid.H-1;j>=0;j--){
            bool empty=true;
            for(int i=0;i<grid.W;i++) if(grid.cells[i][j]){empty=false;break;}
            if(empty){target=j;break;}
        }
        for(int i=0;i<grid.W;i++) if(i!=hole) grid.cells[i][target]=1;
    }

    // ─── Buffer circulaire chat ────────────────────────────────
    void push_chat(const std::string& text, bool received){
        chat_buf_[chat_total_%CHAT_BUF]      = text;
        chat_received_[chat_total_%CHAT_BUF] = received;
        chat_total_++;
    }

    // ───────────────────────────────────────────────────────────
    //  PUBLIC
    // ───────────────────────────────────────────────────────────
public:
    Piece grid;
    Piece current;
    Piece next;

    bool justLost;   // spawn en collision → partie perdue
    int  opp_score;
    int  opp_level;

    // Audio public pour SetMusicVolume / SetSoundVolume dans main
    Music music;
    Sound snd_rotate;
    Sound snd_line;

    // ── Getters ───────────────────────────────────────────────
    int         score()    const { return score_;    }
    int         level()    const { return level_;    }
    std::string msg()      const { return msg_;      }
    bool        flashing() const { return flashing_; }
    void        set_msg(const std::string& m) { msg_=m; }

    // ─── Constructeur ─────────────────────────────────────────
    Game() : score_(0), level_(0), msg_(""),
             flashing_(false), flashTimer_(0),
             scoreChanged_(false), linesToSend_(0),
             justLost(false), opp_score(0), opp_level(0),
             chat_total_(0)
    {
        bag_    = make_bag();
        current = pull();
        next    = pull();
        chat_buf_      = std::vector<std::string>(CHAT_BUF,"");
        chat_received_ = std::vector<bool>(CHAT_BUF,false);
        music      = LoadMusicStream("sounds/cover.wav");
        snd_rotate = LoadSound("sounds/rotate.wav");
        snd_line   = LoadSound("sounds/destroy.wav");
        PlayMusicStream(music);
    }

    ~Game(){
        UnloadMusicStream(music);
        UnloadSound(snd_rotate);
        UnloadSound(snd_line);
    }

    // ─── Reset ────────────────────────────────────────────────
    void reset() {
        grid=Piece(); bag_=make_bag(); current=pull(); next=pull();
        score_=0; level_=0; msg_="";
        flashing_=false; flashTimer_=0;
        scoreChanged_=false; linesToSend_=0;
        justLost=false; opp_score=0; opp_level=0;
        chat_total_=0;
        for(int i=0;i<CHAT_BUF;i++){ chat_buf_[i].clear(); chat_received_[i]=false; }
    }

    // ═══════════════════════════════════════════════════════════
    //  LOGIQUE — une méthode par responsabilité
    // ═══════════════════════════════════════════════════════════

    // Avance le timer de flash et finalise quand écoulé.
    void tick_flash(float dt) {
        if(!flashing_) return;
        flashTimer_-=dt;
        if(flashTimer_>0) return;
        int nb=grid.destroy_full_lines();
        if(nb>0){
            StopSound(snd_line); PlaySound(snd_line);
            add_score(nb);
            linesToSend_+=nb;
        }
        flashing_=false; flashTimer_=0;
    }

    // Déclenche le flash si des lignes sont complètes.
    void tick_lines() {
        if(justLost||flashing_) return;
        if(!grid.full_lines().empty()){
            flashing_=true; flashTimer_=FLASH_DUR;
        }
    }

    // Descente automatique par gravité.
    void tick_gravity() {
        if(justLost||flashing_) return;
        if(current.hits_bottom(grid)) place();
        else                          current.move_down();
    }

    // Input clavier.
    void input() {
        if(justLost||flashing_) return;
        switch(GetKeyPressed()){
            case KEY_RIGHT: if(current.can_go_right(grid)) current.move_right(); break;
            case KEY_LEFT:  if(current.can_go_left(grid))  current.move_left();  break;
            case KEY_UP:
                if(current.can_rotate(grid)){
                    StopSound(snd_rotate); PlaySound(snd_rotate);
                    current.rotate();
                }
                break;
            case KEY_DOWN:
                if(current.hits_bottom(grid)) place();
                else current.move_down();
                break;
            case KEY_ENTER: hard_drop(); break;
            default: break;
        }
    }

    void hard_drop() {
        if(justLost||flashing_||!current.no_overlap(grid)) return;
        int n=0;
        while(!current.hits_bottom(grid)){ current.move_down(); n++; }
        score_+=n*2; scoreChanged_=true;
        place();
    }

    // ── Helpers broadcast online ──────────────────────────────
    // Retourne les lignes à envoyer et remet le compteur à 0.
    int pop_lines() { int n=linesToSend_; linesToSend_=0; return n; }

    // Retourne true si le score a changé, remet le flag à false.
    bool pop_score_changed() {
        if(!scoreChanged_) return false;
        scoreChanged_=false; return true;
    }

    // ═══════════════════════════════════════════════════════════
    //  RÉSEAU
    // ═══════════════════════════════════════════════════════════

    // Traite un message réseau. Retourne true = adversaire perdu.
    bool apply_net(const std::string& m){
        if(m.rfind("LINES|",0)==0){
            int n=std::stoi(m.substr(6));
            for(int i=0;i<n;i++) add_garbage();
            msg_="ATTAQUE !"; return false;
        }
        if(m=="GAMEOVER")            { msg_="VICTOIRE !"; return true; }
        if(m.rfind("CHAT|",0)==0)    { push_chat(m.substr(5),true); return false; }
        if(m.rfind("SCORE|",0)==0){
            auto d=m.substr(6); auto sep=d.find('|');
            if(sep!=std::string::npos){
                opp_score=std::stoi(d.substr(0,sep));
                opp_level=std::stoi(d.substr(sep+1));
            }
            return false;
        }
        return false;
    }

    // Envoyer un message chat depuis le joueur local.
    void send_chat(const std::string& text){ push_chat(text,false); }

    // ═══════════════════════════════════════════════════════════
    //  RENDU
    // ═══════════════════════════════════════════════════════════

    // Grille : fond + blocs posés + ghost + pièce + flash
    void draw_grid() const {
        float alpha = flashing_ ? (flashTimer_/FLASH_DUR) : 0;
        grid.draw_background(GX,GY);
        grid.draw_blocks(GX,GY);
        if(justLost) return;
        if(!current.no_overlap(grid)) return;

        Piece ghost=current;
        while(!ghost.hits_bottom(grid)) ghost.move_down();
        auto Pg=ghost.pos(), Pc=current.pos();
        bool same=true;
        for(int i=0;i<4;i++) if(Pg[0][i]!=Pc[0][i]||Pg[1][i]!=Pc[1][i]){same=false;break;}
        if(!same) ghost.draw_ghost(GX,GY);

        current.draw_piece(GX,GY);
        if(flashing_&&alpha>0) grid.draw_flash(GX,GY,alpha);
    }

    // Pièce suivante centrée dans le cadre Next
    void draw_next() const {
        auto P=next.pos();
        int mnr=*std::min_element(P[0].begin(),P[0].end());
        int mnc=*std::min_element(P[1].begin(),P[1].end());
        int mxr=*std::max_element(P[0].begin(),P[0].end());
        int mxc=*std::max_element(P[1].begin(),P[1].end());
        int ox=NX+NW/2-(mxr-mnr+1)*next.CS/2;
        int oy=NY+NH/2-(mxc-mnc+1)*next.CS/2+12;
        for(int i=0;i<4;i++)
            DrawRectangle(ox+(P[0][i]-mnr)*next.CS,
                          oy+(P[1][i]-mnc)*next.CS,
                          next.CS-2,next.CS-2,
                          PALETTE[next.cells[P[0][i]][P[1][i]]]);
    }

    // Panneau adversaire — y=347..417
    void draw_opponent() const {
        DrawRectangleRounded({340,347,432,70},0.2f,6,Color{45,12,12,255});
        RLDrawRoundedLines(Rectangle{340,347,432,70},0.2f,6,Color{200,60,60,70});
        DrawText("ADVERSAIRE",352,355,15,Color{210,80,80,255});
        char sc[32],lv[32];
        snprintf(sc, sizeof(sc), "Score : %d", opp_score);
        snprintf(lv, sizeof(lv), "Lvl   : %d", opp_level);
        DrawText(sc,352,377,15,ORANGE);
        DrawText(lv,560,377,15,ORANGE);
    }

    // Messages chat — les CHAT_SHOW plus récents
    void draw_chat() const {
        const int Y0=553, LH=22;
        int total=std::min(chat_total_,CHAT_BUF);
        int show =std::min(total,CHAT_SHOW);
        for(int row=0;row<show;row++){
            int idx=((chat_total_-show+row)%CHAT_BUF+CHAT_BUF)%CHAT_BUF;
            const char* txt=chat_buf_[idx].c_str();
            int y=Y0+row*LH;
            if(chat_received_[idx]){
                DrawText(">",348,y,13,Color{150,80,30,255});
                DrawText(txt,364,y,13,ORANGE);
            } else {
                int x=std::max(364,758-MeasureText(txt,13));
                DrawText(txt,x,y,13,SKYBLUE);
                DrawText("<",762,y,13,Color{30,100,150,255});
            }
        }
    }
};

#endif // TETRISONLINE_GAME_H
