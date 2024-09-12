/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "search.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>

#include "game_elements.h"
#include "game_state.h"
#include "movegen.h"
#include "position.h"
#include "tt.h"
#include "weights.h"

static void print_search_info(const CounterType &depth, const WeightType &eval, const PvList &pv_list,
                              const SearchData &search_data) {
    std::cout << "info depth " << depth << " score cp " << eval << " nodes " << search_data.nodes_searched << " time "
              << search_data.time_manager.time_passed() << " pv ";
    pv_list.print();
    std::cout << std::endl;
}

void SearchData::reset() {
    stop = true;
    nodes_searched = 0;
    node_limit = std::numeric_limits<int>::max();
    depth_limit = MaxSearchDepth;
}

void iterative_deepening(SearchData &search_data) {
    search_data.stop = false;

    Move best_move = MoveNone;
    for (CounterType depth = 1; depth < search_data.depth_limit; ++depth) {
        PvList pv_list;
        WeightType eval = alpha_beta(ScoreNone, -ScoreNone, depth, pv_list, search_data);

        if (!search_data.time_manager.time_over() && !search_data.stop) { // Search was successful
            bool found;
            TTEntry *entry = TranspositionTable::get().probe(search_data.game_state.top(), found);
            assert(found && entry->depth_ply() >= depth);

            best_move = entry->best_move();
            if (best_move == MoveNone) {
                assert(depth == 1);
                break;
            }
            print_search_info(depth, eval, pv_list, search_data);
        }
        if (search_data.time_manager.stop_early()) {
            break;
        }
        search_data.time_manager.can_stop(); // Avoids stopping before depth 1 has been searched through
    }

    std::cout << "bestmove " << (best_move == MoveNone ? "none" : best_move.get_algebraic_notation()) << std::endl;
    search_data.stop = true;
}

TTEntry *aspiration(const CounterType &depth, PvList &pv_list, SearchData &search_data) {
    bool found;
    TTEntry *ttentry = TranspositionTable::get().probe(search_data.game_state.top(), found);
    if (!found) {
        alpha_beta(ScoreNone, -ScoreNone, depth, pv_list, search_data);
        return ttentry;
    }

    WeightType alpha = ttentry->evaluation() - weights::EndGamePawn;
    WeightType beta = ttentry->evaluation() + weights::EndGamePawn;
    WeightType eval = alpha_beta(alpha, beta, depth, pv_list, search_data);
    if (eval >= beta) {
        alpha_beta(alpha, -ScoreNone, depth, pv_list, search_data);
    } else if (eval <= alpha) {
        alpha_beta(ScoreNone, beta, depth, pv_list, search_data);
    }
    assert(eval >= alpha && eval <= beta);

    return ttentry;
}

WeightType quiescence(WeightType alpha, WeightType beta, SearchData &search_data) {
    ++search_data.nodes_searched;
    if (search_data.time_manager.time_over() || search_data.stop) {
        return ScoreNone;
    }

    WeightType stand_pat = search_data.game_state.eval();
    if (stand_pat >= beta) {
        return beta;
    }

    // TODO Probably worth to turn off on end game
    // WeightType delta = weights::MidGameQueen + 200;
    // if (game_state.last_move().move_type == MoveType::PawnPromotionQueen) {
    //   delta += weights::MidGameQueen;
    // }
    // if (stand_pat < alpha - delta) { // Delta pruning
    //   return alpha;
    // }
    if (alpha < stand_pat) {
        alpha = stand_pat;
    }

    MoveList move_list(search_data.game_state, MoveNone);
    move_list.ignore_non_quiet_moves();
    while (!move_list.empty()) {
        Move curr_move = move_list.next_move();
        if (curr_move.move_type != MoveType::Capture) {
            break;
        } else if (!search_data.game_state.make_move(curr_move)) {
            continue;
        }

        WeightType eval = -quiescence(-beta, -alpha, search_data);
        search_data.game_state.undo_move();
        if (eval >= beta) {
            return beta;
        } else if (eval > alpha) {
            alpha = eval;
        }
    }

    return alpha;
}

WeightType alpha_beta(WeightType alpha, WeightType beta, const CounterType &depth_ply, PvList &pv_list,
                      SearchData &search_data) {
    if (search_data.time_manager.time_over() || search_data.stop) { // Out of time
        return ScoreNone;
    } else if (depth_ply == 0) {
        return quiescence(alpha, beta, search_data);
    }

    ++search_data.nodes_searched;

    // Transposition table probe
    bool found;
    TTEntry *entry = TranspositionTable::get().probe(search_data.game_state.top(), found);
    if (found && entry->depth_ply() >= depth_ply) {
        switch (entry->bound()) {
            case TTEntry::BoundType::Exact:
                pv_list.update(entry->best_move(), PvList());
                return entry->evaluation();
            case TTEntry::BoundType::LowerBound:
                alpha = std::max(entry->evaluation(), alpha);
                break;
            case TTEntry::BoundType::UpperBound:
                beta = std::min(entry->evaluation(), beta);
                break;
            default:
                std::cerr << "TT hit, but entry bound is empty." << std::endl;
                assert(false);
        }

        if (alpha >= beta) {
            pv_list.update(entry->best_move(), PvList());
            return entry->evaluation();
        }
    }

    // TODO check for draw

    Move best_move = MoveNone;
    WeightType best_score = ScoreNone;
    MoveList move_list(search_data.game_state, (found ? entry->best_move() : MoveNone));
    while (!move_list.empty()) {
        PvList curr_pv;
        Move move = move_list.next_move();
        if (!search_data.game_state.make_move(move)) { // Avoid illegal moves
            continue;
        }

        WeightType eval = alpha_beta(-beta, -alpha, depth_ply - 1, curr_pv, search_data);
        search_data.game_state.undo_move();
        assert(eval >= ScoreNone);

        if (eval > best_score) {
            best_score = eval;
            best_move = move;
            pv_list.update(best_move, curr_pv);
        }

        if (eval >= beta) {
            search_data.game_state.increment_history(move, depth_ply);
            return alpha;
        } else if (eval > alpha) {
            alpha = eval;
        }
    }

    if (best_move == MoveNone) { // handle positions under stalemate or checkmate,
                                 // i.e. positions with no legal moves to be made
        const Position &pos = search_data.game_state.top();
        Player adversary;
        PiecePlacement king_placement;
        if (pos.side_to_move() == Player::White) {
            adversary = Player::Black;
            king_placement = pos.white_king_position();
        } else {
            adversary = Player::White;
            king_placement = pos.black_king_position();
        }

        return under_attack(pos, adversary, king_placement) ? -MateScore + depth_ply : 0;
    }

    if (!search_data.time_manager.time_over()) { // Save on TT if search was completed
        TTEntry::BoundType bound = TTEntry::BoundType::Exact;
        if (best_score <= alpha) {
            bound = TTEntry::BoundType::LowerBound;
        } else if (best_score >= beta) {
            bound = TTEntry::BoundType::UpperBound;
        }
        entry->save(search_data.game_state.top().get_hash(), depth_ply, best_move, best_score,
                    search_data.game_state.top().get_half_move_counter(), bound);
    }

    return alpha;
}
