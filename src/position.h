/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef POSITION_H
#define POSITION_H

#include <cstdint>
#include <string>
#include <vector>

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

    inline bool in_check() const { return is_attacked(get_king_placement(stm)); }
    bool is_attacked(const Square &sq) const;
    Bitboard attackers(const Square &sq) const;

    inline bool draw() { return insufficient_material() || repetition() || fifty_move_draw(); }
    inline ScoreType eval() const { return nnue.eval(stm); }

    int legal_move_amount();
    void print() const;

    Move get_movement(const std::string &algebraic_notation) const;
    inline Bitboard get_occupancy() const { return occupancies[White] | occupancies[Black]; }
    inline Bitboard get_occupancy(const Color &color) const {
        assert(color == White || color == Black);
        return occupancies[color];
    }
    inline Bitboard get_piece_bb(const Piece &piece) const {
        assert(piece >= WhitePawn && piece <= BlackKing);
        return pieces[piece];
    }
    inline Bitboard get_piece_bb(const PieceType &piece_type, const Color &color) const {
        return get_piece_bb(static_cast<Piece>(piece_type + color * ColorOffset));
    }
    inline Bitboard get_piece_bb(const PieceType &piece_type) const {
        return pieces[piece_type] | pieces[piece_type + ColorOffset];
    }
    inline Square get_king_placement(const Color &color) const { return lsb(pieces[King + color * ColorOffset]); }
    inline uint8_t get_castling_rights() const { return curr_state.castling_rights; }
    inline Color get_stm() const { return stm; }
    inline Color get_adversary() const { return static_cast<Color>(stm ^ 1); }
    inline Square get_en_passant() const { return curr_state.en_passant; }
    inline HashType get_hash() const { return hash_key; }
    inline int get_game_ply() const { return game_clock_ply; }
    inline int get_fifty_move_ply() const { return curr_state.fifty_move_ply; }
    inline int get_material_count(const Piece &piece) const { return count_bits(get_piece_bb(piece)); }
    inline int get_material_count(const PieceType &piece_type, const Color &color) const {
        return get_material_count(static_cast<Piece>(piece_type + color * ColorOffset));
    }
    inline int get_material_count(const PieceType &piece_type) const {
        return count_bits(pieces[piece_type] | pieces[piece_type + ColorOffset]);
    }
    inline int get_material_count() const { return count_bits(get_occupancy()); }
    inline Piece consult(const Square &sq) const { return board[sq]; }

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
    bool make_castle(const Move &move);
    template <bool UPDATE>
    void make_promotion(const Move &move);
    template <bool UPDATE>
    void make_en_passant(const Move &move);
    void update_castling_rights(const Move &move);

    bool insufficient_material() const;
    bool repetition() const;
    bool fifty_move_draw();

    void hash_piece_key(const Piece &piece, const Square &sq);
    void hash_castle_key();
    void hash_ep_key();
    void hash_side_key();

    inline void change_side() { stm = static_cast<Color>(stm ^ 1); }

    Piece board[64];
    Bitboard occupancies[2];
    Bitboard pieces[12];

    Color stm;
    HashType hash_key;
    int game_clock_ply;

    int history_stack_head;
    BoardState curr_state;
    BoardState history_stack[MaxPly];
    std::vector<HashType> played_positions; // TODO this could be an static array

    NNUE nnue;
};

#endif // #ifndef POSITION_H
