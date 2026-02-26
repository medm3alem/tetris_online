#ifndef COMPAT_H
#define COMPAT_H

#include "raylib.h"

// Raylib 5.5 (Homebrew macOS) = 4 args
// Raylib 5.0+ Linux/Windows   = 5 args
// On détecte via la version mineure
#if RAYLIB_VERSION_MAJOR > 5 || \
   (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 6)
  #define RLDrawRoundedLines(rec,round,seg,col) \
      DrawRectangleRoundedLines((rec),(round),(seg),1.0f,(col))
#else
  #define RLDrawRoundedLines(rec,round,seg,col) \
      DrawRectangleRoundedLines((rec),(round),(seg),(col))
#endif

#endif
