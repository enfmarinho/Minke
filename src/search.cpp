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
#include <limits>

#include "attacks.h"
#include "move.h"
#include "movepicker.h"
#include "position.h"
#include "tt.h"
#include "types.h"

static void print_search_info(const CounterType &depth, const ScoreType &eval, const PvList &pv_list,
                              const ThreadData &thread_data) {
    std::cout << "info depth " << depth;
    if (std::abs(eval) > MateFound) {
        std::cout << " score mate " << (eval < 0 ? "-" : "") << (MateScore - std::abs(eval) + 1) / 2;
    } else {
        std::cout << " score cp " << eval;
    }
    // Add 1 to time_passed() to avoid division by 0
    std::cout << " time " << thread_data.time_manager.time_passed() << " nodes " << thread_data.nodes_searched
              << " nps " << thread_data.nodes_searched * 1000 / (thread_data.time_manager.time_passed() + 1) << " pv ";
    pv_list.print();
    std::cout << std::endl;
}

void ThreadData::reset() {
    stop = true;
    searching_ply = 0;
    nodes_searched = -1; // Avoid counting the root
    node_limit = std::numeric_limits<int>::max();
    depth_limit = MaxSearchDepth;
    search_history.reset();
}

void iterative_deepening(ThreadData &thread_data) {
    thread_data.stop = false;

    Move best_move = MoveNone;
    for (CounterType depth = 1; depth <= thread_data.depth_limit; ++depth) {
        PvList pv_list;
        ScoreType eval = negamax(-MaxScore, MaxScore, depth, pv_list, thread_data);
        if (!thread_data.time_manager.time_over() && !thread_data.stop) { // Search was successful
            bool tthit;
            TTEntry *entry = TT.probe(thread_data.position, tthit);

            best_move = entry->best_move();
            if (best_move == MoveNone) { // No legal moves
                assert(depth == 1 && !tthit);
                break;
            }
            assert(tthit && entry->depth() >= depth);
            print_search_info(depth, eval, pv_list, thread_data);
        }
        if (depth > 5)
            thread_data.time_manager.update();
        if (thread_data.time_manager.stop_early())
            break;

        thread_data.time_manager.can_stop(); // Avoids stopping before depth 1 has been searched through
    }

    std::cout << "bestmove " << (best_move == MoveNone ? "none" : best_move.get_algebraic_notation()) << std::endl;
    thread_data.stop = true;
}

ScoreType aspiration(const CounterType &depth, PvList &pv_list, ThreadData &thread_data) {
    bool tthit;
    TTEntry *ttentry = TT.probe(thread_data.position, tthit);
    if (!tthit)
        return negamax(-MaxScore, MaxScore, depth, pv_list, thread_data);

    int delta = 100; // TODO

    ScoreType alpha = ttentry->evaluation() - delta;
    ScoreType beta = ttentry->evaluation() + delta;
    ScoreType eval = negamax(alpha, beta, depth, pv_list, thread_data);
    if (eval >= beta)
        eval = negamax(alpha, MaxScore, depth, pv_list, thread_data);
    else if (eval <= alpha)
        eval = negamax(-MaxScore, beta, depth, pv_list, thread_data);

    return eval;
}

