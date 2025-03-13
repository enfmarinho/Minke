/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "search.h"

#include <cassert>
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
    // Add 1 to time_passed() to avoid division by 0
    std::cout << "info depth " << depth << " score cp " << eval << " time " << thread_data.time_manager.time_passed()
              << " nodes " << thread_data.nodes_searched << " nps "
              << thread_data.nodes_searched * 1000 / (thread_data.time_manager.time_passed() + 1) << " pv ";
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
            bool found;
            TTEntry *entry = TT.probe(thread_data.position, found);

            best_move = entry->best_move();
            if (best_move == MoveNone) {
                assert(depth == 1 && !found);
                break;
            }
            assert(found && entry->depth() >= depth);
            print_search_info(depth, eval, pv_list, thread_data);
        }
        if (thread_data.time_manager.stop_early())
            break;

        thread_data.time_manager.can_stop(); // Avoids stopping before depth 1 has been searched through
    }

    std::cout << "bestmove " << (best_move == MoveNone ? "none" : best_move.get_algebraic_notation()) << std::endl;
    thread_data.stop = true;
}

ScoreType aspiration(const CounterType &depth, PvList &pv_list, ThreadData &thread_data) {
    bool found;
    TTEntry *ttentry = TT.probe(thread_data.position, found);
    if (!found)
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
    else if (depth == 0)
        return quiescence(alpha, beta, thread_data);
    else if (thread_data.position.draw())
        return 0;

    ++thread_data.nodes_searched;

    // Transposition table probe
    bool found;
    TTEntry *entry = TT.probe(thread_data.position, found);
    if (found && entry->depth() >= depth &&
        (entry->bound() == TTEntry::BoundType::Exact ||
         (entry->bound() == TTEntry::BoundType::UpperBound && entry->evaluation() <= alpha) ||
         (entry->bound() == TTEntry::BoundType::LowerBound && entry->evaluation() >= beta))) {
        pv_list.update(entry->best_move(), PvList());
        return entry->evaluation();
    }

    Move ttmove = (found ? entry->best_move() : MoveNone);
    Move move = MoveNone;
    Move best_move = MoveNone;
    ScoreType best_score = -MaxScore;
    ScoreType old_alpha = alpha;

    MovePicker move_picker(ttmove, &thread_data, false);
    while ((move = move_picker.next_move()) != MoveNone) {
        PvList curr_pv;
        if (!thread_data.position.make_move<true>(move)) { // Avoid illegal moves
            thread_data.position.unmake_move<true>(move);
            continue;
        }

        ++thread_data.searching_ply;
        ScoreType score = -negamax(-beta, -alpha, depth - 1, curr_pv, thread_data);
        --thread_data.searching_ply;
        thread_data.position.unmake_move<true>(move);
        assert(score >= -MaxScore);

        if (score > best_score) {
            best_score = score;
            best_move = move;
            pv_list.update(best_move, curr_pv);

            if (score > alpha) {
                alpha = score;
                if (score >= beta) { // fails high
                    thread_data.search_history.update(thread_data.position, move, depth);
                    break;
                }
            }
        }
    }

    if (best_move == MoveNone) { // handle positions under stalemate or checkmate,
                                 // i.e. positions with no legal moves to be made
        return thread_data.position.in_check() ? -MateScore - depth : 0;
    }

    if (!thread_data.time_manager.time_over()) { // Save on TT if search was completed
        TTEntry::BoundType bound = best_score >= beta   ? TTEntry::BoundType::LowerBound
                                   : alpha != old_alpha ? TTEntry::BoundType::Exact
                                                        : TTEntry::BoundType::UpperBound;
        entry->save(thread_data.position.get_hash(), depth, best_move, best_score, thread_data.position.get_game_ply(),
                    bound);
    }

    return best_score;
}

ScoreType quiescence(ScoreType alpha, ScoreType beta, ThreadData &thread_data) {
    ++thread_data.nodes_searched;
    if (thread_data.time_manager.time_over() || thread_data.stop)
        return -MaxScore;
    else if (thread_data.position.draw())
        return 0;

    bool found;
    TTEntry *ttentry = TT.probe(thread_data.position, found);
    Move ttmove = (found ? ttentry->best_move() : MoveNone);

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
