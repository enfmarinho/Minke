/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "search.h"

#include <cassert>
#include <iostream>
#include <limits>

#include "movepicker.h"
#include "position.h"
#include "tt.h"
#include "types.h"

static void print_search_info(const CounterType &depth, const WeightType &eval, const PvList &pv_list,
                              const SearchData &search_data) {
    // Add 1 to time_passed() to avoid division by 0
    std::cout << "info depth " << depth << " score cp " << eval << " time " << search_data.time_manager.time_passed()
              << " nodes " << search_data.nodes_searched << " nps "
              << search_data.nodes_searched * 1000 / (search_data.time_manager.time_passed() + 1) << " pv ";
    pv_list.print();
    std::cout << std::endl;
}

void SearchData::reset() {
    stop = true;
    searching_depth = 0;
    nodes_searched = -1; // Avoid counting the root
    node_limit = std::numeric_limits<int>::max();
    depth_limit = MaxSearchDepth;
}

void iterative_deepening(SearchData &search_data) {
    search_data.stop = false;

    Move best_move = MoveNone;
    for (CounterType depth = 1; depth <= search_data.depth_limit; ++depth) {
        PvList pv_list;
        WeightType eval = aspiration(depth, pv_list, search_data);
        if (!search_data.time_manager.time_over() && !search_data.stop) { // Search was successful
            bool found;
            TTEntry *entry = TT.probe(search_data.position, found);

            best_move = entry->best_move();
            if (best_move == MoveNone) {
                assert(depth == 1 && !found);
                break;
            }
            assert(found && entry->depth_ply() >= depth);
            print_search_info(depth, eval, pv_list, search_data);
        }
        if (search_data.time_manager.stop_early())
            break;

        search_data.time_manager.can_stop(); // Avoids stopping before depth 1 has been searched through
    }

    std::cout << "bestmove " << (best_move == MoveNone ? "none" : best_move.get_algebraic_notation()) << std::endl;
    search_data.stop = true;
}

WeightType aspiration(const CounterType &depth, PvList &pv_list, SearchData &search_data) {
    bool found;
    TTEntry *ttentry = TT.probe(search_data.position, found);
    if (!found)
        return alpha_beta(-MaxScore, MaxScore, depth, pv_list, search_data);

    int delta = 100; // TODO

    WeightType alpha = ttentry->evaluation() - delta;
    WeightType beta = ttentry->evaluation() + delta;
    WeightType eval = alpha_beta(alpha, beta, depth, pv_list, search_data);
    if (eval >= beta)
        eval = alpha_beta(alpha, MaxScore, depth, pv_list, search_data);
    else if (eval <= alpha)
        eval = alpha_beta(-MaxScore, beta, depth, pv_list, search_data);

    assert(eval >= alpha && eval <= beta);
    return eval;
}

WeightType alpha_beta(WeightType alpha, WeightType beta, const CounterType &depth_ply, PvList &pv_list,
                      SearchData &search_data) {
    if (search_data.time_manager.time_over() || search_data.stop) // Out of time
        return -MaxScore;
    else if (depth_ply == 0)
        return quiescence(alpha, beta, search_data);
    else if (search_data.position.draw())
        return 0;

    ++search_data.nodes_searched;

    // Transposition table probe
    bool found;
    TTEntry *entry = TT.probe(search_data.position, found);
    if (found && entry->depth_ply() >= depth_ply &&
        (entry->bound() == TTEntry::BoundType::Exact ||
         (entry->bound() == TTEntry::BoundType::UpperBound && entry->evaluation() <= alpha) ||
         (entry->bound() == TTEntry::BoundType::LowerBound && entry->evaluation() >= beta))) {
        pv_list.update(entry->best_move(), PvList());
        return entry->evaluation();
    }

    Move ttmove = (found ? entry->best_move() : MoveNone);
    Move best_move = MoveNone;
    WeightType best_score = -MaxScore;
    WeightType old_alpha = alpha;
    MovePicker move_picker(ttmove, &search_data);
    while (!move_picker.finished()) {
        PvList curr_pv;
        Move move = move_picker.next_move(false);
        if (!search_data.position.make_move<true>(move)) // Avoid illegal moves
            continue;

        ++search_data.searching_depth;
        WeightType score = -alpha_beta(-beta, -alpha, depth_ply - 1, curr_pv, search_data);
        --search_data.searching_depth;
        search_data.position.unmake_move<true>(move);
        assert(score >= -MaxScore);

        if (score > best_score) {
            best_score = score;
            best_move = move;
            pv_list.update(best_move, curr_pv);

            if (score > alpha) {
                alpha = score;
                if (score >= beta) { // fails high
                    // search_data.position.increment_history(move, depth_ply); // TODO
                    break;
                }
            }
        }
    }

    if (best_move == MoveNone) { // handle positions under stalemate or checkmate,
                                 // i.e. positions with no legal moves to be made
        return search_data.position.in_check() ? -MateScore - depth_ply : 0;
    }

    if (!search_data.time_manager.time_over()) { // Save on TT if search was completed
        TTEntry::BoundType bound = best_score >= beta   ? TTEntry::BoundType::LowerBound
                                   : alpha != old_alpha ? TTEntry::BoundType::Exact
                                                        : TTEntry::BoundType::UpperBound;
        entry->save(search_data.position.get_hash(), depth_ply, best_move, best_score,
                    search_data.position.get_game_ply(), bound);
    }

    return best_score;
}

WeightType quiescence(WeightType alpha, WeightType beta, SearchData &search_data) {
    ++search_data.nodes_searched;
    if (search_data.time_manager.time_over() || search_data.stop)
        return -MaxScore;
    else if (search_data.position.draw())
        return 0;

    bool found;
    TTEntry *ttentry = TT.probe(search_data.position, found);
    Move ttmove = (found ? ttentry->best_move() : MoveNone);

    WeightType stand_pat = search_data.position.eval();
    if (stand_pat >= beta)
        return beta;

    // TODO Probably worth to turn off on end game
    // WeightType delta = weights::MidGameQueen + 200;
    // if (game_state.last_move().move_type == MoveType::PawnPromotionQueen)
    //     delta += weights::MidGameQueen;
    //
    // if (stand_pat < alpha - delta) // Delta pruning
    //     return alpha;

    if (alpha < stand_pat)
        alpha = stand_pat;

    MovePicker move_picker(ttmove, &search_data);
    bool capture_found = false;
    while (!move_picker.finished()) {
        Move curr_move = move_picker.next_move(true);
        if (curr_move.type() != MoveType::Capture) {
            if (capture_found)
                break;
            else
                continue;
        }

        capture_found = true;
        if (!search_data.position.make_move<true>(curr_move))
            continue;

        WeightType eval = -quiescence(-beta, -alpha, search_data);
        search_data.position.unmake_move<true>(curr_move);
        if (eval >= beta)
            return beta;
        else if (eval > alpha)
            alpha = eval;
    }

    return alpha;
}