ScoreType negamax(ScoreType alpha, ScoreType beta, const CounterType &depth, PvList &pv_list, ThreadData &thread_data) {
    if (thread_data.time_manager.time_over() || thread_data.stop) // Out of time
        return -MaxScore;
    else if (depth <= 0)
        return quiescence(alpha, beta, thread_data);
    ++thread_data.nodes_searched;

    bool pv_node = beta - alpha > 1;

    // Transposition table probe
    bool tthit;
    TTEntry *ttentry = TT.probe(thread_data.position, tthit);
    if (!pv_node && tthit && ttentry->depth() >= depth &&
        (ttentry->bound() == TTEntry::BoundType::Exact ||
         (ttentry->bound() == TTEntry::BoundType::UpperBound && ttentry->evaluation() <= alpha) ||
         (ttentry->bound() == TTEntry::BoundType::LowerBound && ttentry->evaluation() >= beta))) {
        if (ttentry->best_move() != MoveNone)
            pv_list.update(ttentry->best_move(), PvList());
        return ttentry->evaluation();
    }

    // Early return conditions
    bool root = thread_data.searching_ply == 0;
    if (!root) {
        if (thread_data.position.draw())
            return 0;

        if (thread_data.searching_ply > MaxSearchDepth - 1)
            return thread_data.position.eval();

        // Mate distance pruning
        alpha = std::max(alpha, -MateScore + thread_data.searching_ply);
        beta = std::min(beta, MateScore - thread_data.searching_ply);
        if (alpha >= beta)
            return alpha;
    }

    bool improving = false; // TODO
    bool in_check = thread_data.position.in_check();
    ScoreType eval = 0;
    if (!in_check) {
        if (tthit) {
            eval = ttentry->evaluation();
        } else {
            eval = thread_data.position.eval();
        }
    }

    // Forward pruning methods
    if (!in_check && !pv_node) {
        // Reverse futility pruning
        if (depth < RFP_depth && eval - RFP_margin * (depth - improving) >= beta) {
            return eval;
        }

        // Null move pruning
        if (!thread_data.position.last_was_null() && depth >= NMP_depth && eval >= beta &&
            thread_data.position.has_non_pawns()) {
            const int reduction = NMP_base + depth / NMP_divisor + std::clamp((eval - beta) / 300, -1, 3);

            thread_data.position.make_null_move();
            ++thread_data.searching_ply;
            ScoreType null_score = -negamax(-beta, -beta + 1, depth - reduction, pv_list, thread_data);
            thread_data.position.unmake_null_move();
            --thread_data.searching_ply;

            if (null_score >= beta) {
                return null_score;
            }
        }
    }

    Move ttmove = (tthit ? ttentry->best_move() : MoveNone);
    Move move = MoveNone;
    Move best_move = MoveNone;
    ScoreType best_score = -MaxScore;
    ScoreType old_alpha = alpha;
    int moves_searched = 0;

    MovePicker move_picker(ttmove, &thread_data, false);
    while ((move = move_picker.next_move()) != MoveNone) {
        PvList curr_pv;
        if (!thread_data.position.make_move<true>(move)) { // Avoid illegal moves
            thread_data.position.unmake_move<true>(move);
            continue;
        }
        ++thread_data.searching_ply;
        ++moves_searched;

        ScoreType score;
        if (moves_searched == 0) {
            score = -negamax(-beta, -alpha, depth - 1, curr_pv, thread_data);
        } else {
            int reduction = 1;
            // Perform LMR in case a minimum amount of moves were searched, the depth is greater than 3, the move is
            // quiet or the move is a bad noisy
            if (moves_searched > 1 + pv_node && depth > 3 &&
                (move.is_quiet() || move_picker.picker_stage() == PickBadNoisy)) {
                reduction = LMRTable[std::min(depth, 63)][std::min(moves_searched, 63)];
                // Reduce more when move is a bad capture and less if move is quiet
                reduction -= !move.is_quiet();
                // Reduce less when in check
                reduction -= in_check;
                // Reduce less if move is killer
                reduction -= thread_data.search_history.is_killer(move, depth);

                reduction = std::clamp(reduction, 1, depth - 1);
            }
            score = -negamax(-alpha - 1, -alpha, depth - reduction, curr_pv, thread_data);
            if (score > alpha && score < beta)
                score = -negamax(-beta, -alpha, depth - 1, curr_pv, thread_data);
        }

        --thread_data.searching_ply;
        thread_data.position.unmake_move<true>(move);
        assert(score >= -MaxScore);

        if (score > best_score) {
            best_score = score;

            if (score > alpha) {
                best_move = move;
                if (pv_node)
                    pv_list.update(best_move, curr_pv);

                if (score >= beta) { // Fails high
                    thread_data.search_history.update(thread_data.position, move, depth);
                    break;
                }
                alpha = score; // Only update alpha if don't  failed high
            }
        }
    }

    if (moves_searched == 0) { // handle positions under stalemate or checkmate,
                               // i.e. positions with no legal moves to be made
        return thread_data.position.in_check() ? -MateScore + thread_data.searching_ply : 0;
    }

    if (!thread_data.time_manager.time_over()) { // Save on TT if search was completed
        TTEntry::BoundType bound = best_score >= beta   ? TTEntry::BoundType::LowerBound
                                   : alpha != old_alpha ? TTEntry::BoundType::Exact
                                                        : TTEntry::BoundType::UpperBound;
        ttentry->save(thread_data.position.get_hash(), depth, best_move, best_score,
                      thread_data.position.get_game_ply(), bound);
    }

    return best_score;
}

