#include "raylib.h"
#include "compat.h"
#include "game.h"
#include "network.h"
#include <string>
#include <csignal>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════
//  MACHINE D'ÉTATS
// ═══════════════════════════════════════════════════════════════
enum class State { MENU, CONNECTING, WAITING, PLAYING, GAMEOVER };

// Timer de gravité : retourne true toutes les `interval` secondes
static double gravity_t = 0;
static bool gravity_tick(double interval){
    double now=GetTime();
    if(now-gravity_t>interval){ gravity_t=now; return true; }
    return false;
}

// ═══════════════════════════════════════════════════════════════
//  PALETTE UI
// ═══════════════════════════════════════════════════════════════
static Color C_BG    = {8,   8,  22, 255};
static Color C_PANEL = {18,  22,  58, 255};
static Color C_PANEL2= {22,  28,  70, 255};
static Color C_CYAN  = {60, 180, 255, 255};
static Color C_PURP  = {160, 60, 255, 255};
static Color C_GREEN = {25, 150,  55, 255};
static Color C_BLUE  = {25,  75, 200, 255};
static Color C_RED   = {170, 25,  25, 255};
static Color C_DARK  = {45,  55, 120, 255};

// ═══════════════════════════════════════════════════════════════
//  HELPERS UI
// ═══════════════════════════════════════════════════════════════
static void ui_panel(Rectangle r, Color bg, const char* label=nullptr, int lfs=14){
    DrawRectangleRounded(r,0.2f,6,bg);
    RLDrawRoundedLines(r,0.2f,6,Color{255,255,255,20});
    if(label&&label[0]){
        int tw=MeasureText(label,lfs);
        DrawText(label,(int)(r.x+(r.width-tw)/2),(int)(r.y+6),lfs,Color{140,160,220,255});
    }
}

static void ui_button(Rectangle r, const char* txt, Color col, int fs=19){
    DrawRectangleRounded(r,0.3f,6,col);
    RLDrawRoundedLines(r,0.3f,6,Color{255,255,255,30});
    int tw=MeasureText(txt,fs);
    DrawText(txt,(int)(r.x+(r.width-tw)/2),(int)(r.y+(r.height-fs)/2),fs,WHITE);
}

