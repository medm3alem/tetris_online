#ifndef COMPAT_H
#define COMPAT_H

#include "raylib.h"

// Wrapper inline évite les problèmes de macro avec les arguments contenant des virgules
inline void RLDrawRoundedLines(Rectangle rec, float round, int seg, Color col) {
#if defined(RAYLIB_VERSION_MAJOR) && (RAYLIB_VERSION_MAJOR > 5 || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 1))
    DrawRectangleRoundedLines(rec, round, seg, 1.0f, col);
#else
    DrawRectangleRoundedLines(rec, round, seg, col);
#endif
}

#endif
