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
#include "tune.h"
#include "types.h"

static void print_search_info(const CounterType &depth, const ScoreType &eval, const PvList &pv_list,
                              const ThreadData &td) {
    std::cout << "info depth " << depth;
    if (std::abs(eval) > MATE_FOUND) {
        std::cout << " score mate " << (eval < 0 ? "-" : "") << (MATE_SCORE - std::abs(eval) + 1) / 2;
    } else {
        std::cout << " score cp " << eval / 2;
    }
    // Add 1 to time_passed() to avoid division by 0
    std::cout << " time " << td.time_manager.time_passed() << " nodes " << td.nodes_searched << " nps "
              << td.nodes_searched * 1000 / (td.time_manager.time_passed() + 1) << " pv ";
    pv_list.print(td.chess960, td.position.get_castle_rooks());
    std::cout << std::endl;
}

SearchLimits::SearchLimits() { reset(); }

SearchLimits::SearchLimits(int _depth, int _optimum_node, int _maximum_node)
    : depth(_depth), optimum_node(_optimum_node), maximum_node(_maximum_node) {}

inline void SearchLimits::reset() {
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
    // TODO i dont think this is necessary
    for (int i = 0; i < MAX_SEARCH_DEPTH; ++i)
        nodes[i].reset();
}

void ThreadData::set_search_limits(const SearchLimits sl) { this->search_limits = sl; }

inline bool stop_search(const ThreadData &td) {
    return td.time_manager.time_over() || td.stop || td.nodes_searched > td.search_limits.maximum_node;
}

ScoreType iterative_deepening(ThreadData &td) {
    td.stop = false;

    Move best_move = MOVE_NONE;
    ScoreType past_eval = -MAX_SCORE;
    for (CounterType depth = 1; depth <= td.search_limits.depth; ++depth) {
        ScoreType eval = aspiration(depth, past_eval, td);
        if (stop_search(td)) // Search did not finished completely
            break;

        best_move = td.best_move;
        past_eval = eval;
        if (best_move == MOVE_NONE) // No legal moves
            break;

        if (td.report)
            print_search_info(depth, eval, td.nodes[0].pv_list, td);

        if (depth > 5)
            td.time_manager.update();
        if (td.time_manager.stop_early())
            break;

        td.time_manager.can_stop(); // Avoids stopping before depth 1 has been searched through
    }

    if (td.report)
        std::cout << "bestmove "
                  << (best_move == MOVE_NONE
                          ? "none"
                          : best_move.get_algebraic_notation(td.chess960, td.position.get_castle_rooks()))
                  << std::endl;

    td.stop = true;
    td.best_move = best_move; // A partial search would mess this up
    return past_eval;
}

ScoreType aspiration(const CounterType &depth, const ScoreType prev_score, ThreadData &td) {
    ScoreType alpha = -MAX_SCORE;
    ScoreType beta = MAX_SCORE;
    ScoreType delta = aw_first_window();
    if (depth >= aw_min_depth()) {
        alpha = prev_score - delta;
        beta = prev_score + delta;
    }

    ScoreType score = SCORE_NONE;
    while (true) {
        ScoreType curr_score = negamax(alpha, beta, depth, false, td);

        if (stop_search(td))
            break;

        score = curr_score;

        if (curr_score <= alpha) {
            beta = (alpha + beta) / 2;
            alpha = std::max(-MAX_SCORE, score - delta);
        } else if (curr_score >= beta) {
            beta = std::min(static_cast<ScoreType>(score + delta), MAX_SCORE);
        } else {
            break;
        }

        delta += delta * (aw_widening_factor() / 100.0);
    }

    return score;
}

