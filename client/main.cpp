#include "raylib.h"
#include "objets.h"
#include "game.h"
#include "network.h"
#include <string>
#include <csignal>
#include <algorithm>

// ═══════════════════════════════════════════════
//  LAYOUT — 780 × 640  (Dark Arcade)
//
//   Grille   : x=20,  y=60,  300×600  (10×20 × 30px)
//   Panneau  : x=340, y=60,  w=432
//
//   Panneau détail :
//     Score cadre   : x=340, y= 60, w=200, h=50
//     Level cadre   : x=550, y= 60, w=222, h=50
//     Next  cadre   : x=340, y=120, w=180, h=140
//     Contrôles     : x=530, y=120
//     Message       : x=340, y=272
//     Boutons       : x=340, y=295, h=42
//     Extra online  : x=340, y=345  (adversaire + chat)
//     Volume        : x=340, y=600
// ═══════════════════════════════════════════════

enum class State { MENU, CONNECTING, WAITING, PLAYING, GAMEOVER };

double last_update=0;
bool event(double iv){
    double now=GetTime();
    if(now-last_update>iv){last_update=now;return true;}
    return false;
}

void dbtn(Rectangle r, const char* txt, Color col, int fs=19){
    DrawRectangleRounded(r,0.3f,6,col);
    DrawRectangleRoundedLines(r,0.3f,6,{255,255,255,30});
    int tw=MeasureText(txt,fs);
    DrawText(txt,(int)(r.x+(r.width-tw)/2),(int)(r.y+(r.height-fs)/2-1),fs,WHITE);
}
bool hit(Rectangle r, Vector2 m){
    return CheckCollisionPointRec(m,r)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}
void panel(Rectangle r, Color bg, const char* label=nullptr, int lfs=13){
    DrawRectangleRounded(r,0.2f,6,bg);
    DrawRectangleRoundedLines(r,0.2f,6,{255,255,255,20});
    if(label){
        int tw=MeasureText(label,lfs);
        DrawText(label,(int)(r.x+(r.width-tw)/2),(int)(r.y+5),lfs,{140,160,220,255});
    }
}

