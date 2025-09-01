/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "search.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "attacks.h"
#include "move.h"
#include "movepicker.h"
#include "position.h"
#include "tt.h"
#include "types.h"

static void print_search_info(const CounterType &depth, const ScoreType &eval, const PvList &pv_list,
                              const ThreadData &td) {
    std::cout << "info depth " << depth;
    if (std::abs(eval) > MATE_FOUND) {
        std::cout << " score mate " << (eval < 0 ? "-" : "") << (MATE_SCORE - std::abs(eval) + 1) / 2;
    } else {
        std::cout << " score cp " << eval;
    }
    // Add 1 to time_passed() to avoid division by 0
    std::cout << " time " << td.time_manager.time_passed() << " nodes " << td.nodes_searched << " nps "
              << td.nodes_searched * 1000 / (td.time_manager.time_passed() + 1) << " pv ";
    pv_list.print();
    std::cout << std::endl;
}

SearchLimits::SearchLimits() { reset(); }

SearchLimits::SearchLimits(int depth, int optimum_node, int maximum_node)
    : depth(depth), optimum_node(optimum_node), maximum_node(maximum_node) {}

void SearchLimits::reset() {
    depth = MAX_SEARCH_DEPTH;
    optimum_node = std::numeric_limits<int>::max();
    maximum_node = std::numeric_limits<int>::max();
}

ThreadData::ThreadData() {
    report = true;
    reset_search_parameters();
}

void ThreadData::reset_search_parameters() {
    best_move = MOVE_NONE;
    stop = true;
    height = 0;
    nodes_searched = -1; // Avoid counting the root
    time_manager.reset();
    search_limits.reset();
}

void ThreadData::set_search_limits(const SearchLimits sl) { search_limits = sl; }

inline bool stop_search(const ThreadData &td) {
    return td.time_manager.time_over() || td.stop || td.nodes_searched > td.search_limits.maximum_node;
}

ScoreType iterative_deepening(ThreadData &td) {
    td.stop = false;

    Move best_move = MOVE_NONE;
    ScoreType eval = -MAX_SCORE;
    for (CounterType depth = 1; depth <= td.search_limits.depth; ++depth) {
        PvList pv_list;
        eval = negamax(-MAX_SCORE, MAX_SCORE, depth, pv_list, td);
        if (!td.time_manager.time_over() && !td.stop) { // Search was successful
            best_move = td.best_move;
            if (best_move == MOVE_NONE) // No legal moves
                break;

            if (td.report)
                print_search_info(depth, eval, pv_list, td);
        }
        if (depth > 5)
            td.time_manager.update();
        if (td.time_manager.stop_early())
            break;

        td.time_manager.can_stop(); // Avoids stopping before depth 1 has been searched through
    }

    if (td.report)
        std::cout << "bestmove " << (best_move == MOVE_NONE ? "none" : best_move.get_algebraic_notation()) << std::endl;

    td.stop = true;
    return eval;
}

ScoreType aspiration(const CounterType &depth, PvList &pv_list, ThreadData &td) {
    bool tthit;
    TTEntry *ttentry = td.tt.probe(td.position, tthit);
    if (!tthit)
        return negamax(-MAX_SCORE, MAX_SCORE, depth, pv_list, td);

    int delta = 100; // TODO

    ScoreType alpha = ttentry->eval() - delta;
    ScoreType beta = ttentry->eval() + delta;
    ScoreType eval = negamax(alpha, beta, depth, pv_list, td);
    if (eval >= beta)
        eval = negamax(alpha, MAX_SCORE, depth, pv_list, td);
    else if (eval <= alpha)
        eval = negamax(-MAX_SCORE, beta, depth, pv_list, td);

    return eval;
}

