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

#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>

#include "core/bitboard.h"
#include "core/move.h"
#include "eval/nnue.h"
#include "types.h"

struct BoardState {
    Piece captured;
    int fifty_move_ply;
    int ply_from_null;
    uint8_t castling_rights;
    Square en_passant;
    Bitboard checkers;
    Bitboard pins;
    Bitboard castle_rooks;
    Bitboard threats;
    void reset() {
        checkers = 0;
        pins = 0;
        captured = EMPTY;
        fifty_move_ply = 0;
        ply_from_null = 0;
        castling_rights = NO_CASTLING;
        en_passant = NO_SQ;
        castle_rooks = 0;
        threats = 0;
    }
};

class Position {
  public:
    Position() = default;
    ~Position() = default;

    bool set_fen(const std::string &fen);
    std::string get_fen() const;

    void reset();

    DirtyPiece make_move(const Move &move);
    void unmake_move(const Move &move);

    void make_null_move();
    void unmake_null_move();

    inline bool in_check() const { return m_curr_state.checkers != Bitboard::EMPTY; }
    inline bool is_threatened(const Square &sq) const { return m_curr_state.threats.is_set(sq); }
    inline Bitboard threats_bb() const { return m_curr_state.threats; }
    bool is_attacked(const Square &sq) const;
    bool is_legal(const Move &move);
    bool is_pseudo_legal(const Move &move) const;
    Bitboard attackers(const Square &sq) const;

    inline bool last_was_null() const { return m_curr_state.ply_from_null == 0; }
    inline bool has_non_pawns() const {
        return piece_bb(KNIGHT) || piece_bb(BISHOP) || piece_bb(ROOK) || piece_bb(QUEEN);
    }
    inline bool is_draw() { return insufficient_material() || repetition() || is_fifty_move_draw(); }

    int legal_move_amount();
    void print() const;

    inline Bitboard occ_bb() const { return m_occupancies[WHITE] | m_occupancies[BLACK]; }
    inline Bitboard occ_bb(const Color &color) const {
        assert(color == WHITE || color == BLACK);
        return m_occupancies[color];
    }
    inline Bitboard piece_bb(const Piece &piece) const {
        assert(piece >= WHITE_PAWN && piece <= BLACK_KING);
        return m_pieces[piece];
    }
    inline Bitboard piece_bb(const PieceType &piece_type, const Color &color) const {
        return piece_bb(static_cast<Piece>(piece_type + color * COLOR_OFFSET));
    }
    inline Bitboard piece_bb(const PieceType &piece_type) const {
        return m_pieces[piece_type] | m_pieces[piece_type + COLOR_OFFSET];
    }
    inline Square king_sq(const Color &color) const { return m_pieces[KING + color * COLOR_OFFSET].lsb(); }
    inline uint8_t castling_rights() const { return m_curr_state.castling_rights; }
    inline Color stm() const { return m_stm; }
    inline Color nstm() const { return static_cast<Color>(m_stm ^ 1); }
    inline Square ep_sq() const { return m_curr_state.en_passant; }
    inline HashType hash() const { return m_position_hash; }
    inline HashType pawn_hash() const { return m_pawn_hash; }
    inline HashType white_nonpawn_hash() const { return m_white_non_pawn_hash; }
    inline HashType black_nonpawn_hash() const { return m_black_non_pawn_hash; }
    inline int game_ply() const { return m_game_clock_ply; }
    inline int halfmove_clock() const { return m_curr_state.fifty_move_ply; }
    inline int material_count(const Piece &piece) const { return piece_bb(piece).popcount(); }
    inline int material_count(const PieceType &piece_type, const Color &color) const {
        return material_count(static_cast<Piece>(piece_type + color * COLOR_OFFSET));
    }
    inline int material_count(const PieceType &piece_type) const {
        return (m_pieces[piece_type] | m_pieces[piece_type + COLOR_OFFSET]).popcount();
    }
    inline int material_count() const { return occ_bb().popcount(); }
    inline Piece piece_at(const Square &sq) const { return m_board[sq]; }
    inline int history_ply() const { return m_history_ply; }
    inline BoardState board_state() const { return m_curr_state; };
    inline Bitboard checkers_bb() const { return m_curr_state.checkers; }
    inline Bitboard pins_bb() const { return m_curr_state.pins; }
    inline Bitboard castle_rooks_bb() const { return m_curr_state.castle_rooks; }
    inline void reset_history() { m_history_ply = 0; }

    // if there is more that 100 positions in the game history stacks, clean up the first ones by shift the array
    void update_game_history() {
        if (m_history_ply <= 100) // nothing to do
            return;

        memmove(m_history_stack, m_history_stack + m_history_ply - 100, sizeof(BoardState) * 100);
        memmove(m_played_positions, m_played_positions + m_history_ply - 100, sizeof(HashType) * 100);

        m_history_ply = 100;
    }

  private:
    void add_piece(const PieceSquare &ps);
    void remove_piece(const PieceSquare &ps);

    DirtyPiece make_regular(const Move &move);
    DirtyPiece make_capture(const Move &move);
    DirtyPiece make_castle(const Move &move);
    DirtyPiece make_promotion(const Move &move);
    DirtyPiece make_en_passant(const Move &move);

    void update_castling_rights(const Move &move);
    void calculate_aux_bbs();
    void calculate_threats_bb();

    bool insufficient_material() const;
    bool repetition() const;
    bool is_fifty_move_draw();

    bool pawn_pseudo_legal(const Square &from, const Square &to, const Move &move) const;
    bool castling_pseudo_legal(const Square &from, const Square &to, const PieceType &moved_piece_type) const;

    void hash_piece_key(const PieceSquare &ps);
    void hash_castle_key();
    void hash_ep_key();
    void hash_side_key();

    inline void change_side() { m_stm = static_cast<Color>(m_stm ^ 1); }

    Piece m_board[64];
    Bitboard m_occupancies[2];
    Bitboard m_pieces[12];

    Color m_stm;
    HashType m_position_hash;
    HashType m_pawn_hash;
    HashType m_white_non_pawn_hash;
    HashType m_black_non_pawn_hash;
    int m_game_clock_ply;

    int m_history_ply;
    BoardState m_curr_state;
    BoardState m_history_stack[MAX_PLY];
    HashType m_played_positions[MAX_PLY];
};