int main(int argc, char** argv){
    signal(SIGPIPE,SIG_IGN);
    const char* ip=(argc>=2)?argv[1]:"127.0.0.1";

    Color BG    = {8,   8,  22, 255};
    Color PNL   = {18,  22,  58, 255};
    Color PNL2  = {22,  28,  70, 255};
    Color CYAN  = {60, 180, 255, 255};
    Color PURP  = {160, 60, 255, 255};
    Color GBTN  = {25, 150,  55, 255};
    Color BBTN  = {25,  75, 200, 255};
    Color RBTN  = {170, 25,  25, 255};
    Color DBTN_C= {45,  55, 120, 255};

    InitWindow(780,800,"TETRIS ONLINE");
    InitAudioDevice();
    SetTargetFPS(60);

    Game jeu=Game();
    SetMusicVolume(jeu.music,0.3f);
    SetSoundVolume(jeu.destroy_sound,1.0f);
    SetSoundVolume(jeu.rotate_sound,0.5f);

    State state=State::MENU;
    bool online_mode=false, paused=false;

    // Volume
    float vol=0.5f;
    const float VX=340,VY=752,VW=380;
    Rectangle vbar ={VX+30,VY+18,VW,5};
    Rectangle vknob={VX+30+VW*vol-6,VY+13,12,15};
    bool drag=false;

    // Chat buttons : 2 lignes × 3, dans zone x=530,y=345
    const char* clbl[]={"GL","WP","Wow","Thx","GG","Oops"};
    const char* ctxt[]={"Good luck!","Well Played!","Wow!","Thanks!","Good Game!","Oops!"};
    Rectangle cbtn[6];
    for(int i=0;i<6;i++)
        cbtn[i]={340.0f+(i%3)*145,442.0f+(i/3)*36,138,30};

    while(!WindowShouldClose()){
        UpdateMusicStream(jeu.music);
        Vector2 mouse=GetMousePosition();

        // -- Réseau -----------------------------
        if(state==State::CONNECTING&&is_connected()){
            network_start_listener();network_send("READY\n");
            state=State::WAITING;jeu.set_msg("En attente...");
        }
        if(state==State::WAITING||state==State::PLAYING){
            while(network_has_message()){
                std::string m=network_pop_message();
                if(m=="MATCH_START"){
                    state=State::PLAYING;paused=false;
                    last_update=GetTime();jeu.set_msg("C'est parti !");
                } else if(m=="OPPONENT_LEFT"){
                    jeu.set_msg("Adversaire parti.");disconnect();state=State::GAMEOVER;
                } else {
                    if(jeu.apply_network_message(m)){disconnect();state=State::GAMEOVER;}
                }
            }
        }

        // -- Logique ----------------------------
        if(state==State::PLAYING){
            // Touche P pour pause (solo uniquement)
            if(!online_mode&&IsKeyPressed(KEY_P))paused=!paused;
        }
        if(state==State::PLAYING&&!paused){
            if(!jeu.justLost)jeu.input();
            if(event(0.2/(jeu.get_niveau()+1)))jeu.move_down();
            if(jeu.justLost){
                if(online_mode&&is_connected()){network_send("GAMEOVER\n");disconnect();}
                state=State::GAMEOVER;
            }
            if(jeu.linesToSend>0&&online_mode&&is_connected()){
                network_send("LINES|"+std::to_string(jeu.linesToSend)+"\n");jeu.linesToSend=0;
            }
            if(jeu.scoreChanged&&online_mode&&is_connected()){
                network_send("SCORE|"+std::to_string(jeu.get_score())+"|"+std::to_string(jeu.get_niveau())+"\n");
                jeu.scoreChanged=false;
            }
        }

        // ═══════════ DESSIN ════════════════════
        BeginDrawing();
        ClearBackground(BG);

        // -- Barre de titre ---------------------
        DrawRectangle(0,0,780,52,{12,14,40,255});
        DrawLine(0,52,780,52,CYAN);
        {const char* T="TETRIS ONLINE";int tw=MeasureText(T,26);DrawText(T,(780-tw)/2,11,30,CYAN);}

        // -- Bordure grille (accent) -------------
        // Bord gauche/droite/bas cyan
        DrawRectangle(GX-3, GY-3, 306,   3,   CYAN);   // haut
        DrawRectangle(GX-3, GY,     3, 602,   CYAN);   // gauche
        DrawRectangle(GX+300,GY,    3, 603,   CYAN);   // droite
        DrawRectangle(GX-3, GY+600, 306,  3,  CYAN);   // bas

        // -- Grille -----------------------------
        if(state==State::PLAYING||state==State::GAMEOVER){
            jeu.dessiner();
            jeu.dessiner_next();
            jeu.destroy();
        } else {
            // Grille vide avec lignes
            DrawRectangle(GX,GY,300,600,{10,10,30,255});
            Color gl={28,32,65,255};
            for(int i=0;i<=10;i++) DrawLine(GX+i*30,GY,GX+i*30,GY+600,gl);
            for(int j=0;j<=20;j++) DrawLine(GX,GY+j*30,GX+300,GY+j*30,gl);

            // Texte d'accueil dans la grille
            if(state==State::MENU){
                auto ct=[](const char* t,int y,int fs,Color c){
                    DrawText(t,(int)(GX+300/2-MeasureText(t,fs)/2),y,fs,c);
                };
                ct("TETRIS",    150, 36, CYAN);
                ct("ONLINE",    190, 36, PURP);
                ct("---------", 238, 16, {50,60,120,255});
                ct("SOLO",      258, 22, {180,220,255,255});
                ct("ou",        288, 17, {80,100,160,255});
                ct("ONLINE",    312, 22, {220,180,255,255});
            }
            if(state==State::WAITING){
                auto ct=[](const char* t,int y,int fs,Color c){
                    DrawText(t,(int)(GX+300/2-MeasureText(t,fs)/2),y,fs,c);
                };
                ct("EN ATTENTE", 250, 18, YELLOW);
                ct("d'un adversaire", 278, 15, {180,180,100,255});
            }
            if(state==State::CONNECTING){
                const char* t="CONNEXION...";
                DrawText(t,(int)(GX+300/2-MeasureText(t,18)/2),270,18,YELLOW);
            }
        }

        // ══ PANNEAU DROIT ══════════════════════

        // -- Score ------------------------------
        panel({340,60,200,50},PNL,"SCORE",15);
        {char s[16];sprintf(s,"%d",jeu.get_score());
         int tw=MeasureText(s,26);DrawText(s,(int)(340+(200-tw)/2),84,26,WHITE);}

        // -- Level ------------------------------
        panel({550,60,222,50},PNL,"LEVEL",15);
        {char l[16];sprintf(l,"%d",jeu.get_niveau());
         int tw=MeasureText(l,26);DrawText(l,(int)(550+(222-tw)/2),84,26,WHITE);}

        // -- Next -------------------------------
        panel({NX,NY,NW,NH},PNL,"NEXT",15);
        if(state==State::PLAYING||state==State::GAMEOVER)
            jeu.dessiner_next();

        // -- Contrôles (à droite du Next) -------
        if(state==State::PLAYING||state==State::MENU){
            DrawText("CONTROLES",532,124,14,{120,140,210,255});
            Color kc={140,165,230,255};
            DrawText("< >   Deplacer",  530,145,15,kc);
            DrawText("^     Rotation",  530,165,15,kc);
            DrawText("v     Descente",  530,185,15,kc);
            DrawText("Enter Hard drop", 530,205,15,kc);
            if(!online_mode)DrawText("P     Pause",530,225,15,kc);
        }

        // -- Message ----------------------------
        {const char* m=jeu.get_msg().c_str();
         if(m[0]){
             Color mc=(jeu.get_msg()=="GAME OVER")?RBTN:
                      (jeu.get_msg()=="VICTOIRE !")?PURP:YELLOW;
             int tw=MeasureText(m,16);
             DrawText(m,(int)(340+(430-tw)/2),272,18,mc);
         }}

        // -- Boutons selon l'état ---------------
        switch(state){
            case State::MENU:
                dbtn({340,295,205,42},"SOLO",  GBTN);
                dbtn({555,295,217,42},"ONLINE",BBTN);
                if(hit({340,295,205,42},mouse)){
                    online_mode=false;paused=false;
                    jeu.reset();last_update=GetTime();state=State::PLAYING;
                }
                if(hit({555,295,217,42},mouse)){
                    online_mode=true;jeu.reset();
                    network_connect(ip);state=State::CONNECTING;jeu.set_msg("Connexion...");
                }
                break;

            case State::CONNECTING:
            case State::WAITING:
                dbtn({340,295,432,42},"Annuler",RBTN);
                if(hit({340,295,432,42},mouse)){disconnect();jeu.reset();state=State::MENU;}
                break;

            case State::PLAYING:
                if(online_mode){
                    jeu.dessiner_opponent();
                } else {
                    dbtn({340,295,432,42},paused?"REPRENDRE":"PAUSE",paused?GBTN:DBTN_C);
                    if(hit({340,295,432,42},mouse))paused=!paused;
                }
                break;

            case State::GAMEOVER:
                dbtn({340,295,210,42},"REJOUER",GBTN);
                dbtn({560,295,212,42},"MENU",   BBTN);
                if(hit({340,295,210,42},mouse)){
                    jeu.reset();paused=false;last_update=GetTime();
                    if(online_mode){network_connect(ip);state=State::CONNECTING;jeu.set_msg("Connexion...");}
                    else state=State::PLAYING;
                }
                if(hit({560,295,212,42},mouse)){
                    jeu.reset();online_mode=false;paused=false;state=State::MENU;
                }
                break;
        }

        // -- Chat (online) ----------------------
        if(online_mode&&(state==State::PLAYING||state==State::WAITING)){
            // Boutons chat
            panel({340,424,432,108},PNL2,"CHAT RAPIDE",14);
            for(int i=0;i<6;i++){
                DrawRectangleRounded(cbtn[i],0.3f,6,DBTN_C);
                int tw=MeasureText(clbl[i],12);
                DrawText(clbl[i],(int)(cbtn[i].x+(cbtn[i].width-tw)/2),
                         (int)(cbtn[i].y+8),14,WHITE);
                if(hit(cbtn[i],mouse)&&is_connected()){
                    network_send(std::string("CHAT|")+ctxt[i]+"\n");
                    jeu.ajouter_msg(ctxt[i],false);jeu.max_chat++;
                }
            }
            // Zone messages
            panel({340,534,432,210},PNL2,"MESSAGES",14);
            jeu.draw_msg();
        }

        // -- Volume -----------------------------
        DrawText("VOL",(int)VX,VY,15,{90,110,170,255});
        DrawRectangleRounded(vbar,0.5f,6,{30,35,80,255});
        DrawRectangleRounded({VX+30,VY+16,(VX+30+VW*vol)-(VX+30),5},0.5f,6,CYAN);
        DrawRectangleRounded(vknob,0.5f,6,WHITE);

        if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&CheckCollisionPointRec(mouse,vknob))drag=true;
        if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON))drag=false;
        if(drag){
            vknob.x=std::max(VX+30,std::min(mouse.x-vknob.width/2,VX+30+VW-vknob.width));
            vol=(vknob.x-(VX+30))/(VW-vknob.width);
            SetMasterVolume(vol);
        }

        EndDrawing();
    }

    if(is_connected())disconnect();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
