/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "position.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <vector>

#include "attacks.h"
#include "hash.h"
#include "move.h"
#include "movegen.h"
#include "types.h"
#include "utils.h"

Position::Position() {
    played_positions.reserve(MaxPly);
    set_fen<true>(StartFEN);
}

template <bool UPDATE>
bool Position::set_fen(const std::string &fen) {
    reset<UPDATE>();

    std::stringstream iss(fen);
    std::array<std::string, 6> fen_arguments;
    for (int index = 0; index < 6; ++index) {
        iss >> std::skipws >> fen_arguments[index];
        if (iss.fail()) {
            std::cerr << "INVALID FEN: wrong format." << std::endl;
            return false;
        }
    }

    int file = 7, rank = 0;
    for (char c : fen_arguments[0]) {
        if (c == '/') {
            --file;
            rank = 0;
            continue;
        }
        if (!std::isdigit(c)) {
            char piece_char = std::tolower(c);
            Color player = std::isupper(c) ? White : Black;
            Square sq = get_square(file, rank);
            if (piece_char == 'p')
                add_piece<UPDATE>(get_piece(Pawn, player), sq);
            else if (piece_char == 'n')
                add_piece<UPDATE>(get_piece(Knight, player), sq);
            else if (piece_char == 'b')
                add_piece<UPDATE>(get_piece(Bishop, player), sq);
            else if (piece_char == 'r')
                add_piece<UPDATE>(get_piece(Rook, player), sq);
            else if (piece_char == 'q')
                add_piece<UPDATE>(get_piece(Queen, player), sq);
            else if (piece_char == 'k')
                add_piece<UPDATE>(get_piece(King, player), sq);
            else
                assert(false);

            ++rank;
        }
    }

    if (fen_arguments[1] == "w") {
        stm = White;
    } else if (fen_arguments[1] == "b") {
        stm = Black;
        hash_side_key();
    } else {
        std::cerr << "INVALID FEN: invalid player, it should be 'w' or 'b'." << std::endl;
        return false;
    }

    for (char castling : fen_arguments[2]) {
        if (castling == 'K')
            set_bits(curr_state.castling_rights, WhiteShortCastleRight);
        else if (castling == 'Q')
            set_bits(curr_state.castling_rights, WhiteLongCastleRight);
        else if (castling == 'k')
            set_bits(curr_state.castling_rights, BlackShortCastleRight);
        else if (castling == 'q')
            set_bits(curr_state.castling_rights, BlackLongCastleRight);
    }
    hash_castle_key();

    if (fen_arguments[3] == "-") {
        curr_state.en_passant = NoSquare;
    } else {
        curr_state.en_passant = get_square(fen_arguments[3][1] - '1', fen_arguments[3][0] - 'a');
        hash_ep_key();
    }

    try {
        curr_state.fifty_move_ply = std::stoi(fen_arguments[4]);
    } catch (const std::exception &) {
        std::cerr << "INVALID FEN: halfmove clock is not a number." << std::endl;
        return false;
    }
    try {
        game_clock_ply = std::stoi(fen_arguments[5]);
    } catch (const std::exception &) {
        std::cerr << "INVALID FEN: game clock is not a number." << std::endl;
        return false;
    }

    return true;
}

std::string Position::get_fen() const {
    std::string fen;
    for (int file = 7; file >= 0; --file) {
        int counter = 0;
        for (int rank = 0; rank < 8; ++rank) {
            const Piece &piece = consult(get_square(file, rank));
            const Color color = get_color(piece);
            const PieceType piece_type = get_piece_type(piece, color);
            if (piece == Empty) {
                ++counter;
                continue;
            } else if (counter > 0) {
                fen += ('0' + counter);
                counter = 0;
            }

            char piece_char;
            if (piece_type == Pawn)
                piece_char = 'p';
            else if (piece_type == Knight)
                piece_char = 'n';
            else if (piece_type == Bishop)
                piece_char = 'b';
            else if (piece_type == Rook)
                piece_char = 'r';
            else if (piece_type == Queen)
                piece_char = 'q';
            else if (piece_type == King)
                piece_char = 'k';
            else
                __builtin_unreachable();

            if (color == White)
                fen += toupper(piece_char);
            else
                fen += piece_char;
        }
        if (counter > 0)
            fen += ('0' + counter);
        if (file != 0)
            fen += '/';
    }
    fen += (stm == White ? " w " : " b ");
    bool none = true;
    if (curr_state.castling_rights & WhiteShortCastleRight) {
        none = false;
        fen += "K";
    }
    if (curr_state.castling_rights & WhiteLongCastleRight) {
        none = false;
        fen += "Q";
    }
    if (curr_state.castling_rights & BlackShortCastleRight) {
        none = false;
        fen += "k";
    }
    if (curr_state.castling_rights & BlackLongCastleRight) {
        none = false;
        fen += "q";
    }
    if (none)
        fen += "-";

    fen += ' ';
    if (get_en_passant() == NoSquare)
        fen += "-";
    else {
        fen += (get_rank(get_en_passant()) + 'a');
        fen += (stm == White ? '6' : '3');
    }
    fen += ' ';

    fen += std::to_string(get_fifty_move_ply());
    fen += ' ';
    fen += std::to_string((game_clock_ply + 1) / 2); // TODO check this, should i really add 1 ?

    return fen;
}

