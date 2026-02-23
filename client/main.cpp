#include "raylib.h"
#include "objets.h"
#include "game.h"
#include "network.h"
#include <string>
#include <csignal>

// ─────────────────────────────────────────────
//  Machine à états
// ─────────────────────────────────────────────
enum class State {
    MENU,        // Accueil, choix solo/online
    CONNECTING,  // Tentative de connexion TCP
    WAITING,     // Connecté, attend l'adversaire
    PLAYING,     // Partie en cours
    GAMEOVER     // Partie terminée (solo ou online)
};

// ─────────────────────────────────────────────
//  Timer pour move_down
// ─────────────────────────────────────────────
double last_update = 0;
bool event(double interval) {
    double now = GetTime();
    if (now - last_update > interval) {
        last_update = now;
        return true;
    }
    return false;
}

// ─────────────────────────────────────────────
//  Helpers UI
// ─────────────────────────────────────────────
void draw_button(Rectangle r, const char* text, Color col) {
    DrawRectangleRounded(r, 0.3f, 6, col);
    DrawText(text, (int)(r.x + 10), (int)(r.y + 10), 20, WHITE);
}

bool button_clicked(Rectangle r, Vector2 mouse) {
    return CheckCollisionPointRec(mouse, r) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// ─────────────────────────────────────────────
//  Main
// ─────────────────────────────────────────────
int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);

    const char* ip_serveur = (argc >= 2) ? argv[1] : "127.0.0.1";

    Color darkblue = {44, 44, 127, 255};
    Color panel    = {59, 85, 162, 255};

    InitWindow(780, 620, "TETRIS ONLINE");
    InitAudioDevice();
    SetTargetFPS(60);

    Game jeu = Game();
    SetMusicVolume(jeu.music, 0.3f);
    SetSoundVolume(jeu.destroy_sound, 1.0f);
    SetSoundVolume(jeu.rotate_sound, 0.5f);

    State state = State::MENU;
    bool online_mode = false;

    // Volume slider
    float volume = 0.5f;
    Rectangle sliderBar  = {320, 580, 150, 8};
    Rectangle sliderKnob = {320 + 150 * volume - 6, 574, 12, 16};
    bool dragging = false;

    // Boutons principaux
    Rectangle btn_solo   = {320, 490, 120, 40};
    Rectangle btn_online = {450, 490, 120, 40};

    // Boutons chat
    Rectangle btn_gl   = {530,  30, 60, 50};
    Rectangle btn_wp   = {620,  30, 60, 50};
    Rectangle btn_wow  = {700,  30, 60, 50};
    Rectangle btn_thx  = {530, 100, 60, 50};
    Rectangle btn_gg   = {620, 100, 60, 50};
    Rectangle btn_oups = {700, 100, 60, 50};

    while (!WindowShouldClose()) {
        UpdateMusicStream(jeu.music);
        Vector2 mouse = GetMousePosition();

        // ─── Transitions réseau ───────────────────────
        if (state == State::CONNECTING && is_connected()) {
            network_start_listener();
            network_send("READY\n");
            state = State::WAITING;
            jeu.set_msg("En attente d'un adversaire...");
        }

        if (state == State::WAITING || state == State::PLAYING) {
            while (network_has_message()) {
                std::string msg = network_pop_message();

                if (msg == "MATCH_START") {
                    state = State::PLAYING;
                    jeu.waiting = false;
                    jeu.start   = true;
                    jeu.set_msg("C'est parti !");
                }
                else if (msg == "OPPONENT_LEFT") {
                    jeu.set_msg("Adversaire deconnecte.");
                    disconnect();
                    state = State::GAMEOVER;
                }
                else {
                    jeu.apply_network_message(msg);
                }

                // Vérifier si la partie online est finie (GAMEOVER reçu)
                if (jeu.fin_partie_online) {
                    jeu.fin_partie_online = false;
                    disconnect();
                    state = State::GAMEOVER;
                }
            }
        }

        // ─── Logique de jeu ───────────────────────────
        if (state == State::PLAYING) {
            jeu.input();
            if (event(0.2 / (jeu.get_niveau() + 1)))
                jeu.move_down();

            // Défaite locale
            if (jeu.justLost) {
                if (online_mode) {
                    network_send("GAMEOVER\n");
                    disconnect();
                }
                state = State::GAMEOVER;
            }

            // Envoyer les lignes gagnées à l'adversaire
            if (jeu.linesToSend > 0 && online_mode && is_connected()) {
                network_send("LINES|" + std::to_string(jeu.linesToSend) + "\n");
                jeu.linesToSend = 0;
            }
        }

        // ─── Dessin ───────────────────────────────────
        BeginDrawing();
        ClearBackground(darkblue);

        // Grille + pièce suivante
        jeu.dessiner();
        jeu.dessiner_next();
        jeu.destroy();

        // Labels fixes
        DrawText("Score", 345,  15, 38, WHITE);
        DrawText("level", 365, 125, 38, WHITE);
        DrawText("Next",  365, 210, 38, WHITE);

        // Cadres
        DrawRectangleRounded({320,  55, 170,  60}, 0.3f, 6, panel);
        DrawRectangleRounded({320, 160, 170,  40}, 0.3f, 6, panel);
        DrawRectangleRounded({320, 260, 170, 140}, 0.3f, 6, panel);
        DrawRectangleRounded({530, 180, 230, 300}, 0.3f, 6, panel);

        // Score + niveau
        char sc[16], lvl[16];
        sprintf(sc,  "%d", jeu.get_score());
        sprintf(lvl, "%d", jeu.get_niveau());
        DrawText(sc,  345,  70, 27, BLACK);
        DrawText(lvl, 345, 168, 27, BLACK);

        // Message d'état
        DrawText(jeu.get_msg().c_str(), 320, 420, 22, WHITE);

        // ─── Boutons selon l'état ─────────────────────
        switch (state) {

            case State::MENU: {
                draw_button(btn_solo,   "SOLO",   GREEN);
                draw_button(btn_online, "ONLINE", BLUE);

                if (button_clicked(btn_solo, mouse)) {
                    online_mode = false;
                    jeu.reset();
                    jeu.start = true;
                    state = State::PLAYING;
                }
                if (button_clicked(btn_online, mouse)) {
                    online_mode = true;
                    jeu.reset();
                    network_connect(ip_serveur);
                    state = State::CONNECTING;
                    jeu.set_msg("Connexion...");
                }
                break;
            }

            case State::CONNECTING: {
                DrawText("Connexion au serveur...", 320, 490, 20, YELLOW);
                break;
            }

            case State::WAITING: {
                DrawText("En attente d'un adversaire...", 320, 490, 18, YELLOW);
                // Bouton annuler
                Rectangle btn_cancel = {320, 530, 150, 36};
                draw_button(btn_cancel, "Annuler", RED);
                if (button_clicked(btn_cancel, mouse)) {
                    disconnect();
                    jeu.reset();
                    state = State::MENU;
                }
                break;
            }

            case State::PLAYING: {
                // Bouton pause (solo uniquement)
                if (!online_mode) {
                    Rectangle btn_pause = {320, 490, 120, 40};
                    Color col = jeu.start ? GREEN : RED;
                    draw_button(btn_pause, jeu.start ? "PAUSE" : "REPRENDRE", col);
                    if (button_clicked(btn_pause, mouse))
                        jeu.start = !jeu.start;
                }
                break;
            }

            case State::GAMEOVER: {
                DrawText("GAME OVER", 330, 460, 30, RED);
                Rectangle btn_retry  = {320, 510, 120, 40};
                Rectangle btn_menu   = {450, 510, 120, 40};
                draw_button(btn_retry, "REJOUER", GREEN);
                draw_button(btn_menu,  "MENU",    BLUE);

                if (button_clicked(btn_retry, mouse)) {
                    jeu.reset();
                    if (online_mode) {
                        network_connect(ip_serveur);
                        state = State::CONNECTING;
                        jeu.set_msg("Connexion...");
                    } else {
                        jeu.start = true;
                        state = State::PLAYING;
                    }
                }
                if (button_clicked(btn_menu, mouse)) {
                    jeu.reset();
                    online_mode = false;
                    state = State::MENU;
                }
                break;
            }
        }

        // ─── Chat (visible seulement en online) ───────
        if (online_mode && (state == State::PLAYING || state == State::WAITING)) {
            auto send_chat = [&](Rectangle btn, const char* label, const char* msg_text) {
                DrawRectangleRounded(btn, 0.3f, 6, WHITE);
                DrawText(label, (int)(btn.x + 8), (int)(btn.y + 12), 15, BLACK);
                if (button_clicked(btn, mouse) && is_connected()) {
                    network_send(std::string("CHAT|") + msg_text + "\n");
                    jeu.ajouter_msg(msg_text, false);
                    jeu.max_chat++;
                }
            };

            send_chat(btn_gl,   "Good\nluck!", "Good luck!");
            send_chat(btn_wp,   "Well\nplayed!", "Well Played!");
            send_chat(btn_wow,  "Wow!",          "Wow!");
            send_chat(btn_thx,  "Thanks!",       "Thanks!");
            send_chat(btn_gg,   "Good\nGame!",   "Good Game!");
            send_chat(btn_oups, "Oops!",         "Oops!");

            jeu.draw_msg();
        }

        // ─── Volume slider ────────────────────────────
        DrawText("Volume", 320, 550, 20, WHITE);
        DrawRectangleRounded(sliderBar,  0.5f, 6, GRAY);
        DrawRectangleRounded(sliderKnob, 0.5f, 6, WHITE);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(mouse, sliderKnob)) dragging = true;
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) dragging = false;

        if (dragging) {
            sliderKnob.x = mouse.x - sliderKnob.width / 2;
            sliderKnob.x = std::max(sliderKnob.x, sliderBar.x);
            sliderKnob.x = std::min(sliderKnob.x, sliderBar.x + sliderBar.width - sliderKnob.width);
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
