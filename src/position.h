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

    Move get_movement(const std::string &algebraic_notation) const;
    inline Bitboard get_occupancy() const;
    inline Bitboard get_occupancy(const Color &color) const;
    inline Bitboard get_piece_bb(const Piece &piece) const;
    inline Bitboard get_piece_bb(const PieceType &piece_type, const Color &color) const;
    inline Square get_king_placement(const Color &color) const;
    inline uint8_t get_castling_rights() const;
    inline Color get_stm() const;
    inline Square get_ep_rank() const;
    inline HashType get_hash() const;
    inline int get_game_ply() const;
    inline int get_fifty_move_ply() const;
    inline int get_material_count(const Piece &piece) const;
    inline int get_material_count(const PieceType &piece_type, const Color &color) const;
    inline int get_material_count(const PieceType &piece_type) const;
    inline int get_material_count() const;

    inline Piece consult(const Square &sq) const;
    bool is_attacked(const Square &sq) const;
    bool in_check();

    inline WeightType eval() const;

    bool draw();

    void print() const;

  private:
    template <bool UPDATE>
    void add_piece(const Piece &piece, const Square &sq);
    template <bool UPDATE>
    void remove_piece(const Piece &piece, const Square &sq);
    template <bool UPDATE>
    void move_piece(const Piece &piece, const Square &from, const Square &to);
    void update_castling_rights(const Move &move);

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

    bool insufficient_material() const;
    bool repetition() const;
    bool fifty_move_draw();

    void hash_piece_key(const Piece &piece, const Square &sq);
    void hash_castle_key();
    void hash_ep_key();
    void hash_side_key();

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