void Position::reset_nnue() { nnue.reset(*this); }

template <bool UPDATE>
void Position::reset() {
    for (int sqi = a1; sqi <= h8; ++sqi) {
        board[sqi] = Empty;
    }
    std::memset(occupancies, 0ULL, sizeof(occupancies));
    std::memset(pieces, 0ULL, sizeof(pieces));

    hash_key = 0ULL;
    history_stack_head = 0;
    curr_state.reset();
    played_positions.clear();

    if constexpr (UPDATE) {
        reset_nnue();
    }
}

template <bool UPDATE>
void Position::add_piece(const Piece &piece, const Square &sq) {
    assert(piece >= WhitePawn && piece <= BlackKing);
    assert(sq >= a1 && sq <= h8);

    Color color = get_color(piece);
    set_bit(occupancies[color], sq);
    set_bit(pieces[piece], sq);
    board[sq] = piece;

    hash_piece_key(piece, sq);

    if constexpr (UPDATE) {
        nnue.add_feature(piece, sq);
    }
}

template <bool UPDATE>
void Position::remove_piece(const Piece &piece, const Square &sq) {
    assert(piece >= WhitePawn && piece <= BlackKing);
    assert(sq >= a1 && sq <= h8);

    Color color = get_color(piece);
    unset_bit(occupancies[color], sq);
    unset_bit(pieces[piece], sq);
    board[sq] = Empty;

    hash_piece_key(piece, sq);

    if constexpr (UPDATE) {
        nnue.remove_feature(piece, sq);
    }
}

template <bool UPDATE>
void Position::move_piece(const Piece &piece, const Square &from, const Square &to) {
    remove_piece<UPDATE>(piece, from);
    add_piece<UPDATE>(piece, to);
}

template <bool UPDATE>
bool Position::make_move(const Move &move) {
    if constexpr (UPDATE) {
        nnue.push();
    }
    history_stack[history_stack_head++] = curr_state;
    ++game_clock_ply;
    ++curr_state.fifty_move_ply;
    ++curr_state.ply_from_null;
    played_positions.emplace_back(hash_key);

    if (curr_state.en_passant != NoSquare) {
        hash_ep_key();
        curr_state.en_passant = NoSquare;
    }

    curr_state.captured = consult(move.to());

    bool legal = true;
    if (move.is_regular()) {
        make_regular<UPDATE>(move);
    } else if (move.is_capture()) {
        make_capture<UPDATE>(move);
    } else if (move.is_castle()) {
        legal = make_castle<UPDATE>(move);
    } else if (move.is_promotion()) {
        make_promotion<UPDATE>(move);
    } else if (move.is_ep()) {
        make_en_passant<UPDATE>(move);
    }

    hash_castle_key();
    update_castling_rights(move);
    hash_castle_key();

    hash_side_key();
    stm = static_cast<Color>(~stm);

    if (move.is_castle()) { // If move is a castle, the legality has already been checked by make_castle()
        return legal;
    }
    return !in_check();
}

template <bool UPDATE>
void Position::make_regular(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);

    move_piece<UPDATE>(piece, from, to);
    if (get_piece_type(piece, stm) == Pawn) {
        curr_state.fifty_move_ply = 0;
        int pawn_offset = stm == White ? North : South;
        if (to - from == 2 * pawn_offset) { // Double push
            curr_state.en_passant = static_cast<Square>(to - pawn_offset);
            hash_ep_key();
        }
    }
}

