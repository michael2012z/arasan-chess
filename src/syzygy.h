// Copyright 2016, 2018 by Jon Dart. All Rights Reserved.
#ifndef _SYZYGY_H_
#define _SYZYGY_H_

#include "board.h"
// To avoid conflict with Gaviota tablebases:
#define tb_init syzygy_tb_init
extern "C"
{
#include "syzygy/src/tbprobe.h"
};
#undef tb_init

// Support for Syzygy tablebases. Interfaces between Arasan
// datatypes and the "Fathom" probing code by Roland de Man.

struct SyzygyTb {

    static const score_t CURSED_SCORE;

    // Initialize the tablebases. 'path' is the
    // path to the TB directories.
    // Returns the highest number of pieces that the tbs
    // support (3/4/5/6).
    static int initTB(const string &path);

    // Probe the tablebases. "score" is the score for the position.
    // If found and the score is winning or drawing then
    // "rootMoves" is filled with the moves that preserve
    // the draw or win.
    // Returns the minimum DTZ value if score was obtained,
    // -1 if not.
    static int probe_root(const Board &b, score_t &score, MoveSet &rootMoves);

    // Probe the wdl tablebases (not at root).
    // Return 1 if score was obtained,
    // 0 if not. "score" is the score for the position.
    static int probe_wdl(const Board &b, score_t &score, bool use50MoveRule);

};

#endif
