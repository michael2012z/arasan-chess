// Copyright 1992-2019 by Jon Dart. All Rights Reserved.

#ifndef _SCORING_H
#define _SCORING_H

#include "board.h"
#include "hash.h"
#include "attacks.h"
#include "params.h"

#ifdef TUNE
#include <array>
#endif

#include <iostream>
using namespace std;

class Scoring
{
    // This class does static evaluation of chess positions.

    public:

    enum {Midgame = 0, Endgame = 1};

    static void init();

    static void cleanup();

    Scoring();

    ~Scoring();

    // evaluate "board" from the perspective of the side to move.
    score_t evalu8( const Board &board, bool useCache = true );

    // checks for draw by repetition (returning repetition count) +
    // other draw situations. It is intended to be called from the
    // interior of the search. It does not strictly detect legal
    // draws: use isLegalDraw for that.
    static int isDraw( const Board &board, int &rep_count, int ply);

    static int repetitionDraw( const Board &board );

    static int materialDraw( const Board &board ) {
        return board.materialDraw();
    }

    static int fiftyMoveDraw(const Board &board);

    // Check only for legal draws
    static int isLegalDraw(const Board &board);

    static int theoreticalDraw(const Board &board);

    // Turn a score into a formatted string (mate scores are
    // shown like +Mate6).
    static void printScore( score_t score, ostream & );

    // Output scores in the format required by the UCI protocol.
    static void printScoreUCI( score_t score, ostream & );

    static bool mateScore(score_t score) {
      return score != Constants::INVALID_SCORE &&
        (score>=Constants::TABLEBASE_WIN || score<=-Constants::TABLEBASE_WIN);
    }

    // Try to return a score based on bitbases, INVALID_SCORE if not found
    static score_t tryBitbase(const Board &board);

    void clearHashTables();

    // return a material score
    score_t materialScore( const Board &board ) const;

    score_t outpost(const Board &board, Square sq, ColorType side) const;

    int outpost_defenders(const Board &board,
                          Square sq, ColorType side) const {
      return (Attacks::pawn_attacks[sq][side] & board.pawn_bits[side]).bitCountOpt();
    }

    struct PawnDetail {
      static const int PASSED=1;
      static const int POTENTIAL_PASSER=2;
      static const int CONNECTED_PASSER=4;
      static const int ADJACENT_PASSER=8;
      static const int BACKWARD=16;
      static const int DOUBLED=32;
      static const int TRIPLED=64;
      static const int WEAK=128;
      static const int ISOLATED=256;
      uint16_t flags;
      byte space_weight;
      Square sq;
    };

    typedef PawnDetail PawnDetails[8];

#ifdef TUNE
    static const int PAWN_HASH_SIZE = 8096;
#else
    static const int PAWN_HASH_SIZE = 16384;
#endif
    static const int KING_PAWN_HASH_SIZE = 8132;

    static CACHE_ALIGN Bitboard kingProximity[2][64];
    static CACHE_ALIGN Bitboard kingNearProximity[64];
    static CACHE_ALIGN Bitboard kingPawnProximity[2][4][64];

    struct PawnHashEntry {

       hash_t hc;

       struct PawnData {
           Bitboard passers;
           Bitboard opponent_pawn_attacks;
           Bitboard weak_pawns;
	   byte weakopen;
           byte pawn_file_mask;
           byte passer_file_mask;
           byte pad;
#ifdef TUNE
           score_t endgame_score, midgame_score;
#else
           int32_t endgame_score, midgame_score;
#endif
  	   int w_square_pawns,b_square_pawns;
           int outside;
#ifdef TUNE
         int center_pawn_factor;
#endif
         PawnDetail details[8];
       } wPawnData, bPawnData;

       const PawnData &pawnData(ColorType side) const {
	 return (side==White) ? wPawnData : bPawnData;
       }
    } pawnHashTable[PAWN_HASH_SIZE];

    struct KingPawnHashEntry {
       hash_t hc;
#ifdef TUNE
       score_t cover;
       score_t storm;
       score_t pawn_attacks;
       score_t king_endgame_position;
       float counts[6][4];
       int pawn_attack_count;
       array<int,8> storm_counts;
#else
       int32_t cover, storm, pawn_attacks;
       int32_t king_endgame_position;
#endif
    };