static bool ui_clicked(Rectangle r, Vector2 m){
    return CheckCollisionPointRec(m,r)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static void ui_text(const char* t, int x, int w, int y, int fs, Color c){
    DrawText(t, x+(w-MeasureText(t,fs))/2, y, fs, c);
}

// ═══════════════════════════════════════════════════════════════
int main(int argc, char** argv){
#ifndef _WIN32
    signal(SIGPIPE,SIG_IGN);
#endif
    const char* SERVER=(argc>=2)?argv[1]:"bore.pub:19050";

    InitWindow(780,800,"TETRIS ONLINE");
    SetAudioStreamBufferSizeDefault(512);
    InitAudioDevice();
    SetTargetFPS(60);

    Game  jeu;
    State state  = State::MENU;
    bool  online = false;
    bool  paused = false;

    SetMusicVolume(jeu.music,    0.3f);
    SetSoundVolume(jeu.snd_line,   1.0f);
    SetSoundVolume(jeu.snd_rotate, 0.5f);

    // ── Slider volume ─────────────────────────────────────────
    float vol=0.5f;
    const float SLX=370,SLY=757,SLW=340;
    Rectangle sl_bar ={SLX,SLY+18,SLW,5};
    Rectangle sl_knob={SLX+SLW*vol-6,SLY+13,12,15};
    bool sl_drag=false;

    // ── Boutons chat (2×3, x=340, y=445) ─────────────────────
    const char* CLBL[]={"GL","WP","Wow","Thx","GG","Oops"};
    const char* CTXT[]={"Good luck!","Well Played!","Wow!","Thanks!","Good Game!","Oops!"};
    Rectangle CBTNS[6];
    for(int i=0;i<6;i++)
        CBTNS[i]={340.f+(i%3)*145,445.f+(i/3)*36,138,30};

    // ══════════════════════════════════════════════════════════
    while(!WindowShouldClose()){
        UpdateMusicStream(jeu.music);
        Vector2 mouse=GetMousePosition();
        float   dt   =GetFrameTime();

        // ── Réseau ───────────────────────────────────────────

        // Connexion établie → s'enregistrer et passer en attente
        if(state==State::CONNECTING && is_connected()){
            network_start_listener();
            network_send("READY\n");
            state=State::WAITING;
            jeu.set_msg("En attente d'un adversaire...");
        }

        // Détection de déconnexion inattendue (timeout serveur, crash réseau…)
        if((state==State::WAITING||state==State::PLAYING) && online && !is_connected()){
            jeu.set_msg("Connexion perdue.");
            state=State::GAMEOVER;
        }

        if(state==State::WAITING||state==State::PLAYING){
            while(network_has_message()){
                std::string m=network_pop_message();

                if(m=="MATCH_START"){
                    state=State::PLAYING; paused=false;
                    gravity_t=GetTime();
                    jeu.set_msg("C'est parti !");
                }
                else if(m=="OPPONENT_LEFT"){
                    jeu.set_msg("Adversaire deconnecte.");
                    disconnect(); state=State::GAMEOVER;
                }
                // ─── Serveur plein ────────────────────────────
                else if(m=="SERVER_FULL"){
                    jeu.set_msg("Serveur plein. Reessayez.");
                    disconnect(); state=State::GAMEOVER;
                }
                else if(jeu.apply_net(m)){
                    disconnect(); state=State::GAMEOVER;
                }
            }
        }

        // ── Logique de jeu ───────────────────────────────────
        if(state==State::PLAYING){
            if(!online&&IsKeyPressed(KEY_P)) paused=!paused;

            if(!paused){
                jeu.tick_flash(dt);
                jeu.input();
                if(gravity_tick(0.2/(jeu.level()+1))) jeu.tick_gravity();
                jeu.tick_lines();

                if(jeu.justLost){
                    if(online&&is_connected()){ network_send("GAMEOVER\n"); disconnect(); }
                    state=State::GAMEOVER;
                }

                // Broadcast attaque
                int lines=jeu.pop_lines();
                if(lines>0&&online&&is_connected())
                    network_send("LINES|"+std::to_string(lines)+"\n");

                // Broadcast score
                if(jeu.pop_score_changed()&&online&&is_connected())
                    network_send("SCORE|"+std::to_string(jeu.score())
                                 +"|"+std::to_string(jeu.level())+"\n");
            }
        }

        // ══════════ DESSIN ════════════════════════════════════
        BeginDrawing();
        ClearBackground(C_BG);

        // ── Titre ─────────────────────────────────────────────
        DrawRectangle(0,0,780,52,Color{12,14,40,255});
        DrawLine(0,52,780,52,C_CYAN);
        ui_text("TETRIS ONLINE",0,780,13,26,C_CYAN);

        // ── Bordure grille ─────────────────────────────────────
        DrawRectangle(GX-3,GY-3,  306,  3,C_CYAN);
        DrawRectangle(GX-3,GY,      3,603,C_CYAN);
        DrawRectangle(GX+300,GY,    3,603,C_CYAN);
        DrawRectangle(GX-3,GY+600,306,  3,C_CYAN);

        // ── Grille ─────────────────────────────────────────────
        if(state==State::PLAYING||state==State::GAMEOVER){
            jeu.draw_grid();
            jeu.draw_next();
        } else {
            jeu.grid.draw_background(GX,GY);
            if(state==State::MENU){
                ui_text("TETRIS",GX,300,160,34,C_CYAN);
                ui_text("ONLINE",GX,300,198,34,C_PURP);
                ui_text("- - - - - - - -",GX,300,246,14,Color{50,60,120,255});
                ui_text("SOLO",  GX,300,272,20,Color{180,220,255,255});
                ui_text("ou",    GX,300,300,15,Color{80,100,160,255});
                ui_text("ONLINE",GX,300,322,20,Color{220,180,255,255});
            }
            if(state==State::WAITING||state==State::CONNECTING)
                ui_text(state==State::WAITING?"En attente...":"Connexion...",
                        GX,300,270,18,YELLOW);
        }

        // ══ PANNEAU DROIT ════════════════════════════════════

        // Score
        ui_panel({340,60,200,50},C_PANEL,"SCORE");
        { char s[16]; snprintf(s, sizeof(s), "%d", jeu.score());
          ui_text(s,340,200,84,26,WHITE); }

        // Level
        ui_panel({550,60,222,50},C_PANEL,"LEVEL");
        { char l[16]; snprintf(l, sizeof(l), "%d", jeu.level());
          ui_text(l,550,222,84,26,WHITE); }

        // Next
        ui_panel({(float)NX,(float)NY,(float)NW,(float)NH},C_PANEL,"NEXT");
        if(state==State::PLAYING||state==State::GAMEOVER) jeu.draw_next();

        // Contrôles
        if(state==State::PLAYING||state==State::MENU){
            DrawText("CONTROLES", 532,128,14,Color{120,140,210,255});
            Color kc=Color{140,165,230,255};
            DrawText("< >  Deplacer",530,148,15,kc);
            DrawText("^    Rotation", 530,168,15,kc);
            DrawText("v    Descente", 530,188,15,kc);
            DrawText("Ent  Hard drop",530,208,15,kc);
            if(!online) DrawText("P    Pause",530,228,15,kc);
        }

        // Message d'état
        { const char* m=jeu.msg().c_str();
          if(m[0]){
              Color mc=(jeu.msg()=="GAME OVER")?C_RED:
                       (jeu.msg()=="VICTOIRE !")?C_PURP:YELLOW;
              ui_text(m,340,432,272,18,mc);
          }}

        // ── Boutons selon l'état ──────────────────────────────
        switch(state){
            case State::MENU:
                ui_button({340,295,205,42},"SOLO",  C_GREEN);
                ui_button({555,295,217,42},"ONLINE",C_BLUE);
                if(ui_clicked({340,295,205,42},mouse)){
                    online=false; paused=false;
                    jeu.reset(); gravity_t=GetTime();
                    state=State::PLAYING;
                }
                if(ui_clicked({555,295,217,42},mouse)){
                    online=true; jeu.reset();
                    network_connect(SERVER);
                    state=State::CONNECTING;
                }
                break;

            case State::CONNECTING:
            case State::WAITING:
                ui_button({340,295,432,42},"Annuler",C_RED);
                if(ui_clicked({340,295,432,42},mouse)){
                    disconnect(); jeu.reset(); state=State::MENU;
                }
                break;

            case State::PLAYING:
                if(online){
                    jeu.draw_opponent();
                } else {
                    ui_button({340,295,432,42},
                              paused?"REPRENDRE":"PAUSE",
                              paused?C_GREEN:C_DARK);
                    if(ui_clicked({340,295,432,42},mouse)) paused=!paused;
                }
                break;

            case State::GAMEOVER:
                ui_button({340,295,210,42},"REJOUER",C_GREEN);
                ui_button({560,295,212,42},"MENU",   C_BLUE);
                if(ui_clicked({340,295,210,42},mouse)){
                    jeu.reset(); paused=false; gravity_t=GetTime();
                    if(online){ network_connect(SERVER); state=State::CONNECTING; }
                    else        state=State::PLAYING;
                }
                if(ui_clicked({560,295,212,42},mouse)){
                    jeu.reset(); online=false; paused=false;
                    state=State::MENU;
                }
                break;
        }

        // ── Chat (online) ─────────────────────────────────────
        if(online&&(state==State::PLAYING||state==State::WAITING)){
            ui_panel({340,427,432,100},C_PANEL2,"CHAT RAPIDE");
            for(int i=0;i<6;i++){
                DrawRectangleRounded(CBTNS[i],0.3f,6,C_DARK);
                int tw=MeasureText(CLBL[i],14);
                DrawText(CLBL[i],(int)(CBTNS[i].x+(CBTNS[i].width-tw)/2),
                         (int)(CBTNS[i].y+7),14,WHITE);
                if(ui_clicked(CBTNS[i],mouse)&&is_connected()){
                    network_send(std::string("CHAT|")+CTXT[i]+"\n");
                    jeu.send_chat(CTXT[i]);
                }
            }
            ui_panel({340,535,432,210},C_PANEL2,"MESSAGES");
            jeu.draw_chat();
        }

        // ── Volume ────────────────────────────────────────────
        DrawText("VOL",(int)SLX-28,(int)SLY+13,14,Color{90,110,170,255});
        DrawRectangleRounded(sl_bar,0.5f,6,Color{30,35,80,255});
        DrawRectangleRounded({SLX,SLY+18,SLW*vol,5},0.5f,6,C_CYAN);
        DrawRectangleRounded(sl_knob,0.5f,6,WHITE);

        if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&CheckCollisionPointRec(mouse,sl_knob))
            sl_drag=true;
        if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) sl_drag=false;
        if(sl_drag){
            sl_knob.x=std::max(SLX,std::min(mouse.x-sl_knob.width/2,SLX+SLW-sl_knob.width));
            vol=(sl_knob.x-SLX)/(SLW-sl_knob.width);
            SetMasterVolume(vol);
        }

        EndDrawing();
    }

    if(is_connected()) disconnect();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