template <bool UPDATE>
void Position::make_capture(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);

    curr_state.fifty_move_ply = 0;
    curr_state.captured = consult(to);

    remove_piece<UPDATE>(curr_state.captured, to);
    remove_piece<UPDATE>(piece, from);

    if (move.is_promotion()) {
        switch (move.type()) {
            case PawnPromotionQueenCapture:
                piece = get_piece(Queen, stm);
                break;
            case PawnPromotionRookCapture:
                piece = get_piece(Rook, stm);
                break;
            case PawnPromotionKnightCapture:
                piece = get_piece(Knight, stm);
                break;
            case PawnPromotionBishopCapture:
                piece = get_piece(Bishop, stm);
                break;
            default:
                __builtin_unreachable();
        }
    }
    add_piece<UPDATE>(piece, to);
}

template <bool UPDATE>
bool Position::make_castle(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);
    move_piece<UPDATE>(piece, from, to);
    switch (to) {
        case g1: // White castle short
            move_piece<UPDATE>(BlackRook, h1, f1);
            return !(is_attacked(e1) || is_attacked(f1) || is_attacked(g1));
        case c1: // White castle long
            move_piece<UPDATE>(BlackRook, a1, d1);
            return !(is_attacked(e1) || is_attacked(d1) || is_attacked(c1));
        case g8: // Black castle short
            move_piece<UPDATE>(BlackRook, h8, f8);
            return !(is_attacked(e8) || is_attacked(f8) || is_attacked(g8));
        case c8: // Black castle long
            move_piece<UPDATE>(BlackRook, a8, d8);
            return !(is_attacked(e8) || is_attacked(d8) || is_attacked(c8));
        default:
            __builtin_unreachable();
    }
}

template <bool UPDATE>
void Position::make_promotion(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);
    remove_piece<UPDATE>(piece, from);
    switch (move.type()) {
        case PawnPromotionQueen:
            piece = get_piece(Queen, stm);
            break;
        case PawnPromotionRook:
            piece = get_piece(Rook, stm);
            break;
        case PawnPromotionKnight:
            piece = get_piece(Knight, stm);
            break;
        case PawnPromotionBishop:
            piece = get_piece(Bishop, stm);
            break;
        default:
            __builtin_unreachable();
    }
    add_piece<UPDATE>(piece, to);
}

template <bool UPDATE>
void Position::make_en_passant(const Move &move) {
    curr_state.fifty_move_ply = 0;
    Square from = move.from();
    Square to = move.from();
    Piece piece = consult(from);
    Piece captured = consult(to);
    Square captured_square = static_cast<Square>(to - static_cast<int>(stm == White ? North : South));
    remove_piece<UPDATE>(captured, captured_square);
    move_piece<UPDATE>(piece, from, to);
}

void Position::update_castling_rights(const Move &move) {
    Square from = move.from();
    PieceType piece_type = get_piece_type(consult(from), stm);

    if (piece_type == King) {
        switch (stm) {
            case White:
                unset_mask(curr_state.castling_rights, WhiteShortCastleRight & WhiteLongCastleRight);
                break;
            case Black:
                unset_mask(curr_state.castling_rights, BlackShortCastleRight & BlackLongCastleRight);
                break;
            default:
                __builtin_unreachable();
        }
    } else if (piece_type == Rook) {
        switch (from) {
            case a1:
                unset_mask(curr_state.castling_rights, WhiteLongCastleRight);
                break;
            case h1:
                unset_mask(curr_state.castling_rights, WhiteShortCastleRight);
                break;
            case a8:
                unset_mask(curr_state.castling_rights, BlackLongCastleRight);
                break;
            case h8:
                unset_mask(curr_state.castling_rights, BlackShortCastleRight);
                break;
            default:
                break;
        }
    }
}

template <bool UPDATE>
void Position::unmake_move(const Move &move) {
    assert(history_stack_head > 0); // check if there is a move to unmake
    if constexpr (UPDATE) {
        nnue.pop();
    }

    --game_clock_ply;
    played_positions.pop_back();
    stm = static_cast<Color>(~stm);

    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(to);

    if (move.is_regular()) {
        move_piece<false>(piece, to, from);
    } else if (move.is_capture()) {
        remove_piece<false>(piece, to);
        add_piece<false>(curr_state.captured, to);
        if (move.is_promotion()) {
            piece = static_cast<Piece>(Pawn + ColorOffset * stm);
        }
        add_piece<false>(piece, from);
    } else if (move.is_castle()) {
        move_piece<false>(piece, to, from);
        switch (to) {
            case g1: // White castle short
                move_piece<false>(WhiteRook, f1, h1);
                break;
            case c1: // White castle long
                move_piece<false>(WhiteRook, d1, a1);
                break;
            case g8: // Black castle short
                move_piece<false>(BlackRook, f8, h8);
                break;
            case c8: // Black castle long
                move_piece<false>(BlackRook, d8, a8);
                break;
            default:
                __builtin_unreachable();
        }
    } else if (move.is_promotion()) {
        remove_piece<false>(piece, to);
        piece = static_cast<Piece>(Pawn + ColorOffset * stm);
        add_piece<false>(piece, from);
    } else if (move.is_ep()) {
        move_piece<false>(piece, to, from);
        Square captured_square = static_cast<Square>(to - static_cast<int>(stm == White ? North : South));
        add_piece<false>(curr_state.captured, captured_square);
    }

    if (curr_state.en_passant != NoSquare) {
        hash_ep_key();
    }
    hash_castle_key();

    curr_state = history_stack[--history_stack_head];

    if (curr_state.en_passant != NoSquare) {
        hash_ep_key();
    }
    hash_castle_key();
    hash_side_key();
}

