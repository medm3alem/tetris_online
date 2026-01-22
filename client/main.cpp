#include "raylib.h"
#include "objets.h"
#include "game.h"
#include <string>
#include "network.h"
#include <csignal>


double last_update = 0;
bool connected = false;
bool started = false;
bool block_start = false;
bool block_mode = false;
bool connecting = false;
bool waiting_opponent = true;

bool event(double time) {
    double current_time = GetTime();
    if (current_time - last_update > time) {
        last_update = current_time;
        return true;
    }
    return false;
}



int main() {
    signal(SIGPIPE, SIG_IGN); // pour ignorer quand on envoie sur une socket fermée

    Color darkblue = {44,44,127, 255};
    InitWindow(600, 620, "TETRIS");
    InitAudioDevice();
    //SetAudioStreamBufferSizeDefault(4096);
    SetTargetFPS(60);
    Game jeu = Game();

//jeu.mode : false solo
//            true en ligne





    SetMusicVolume(jeu.music, 0.3f);
    SetSoundVolume(jeu.destroy_sound, 1.0f);
    SetSoundVolume(jeu.rotate_sound, 0.5f);
    ClearBackground(darkblue);

    float volume = 0.5f;

    Rectangle sliderBar   = { 320, 580, 150, 8 };
    Rectangle sliderKnob  = { 320 + 150 * volume - 6, 574, 12, 16 };

    bool dragging = false;

    while (!WindowShouldClose()) {
        UpdateMusicStream(jeu.music);


        if (jeu.mode && !connected && !connecting) {
            // On vient de passer en mode online
            std::cout << "Switching to ONLINE mode - connecting to server..." << std::endl;
            network_connect();
            connecting = true;
        }

        if (jeu.mode && !connected && is_connected()) {
            // Connexion établie
            std::cout << "Connected to server!" << std::endl;
            network_start_listener();
            connected = true;
            connecting = false;
            network_send("READY\n");
        }


        if (!jeu.mode && connected) {
            // On vient de passer en mode solo
            std::cout << "Switching to SOLO mode - disconnecting..." << std::endl;
            disconnect();
            connected = false;
            connecting = false;
        }

        if (jeu.fin_partie_online && connected) {
            // La partie en ligne est terminée
            std::cout << "Online game finished - disconnecting..." << std::endl;
            /*
            disconnect();
            jeu.reset();
            jeu.set_msg("Victory!");
            jeu.justLost = false;
            connected = false;
            connecting = false;*/


            jeu.fin_partie_online = false;
            disconnect();
            jeu.reset();

            jeu.set_msg("Victory!");
            block_mode = false;
            block_start = false;
            jeu.mode = false;
            jeu.justLost = false; //
            connected = false;
            connecting = false;


/*
            jeu.justLost = false;
            disconnect();
            jeu.reset();
            jeu.set_msg("Victory!");
            jeu.mode = false;
            connected = false;
            connecting = false;*/
        }

        if (connected){
            while (network_has_message()) {
                std::string msg = network_pop_message();

                if (msg == "MATCH_START") {
                    std::cout << "Opponent found!\nStarting game" << std::endl;
                    jeu.set_msg("Opponent found!\n Starting...");
                    waiting_opponent = false;
                    jeu.waiting = false;
                    jeu.start = true;
                    block_start = false;
                }
                else jeu.apply_network_message(msg);
                std::cout << "Processing message: " << msg << std::endl;

            }
        }




        if (jeu.start && (!jeu.mode || !waiting_opponent)) jeu.input();
        if (event(0.2/(jeu.get_niveau()+1)) && jeu.start && (!jeu.mode || !waiting_opponent)) jeu.move_down();

        BeginDrawing();
        ClearBackground(darkblue);
        DrawText("Score", 345,15, 38, WHITE);
        DrawText("level", 365,125, 38, WHITE);
        DrawText("Next", 365,210, 38, WHITE);



        Color col_mode;
        Color col_start;

        Rectangle btn_mode = { 320, 490, 120, 40 };
        Rectangle btn_start = { 450, 490, 120, 40 };

        Vector2 mouse = GetMousePosition();

        bool hover_mode = CheckCollisionPointRec(mouse, btn_mode);

        if (hover_mode && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !block_mode && ! jeu.justLost) {
            jeu.mode = ! jeu.mode;
            if (jeu.mode){
                block_start = true;
                block_mode = true;
                waiting_opponent = true;
            }
            else block_start = false;
        }

        bool hover_start = CheckCollisionPointRec(mouse, btn_start);

        if (hover_start && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && (jeu.waiting || !jeu.mode) && ! jeu.justLost) {
            jeu.start = !jeu.start;
            started = true;
            block_mode = true;
            jeu.waiting = false;
        }



        if (! block_mode && !jeu.justLost) col_mode = jeu.mode ? GREEN : RED;
        else {
            col_mode = GRAY;
        }


        const char* text_start = "PAUSED";

        if ((jeu.waiting || !jeu.mode) && !jeu.justLost) {
            if (jeu.start) {
                col_start = GREEN;
                text_start = "START";
            } else {
                col_start = RED;
                text_start = "PAUSED";
            }
        } else {
            col_start = GRAY;
            text_start = "START";
        }



        const char* text_mode = (jeu.mode) ? "ONLINE" : "SOLO";


        DrawRectangleRounded(btn_mode, 0.3f, 6, col_mode);
        DrawText(
            text_mode,
            btn_mode.x + 10,
            btn_mode.y + 10,
            20,
            WHITE
        );

        DrawRectangleRounded(btn_start, 0.3f, 6, col_start);
        DrawText(
            text_start,
            btn_start.x + 10,
            btn_start.y + 10,
            20,
            WHITE
        );



        // volume slider
        DrawText("Volume", 320, 550, 20, WHITE);
        DrawRectangleRounded(sliderBar, 0.5f, 6, GRAY);
        DrawRectangleRounded(sliderKnob, 0.5f, 6, WHITE);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(mouse, sliderKnob)) {
            dragging = true;
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            dragging = false;
        }

        if (dragging) {
            sliderKnob.x = mouse.x - sliderKnob.width / 2;

            if (sliderKnob.x < sliderBar.x)
                sliderKnob.x = sliderBar.x;

            if (sliderKnob.x > sliderBar.x + sliderBar.width - sliderKnob.width)
                sliderKnob.x = sliderBar.x + sliderBar.width - sliderKnob.width;

            volume = (sliderKnob.x - sliderBar.x) /
                     (sliderBar.width - sliderKnob.width);

            SetMasterVolume(volume);
        }
        // fin volume slider


        DrawText(jeu.get_msg().c_str(), 320, 440, 27, WHITE);
        DrawRectangleRounded({320, 55, 170, 60}, 0.3, 6, {59, 85, 162, 255}); // cadre score
        DrawRectangleRounded({320, 260, 170, 140}, 0.3, 6, {59, 85, 162, 255});// cadre next
        DrawRectangleRounded({320, 160, 170, 40}, 0.3, 6, {59, 85, 162, 255});// cadre level

        jeu.dessiner();
        jeu.dessiner_next();
        jeu.destroy();

        // Envoi des lignes à l'adversaire
        if (jeu.linesToSend > 0 && connected) {
            std::string msg = "LINES|" + std::to_string(jeu.linesToSend) + "\n";
            network_send(msg);
            std::cout << "Sending " << jeu.linesToSend << " lines to opponent" << std::endl;
            jeu.linesToSend = 0;
        }

        if (jeu.justLost){
            started = false;
            block_mode = false;
            block_start = false;
        }

        // Envoi du game over
        if (jeu.justLost && connected) {

            network_send("GAMEOVER\n");
            std::cout << "Sending GAMEOVER" << std::endl;
            jeu.justLost = false;
            disconnect();
            jeu.mode = false;
            connected = false;
            connecting = false;
        }



        char sc[10];
        sprintf(sc, "%d", jeu.get_score());
        DrawText(sc, 345,70, 27, BLACK);

        char lvl[10];
        sprintf(lvl, "%d", jeu.get_niveau());
        DrawText(lvl, 345,168, 27, BLACK);

        EndDrawing();
    }

    if (connected) {
        disconnect();
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