ScoreType quiescence(ScoreType alpha, ScoreType beta, ThreadData &thread_data) {
    ++thread_data.nodes_searched;
    if (thread_data.time_manager.time_over() || thread_data.stop)
        return -MaxScore;
    else if (thread_data.position.draw())
        return 0;

    bool tthit;
    TTEntry *ttentry = TT.probe(thread_data.position, tthit);
    Move ttmove = (tthit ? ttentry->best_move() : MoveNone);

    ScoreType stand_pat = thread_data.position.eval();
    if (stand_pat >= beta)
        return beta;

    if (alpha < stand_pat)
        alpha = stand_pat;

    Move move = MoveNone;
    MovePicker move_picker(ttmove, &thread_data, true);
    while ((move = move_picker.next_move()) != MoveNone) {
        assert(move.is_capture() || move.is_promotion());
        if (!thread_data.position.make_move<true>(move)) { // Avoid illegal moves
            thread_data.position.unmake_move<true>(move);
            continue;
        }

        ScoreType eval = -quiescence(-beta, -alpha, thread_data);
        thread_data.position.unmake_move<true>(move);
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

    int score = SEE_values[target] - threshold;
    if (move.is_promotion())
        score += SEE_values[move.promotee()] - SEE_values[Pawn];
    if (score < 0) // Cannot beat threshold
        return false;

    score -= (move.is_promotion() ? SEE_values[move.promotee()] : SEE_values[attacker]);
    if (score >= 0) // Already surpassed threshold
        return true;

    Bitboard attackers = position.attackers(to);
    Bitboard occupancy = position.get_occupancy() ^ (1ULL << from); // Removed already used attacker
    Bitboard diagonal_attackers = position.get_piece_bb(Bishop) | position.get_piece_bb(Queen);
    Bitboard line_attackers = position.get_piece_bb(Rook) | position.get_piece_bb(Queen);
    Color stm = static_cast<Color>(!position.get_stm());

    while (true) {
        attackers &= occupancy; // Remove used piece from attackers bitboard

        Bitboard my_attackers = attackers & position.get_occupancy(static_cast<Color>(stm));
        if (!my_attackers) // There is no attacker from stm
            break;

        // Get cheapest attacker
        int cheapest_attacker;
        for (cheapest_attacker = Pawn; cheapest_attacker <= King; ++cheapest_attacker) {
            if ((my_attackers = attackers & position.get_piece_bb(static_cast<PieceType>(cheapest_attacker), stm)))
                break;
        }
        stm = static_cast<Color>(!stm);

        score = -score - SEE_values[cheapest_attacker] - 1; // Updating negamaxed score

        if (score >= 0) { // Score beats threshold
            if (cheapest_attacker == King && (attackers & position.get_occupancy(static_cast<Color>(!stm))))
                // King is the only attacker and square is still attacked by opponent, so we don't have a valid attacker
                stm = static_cast<Color>(!stm);
            break;
        }

        occupancy ^= (my_attackers & -my_attackers); // Remove used piece, i.e. unset lsb

        // Add x-ray attackers, if there is any
        switch (cheapest_attacker) {
            case Pawn:
                // Fall-through
            case Bishop:
                attackers |= get_piece_attacks(to, occupancy, Bishop) & diagonal_attackers;
                break;
            case Rook:
                attackers |= get_piece_attacks(to, occupancy, Rook) & line_attackers;
                break;
            case Queen:
                attackers |= (get_piece_attacks(to, occupancy, Bishop) & diagonal_attackers) |
                             (get_piece_attacks(to, occupancy, Rook) & line_attackers);
                break;
            default:
                break;
        }
    }

    return stm != position.get_stm();
}
