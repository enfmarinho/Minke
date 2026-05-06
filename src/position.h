/*
 *  Minke is a UCI chess engine
 *  Copyright (C) 2026 Eduardo Marinho <eduardomarinho@pm.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef POSITION_H
#define POSITION_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>

#include "eval/nnue.h"
#include "move.h"
#include "types.h"
#include "utils.h"

struct Board {
    Piece mailbox[64];
    Bitboard piece_bbs[12];
    Bitboard occupancy_bbs[2];
    Bitboard checkers_bb;
    Bitboard pins_bb;
    Bitboard castle_rooks_bb;

    int ply_from_null;
    int fifty_move_ply;
    int history_ply;
    int game_clock_ply;

    HashType hash_key;

    uint8_t castling_rights;
    Square en_passant;
    Color stm;
    // 5 bytes padding

    void reset();
};

class Position {
  public:
    Position();
    ~Position() = default;

    template <bool UPDATE_NNUE>
    bool set_fen(const std::string &fen);
    std::string fen() const;

    template <bool UPDATE_NNUE>
    void reset();
    void reset_nnue();

    template <bool UPDATE_NNUE>
    bool make_move(const Move &move);
    template <bool UPDATE_NNUE>
    void unmake_move();

    void make_null_move();
    void unmake_null_move();

    inline bool in_check() const { return checkers(); }
    bool is_attacked(const Square &sq) const;
    bool is_legal(const Move &move);
    bool is_pseudo_legal(const Move &move) const;
    Bitboard attackers(const Square &sq) const;

    inline bool last_was_null() const { return ply_from_null() == 0; }
    inline bool has_non_pawns() const {
        return piece_bb(KNIGHT) || piece_bb(BISHOP) || piece_bb(ROOK) || piece_bb(QUEEN);
    }
    inline bool draw() { return insufficient_material() || repetition() || fifty_move_draw(); }
    inline ScoreType eval() { return m_nnue.eval(stm()); }

    int legal_move_amount();
    bool no_legal_moves();
    void print();

    inline Bitboard &occupancy(const Color &color) {
        assert(color == WHITE || color == BLACK);
        return board().occupancy_bbs[color];
    }
    inline Bitboard occupancy(const Color &color) const {
        assert(color == WHITE || color == BLACK);
        return board().occupancy_bbs[color];
    }
    inline Bitboard occupancy() const { return occupancy(WHITE) | occupancy(BLACK); }

    inline Bitboard &piece_bb(const Piece &piece) {
        assert(piece >= WHITE_PAWN && piece <= BLACK_KING);
        return board().piece_bbs[piece];
    }
    inline Bitboard piece_bb(const Piece &piece) const {
        assert(piece >= WHITE_PAWN && piece <= BLACK_KING);
        return board().piece_bbs[piece];
    }
    inline Bitboard piece_bb(const PieceType &piece_type, const Color &color) const {
        return piece_bb(static_cast<Piece>(piece_type + color * COLOR_OFFSET));
    }
    inline Bitboard piece_bb(const PieceType &piece_type) const {
        return board().piece_bbs[piece_type] | board().piece_bbs[piece_type + COLOR_OFFSET];
    }
    inline Square king_placement(const Color &color) const {
        return lsb(board().piece_bbs[KING + color * COLOR_OFFSET]);
    }

    inline Bitboard &checkers() { return board().checkers_bb; }
    inline Bitboard checkers() const { return board().checkers_bb; }
    inline Bitboard &pins() { return board().pins_bb; }
    inline Bitboard pins() const { return board().pins_bb; }

    inline uint8_t &castling_rights() { return board().castling_rights; }
    inline uint8_t castling_rights() const { return board().castling_rights; }
    inline Color &stm() { return board().stm; }
    inline Color stm() const { return board().stm; }
    inline Color ntm() const { return static_cast<Color>(stm() ^ 1); }
    inline Square &en_passant() { return board().en_passant; }
    inline Square en_passant() const { return board().en_passant; }
    inline HashType hash() const { return board().hash_key; }
    inline HashType &hash() { return board().hash_key; }
    inline int &game_ply() { return board().game_clock_ply; }
    inline int game_ply() const { return board().game_clock_ply; }
    inline int &fifty_move_ply() { return board().fifty_move_ply; }
    inline int fifty_move_ply() const { return board().fifty_move_ply; }
    inline int &ply_from_null() { return board().ply_from_null; }
    inline int ply_from_null() const { return board().ply_from_null; }
    inline int material_count(const Piece &piece) const { return count_bits(piece_bb(piece)); }
    inline int material_count(const PieceType &piece_type, const Color &color) const {
        return material_count(static_cast<Piece>(piece_type + color * COLOR_OFFSET));
    }
    inline int material_count(const PieceType &piece_type) const {
        return count_bits(board().piece_bbs[piece_type] | board().piece_bbs[piece_type + COLOR_OFFSET]);
    }
    inline int material_count() const { return count_bits(occupancy()); }
    inline Piece &consult(const Square &sq) { return board().mailbox[sq]; }
    inline Piece consult(const Square &sq) const { return board().mailbox[sq]; }
    inline int history_ply() const { return m_history_top; }
    inline Bitboard &castle_rooks() { return board().castle_rooks_bb; }
    inline Bitboard castle_rooks() const { return board().castle_rooks_bb; }
    inline void reset_game_history() {
        m_history_top = 0;
        reset_board_history();
    }
    inline void reset_board_history() {
        std::swap(m_board_stack[0], m_board_stack[m_stack_top]);
        m_stack_top = 0;
    }

    // if there is more that 100 positions in the game history stacks, clean up the first ones by shift the array
    // TODO this is necessary for datagen
    // void update_game_history() {
    //     if (m_history_top <= 100) // nothing to do
    //         return;
    //
    //     memmove(m_history_stack, m_history_stack + m_history_ply - 100, sizeof(BoardState) * 100);
    //     memmove(m_history, m_history + m_history_top - 100, sizeof(HashType) * 100);
    //
    //     m_history_top = 100;
    // }

  private:
    template <bool UPDATE_NNUE>
    void add_piece(const PieceSquare &ps);
    template <bool UPDATE_NNUE>
    void remove_piece(const PieceSquare &ps);

    template <bool UPDATE_NNUE>
    DirtyPiece make_regular(const Move &move);
    template <bool UPDATE_NNUE>
    DirtyPiece make_capture(const Move &move);
    template <bool UPDATE_NNUE>
    DirtyPiece make_castle(const Move &move);
    template <bool UPDATE_NNUE>
    DirtyPiece make_promotion(const Move &move);
    template <bool UPDATE_NNUE>
    DirtyPiece make_en_passant(const Move &move);

    void update_castling_rights(const Move &move);
    void update_pin_and_checkers_bb();

    bool insufficient_material() const;
    bool repetition() const;
    bool fifty_move_draw();

    bool pawn_pseudo_legal(const Square &from, const Square &to, const Move &move) const;
    bool castling_pseudo_legal(const Square &from, const Square &to, const PieceType &moved_piece_type) const;

    void hash_piece_key(const PieceSquare &ps);
    void hash_castle_key();
    void hash_ep_key();
    void hash_side_key();

    inline void change_side() { stm() = static_cast<Color>(stm() ^ 1); }

    inline Board &board() { return m_board_stack[m_stack_top]; }
    inline const Board &board() const { return m_board_stack[m_stack_top]; }

    inline void pop_board() {
        // check if there is a move to unmake
        assert(m_history_top > 0);
        assert(m_stack_top > 0);

        --m_history_top;
        --m_stack_top;
    }
    inline void push_board() {
        assert(m_stack_top < MAX_SEARCH_DEPTH);
        assert(m_history_top <= MAX_PLY);
        m_history[m_history_top++] = board().hash_key;

        m_board_stack[m_stack_top + 1] = m_board_stack[m_stack_top];
        ++m_stack_top;
    }

    HashType m_history[MAX_PLY]; //!< all played positions for draw detection
    size_t m_history_top;        //!< index to the top of the key history stack

    Board m_board_stack[MAX_SEARCH_DEPTH];
    size_t m_stack_top; //!< index to the top of the board stack

    NNUE m_nnue;
};

#endif // #ifndef POSITION_H