    KingPawnHashEntry kingPawnHashTable[2][KING_PAWN_HASH_SIZE];

    PawnHashEntry &pawnEntry(const Board &board, bool useCache);

    template <ColorType side>
      KingPawnHashEntry &getKPEntry(const Board &board,
                        const PawnHashEntry::PawnData &ourPawnData,
                        const PawnHashEntry::PawnData &oppPawnData,
                        bool useCache);

    static int tradeDownIndex(const Material &ourmat, const Material &oppmat);

    static int distance(Square sq1, Square sq);

    // Public, for use by the tuner.
    void calcCover(const Board &board, ColorType side, KingPawnHashEntry &cover);

    void calcStorm(const Board &board, ColorType side, KingPawnHashEntry &cover, const Bitboard &oppPawnAttacks);

#ifdef TUNE
    score_t kingAttackSigmoid(score_t weight) const;
#endif

    // The scores for opening, middlegame and endgame
    struct Scores {
      Scores()
      :mid(0), end(0), any(0)
      {
      }
      Scores(const Scores &s)
      :mid(s.mid),end(s.end),any(s.any) {
      }
      score_t mid, end, any;
      score_t blend(int materialLevel ) {
        return any + mid*Params::MATERIAL_SCALE[materialLevel]/128 +
          end*(128-Params::MATERIAL_SCALE[materialLevel])/128;
      }

      int operator == (const Scores &s) const {
        return s.mid == mid && s.end == end && s.any == any;
      }
      int operator != (const Scores &s) const {
        return s.mid != mid || s.end != end || s.any != any;
      }
    };

    template <ColorType side>
      int specialCaseEndgame(const Board &,
                             const Material &ourMaterial,
                             const Material &oppMaterial,
                             Scores &);

 private:

    template <ColorType side>
     void  positionalScore( const Board &board,
                            const PawnHashEntry &pawnEntry,
                            const KingPawnHashEntry &ourKPEntry,
                            const KingPawnHashEntry &oppKPEntry,
                            Scores &scores,
                            Scores &oppScores);

    template <ColorType side>
        static int theoreticalDraw(const Board &board);

    score_t adjustMaterialScore(const Board &board, ColorType side) const;

    score_t adjustMaterialScoreNoPawns(const Board &board, ColorType side) const;

    template <ColorType bishopColor>
      void scoreBishopAndPawns(const PawnHashEntry::PawnData &ourPawnData,const PawnHashEntry::PawnData &oppPawnData,Scores &scores,Scores &opp_scores);

   template <ColorType side>
    void pieceScore(const Board &board,
                    const PawnHashEntry::PawnData &ourPawnData,
		    const PawnHashEntry::PawnData &oppPawnData,
                    const KingPawnHashEntry &oppKPEntry,
                    Scores &, Scores &opp_scores,
                    bool early_endgame,
                    bool deep_endgame);

#ifdef TUNE
    static score_t calcCover(const Board &board, ColorType side, int file, int rank, int (&counts)[6][4]);
#else
    static score_t calcCover(const Board &board, ColorType side, int file, int rank);
#endif

    // Compute king cover for King on square 'kp' of color 'side'
#ifdef TUNE
    static score_t calcCover(const Board &board, ColorType side, Square kp, int (&counts)[6][4]);
#else
    static score_t calcCover(const Board &board, ColorType side, Square kp);
#endif

    void calcPawnData(const Board &board, ColorType side,
			   PawnHashEntry::PawnData &entr);

    void calcKingEndgamePosition(const Board &board,ColorType side,
                                 const PawnHashEntry::PawnData &oppPawnData,
                                 KingPawnHashEntry &entry);

    void evalOutsidePassers(PawnHashEntry &pawnEntry);

    void calcPawnEntry(const Board &board, PawnHashEntry &pawnEntry);

    void pawnScore(const Board &board, ColorType side,
		  const PawnHashEntry::PawnData &oppPawnData, Scores &);

    template <ColorType side>
      void scoreEndgame(const Board &,score_t k_pos,Scores &);

    score_t kingDistanceScore(const Board &) const;

    template<ColorType side>
      static int KBPDraw(const Board &board);

    static void initBitboards();

    template<ColorType side>
      static void initProximity(Square i);

};

#endif