ScoreType negamax(ScoreType alpha, ScoreType beta, CounterType depth, PvList &pv_list, ThreadData &td) {
    if (stop_search(td)) // Out of time
        return -MAX_SCORE;
    else if (depth <= 0)
        return quiescence(alpha, beta, td);
    ++td.nodes_searched;

    bool pv_node = alpha != beta - 1;
    Position &position = td.position;

    // Early return conditions
    bool root = td.height == 0;
    if (!root) {
        if (position.draw())
            return 0;

        if (td.height > MAX_SEARCH_DEPTH - 1)
            return position.eval();

        // Mate distance pruning
        alpha = std::max(alpha, -MATE_SCORE + td.height);
        beta = std::min(beta, MATE_SCORE - td.height - 1);
        if (alpha >= beta)
            return alpha;
    }

    // Transposition table probe
    bool tthit;
    TTEntry *ttentry = td.tt.probe(position, tthit);
    if (!pv_node && tthit && ttentry->depth() >= depth &&
        (ttentry->bound() == EXACT || (ttentry->bound() == UPPER && ttentry->eval() <= alpha) ||
         (ttentry->bound() == LOWER && ttentry->eval() >= beta))) {
        return ttentry->eval();
    }
    // Extraction data from ttentry if tthit
    Move ttmove = (tthit ? ttentry->best_move() : MOVE_NONE);
    ScoreType tteval = (tthit ? ttentry->eval() : -MAX_SCORE);

    // Internal Iterative Reductions
    if (!tthit && depth >= 4) {
        --depth;
    }

    bool improving = false; // TODO
    bool in_check = position.in_check();
    ScoreType eval = -MAX_SCORE;
    if (!in_check) {
        if (tthit)
            eval = tteval;
        else
            eval = position.eval();
    }

    // Forward pruning methods
    if (!in_check && !pv_node && !root) {
        // Reverse futility pruning
        if (depth < RFP_DEPTH && eval - RFP_MARGIN * (depth - improving) >= beta)
            return eval;

        // Null move pruning
        if (!position.last_was_null() && depth >= NMP_DEPTH && eval >= beta && position.has_non_pawns()) {
            // TODO check for advanced tweaks
            const int reduction = NMP_BASE + depth / NMP_DIVISOR + std::clamp((eval - beta) / 300, -1, 3);

            position.make_null_move();
            ++td.height;
            ScoreType null_score = -negamax(-beta, -beta + 1, depth - reduction, pv_list, td);
            position.unmake_null_move();
            --td.height;

            if (null_score >= beta)
                return null_score;
        }
    }

    Move move = MOVE_NONE;
    Move best_move = MOVE_NONE;
    ScoreType best_score = -MAX_SCORE;
    ScoreType old_alpha = alpha;
    int moves_searched = 0;

    MoveList quiets_tried, tacticals_tried;
    bool skip_quiets = false;
    MovePicker move_picker(ttmove, &td, false);
    while ((move = move_picker.next_move(skip_quiets)) != MOVE_NONE) {
        if (!position.make_move<true>(move)) { // Avoid illegal moves
            position.unmake_move<true>(move);
            continue;
        }

        // Add move to tried list
        if (move.is_quiet())
            quiets_tried.push(move);
        else
            tacticals_tried.push(move);

        ++td.height;
        ++moves_searched;

        PvList curr_pv;
        ScoreType score;
        if (moves_searched == 1) {
            score = -negamax(-beta, -alpha, depth - 1, curr_pv, td);
        } else {
            int reduction = 1;
            // Perform LMR in case a minimum amount of moves were searched, the depth is greater than 3, the move is
            // quiet or the move is a bad noisy
            if (moves_searched > 1 + pv_node && depth > 3 &&
                (move.is_quiet() || move_picker.picker_stage() == PICK_BAD_NOISY)) {
                reduction = LMR_TABLE[std::min(depth, 63)][std::min(moves_searched, 63)];
                // Reduce more when move is a bad capture and less if move is quiet
                reduction -= !move.is_quiet();
                // Reduce less when in check
                reduction -= in_check;
                // Reduce less if move is killer
                reduction -= td.search_history.is_killer(move, depth);

                reduction = std::clamp(reduction, 1, depth - 1);
            }
            score = -negamax(-alpha - 1, -alpha, depth - reduction, curr_pv, td);
            if (score > alpha && score < beta)
                score = -negamax(-beta, -alpha, depth - 1, curr_pv, td);
        }

        --td.height;
        position.unmake_move<true>(move);
        assert(score >= -MAX_SCORE);

        if (score > best_score) {
            best_score = score;

            if (score > alpha) {
                best_move = move;
                if (pv_node)
                    pv_list.update(best_move, curr_pv);

                if (score >= beta) { // Failed high
                    td.search_history.update_history(position, quiets_tried, tacticals_tried, best_move.is_quiet(),
                                                     depth);
                    break;
                }
                alpha = score; // Only update alpha if don't failed high
            }
        }
    }

    if (moves_searched == 0) { // handle positions under stalemate or checkmate,
                               // i.e. positions with no legal moves to be made
        return position.in_check() ? -MATE_SCORE + td.height : 0;
    }

    if (!stop_search(td)) { // Save on TT if search was completed
        BoundType bound = best_score >= beta ? LOWER : (alpha != old_alpha ? EXACT : UPPER);
        ttentry->save(position.get_hash(), depth, best_move, best_score, position.get_game_ply(), bound);
        td.best_move = best_move;
    }

    return best_score;
}

