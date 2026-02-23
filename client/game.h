#ifndef TETRISONLINE_GAME_H
#define TETRISONLINE_GAME_H

#include <random>
#include "objets.h"
#include <vector>
#include <string>
#include <set>
#include "network.h"

// ─── Layout global (partagé avec main.cpp) ────
//   Fenêtre     : 780 × 640
//   Grille      : ox=20, oy=60, 10×20 cellules × 30px = 300×600
//   Panneau     : x=340, y=60, w=430
//   Next cadre  : x=340, y=120, w=180, h=140

static const int GX = 20;   // grille offset X
static const int GY = 60;   // grille offset Y
static const int CS = 30;   // cell size

static const int NX = 340, NY = 120, NW = 180, NH = 140;  // cadre Next

class Game {
private:
    int score, niveau;
    std::string msg;

public:
    object grid;
    std::vector<object> objs;
    object current, next;
    bool justLost, scoreChanged;
    int  linesToSend;
    Music music;
    Sound rotate_sound, destroy_sound;
    int opp_score, opp_niveau;
    int max_chat;
    std::vector<std::string> chat_messages;
    std::vector<bool>        chat_recu;

    std::string get_msg()  const { return msg; }
    int get_score()        const { return score; }
    int get_niveau()       const { return niveau; }
    void set_msg(std::string m)  { msg=m; }
    void set_score(int s)        { score=s; scoreChanged=true; }
    void set_niveau(int n)       { niveau=n; }

    Game(){
        grid=object(); objs=get_all_objects();
        current=get_random_object(); next=get_random_object();
        score=0;niveau=0;msg="";
        justLost=false;scoreChanged=false;linesToSend=0;
        opp_score=0;opp_niveau=0;max_chat=0;
        chat_messages=std::vector<std::string>(10,"");
        chat_recu=std::vector<bool>(10,false);
        music=LoadMusicStream("sounds/cover.wav");
        rotate_sound=LoadSound("sounds/rotate.wav");
        destroy_sound=LoadSound("sounds/destroy.wav");
        PlayMusicStream(music);
    }
    ~Game(){UnloadMusicStream(music);UnloadSound(destroy_sound);UnloadSound(rotate_sound);}

    std::vector<object> get_all_objects(){
        object T,O,I,J,L,S,Z;
        T.make_T();O.make_O();I.make_I();J.make_J();L.make_L();S.make_S();Z.make_Z();
        return {T,O,I,J,L,S,Z};
    }
    object get_random_object(){
        if(objs.empty())objs=get_all_objects();
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<> d(0,(int)objs.size()-1);
        int i=d(gen); object o=objs[i]; objs.erase(objs.begin()+i); return o;
    }

    void reset(){
        grid=object();objs=get_all_objects();
        current=get_random_object();next=get_random_object();
        score=0;niveau=0;msg="";
        justLost=false;scoreChanged=false;linesToSend=0;
        opp_score=0;opp_niveau=0;max_chat=0;
        for(int i=0;i<10;i++){chat_messages[i].clear();chat_recu[i]=false;}
    }

    bool apply_network_message(const std::string& m){
        if(m.rfind("LINES|",0)==0){int n=std::stoi(m.substr(6));for(int i=0;i<n;i++)add_garbage_line();set_msg("ATTAQUE !");return false;}
        if(m=="GAMEOVER"){set_msg("VICTOIRE !");return true;}
        if(m.rfind("CHAT|",0)==0){ajouter_msg(m.substr(5),true);max_chat++;return false;}
        if(m.rfind("SCORE|",0)==0){
            std::string d=m.substr(6);size_t s=d.find('|');
            if(s!=std::string::npos){opp_score=std::stoi(d.substr(0,s));opp_niveau=std::stoi(d.substr(s+1));}
            return false;
        }
        return false;
    }

    void add_garbage_line(){
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<> d(0,grid.line-1);
        int hole=d(gen),tc=grid.column-1;
        for(int j=grid.column-1;j>=0;j--){
            bool e=true;for(int i=0;i<grid.line;i++)if(grid.matrice[i][j]!=0){e=false;break;}
            if(e){tc=j;break;}
        }
        for(int i=0;i<grid.line;i++)if(i!=hole)grid.matrice[i][tc]=1;
    }

    void ajouter_msg(const std::string& message,bool recu){
        if(max_chat>=10){for(int i=0;i<10;i++){chat_messages[i].clear();chat_recu[i]=false;}max_chat=0;}
        chat_messages[max_chat]=message;chat_recu[max_chat]=recu;
    }

    // Messages : zone x=348..540, y=408..590
    void draw_msg(){
        for(int i=0;i<max_chat;i++){
            Color col=chat_recu[i]?ORANGE:SKYBLUE;
            int x=chat_recu[i]?352:440;
            DrawText(chat_messages[i].c_str(),x,435+i*24,14,col);
        }
    }