ScoreType negamax(ScoreType alpha, ScoreType beta, CounterType depth, const bool cutnode, ThreadData &td) {
    if (stop_search(td)) // Out of time
        return -MAX_SCORE;
    else if (depth <= 0)
        return quiescence(alpha, beta, td);
    ++td.nodes_searched;

    bool pv_node = alpha != beta - 1;
    bool singular_search = td.nodes[td.height].excluded_move != MOVE_NONE;
    Position &position = td.position;
    NodeData &node = td.nodes[td.height];

    // Early return conditions
    bool root = td.height == 0;
    if (!root) {
        if (position.draw())
            return 0;

        if (td.height >= MAX_SEARCH_DEPTH - 1)
            return position.in_check() ? 0 : position.eval();

        // Mate distance pruning
        alpha = std::max(alpha, static_cast<ScoreType>(-MATE_SCORE + td.height));
        beta = std::min(beta, static_cast<ScoreType>(MATE_SCORE - td.height - 1));
        if (alpha >= beta)
            return alpha;
    }

    // Transposition table probe
    bool tthit;
    TTEntry *ttentry = td.tt.probe(position, tthit);
    tthit = tthit && !singular_search; // Don't use ttentry result if in singular search
    if (!pv_node && !singular_search && tthit && ttentry->depth() >= depth &&
        (ttentry->bound() == EXACT || (ttentry->bound() == UPPER && ttentry->score() <= alpha) ||
         (ttentry->bound() == LOWER && ttentry->score() >= beta))) {
        return ttentry->score();
    }
    // Extraction data from ttentry if tthit
    Move ttmove = (tthit ? ttentry->best_move() : MOVE_NONE);

    // Internal Iterative Reductions
    if (!tthit && depth >= iir_min_depth()) {
        depth -= iir_depth_reduction();
    }

    bool in_check = position.in_check();
    ScoreType eval;
    if (in_check) {
        eval = node.static_eval = SCORE_NONE;
    } else if (singular_search) {
        eval = node.static_eval;
    } else if (tthit) {
        eval = node.static_eval = position.eval();
        if (ttentry->score() != SCORE_NONE &&
            (ttentry->bound() == EXACT || (ttentry->bound() == UPPER && ttentry->score() < eval) ||
             (ttentry->bound() == LOWER && ttentry->score() > eval)))
            eval = ttentry->score();

    } else {
        eval = node.static_eval = position.eval();
    }

    bool improving = td.height >= 2 && (node.static_eval > td.nodes[td.height - 2].static_eval ||
                                        td.nodes[td.height - 2].static_eval == SCORE_NONE);

    // Forward pruning methods
    if (!in_check && !pv_node && !root && !singular_search) {
        // Reverse futility pruning
        if (depth < rfp_max_depth() && eval - rfp_margin() * (depth - improving) >= beta)
            return eval;

        // Null move pruning
        if (!position.last_was_null() && depth >= nmp_min_depth() && eval >= beta && position.has_non_pawns()) {
            // TODO check for advanced tweaks
            const int reduction =
                nmp_base_reduction() + depth / nmp_depth_reduction_divisor() + std::clamp((eval - beta) / 300, -1, 3);

            position.make_null_move();
            ++td.height;
            node.curr_move = MOVE_NONE;
            ScoreType null_score = -negamax(-beta, -beta + 1, depth - reduction, !cutnode, td);
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

    bool skip_quiets = false;
    MovePicker move_picker(ttmove, &td, false);
    node.quiets_tried.clear();
    node.tacticals_tried.clear();
    while ((move = move_picker.next_move(skip_quiets)) != MOVE_NONE) {
        if (move == td.nodes[td.height].excluded_move) // Skip excluded moves
            continue;
        if (!position.make_move<true>(move)) { // Avoid illegal moves
            position.unmake_move<true>(move);
            continue;
        }
        node.curr_move = move;

        if (!root && best_score > -MAX_SCORE && !skip_quiets) {
            // Late Move Pruning
            if (moves_searched > LMP_TABLE[improving][std::min(depth, LMP_DEPTH - 1)]) {
                skip_quiets = true;
            }
        }

        // Extensions
        int extension = 0;
        if (!root && depth > singular_extension_min_depth() && move == ttmove && ttentry->depth() > depth - 4 &&
            move != td.nodes[td.height].excluded_move && ttentry->bound() == LOWER) {
            ScoreType singular_beta = ttentry->score() - depth;
            ScoreType singular_depth = (depth - 1) / 2;

            position.unmake_move<true>(ttmove);

            td.nodes[td.height].excluded_move = ttmove;
            ScoreType singular_score = negamax(singular_beta - 1, singular_beta, singular_depth, cutnode, td);
            td.nodes[td.height].excluded_move = MOVE_NONE;

            if (singular_score < singular_beta) {
                extension = 1;

                // TODO double extension
            } else if (singular_score >= beta) { // Multi-Cut
                return singular_score;
                // } else if (ttentry->score() <= alpha && ttentry->score() >= beta) {
                //     extension = -1;
            }

            position.make_move<true>(ttmove);
        }
        int new_depth = depth + extension;

        // Add move to tried list
        if (move.is_quiet())
            node.quiets_tried.push(move);
        else
            node.tacticals_tried.push(move);

        ++td.height;
        ++moves_searched;

        td.nodes[td.height].pv_list.clear();
        ScoreType score;
        if (moves_searched == 1) {
            score = -negamax(-beta, -alpha, new_depth - 1, false, td);
        } else {
            int reduction = 1;
            // Late Move Reduction
            if (moves_searched > 1 + pv_node && depth > 2 && move.is_quiet()) {
                reduction = LMR_TABLE[std::min(depth, 63)][std::min(moves_searched, 63)];

                reduction -= in_check;   // Reduce less when in check
                reduction += !improving; // Reduce more if not improving

                // Reduce less if move is killer or counter
                reduction -= td.search_history.is_killer(move, depth);
                reduction = std::clamp(reduction, 1, depth - 1);
            } else {
                // reduce noisy
            }
            score = -negamax(-alpha - 1, -alpha, new_depth - reduction, true, td);
            if (score > alpha && score < beta)
                score = -negamax(-beta, -alpha, new_depth - 1, !cutnode, td);
        }

        --td.height;
        position.unmake_move<true>(move);
        assert(score >= -MAX_SCORE);

        if (score > best_score) {
            best_score = score;

            if (score > alpha) {
                best_move = move;
                if (pv_node)
                    node.pv_list.update(best_move, td.nodes[td.height + 1].pv_list);

                if (score >= beta) { // Failed high
                    td.search_history.update_history(td, best_move, depth);
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
    else if (td.height >= MAX_SEARCH_DEPTH - 1)
        return position.in_check() ? 0 : position.eval();

    bool pv_node = alpha != beta - 1;
    bool tthit;
    TTEntry *tte = td.tt.probe(position, tthit);
    if (!pv_node && tthit && tte->score() != SCORE_NONE &&
        (tte->bound() == EXACT || (tte->bound() == UPPER && tte->score() <= alpha) ||
         (tte->bound() == LOWER && tte->score() >= beta))) {
        return tte->score();
    }

    ScoreType best_score = position.eval();
    // Stand-pat
    if (best_score >= beta)
        return beta;
    alpha = std::max(alpha, best_score);

    Move move = MOVE_NONE;
    MovePicker move_picker((tthit ? tte->best_move() : MOVE_NONE), &td, true);
    int moves_searched = 0;
    bool in_check = position.in_check();
    while ((move = move_picker.next_move(!in_check)) != MOVE_NONE) {
        if (!position.make_move<true>(move)) { // Avoid illegal moves
            position.unmake_move<true>(move);
            continue;
        }
        ++moves_searched;
        ++td.height;

        ScoreType score = -quiescence(-beta, -alpha, td);

        position.unmake_move<true>(move);
        --td.height;

        if (score > best_score) {
            best_score = score;
            if (score > alpha) {
                if (score >= beta) {
                    break;
                }
                alpha = score;
            }
        }
    }

    if (moves_searched == 0 && in_check) {
        return -MATE_SCORE + td.height;
    }

    return best_score;
}

bool SEE(Position &position, const Move &move, int threshold) {
    if (move.is_castle()) // Cannot win or lose material by castling
        return threshold <= 0;

    Square from = move.from();
    Square to = move.to();
    Piece target = move.is_ep() ? WHITE_PAWN : position.consult(to); // piece color does not matter
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