Move Position::get_movement(const std::string &algebraic_notation) const {
    Square from = get_square(algebraic_notation[1] - '1', algebraic_notation[0] - 'a');
    Square to = get_square(algebraic_notation[3] - '1', algebraic_notation[2] - 'a');
    MoveType move_type = MoveType::Regular;

    if (algebraic_notation.size() == 5) { // Pawn promotion
        if (tolower(algebraic_notation[4]) == 'q')
            move_type = PawnPromotionQueen;
        else if (tolower(algebraic_notation[4]) == 'n')
            move_type = PawnPromotionKnight;
        else if (tolower(algebraic_notation[4]) == 'r')
            move_type = PawnPromotionRook;
        else if (tolower(algebraic_notation[4]) == 'b')
            move_type = PawnPromotionBishop;
    }
    if (consult(to) != Empty) { // Capture
        move_type = static_cast<MoveType>(move_type | Capture);
    } else if (get_piece_type(consult(from)) == King && get_rank(from) == 4 &&
               (get_rank(to) == 2 || get_rank(to) == 6)) { // Castle
        move_type = Castling;
    } else if (get_piece_type(consult(from)) == Pawn && get_rank(to) == get_rank(get_en_passant()) &&
               (get_file(to) == 2 || get_file(to) == 5)) { // En passant
        move_type = EnPassant;
    }

    return Move(from, to, move_type);
}

inline Bitboard Position::get_occupancy() const { return occupancies[White] | occupancies[Black]; }

inline Bitboard Position::get_occupancy(const Color &color) const {
    assert(color == White || color == Black);
    return occupancies[color];
}

inline Bitboard Position::get_piece_bb(const Piece &piece) const {
    assert(piece >= WhitePawn && piece <= BlackKing);
    return pieces[piece];
}

inline Bitboard Position::get_piece_bb(const PieceType &piece_type, const Color &color) const {
    return get_piece_bb(static_cast<Piece>(piece_type + color * ColorOffset));
}

inline Square Position::get_king_placement(const Color &color) const { return lsb(pieces[King + color * ColorOffset]); }

inline uint8_t Position::get_castling_rights() const { return curr_state.castling_rights; }

inline Color Position::get_stm() const { return stm; }

inline Square Position::get_en_passant() const { return curr_state.en_passant; }

inline HashType Position::get_hash() const { return hash_key; }

inline int Position::get_game_ply() const { return game_clock_ply; }

inline int Position::get_fifty_move_ply() const { return curr_state.fifty_move_ply; }

inline int Position::get_material_count(const Piece &piece) const { return count_bits(get_piece_bb(piece)); }

inline int Position::get_material_count(const PieceType &piece_type, const Color &color) const {
    return get_material_count(static_cast<Piece>(piece_type + color * ColorOffset));
}

inline int Position::get_material_count(const PieceType &piece_type) const {
    return count_bits(pieces[piece_type] | pieces[piece_type + ColorOffset]);
}

inline int Position::get_material_count() const { return count_bits(get_occupancy()); }

inline Piece Position::consult(const Square &sq) const { return board[sq]; }

bool Position::is_attacked(const Square &sq) const {
    Color opponent = static_cast<Color>(~stm);
    Bitboard occupancy = get_occupancy();
    unset_bit(occupancy, sq); // square to be checked has to be unset on occupancy bitboard

    // Check if sq is attacked by opponent pawns. Note: pawn attack mask has to be "stm" because the logic is reversed
    if (get_piece_bb(Pawn, opponent) & PawnAttacks[stm][sq])
        return true;

    // Check if sq is attacked by opponent knights
    if (get_piece_bb(Knight, opponent) & KnightAttacks[sq])
        return true;

    // Check if sq is attacked by opponent bishops or queens
    if ((get_piece_bb(Bishop, opponent) | get_piece_bb(Queen, opponent)) & get_bishop_attack(sq, occupancy))
        return true;

    // Check if sq is attacked by opponent rooks or queens
    if ((get_piece_bb(Rook, opponent) | get_piece_bb(Queen, opponent)) & get_rook_attack(sq, occupancy))
        return true;

    // Check if sq is attacked by opponent king. Unnecessary when checking for checks
    if (get_piece_bb(King, opponent) & KingAttacks[sq])
        return true;

    return false;
}

