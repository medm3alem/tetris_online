#ifndef TETRISONLINE_GAME_H
#define TETRISONLINE_GAME_H

#include <random>
#include <string>
#include <vector>
#include <algorithm>
#include "objets.h"
#include "network.h"

// ═══════════════════════════════════════════════════════════════
//  LAYOUT
//  Fenêtre      : 780 × 800
//  Grille       : ox=20, oy=60  → 300×600px (10×20 cellules × 30px)
//  Panneau droit: x=340, y=60, w=432
//    Score      : x=340, y= 60, w=200, h=50
//    Level      : x=550, y= 60, w=222, h=50
//    Next       : x=340, y=120, w=180, h=140
//    Controles  : x=530, y=124
//    Message    : y=272
//    Bouton     : y=295, h=42
//    Adversaire : y=347, h=100   (agrandi pour plus d'infos)
//    Chat btns  : y=457, h=100
//    Messages   : y=565, h=180
//    Volume     : y=757
// ═══════════════════════════════════════════════════════════════

static const int   GX        = 20;
static const int   GY        = 60;
static const int   NX        = 340;
static const int   NY        = 120;
static const int   NW        = 180;
static const int   NH        = 140;
static const int   CHAT_BUF  = 20;
static const int   CHAT_SHOW =  6;
static const float FLASH_DUR = 0.35f;

// ═══════════════════════════════════════════════════════════════
//  État de l'adversaire
// ═══════════════════════════════════════════════════════════════
enum class OppState {
    WAITING,        // pas encore en partie
    PLAYING,        // joue normalement
    UNDER_ATTACK,   // on vient de lui envoyer des lignes
    ATTACKING,      // il nous envoie des lignes
    LOST,           // il a perdu → on a gagné
    DISCONNECTED    // déconnecté brutalement
};

// ═══════════════════════════════════════════════════════════════
class Game {

    int         score_;
    int         level_;
    std::string msg_;

    bool  flashing_;
    float flashTimer_;

    bool scoreChanged_;
    int  linesToSend_;

    std::vector<Piece> bag_;

    int  chat_total_;
    std::vector<std::string> chat_buf_;
    std::vector<bool>        chat_received_;

    // Timer pour affichage temporaire de l'état adversaire
    float opp_state_timer_;

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

    void add_score(int lines) {
        static const int BASE[5]={0,40,100,300,1200};
        score_ += BASE[std::min(lines,4)] * (level_+1);
        level_  = score_/1000;
        scoreChanged_=true;
    }

    void place() {
        grid.merge(current);
        current.clear();
        current = next;
        next    = pull();
        if(!current.no_overlap(grid)){
            msg_="GAME OVER"; justLost=true; current.clear();
        }
    }

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

    void push_chat(const std::string& text, bool received){
        chat_buf_[chat_total_%CHAT_BUF]      = text;
        chat_received_[chat_total_%CHAT_BUF] = received;
        chat_total_++;
    }

    // Hauteur max de la grille adversaire (0-20)
    int opp_danger() const {
        // Estimé depuis opp_score : plus le score est élevé, plus il a survécu
        // Valeur entre 0 et 20 basée sur le level
        return std::min(20, opp_level * 3);
    }

public:
    Piece grid;
    Piece current;
    Piece next;

    bool     justLost;
    int      opp_score;
    int      opp_level;
    int      opp_lines_sent;   // lignes qu'il nous a envoyées
    OppState opp_state;

    Music music;
    Sound snd_rotate;
    Sound snd_line;

    int         score()    const { return score_;    }
    int         level()    const { return level_;    }
    std::string msg()      const { return msg_;      }
    bool        flashing() const { return flashing_; }
    void        set_msg(const std::string& m) { msg_=m; }

    void set_opp_state(OppState s, float duration=0.f) {
        opp_state       = s;
        opp_state_timer_ = duration;
    }

    Game() : score_(0), level_(0), msg_(""),
             flashing_(false), flashTimer_(0),
             scoreChanged_(false), linesToSend_(0),
             justLost(false), opp_score(0), opp_level(0),
             opp_lines_sent(0), opp_state(OppState::WAITING),
             opp_state_timer_(0), chat_total_(0)
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

    void reset() {
        grid=Piece(); bag_=make_bag(); current=pull(); next=pull();
        score_=0; level_=0; msg_="";
        flashing_=false; flashTimer_=0;
        scoreChanged_=false; linesToSend_=0;
        justLost=false; opp_score=0; opp_level=0;
        opp_lines_sent=0; opp_state=OppState::WAITING;
        opp_state_timer_=0; chat_total_=0;
        for(int i=0;i<CHAT_BUF;i++){ chat_buf_[i].clear(); chat_received_[i]=false; }
    }

    // ═══════════════════════════════════════════════════════════
    //  LOGIQUE
    // ═══════════════════════════════════════════════════════════

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

    void tick_lines() {
        if(justLost||flashing_) return;
        if(!grid.full_lines().empty()){
            flashing_=true; flashTimer_=FLASH_DUR;
        }
    }

    void tick_gravity() {
        if(justLost||flashing_) return;
        if(current.hits_bottom(grid)) place();
        else                          current.move_down();
    }

