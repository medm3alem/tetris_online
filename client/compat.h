#ifndef COMPAT_H
#define COMPAT_H

#include "raylib.h"

// Raylib 5.0 (Ubuntu/Windows) = 5 args (avec lineThick)
// Raylib 5.5 (macOS Homebrew) = 4 args (sans lineThick)
// On détecte : si MINOR >= 5 → ancienne API 4 args
inline void RLDrawRoundedLines(Rectangle rec, float round, int seg, Color col) {
#if RAYLIB_VERSION_MINOR >= 5 && RAYLIB_VERSION_MAJOR == 5
    DrawRectangleRoundedLines(rec, round, seg, col);
#else
    DrawRectangleRoundedLines(rec, round, seg, 1.0f, col);
#endif
}

#endif