inline bool Position::in_check() { return is_attacked(get_king_placement(stm)); }

inline WeightType Position::eval() const { return nnue.eval(stm); }

void Position::print() const {
    auto print_line = []() -> void {
        for (IndexType i = 0; i < 8; ++i) {
            std::cout << "+";
            for (IndexType j = 0; j < 3; ++j)
                std::cout << "-";
        }
        std::cout << "+\n";
    };

    for (int sqi = a1; sqi <= h8; ++sqi) {
        if (sqi % 8 == 0) // First column
            print_line();

        Piece piece = board[sqi];
        PieceType piece_type = get_piece_type(piece);
        char piece_char = ' ';
        if (piece_type == Pawn)
            piece_char = 'p';
        else if (piece_type == Knight)
            piece_char = 'n';
        else if (piece_type == Bishop)
            piece_char = 'b';
        else if (piece_type == Rook)
            piece_char = 'r';
        else if (piece_type == Queen)
            piece_char = 'q';
        else if (piece_type == King)
            piece_char = 'k';

        if (piece <= WhiteKing) // Piece is white
            piece_char = toupper(piece_char);

        std::cout << "| " << piece_char << " ";
        if (sqi % 8 == 0) // First column
            std::cout << "| " << get_file(static_cast<Square>(sqi)) + 1 << "\n";
    }

    print_line();
    for (char rank_simbol = 'a'; rank_simbol <= 'h'; ++rank_simbol)
        std::cout << "  " << rank_simbol << " ";

    std::cout << "\n\nFEN: " << get_fen();
    std::cout << "\nHash: " << hash_key;
    std::cout << "\nEval: " << eval() << "\n";
}

bool Position::draw() { return insufficient_material() || repetition() || fifty_move_draw(); }

bool Position::insufficient_material() const {
    int piece_amount = get_material_count();
    if (piece_amount == 2) {
        return true;
    } else if (piece_amount == 3 && (get_material_count(Knight) == 1 || get_material_count(Bishop) == 1)) {
        return true;
    } else if (piece_amount == 4 && (get_material_count(Knight) == 2 ||
                                     (get_material_count(WhiteBishop) == 1 && get_material_count(BlackBishop) == 1))) {
        return true;
    }

    return false;
}

bool Position::repetition() const {
    assert(game_clock_ply >= curr_state.fifty_move_ply);

    int counter = 0;
    int distance = std::min(curr_state.fifty_move_ply, curr_state.ply_from_null);
    int starting_index = played_positions.size();

    for (int index = 4; index <= distance; index += 2)
        if (played_positions[starting_index - index] == hash_key) {
            if (index < history_stack_head) // 2-fold repetition within the search tree, this avoids cycles
                return true;

            counter++;

            if (counter >= 2) // 3-fold repetition
                return true;
        }
    return false;
}

// TODO it would be a little more efficient to do a specific legality check for the move
bool Position::fifty_move_draw() {
    if (curr_state.fifty_move_ply >= 100) {
        MoveList move_list(*this);
        while (!move_list.empty()) {
            Move move = move_list.next_move();
            bool legal = make_move<false>(move);
            unmake_move<false>(move);
            if (legal) {
                return true;
            }
        }
    }

    return false;
}

void Position::hash_piece_key(const Piece &piece, const Square &sq) {
    assert(piece >= WhitePawn && piece <= BlackKing);
    assert(sq >= a1 && sq <= h8);
    hash_key ^= hash_keys.pieces[piece][sq];
}

void Position::hash_castle_key() {
    assert(curr_state.castling_rights >= 0 && curr_state.castling_rights <= EveryRight);
    hash_key ^= hash_keys.castle[curr_state.castling_rights];
}

void Position::hash_ep_key() {
    assert(get_rank(curr_state.en_passant) >= 0 && get_rank(curr_state.en_passant) < 8);
    hash_key ^= hash_keys.en_passant[get_rank(curr_state.en_passant)];
}

void Position::hash_side_key() { hash_key ^= hash_keys.side; }
