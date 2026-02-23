#include "raylib.h"
#include "objets.h"
#include "game.h"
#include "network.h"
#include <string>
#include <csignal>
#include <algorithm>

// ─────────────────────────────────────────────
//  Layout (fenêtre 780x620, grille 300x600)
//
//  x=0..299   : grille de jeu
//  x=305..490 : colonne gauche (score, level, next, msg, boutons)
//  x=500..775 : colonne droite (chat online, volume)
// ─────────────────────────────────────────────

enum class State { MENU, CONNECTING, WAITING, PLAYING, GAMEOVER };

double last_update = 0;
bool event(double interval) {
    double now = GetTime();
    if (now - last_update > interval) { last_update = now; return true; }
    return false;
}

void draw_button(Rectangle r, const char* text, Color col) {
    DrawRectangleRounded(r, 0.3f, 6, col);
    DrawText(text, (int)(r.x + 10), (int)(r.y + 10), 18, WHITE);
}

bool button_clicked(Rectangle r, Vector2 mouse) {
    return CheckCollisionPointRec(mouse, r) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);
    const char* ip_serveur = (argc >= 2) ? argv[1] : "127.0.0.1";

    Color darkblue = {44,  44,  127, 255};
    Color panel    = {59,  85,  162, 255};
    Color panel_r  = {40,  55,  120, 255}; // colonne droite légèrement différente

    InitWindow(780, 620, "TETRIS ONLINE");
    InitAudioDevice();
    SetTargetFPS(60);

    Game jeu = Game();
    SetMusicVolume(jeu.music, 0.3f);
    SetSoundVolume(jeu.destroy_sound, 1.0f);
    SetSoundVolume(jeu.rotate_sound, 0.5f);

    State state      = State::MENU;
    bool online_mode = false;
    bool paused      = false;

    // ── Volume slider (colonne droite, bas) ──
    float volume = 0.5f;
    float sliderX0 = 530;
    float sliderW  = 150;
    Rectangle sliderBar  = {sliderX0, 590, sliderW, 8};
    Rectangle sliderKnob = {sliderX0 + sliderW * volume - 6, 584, 12, 16};
    bool dragging = false;

    // ── Boutons chat (colonne droite, 2 lignes x 3) ──
    float cx = 505;
    Rectangle btn_gl   = {cx,      130, 80, 38};
    Rectangle btn_wp   = {cx+85,   130, 80, 38};
    Rectangle btn_wow  = {cx+170,  130, 75, 38};
    Rectangle btn_thx  = {cx,      174, 80, 38};
    Rectangle btn_gg   = {cx+85,   174, 80, 38};
    Rectangle btn_oups = {cx+170,  174, 75, 38};

    while (!WindowShouldClose()) {
        UpdateMusicStream(jeu.music);
        Vector2 mouse = GetMousePosition();

        // ── Transitions réseau ────────────────────────
        if (state == State::CONNECTING && is_connected()) {
            network_start_listener();
            network_send("READY\n");
            state = State::WAITING;
            jeu.set_msg("En attente...");
        }

        if (state == State::WAITING || state == State::PLAYING) {
            while (network_has_message()) {
                std::string msg = network_pop_message();
                if (msg == "MATCH_START") {
                    state       = State::PLAYING;
                    paused      = false;
                    last_update = GetTime();
                    jeu.set_msg("C'est parti !");
                } else if (msg == "OPPONENT_LEFT") {
                    jeu.set_msg("Adversaire deconnecte.");
                    disconnect();
                    state = State::GAMEOVER;
                } else {
                    bool victoire = jeu.apply_network_message(msg);
                    if (victoire) { disconnect(); state = State::GAMEOVER; }
                }
            }
        }

        // ── Logique de jeu ────────────────────────────
        if (state == State::PLAYING && !paused) {
            if (!jeu.justLost) jeu.input();
            if (event(0.2 / (jeu.get_niveau() + 1))) jeu.move_down();

            if (jeu.justLost) {
                if (online_mode && is_connected()) { network_send("GAMEOVER\n"); disconnect(); }
                state = State::GAMEOVER;
            }
            if (jeu.linesToSend > 0 && online_mode && is_connected()) {
                network_send("LINES|" + std::to_string(jeu.linesToSend) + "\n");
                jeu.linesToSend = 0;
            }
            if (jeu.scoreChanged && online_mode && is_connected()) {
                network_send("SCORE|" + std::to_string(jeu.get_score())
                             + "|" + std::to_string(jeu.get_niveau()) + "\n");
                jeu.scoreChanged = false;
            }
        }

        // ══════════════════════════════════════════════
        BeginDrawing();
        ClearBackground(darkblue);

        // ── Colonne gauche : Score, Level, Next ───────
        //  Score
        DrawText("Score", 320, 12, 24, WHITE);
        DrawRectangleRounded({308, 40, 178, 42}, 0.3f, 6, panel);
        char sc[16]; sprintf(sc, "%d", jeu.get_score());
        DrawText(sc, 360, 50, 20, WHITE);

        //  Level
        DrawText("Level", 320, 96, 24, WHITE);
        DrawRectangleRounded({308, 124, 178, 38}, 0.3f, 6, panel);
        char lvl[16]; sprintf(lvl, "%d", jeu.get_niveau());
        DrawText(lvl, 382, 132, 20, WHITE);

        //  Next
        DrawText("Next", 330, 174, 24, WHITE);
        DrawRectangleRounded({308, 202, 178, 130}, 0.3f, 6, panel);

        // ── Colonne droite : fond ──────────────────────
        DrawRectangleRounded({500, 10, 272, 560}, 0.3f, 6, panel_r);

        // ── Grille + pièces ───────────────────────────
        if (state == State::PLAYING || state == State::GAMEOVER) {
            jeu.dessiner();
            jeu.dessiner_next();
            jeu.destroy();
        } else {
            jeu.grid.dessiner();
        }

        // ── Message d'état ────────────────────────────
        DrawText(jeu.get_msg().c_str(), 308, 345, 17, YELLOW);

        // ── Boutons selon l'état ──────────────────────
        switch (state) {

            case State::MENU:
                draw_button({308, 380, 130, 38}, "SOLO",   GREEN);
                draw_button({448, 380, 130, 38}, "ONLINE", BLUE);
                if (button_clicked({308, 380, 130, 38}, mouse)) {
                    online_mode = false; paused = false;
                    jeu.reset(); last_update = GetTime();
                    state = State::PLAYING;
                }
                if (button_clicked({448, 380, 130, 38}, mouse)) {
                    online_mode = true; jeu.reset();
                    network_connect(ip_serveur);
                    state = State::CONNECTING;
                    jeu.set_msg("Connexion...");
                }
                break;

            case State::CONNECTING:
                DrawText("Connexion...", 320, 380, 18, YELLOW);
                break;

            case State::WAITING:
                DrawText("En attente d'un adversaire...", 308, 370, 15, YELLOW);
                draw_button({308, 400, 150, 36}, "Annuler", RED);
                if (button_clicked({308, 400, 150, 36}, mouse)) {
                    disconnect(); jeu.reset(); state = State::MENU;
                }
                break;

            case State::PLAYING:
                if (online_mode) {
                    jeu.dessiner_opponent();
                } else {
                    draw_button({308, 390, 178, 38},
                                paused ? "REPRENDRE" : "PAUSE",
                                paused ? GREEN : RED);
                    if (button_clicked({308, 390, 178, 38}, mouse))
                        paused = !paused;
                }
                break;

            case State::GAMEOVER:
                draw_button({308, 390, 130, 38}, "REJOUER", GREEN);
                draw_button({448, 390, 130, 38}, "MENU",    BLUE);
                if (button_clicked({308, 390, 130, 38}, mouse)) {
                    jeu.reset(); paused = false; last_update = GetTime();
                    if (online_mode) {
                        network_connect(ip_serveur);
                        state = State::CONNECTING;
                        jeu.set_msg("Connexion...");
                    } else state = State::PLAYING;
                }
                if (button_clicked({448, 390, 130, 38}, mouse)) {
                    jeu.reset(); online_mode = false; paused = false;
                    state = State::MENU;
                }
                break;
        }

        // ── Chat (colonne droite, online) ─────────────
        if (online_mode && (state == State::PLAYING || state == State::WAITING)) {
            DrawText("Chat rapide", 515, 112, 16, WHITE);

            auto send_chat = [&](Rectangle btn, const char* label, const char* text) {
                DrawRectangleRounded(btn, 0.3f, 6, {200,200,200,255});
                DrawText(label, (int)(btn.x+6), (int)(btn.y+6), 12, BLACK);
                if (button_clicked(btn, mouse) && is_connected()) {
                    network_send(std::string("CHAT|") + text + "\n");
                    jeu.ajouter_msg(text, false);
                    jeu.max_chat++;
                }
            };
            send_chat(btn_gl,   "Good\nluck!",   "Good luck!");
            send_chat(btn_wp,   "Well\nplayed!", "Well Played!");
            send_chat(btn_wow,  "Wow!",          "Wow!");
            send_chat(btn_thx,  "Thanks!",       "Thanks!");
            send_chat(btn_gg,   "Good\nGame!",   "Good Game!");
            send_chat(btn_oups, "Oops!",         "Oops!");

            // Zone messages reçus/envoyés
            DrawRectangleRounded({505, 220, 265, 330}, 0.3f, 6, {30,40,90,255});
            jeu.draw_msg();
        }

        // ── Volume (colonne droite, bas) ───────────────
        DrawText("Volume", sliderX0, 570, 16, WHITE);
        DrawRectangleRounded(sliderBar,  0.5f, 6, GRAY);
        DrawRectangleRounded(sliderKnob, 0.5f, 6, WHITE);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(mouse, sliderKnob)) dragging = true;
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) dragging = false;
        if (dragging) {
            sliderKnob.x = std::max(sliderBar.x,
                           std::min(mouse.x - sliderKnob.width/2,
                                    sliderBar.x + sliderBar.width - sliderKnob.width));
            volume = (sliderKnob.x - sliderBar.x) / (sliderBar.width - sliderKnob.width);
            SetMasterVolume(volume);
        }

        EndDrawing();
    }

    if (is_connected()) disconnect();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