    // ─── Rendu grille avec offset GX,GY ──────
    void dessiner(){
        // Fond + lignes de grille
        DrawRectangle(GX,GY,grid.line*CS,grid.column*CS,{10,10,30,255});
        Color gl={30,35,70,255};
        for(int i=0;i<=grid.line;i++)
            DrawLine(GX+i*CS,GY,GX+i*CS,GY+grid.column*CS,gl);
        for(int j=0;j<=grid.column;j++)
            DrawLine(GX,GY+j*CS,GX+grid.line*CS,GY+j*CS,gl);

        // Blocs posés
        grid.dessiner(GX,GY);

        if(justLost)return;
        if(!current.checkintersection(grid))return;

        // Ghost
        object ghost=current;
        while(!ghost.check_collision(grid))ghost.translate_bas();
        auto Pg=ghost.get_pos();auto Pc=current.get_pos();
        bool same=true;
        for(int i=0;i<4;i++)if(Pg[0][i]!=Pc[0][i]||Pg[1][i]!=Pc[1][i]){same=false;break;}
        if(!same)
            ghost.dessiner_piece(GX,GY,{160,160,160,160},true);

        // Pièce courante
        current.dessiner_piece(GX,GY,WHITE,false);
    }

    // ─── Next centré dans son cadre ───────────
    void dessiner_next(){
        auto P=next.get_pos();
        auto cols=next.GetCellColors();
        int mnx=*std::min_element(P[0].begin(),P[0].end());
        int mny=*std::min_element(P[1].begin(),P[1].end());
        int mxx=*std::max_element(P[0].begin(),P[0].end());
        int mxy=*std::max_element(P[1].begin(),P[1].end());
        int pw=(mxy-mny+1)*CS, ph=(mxx-mnx+1)*CS;
        int ox=NX+NW/2-pw/2;
        int oy=NY+NH/2-ph/2;
        for(int i=0;i<4;i++){
            int c=next.matrice[P[0][i]][P[1][i]];
            DrawRectangle(ox+(P[1][i]-mny)*CS, oy+(P[0][i]-mnx)*CS, CS-2,CS-2, cols[c]);
        }
    }

    void dessiner_opponent(){
        DrawRectangleRounded({340,410,180,75},0.25f,6,{50,15,15,255});
        DrawText("ADVERSAIRE",352,420,15,{200,80,80,255});
        char sc[32],lv[32];
        sprintf(sc,"Score  %d",opp_score);
        sprintf(lv,"Lvl    %d",opp_niveau);
        DrawText(sc,352,442,14,ORANGE);
        DrawText(lv,352,460,14,ORANGE);
    }

    void input(){
        int key=GetKeyPressed();
        switch(key){
            case KEY_RIGHT:if(current.check_right(grid))current.translate_d();break;
            case KEY_LEFT: if(current.check_left(grid)) current.translate_g();break;
            case KEY_DOWN:
                if(current.check_collision(grid)&&!justLost&&current.checkintersection(grid)){
                    grid.add(current);current.set_zero();
                    current=next;next=get_random_object();
                    if(!current.checkintersection(grid)){set_msg("GAME OVER");justLost=true;current.set_zero();}
                } else current.translate_bas();
                break;
            case KEY_UP:if(current.check_rotate(grid))rotate();break;
            case KEY_ENTER:hard_drop();break;
            default:break;
        }
    }
    void rotate(){PlaySound(rotate_sound);current.rotate();}

    void hard_drop(){
        if(justLost||!current.checkintersection(grid))return;
        int d=0;while(!current.check_collision(grid)){current.translate_bas();d++;}
        set_score(score+d*2);
        grid.add(current);current.set_zero();
        current=next;next=get_random_object();
        if(!current.checkintersection(grid)){set_msg("GAME OVER");justLost=true;current.set_zero();}
    }

    void move_down(){
        if(justLost)return;
        if(current.check_collision(grid)){
            if(current.checkintersection(grid)){
                grid.add(current);current.set_zero();
                current=next;next=get_random_object();
                if(!current.checkintersection(grid)){set_msg("GAME OVER");justLost=true;current.set_zero();}
            }
        } else current.translate_bas();
    }

    bool loose(){
        if(justLost)return true;
        for(int i=0;i<grid.line;i++)
            if(grid.matrice[i][0]!=0){
                if(!justLost){set_msg("GAME OVER");justLost=true;current.set_zero();}
                return true;
            }
        return false;
    }

    int calcscore(int nb,int niv){
        int b=0;if(nb==1)b=40;else if(nb==2)b=100;else if(nb==3)b=300;else if(nb==4)b=1200;
        return b*(niv+1);
    }
    void destroy(){
        int nb=grid.destroy();
        if(nb!=0){PlaySound(destroy_sound);set_score(score+calcscore(nb,niveau));set_niveau(score/1000);linesToSend+=nb;}
    }
};

#endif