    // Tick état adversaire (réinitialise les états temporaires)
    void tick_opp_state(float dt) {
        if(opp_state_timer_ > 0) {
            opp_state_timer_ -= dt;
            if(opp_state_timer_ <= 0) {
                opp_state_timer_ = 0;
                if(opp_state == OppState::ATTACKING || opp_state == OppState::UNDER_ATTACK)
                    opp_state = OppState::PLAYING;
            }
        }
    }

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

    int  pop_lines() { int n=linesToSend_; linesToSend_=0; return n; }
    bool pop_score_changed() {
        if(!scoreChanged_) return false;
        scoreChanged_=false; return true;
    }

    // ═══════════════════════════════════════════════════════════
    //  RÉSEAU
    // ═══════════════════════════════════════════════════════════
    bool apply_net(const std::string& m){
        if(m.rfind("LINES|",0)==0){
            int n=std::stoi(m.substr(6));
            for(int i=0;i<n;i++) add_garbage();
            opp_lines_sent += n;
            msg_="ATTAQUE !";
            set_opp_state(OppState::ATTACKING, 2.0f);
            return false;
        }
        if(m=="GAMEOVER"){
            msg_="VICTOIRE !";
            set_opp_state(OppState::LOST);
            return true;
        }
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

    void send_chat(const std::string& text){ push_chat(text,false); }

    // ═══════════════════════════════════════════════════════════
    //  RENDU
    // ═══════════════════════════════════════════════════════════
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

    // ── Panneau adversaire redessiné — y=347..447 ─────────────
    void draw_opponent() const {
        // Couleur du panneau selon l'état
        Color bg_col, border_col, label_col;
        const char* state_label = "";
        const char* state_icon  = "";

        switch(opp_state){
            case OppState::WAITING:
                bg_col     = {25, 25, 60, 255};
                border_col = {80, 80, 160, 70};
                label_col  = {100, 100, 180, 255};
                state_label= "En attente";
                state_icon = "...";
                break;
            case OppState::PLAYING:
                bg_col     = {20, 45, 20, 255};
                border_col = {60, 180, 60, 70};
                label_col  = {80, 200, 80, 255};
                state_label= "En jeu";
                state_icon = ">>>";
                break;
            case OppState::UNDER_ATTACK:
                bg_col     = {50, 20, 50, 255};
                border_col = {200, 60, 200, 100};
                label_col  = {220, 80, 220, 255};
                state_label= "Sous attaque!";
                state_icon = "!!!";
                break;
            case OppState::ATTACKING:
                bg_col     = {60, 30, 10, 255};
                border_col = {255, 140, 0, 100};
                label_col  = {255, 160, 30, 255};
                state_label= "ATTAQUE!";
                state_icon = ">>!";
                break;
            case OppState::LOST:
                bg_col     = {10, 30, 10, 255};
                border_col = {40, 180, 40, 100};
                label_col  = {40, 220, 40, 255};
                state_label= "A PERDU";
                state_icon = "GG";
                break;
            case OppState::DISCONNECTED:
                bg_col     = {40, 15, 15, 255};
                border_col = {160, 40, 40, 70};
                label_col  = {180, 60, 60, 255};
                state_label= "Deconnecte";
                state_icon = "---";
                break;
        }

        // Fond du panneau
        DrawRectangleRounded({340,347,432,100},0.15f,6, bg_col);
        RLDrawRoundedLines(Rectangle{340,347,432,100},0.15f,6, border_col);

        // Titre
        DrawText("ADVERSAIRE", 352, 354, 14, label_col);

        // Icone état (coin droit)
        int iw = MeasureText(state_icon, 13);
        DrawText(state_icon, 762-iw, 354, 13, label_col);

        // Score et level
        char sc[32], lv[32];
        snprintf(sc, sizeof(sc), "Score  %d", opp_score);
        snprintf(lv, sizeof(lv), "Lvl  %d",   opp_level);
        DrawText(sc, 352, 374, 15, ORANGE);
        DrawText(lv, 560, 374, 15, ORANGE);

        // Lignes envoyées
        if(opp_lines_sent > 0){
            char atk[32];
            snprintf(atk, sizeof(atk), "Lignes envoyees  %d", opp_lines_sent);
            DrawText(atk, 352, 396, 13, Color{200,120,50,255});
        }

        // Label état centré en bas du panneau
        int sw = MeasureText(state_label, 14);
        DrawText(state_label, 340+(432-sw)/2, 418, 14, label_col);

        // Barre de danger (hauteur estimée de sa grille)
        // Fond de la barre
        DrawRectangleRounded({352,434,408,10},0.5f,4,Color{20,20,50,255});
        // Remplissage selon le level
        float danger = std::min(1.0f, opp_level / 10.0f);
        Color bar_col = danger < 0.4f ? Color{40,180,40,255} :
                        danger < 0.7f ? Color{220,160,20,255} :
                                        Color{200,40,40,255};
        if(danger > 0)
            DrawRectangleRounded({352,434,(int)(408*danger),10},0.5f,4, bar_col);
        DrawText("DANGER", 352, 434, 10, Color{80,80,120,200});
    }

    // Messages chat
    void draw_chat() const {
        const int Y0=583, LH=22;
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
