/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef POSITION_H
#define POSITION_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>

#include "move.h"
#include "nnue.h"
#include "types.h"
#include "utils.h"

class Position {
  public:
    Position();
    ~Position() = default;

    template <bool UPDATE>
    bool set_fen(const std::string &fen);
    std::string get_fen() const;

    template <bool UPDATE>
    void reset();
    void reset_nnue();

    template <bool UPDATE>
    bool make_move(const Move &move);
    template <bool UPDATE>
    void unmake_move(const Move &move);

    void make_null_move();
    void unmake_null_move();

    inline bool in_check() const { return m_curr_state.checkers; }
    bool is_attacked(const Square &sq) const;
    bool is_legal(const Move &move);
    bool is_pseudo_legal(const Move &move) const;
    Bitboard attackers(const Square &sq) const;

    inline bool last_was_null() const { return m_curr_state.ply_from_null == 0; }
    inline bool has_non_pawns() const {
        return get_piece_bb(KNIGHT) || get_piece_bb(BISHOP) || get_piece_bb(ROOK) || get_piece_bb(QUEEN);
    }
    inline bool draw() { return insufficient_material() || repetition() || fifty_move_draw(); }
    inline ScoreType eval() const { return m_nnue.eval(m_stm); }

    int legal_move_amount();
    bool no_legal_moves();
    void print() const;

    inline Bitboard get_occupancy() const { return m_occupancies[WHITE] | m_occupancies[BLACK]; }
    inline Bitboard get_occupancy(const Color &color) const {
        assert(color == WHITE || color == BLACK);
        return m_occupancies[color];
    }
    inline Bitboard get_piece_bb(const Piece &piece) const {
        assert(piece >= WHITE_PAWN && piece <= BLACK_KING);
        return m_pieces[piece];
    }
    inline Bitboard get_piece_bb(const PieceType &piece_type, const Color &color) const {
        return get_piece_bb(static_cast<Piece>(piece_type + color * COLOR_OFFSET));
    }
    inline Bitboard get_piece_bb(const PieceType &piece_type) const {
        return m_pieces[piece_type] | m_pieces[piece_type + COLOR_OFFSET];
    }
    inline Square get_king_placement(const Color &color) const { return lsb(m_pieces[KING + color * COLOR_OFFSET]); }
    inline uint8_t get_castling_rights() const { return m_curr_state.castling_rights; }
    inline Color get_stm() const { return m_stm; }
    inline Color get_adversary() const { return static_cast<Color>(m_stm ^ 1); }
    inline Square get_en_passant() const { return m_curr_state.en_passant; }
    inline HashType get_hash() const { return m_hash_key; }
    inline int get_game_ply() const { return m_game_clock_ply; }
    inline int get_fifty_move_ply() const { return m_curr_state.fifty_move_ply; }
    inline int get_material_count(const Piece &piece) const { return count_bits(get_piece_bb(piece)); }
    inline int get_material_count(const PieceType &piece_type, const Color &color) const {
        return get_material_count(static_cast<Piece>(piece_type + color * COLOR_OFFSET));
    }
    inline int get_material_count(const PieceType &piece_type) const {
        return count_bits(m_pieces[piece_type] | m_pieces[piece_type + COLOR_OFFSET]);
    }
    inline int get_material_count() const { return count_bits(get_occupancy()); }
    inline Piece consult(const Square &sq) const { return m_board[sq]; }
    inline int get_history_ply() const { return m_history_ply; }
    inline BoardState get_board_state() const { return m_curr_state; };
    inline Bitboard get_checkers() const { return m_curr_state.checkers; }
    inline Bitboard get_pins() const { return m_curr_state.pins; }
    inline Bitboard get_castle_rooks() const { return m_curr_state.castle_rooks; }
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
    template <bool UPDATE>
    void add_piece(const Piece &piece, const Square &sq);
    template <bool UPDATE>
    void remove_piece(const Piece &piece, const Square &sq);
    template <bool UPDATE>
    void move_piece(const Piece &piece, const Square &from, const Square &to);

    template <bool UPDATE>
    void make_regular(const Move &move);
    template <bool UPDATE>
    void make_capture(const Move &move);
    template <bool UPDATE>
    void make_castle(const Move &move);
    template <bool UPDATE>
    void make_promotion(const Move &move);
    template <bool UPDATE>
    void make_en_passant(const Move &move);
    void update_castling_rights(const Move &move);
    void update_pin_and_checkers_bb();

    bool insufficient_material() const;
    bool repetition() const;
    bool fifty_move_draw();

    bool pawn_pseudo_legal(const Square &from, const Square &to, const Move &move) const;
    bool castling_pseudo_legal(const Square &from, const Square &to, const PieceType &moved_piece_type) const;

    void hash_piece_key(const Piece &piece, const Square &sq);
    void hash_castle_key();
    void hash_ep_key();
    void hash_side_key();

    inline void change_side() { m_stm = static_cast<Color>(m_stm ^ 1); }

    Piece m_board[64];
    Bitboard m_occupancies[2];
    Bitboard m_pieces[12];

    Color m_stm;
    HashType m_hash_key;
    int m_game_clock_ply;

    int m_history_ply;
    BoardState m_curr_state;
    BoardState m_history_stack[MAX_PLY];
    HashType m_played_positions[MAX_PLY];

    NNUE m_nnue;
};

#endif // #ifndef POSITION_H
