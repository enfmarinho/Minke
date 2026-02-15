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
    int alpha = -MAX_SCORE;
    int beta = MAX_SCORE;
    int delta = aw_first_window();
    if (depth >= aw_min_depth()) {
        alpha = prev_score - delta;
        beta = prev_score + delta;
    }

    int score = SCORE_NONE;
    CounterType curr_depth = depth;
    while (true) {
        ScoreType curr_score = negamax(alpha, beta, curr_depth, false, td);

        if (stop_search(td))
            break;

        score = curr_score;

        if (curr_score <= alpha) {
            beta = (alpha + beta) / 2;
            alpha = std::max(static_cast<int>(-MAX_SCORE), alpha - delta);
            curr_depth = depth;
        } else if (curr_score >= beta) {
            beta = std::min(static_cast<int>(MAX_SCORE), beta + delta);
            curr_depth = std::max(1, curr_depth - 1);
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
    TTEntry *tte = td.tt.probe(position, tthit);
    tthit = tthit && !singular_search; // Don't use ttentry result if in singular search
    // Extraction data from ttentry if tthit
    Move ttmove = (tthit ? tte->best_move() : MOVE_NONE);
    ScoreType ttscore = tthit ? tte->score() : SCORE_NONE;
    ScoreType tteval = tthit ? tte->eval() : SCORE_NONE;
    IndexType ttbound = tthit ? tte->bound() : BOUND_EMPTY;
    IndexType ttdepth = tthit ? tte->depth() : 0;
    const bool ttpv = pv_node || (tthit && tte->was_pv());
    if (!pv_node && !singular_search && tthit && ttdepth >= depth &&
        (ttbound == EXACT || (ttbound == UPPER && ttscore <= alpha) || (ttbound == LOWER && ttscore >= beta))) {
        return ttscore;
    }

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
        eval = node.static_eval = tteval != SCORE_NONE ? tteval : position.eval();
        if (ttscore != SCORE_NONE &&
            (ttbound == EXACT || (ttbound == UPPER && ttscore < eval) || (ttbound == LOWER && ttscore > eval)))
            eval = ttscore;

    } else {
        eval = node.static_eval = position.eval();
        tte->save(position.get_hash(), 0, MOVE_NONE, SCORE_NONE, eval, BOUND_EMPTY, ttpv, tthit);
    }

    // Clean killer moves for the next ply
    td.nodes[td.height + 1].excluded_move = MOVE_NONE;
    td.search_history.clear_killers(td.height + 1);

    bool improving = td.height >= 2 && (node.static_eval > td.nodes[td.height - 2].static_eval ||
                                        td.nodes[td.height - 2].static_eval == SCORE_NONE);

    // Forward pruning methods
    if (!in_check && !pv_node && !root && !singular_search) {
        // Reverse futility pruning
        if (depth < rfp_max_depth() && eval - rfp_margin() * (depth - improving) >= beta)
            return eval;

        // Razoring heuristic
        if (depth <= razoring_max_depth() && node.static_eval + razoring_mult() * depth < alpha) {
            const ScoreType razor_score = quiescence(alpha, beta, td);
            if (razor_score <= alpha)
                return razor_score;
        }

        // Null move pruning
        if (!position.last_was_null() && depth >= nmp_min_depth() && eval >= beta && position.has_non_pawns()) {
            // TODO check for advanced tweaks
            const int reduction =
                nmp_base_reduction() + depth / nmp_depth_reduction_divisor() + std::clamp((eval - beta) / 300, -1, 3);

            position.make_null_move();
            td.tt.prefetch(position.get_hash());
            ++td.height;
            node.curr_pmove = PIECE_MOVE_NONE;
            ScoreType null_score = -negamax(-beta, -beta + 1, depth - reduction, !cutnode, td);
            position.unmake_null_move();
            --td.height;

            if (null_score >= beta)
                return null_score;
        }

        // Prob Cut
        ScoreType pc_beta = std::min(beta + probcut_margin(), MATE_FOUND - 1);
        if (depth >= probcut_min_depth() && std::abs(beta) < MATE_FOUND &&
            (!tthit || ttdepth < depth - 3 || (ttscore != SCORE_NONE && ttscore >= pc_beta))) {
            MovePicker move_picker(ttmove, td, true, pc_beta - node.static_eval);
            Move move;
            while ((move = move_picker.next_move(true)) != MOVE_NONE) {
                if (!position.make_move<true>(move)) { // Avoid illegal moves
                    position.unmake_move<true>(move);
                    continue;
                }

                node.curr_pmove.move = move;
                ++td.height;

                td.tt.prefetch(position.get_hash());

                int pc_score = -quiescence(-pc_beta, -pc_beta + 1, td);
                if (pc_score >= pc_beta)
                    pc_score = -negamax(-pc_beta, -pc_beta + 1, depth - 4, !cutnode, td);

                --td.height;
                position.unmake_move<true>(move);

                if (pc_score >= pc_beta) {
                    tte->save(position.get_hash(), depth - 3, move, pc_score, eval, LOWER, ttpv, tthit);
                    return pc_score;
                }
            }
        }
    }

    Move move = MOVE_NONE;
    Move best_move = MOVE_NONE;
    ScoreType best_score = -MAX_SCORE;
    ScoreType old_alpha = alpha;
    int moves_searched = 0;

    bool skip_quiets = false;
    MovePicker move_picker(ttmove, td, false);
    PieceMoveList quiets_tried, tacticals_tried;
    while ((move = move_picker.next_move(skip_quiets)) != MOVE_NONE) {
        if (move == td.nodes[td.height].excluded_move) // Skip excluded moves
            continue;
        if (!position.make_move<true>(move)) { // Avoid illegal moves
            position.unmake_move<true>(move);
            continue;
        }
        PieceMove curr_pmove = {move, position.consult(move.to())}; // move.to() because move has already been made
        node.curr_pmove = curr_pmove;

        if (!root && best_score >= -MATE_FOUND && !skip_quiets) {
            // Late Move Pruning
            if (moves_searched > LMP_TABLE[improving][std::min(depth, LMP_DEPTH - 1)]) {
                skip_quiets = true;
            }
        }

        // Extensions
        int extension = 0;
        if (!root && depth > singular_extension_min_depth() && move == ttmove && ttdepth > depth - 4 &&
            move != td.nodes[td.height].excluded_move && ttbound == LOWER) {
            ScoreType singular_beta = ttscore - depth;
            ScoreType singular_depth = (depth - 1) / 2;

            position.unmake_move<true>(ttmove);
            td.tt.prefetch(position.get_hash());

            td.nodes[td.height].excluded_move = ttmove;
            ScoreType singular_score = negamax(singular_beta - 1, singular_beta, singular_depth, cutnode, td);
            td.nodes[td.height].excluded_move = MOVE_NONE;
            node.curr_pmove = curr_pmove; // reassign this, since singular search messed it up

            if (singular_score < singular_beta) {
                extension = 1;

                // TODO double extension
            } else if (singular_score >= beta) { // Multi-Cut
                return singular_score;
                // } else if (ttscore <= alpha && ttscore >= beta) {
                //     extension = -1;
            }

            position.make_move<true>(ttmove);
        }
        td.tt.prefetch(position.get_hash());
        int new_depth = depth + extension;

        // Add move to tried list
        if (move.is_quiet())
            quiets_tried.push(curr_pmove);
        else
            tacticals_tried.push(curr_pmove);

        ++td.height;
        ++moves_searched;

        td.nodes[td.height].pv_list.clear();
        ScoreType score;
        if (moves_searched == 1) {
            score = -negamax(-beta, -alpha, new_depth - 1, false, td);
        } else {
            int reduction = 1;
            // Late Move Reduction
            if (moves_searched > 1 && depth > 2 && move.is_quiet()) {
                reduction = LMR_TABLE[std::min(depth, 63)][std::min(moves_searched, 63)];

                reduction -= in_check;   // Reduce less when in check
                reduction += !improving; // Reduce more if not improving

                // Reduce less if move is killer or counter
                reduction -= td.search_history.is_killer(move, td.height - 1);
                if (td.height >= 2)
                    reduction -= td.search_history.is_counter(move, td.nodes[td.height - 2].curr_pmove.move);

                reduction = std::clamp(reduction, 1, depth - 1);

                // Reduce less if this move is or was a principal variation
                reduction -= ttpv;
            } else {
                // reduce noisy
            }
            const int lmr_depth = new_depth - reduction;
            score = -negamax(-alpha - 1, -alpha, lmr_depth, true, td);

            if (score > alpha && reduction > 1) {
                new_depth += score > best_score + 35;
                new_depth -= score < best_score + new_depth;

                score = -negamax(-alpha - 1, -alpha, new_depth - 1, !cutnode, td);
            }

            if (pv_node && score > alpha) {
                score = -negamax(-beta, -alpha, new_depth - 1, false, td);
            }
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
                    td.search_history.update_history(td, best_move, depth, quiets_tried, tacticals_tried);
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
        tte->save(position.get_hash(), depth, best_move, best_score, eval, bound, ttpv, tthit);
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
    NodeData &node = td.nodes[td.height];
    bool tthit;
    TTEntry *tte = td.tt.probe(position, tthit);
    Move ttmove = tthit ? tte->best_move() : MOVE_NONE;
    ScoreType ttscore = tthit ? tte->score() : SCORE_NONE;
    ScoreType tteval = tthit ? tte->eval() : SCORE_NONE;
    IndexType ttbound = tthit ? tte->bound() : BOUND_EMPTY;
    const bool ttpv = pv_node || (tthit && tte->was_pv());
    if (!pv_node && tthit && ttscore != SCORE_NONE &&
        (ttbound == EXACT || (ttbound == UPPER && ttscore <= alpha) || (ttbound == LOWER && ttscore >= beta))) {
        return ttscore;
    }

    bool in_check = position.in_check();
    ScoreType best_score, static_eval;
    if (in_check) {
        static_eval = node.static_eval = SCORE_NONE;
        best_score = -MAX_SCORE;
    } else if (tthit) {
        static_eval = node.static_eval = tteval != SCORE_NONE ? tteval : position.eval();

        if (ttscore != SCORE_NONE && (ttbound == EXACT || (ttbound == UPPER && ttscore < static_eval) ||
                                      (ttbound == LOWER && ttscore > static_eval))) {
            static_eval = ttscore;
        }
        best_score = static_eval;

    } else {
        best_score = static_eval = node.static_eval = position.eval();
        tte->save(position.get_hash(), 0, MOVE_NONE, SCORE_NONE, static_eval, BOUND_EMPTY, ttpv, tthit);
    }

    // Stand-pat
    if (!in_check && best_score >= beta)
        return best_score;
    alpha = std::max(alpha, best_score);

    Move move = MOVE_NONE;
    Move best_move = MOVE_NONE;
    MovePicker move_picker((tthit ? ttmove : MOVE_NONE), td, true);
    int moves_searched = 0;
    while ((move = move_picker.next_move(!in_check)) != MOVE_NONE) {
        if (!position.is_legal(move)) { // Avoid illegal moves
            continue;
        }
        node.curr_pmove = {move, position.consult(move.to())};
        ++moves_searched;

        if (best_score > -MATE_FOUND) {
            ScoreType futility = static_eval + qsFutilityMargin();
            if (!in_check && futility <= alpha && !SEE(position, move, 1)) {
                best_score = std::max(best_score, futility);
                continue;
            }
        }
        position.make_move<true>(move);
        td.tt.prefetch(position.get_hash());

        ++td.height;
        ScoreType score = -quiescence(-beta, -alpha, td);
        --td.height;

        position.unmake_move<true>(move);

        if (score > best_score) {
            best_score = score;
            best_move = move;
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

    BoundType bound = best_score >= beta ? LOWER : UPPER;
    tte->save(position.get_hash(), 0, best_move, best_score, static_eval, bound, ttpv, tthit);

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