ScoreType quiescence(ScoreType alpha, ScoreType beta, ThreadData &td) {
    ++td.nodes_searched;
    Position &position = td.position;
    if (stop_search(td))
        return -MAX_SCORE;
    else if (position.draw())
        return 0;

    bool tthit;
    TTEntry *ttentry = td.tt.probe(position, tthit);
    Move ttmove = (tthit ? ttentry->best_move() : MOVE_NONE);

    ScoreType stand_pat = position.eval();
    alpha = std::max(alpha, stand_pat);
    if (alpha >= beta)
        return beta;

    Move move = MOVE_NONE;
    MovePicker move_picker(ttmove, &td, true);
    // TODO check if its worth to check for quiet moves if in check
    while ((move = move_picker.next_move(true)) != MOVE_NONE) {
        assert(move.is_capture() || move.is_promotion());
        if (!position.make_move<true>(move)) { // Avoid illegal moves
            position.unmake_move<true>(move);
            continue;
        }

        ScoreType eval = -quiescence(-beta, -alpha, td);
        position.unmake_move<true>(move);
        if (eval >= beta)
            return beta;
        else if (eval > alpha)
            alpha = eval;
    }

    return alpha;
}

bool SEE(Position &position, const Move &move, int threshold) {
    if (move.is_castle()) // Cannot win or lose material by castling
        return threshold <= 0;

    Square from = move.from();
    Square to = move.to();
    Piece target = position.consult(to);
    Piece attacker = position.consult(from);

    int score = SEE_VALUES[target] - threshold;
    if (move.is_promotion())
        score += SEE_VALUES[move.promotee()] - SEE_VALUES[PAWN];
    if (score < 0) // Cannot beat threshold
        return false;

    score -= (move.is_promotion() ? SEE_VALUES[move.promotee()] : SEE_VALUES[attacker]);
    if (score >= 0) // Already surpassed threshold
        return true;

    Bitboard attackers = position.attackers(to);
    Bitboard occupancy = position.get_occupancy() ^ (1ULL << from); // Removed already used attacker
    Bitboard diagonal_attackers = position.get_piece_bb(BISHOP) | position.get_piece_bb(QUEEN);
    Bitboard line_attackers = position.get_piece_bb(ROOK) | position.get_piece_bb(QUEEN);
    Color stm = static_cast<Color>(!position.get_stm());

    while (true) {
        attackers &= occupancy; // Remove used piece from attackers bitboard

        Bitboard my_attackers = attackers & position.get_occupancy(static_cast<Color>(stm));
        if (!my_attackers) // There is no attacker from stm
            break;

        // Get cheapest attacker
        int cheapest_attacker;
        for (cheapest_attacker = PAWN; cheapest_attacker <= KING; ++cheapest_attacker) {
            if ((my_attackers = attackers & position.get_piece_bb(static_cast<PieceType>(cheapest_attacker), stm)))
                break;
        }
        stm = static_cast<Color>(!stm);

        score = -score - SEE_VALUES[cheapest_attacker] - 1; // Updating negamaxed score

        if (score >= 0) { // Score beats threshold
            if (cheapest_attacker == KING && (attackers & position.get_occupancy(static_cast<Color>(!stm))))
                // King is the only attacker and square is still attacked by opponent, so we don't have a valid attacker
                stm = static_cast<Color>(!stm);
            break;
        }

        occupancy ^= (my_attackers & -my_attackers); // Remove used piece, i.e. unset lsb

        // Add x-ray attackers, if there is any
        switch (cheapest_attacker) {
            case PAWN:
                // Fall-through
            case BISHOP:
                attackers |= get_piece_attacks(to, occupancy, BISHOP) & diagonal_attackers;
                break;
            case ROOK:
                attackers |= get_piece_attacks(to, occupancy, ROOK) & line_attackers;
                break;
            case QUEEN:
                attackers |= (get_piece_attacks(to, occupancy, BISHOP) & diagonal_attackers) |
                             (get_piece_attacks(to, occupancy, ROOK) & line_attackers);
                break;
            default:
                break;
        }
    }

    return stm != position.get_stm();
}
