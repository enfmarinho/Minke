/*
 *  Minke is a UCI chess engine
 *  Copyright (C) 2026 Eduardo Marinho <eduardomarinho@pm.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "search/search.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "core/attacks.h"
#include "core/move.h"
#include "core/position.h"
#include "core/types.h"
#include "eval/eval.h"
#include "search/movepicker.h"
#include "search/tt.h"
#include "uci/tune.h"

static void report_search_info(const CounterType &depth, const ScoreType &eval, const PvList &pv_list,
                               const ThreadData &td) {
    std::cout << "info depth " << depth;
    if (is_decisive(eval)) {
        std::cout << " score mate " << (eval < 0 ? "-" : "") << (MATE_SCORE - std::abs(eval) + 1) / 2;
    } else {
        std::cout << " score cp " << normalize_score(eval);
    }

    // Add 1 to time_passed() to avoid division by 0
    std::cout << " time " << td.search_limiter.time_passed() << " nodes " << td.nodes_searched << " nps "
              << td.nodes_searched * 1000 / (td.search_limiter.time_passed() + 1) << " pv ";

    pv_list.print(td.chess960, td.position.castle_rooks_bb());
    std::cout << std::endl;
}

static void report_search_result(const ThreadData &td, Move best_move) {
    std::cout << "bestmove " << (!best_move ? "none" : best_move.to_uci(td.chess960, td.position.castle_rooks_bb()))
              << std::endl;
}

ThreadData::ThreadData() {
    datagen = false;
    report = true;
    reset_search_parameters();
}

void ThreadData::reset_search_parameters() {
    best_move = Move::none();
    stop = true;
    nodes_searched = -1; // Avoid counting the root
    std::memset(node_table, 0, sizeof(node_table));
    search_limiter.init();
    // TODO i dont think this is necessary
    for (int i = 0; i < MAX_SEARCH_DEPTH; ++i)
        search_stack[i].reset();
}

inline bool stop_search(const ThreadData &td) { return td.stop || td.search_limiter.time_over(td.nodes_searched); }

ScoreType iterative_deepening(ThreadData &td) {
    td.stop = false;

    Move best_move = Move::none();
    ScoreType past_score = -MAX_SCORE;
    ScoreType avg_score = SCORE_NONE;
    CounterType pv_stability = 0;
    CounterType score_stability = 0;
    for (CounterType depth = 1; depth <= std::min(td.search_limiter.max_depth(), MAX_SEARCH_DEPTH - 1); ++depth) {
        const ScoreType score = aspiration(depth, past_score, td);
        if (stop_search(td)) // Search did not finished completely
            break;

        if (avg_score == SCORE_NONE) {
            avg_score = score;
        } else {
            avg_score = (avg_score + score) / 2;
        }

        if (std::abs(avg_score - score) < tm_score_stability_delta()) {
            ++score_stability;
        } else {
            score_stability = 0;
        }

        if (best_move == td.best_move) { // prev best move is the same as current
            ++pv_stability;
        } else {
            pv_stability = 0;
        }

        best_move = td.best_move;
        past_score = score;
        if (!best_move) // No legal moves
            break;

        if (td.report)
            report_search_info(depth, score, td.search_stack[0].pv_list, td);

        if (depth > 5)
            td.search_limiter.update(td, pv_stability, score_stability);
        if (td.search_limiter.stop_early(td.nodes_searched))
            break;

        td.search_limiter.can_stop(); // Avoids stopping before depth 1 has been searched through
    }

    td.stop = true;
    td.best_move = best_move; // A partial search would mess this up
    td.tt.update_age();

    if (td.report)
        report_search_result(td, best_move);

    return past_score;
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
        const ScoreType curr_score = negamax(alpha, beta, curr_depth, 0, false, td);

        if (stop_search(td))
            break;

        score = curr_score;

        if (curr_score <= alpha) {
            beta = (alpha + beta) / 2;
            alpha = std::max<int>(-MAX_SCORE, alpha - delta);
            curr_depth = depth;
        } else if (curr_score >= beta) {
            beta = std::min<int>(MAX_SCORE, beta + delta);
            curr_depth = std::max(1, curr_depth - 1);
        } else {
            break;
        }

        delta += delta * (aw_widening_factor() / 100.0);
    }

    return score;
}

ScoreType negamax(ScoreType alpha, ScoreType beta, CounterType depth, CounterType ply, const bool cutnode,
                  ThreadData &td) {
    if (stop_search(td)) // Out of time
        return -MAX_SCORE;
    else if (depth <= 0)
        return quiescence(alpha, beta, ply, td);
    ++td.nodes_searched;

    const bool pv_node = alpha != beta - 1;
    const Move excluded_move = td.search_stack[ply].excluded_move;
    const bool singular_search = !excluded_move.is_none();
    Position &position = td.position;
    SearchStackEntry &node = td.search_stack[ply];

    // Early return conditions
    const bool root = ply == 0;
    if (!root) {
        if (position.is_draw())
            return 0;

        if (ply >= MAX_SEARCH_DEPTH - 1)
            return position.in_check() ? 0 : td.nnue.eval(position);

        // Mate distance pruning
        alpha = std::max<ScoreType>(alpha, (-MATE_SCORE + ply));
        beta = std::min<ScoreType>(beta, (MATE_SCORE - ply - 1));
        if (alpha >= beta)
            return alpha;
    }

    // Transposition table probe
    TTEntry tte;

    const bool tthit = !singular_search && td.tt.probe(position, tte); // Don't use ttentry result if in singular search

    // Extraction data from ttentry if tthit
    const Move ttmove = (tthit ? tte.best_move() : Move::none());
    const ScoreType ttscore = tthit ? tte.score() : SCORE_NONE;
    const ScoreType tteval = tthit ? tte.eval() : SCORE_NONE;
    const IndexType ttbound = tthit ? tte.bound() : static_cast<IndexType>(BOUND_EMPTY);
    const IndexType ttdepth = tthit ? tte.depth() : 0;
    const bool ttpv = pv_node || (tthit && tte.was_pv());
    if (!pv_node                                      //
        && !singular_search                           //
        && tthit                                      //
        && ttdepth >= depth                           //
        && (ttscore <= alpha || cutnode)              //
        && (ttbound == EXACT                          //
            || (ttbound == UPPER && ttscore <= alpha) //
            || (ttbound == LOWER && ttscore >= beta)) //
    ) {
        return ttscore;
    }

    // Internal Iterative Reductions
    if (!singular_search && (!tthit || ttdepth + 4 < depth) && depth >= 3) {
        depth -= 1;
    }

    const bool in_check = position.in_check();
    ScoreType eval, raw_eval;
    const ScoreType correction_value = td.correction_history.correction(td, ply);
    const ScoreType complexity = std::abs(correction_value);
    if (in_check) {
        eval = node.static_eval = raw_eval = SCORE_NONE;
    } else if (singular_search) {
        eval = raw_eval = node.static_eval;
    } else if (tthit) {
        raw_eval = tteval != SCORE_NONE ? tteval : td.nnue.eval(position);
        eval = node.static_eval = adjust_eval(position, raw_eval, correction_value);
        if (ttscore != SCORE_NONE &&
            (ttbound == EXACT || (ttbound == UPPER && ttscore < eval) || (ttbound == LOWER && ttscore > eval)))
            eval = ttscore;

    } else {
        raw_eval = td.nnue.eval(position);
        eval = node.static_eval = adjust_eval(position, raw_eval, correction_value);
        td.tt.store(position.hash(), 0, Move::none(), SCORE_NONE, raw_eval, BOUND_EMPTY, ttpv, td.tt.age());
    }

    // Clean excluded and killer moves for the next ply
    td.search_stack[ply + 1].excluded_move = Move::none();
    td.search_history.clear_killers(ply + 1);

    const bool improving = [&]() -> bool {
        if (in_check)
            return false;
        if (ply >= 2 && td.search_stack[ply - 2].static_eval != SCORE_NONE)
            return node.static_eval > td.search_stack[ply - 2].static_eval;
        if (ply >= 4 && td.search_stack[ply - 4].static_eval != SCORE_NONE)
            return node.static_eval > td.search_stack[ply - 4].static_eval;

        return false;
    }();

    // Forward pruning methods
    if (!in_check && !pv_node && !root && !singular_search) {
        if (ply >= 1 && td.search_stack[ply - 1].static_eval != SCORE_NONE) {
            const ScoreType eval_delta = node.static_eval + td.search_stack[ply - 1].static_eval;
            const CounterType reduction = td.search_stack[ply - 1].reduction;

            // Hindsight extension
            if (reduction > 1 && eval_delta < 0)
                ++depth;

            // Hindsight reduction
            if (depth >= 2 && reduction > 0 && eval_delta >= hindsight_eval())
                --depth;
        }

        // Reverse futility pruning
        const ScoreType rfp_margin = [&]() {
            ScoreType margin = 0;
            margin += rfp_depth_factor() * depth;
            margin += rfp_improving_margin() * improving;
            margin += rfp_complexity_factor() * complexity / 1024;
            return margin;
        }();
        if (depth < rfp_max_depth() && eval - rfp_margin >= beta) {
            return eval;
        }

        // Razoring heuristic
        if (depth <= razoring_max_depth() && node.static_eval + razoring_mult() * depth < alpha) {
            const ScoreType razor_score = quiescence(alpha, beta, ply, td);
            if (razor_score <= alpha)
                return razor_score;
        }

        // Null move pruning (NMP)
        const ScoreType nmp_beta_margin = [&]() {
            int margin = nmp_beta_base_margin();
            margin -= nmp_beta_improving_margin() * improving;
            margin -= nmp_beta_depth_factor() * depth / 128;
            return std::max(margin, 0);
        }();
        if (!position.last_was_null()                     //
            && depth >= nmp_min_depth()                   //
            && position.has_non_pawns()                   //
            && eval >= beta                               //
            && node.static_eval >= beta + nmp_beta_margin //
        ) {
            const int reduction = (nmp_base_reduction() + depth * nmp_depth_factor()) / 64;

            make_null_move(td);
            td.tt.prefetch(position.hash());
            node.curr_pmove = PieceMove::none();
            const ScoreType null_score = -negamax(-beta, -beta + 1, depth - reduction, ply + 1, !cutnode, td);
            unmake_null_move(td);

            if (null_score >= beta)
                return null_score;
        }

        // Prob Cut
        const ScoreType pc_beta = std::min(beta + probcut_margin(), MATE_FOUND - 1);
        if (depth >= probcut_min_depth()                                                        //
            && !is_decisive(beta)                                                               //
            && (!tthit || ttdepth < depth - 3 || (ttscore != SCORE_NONE && ttscore >= pc_beta)) //
        ) {
            MovePicker move_picker(ttmove, td, ply, PROBCUT, pc_beta - node.static_eval);
            while (true) { // iterate through all moves in move_picker
                const Move move = move_picker.next_move(true);
                if (!move) { // no more moves
                    break;
                }

                if (move == excluded_move) { // skip excluded moves
                    continue;
                }

                node.curr_pmove = {move, position.piece_at(move.from())};
                make_move(td, move);

                td.tt.prefetch(position.hash());

                int pc_score = -quiescence(-pc_beta, -pc_beta + 1, ply + 1, td);
                if (pc_score >= pc_beta)
                    pc_score = -negamax(-pc_beta, -pc_beta + 1, depth - 4, ply + 1, !cutnode, td);

                unmake_move(td, move);

                if (pc_score >= pc_beta) {
                    td.tt.store(position.hash(), depth - 3, move, pc_score, raw_eval, LOWER, ttpv, td.tt.age());
                    return pc_score;
                }
            }
        }
    }

    Move best_move = Move::none();
    ScoreType best_score = -MAX_SCORE;
    BoundType bound = UPPER;
    int moves_searched = 0;

    bool skip_quiets = false;
    MovePicker move_picker(ttmove, td, ply, SEARCH);
    PieceMoveList quiets_tried, tacticals_tried;
    while (true) { // iterate through all moves in move_picker
        const Move move = move_picker.next_move(skip_quiets);
        if (!move) { // no more moves
            break;
        }

        if (move == excluded_move) { // skip excluded moves
            continue;
        }

        if (!root && !is_mated(best_score) && !skip_quiets) {
            const CounterType lmr_scaled_depth =
                depth * 1024 - LMR_TABLE[std::min(depth, 63)][std::min(moves_searched, 63)];
            const CounterType lmr_depth = lmr_scaled_depth / 1024;

            // Late Move Pruning
            if (moves_searched > LMP_TABLE[improving][std::min(depth, LMP_DEPTH - 1)]) {
                skip_quiets = true;
            }

            // Quiet Futility pruning
            const ScoreType futility_value = node.static_eval + fp_margin() + fp_depth_factor() * lmr_depth;
            if (!in_check                             //
                && move.is_quiet()                    //
                && lmr_scaled_depth <= fp_max_depth() //
                && futility_value <= alpha            //
            ) {
                if (!is_decisive(best_score) && best_score < futility_value) {
                    best_score = futility_value;
                }
                skip_quiets = true;
                continue;
            }

            // Quiet History Pruning
            if (lmr_scaled_depth <= history_pruning_max_depth_scaled() //
                && move.is_quiet()                                     //
                && td.search_history.get_history(td, move, ply) <
                       quiet_hist_pruning_factor() * depth + quiet_hist_pruning_base() //
            ) {
                skip_quiets = true;
                continue;
            }

            const int see_margin = see_noisy_pruning_factor() * lmr_depth * lmr_depth;
            if (move_picker.picker_stage() >= PICK_BAD_NOISY && !SEE(position, move, see_margin)) {
                continue;
            }
        }

        // Extensions
        int extension = 0;
        if (!root                                      //
            && depth >= singular_extension_min_depth() //
            && move == ttmove                          //
            && ttdepth > depth - 4                     //
            && move != excluded_move                   //
            && ttbound == LOWER                        //
        ) {
            const ScoreType singular_beta = ttscore - depth * singular_extension_depth_factor() / 16;
            const ScoreType singular_depth = (depth - 1) / 2;

            td.tt.prefetch(position.hash());

            td.search_stack[ply].excluded_move = ttmove;
            const ScoreType singular_score =
                negamax(singular_beta - 1, singular_beta, singular_depth, ply, cutnode, td);
            td.search_stack[ply].excluded_move = Move::none();

            if (singular_score < singular_beta) {
                extension = 1;
                extension += !pv_node && singular_score < singular_beta - double_extension_margin();
                extension += !pv_node && singular_score < singular_beta - triple_ext_margin();
            } else if (singular_score >= beta) { // Multi-Cut
                return singular_score;
            } else if (ttscore >= beta) {
                extension = -2;
            } else if (cutnode) {
                extension = -2;
            }
        }

        node.curr_pmove = {move, position.piece_at(move.from())};
        make_move(td, move);

        td.tt.prefetch(position.hash());
        int new_depth = depth + extension - 1;

        // Add move to tried list
        if (move.is_quiet())
            quiets_tried.push(node.curr_pmove);
        else
            tacticals_tried.push(node.curr_pmove);

        ++moves_searched;

        const int64_t nodes_before_search = td.nodes_searched;
        td.search_stack[ply + 1].pv_list.clear();
        ScoreType score;
        if (moves_searched == 1) {
            score = -negamax(-beta, -alpha, new_depth, ply + 1, false, td);
        } else {
            int scaled_reduction = 0;
            // Late Move Reduction
            if (moves_searched > 1 && depth >= 3 && move.is_quiet()) {
                scaled_reduction = LMR_TABLE[std::min(depth, 63)][std::min(moves_searched, 63)];

                if (position.checkers_bb()) // Reduce less for moves that give check
                    scaled_reduction -= lmr_gives_check_delta();

                scaled_reduction += !improving * lmr_non_improving_delta(); // Reduce more if not improving
                scaled_reduction += cutnode * lmr_cutnode_delta();          // Reduce cutnodes more

                // Reduce less if move is killer
                scaled_reduction -= td.search_history.is_killer(move, ply) * lmr_killer_delta();

                // Reduce less if this move is or was a principal variation
                scaled_reduction -= ttpv * lmr_ttpv_delta();

                // Reduce based on correction history.
                scaled_reduction -= complexity / lmr_corrhist_divisor();
            } else {
                // reduce noisy
            }
            const int reduction = scaled_reduction / 1024;
            const int lmr_depth = std::min(std::max(new_depth - reduction, 1), new_depth);

            td.search_stack[ply].reduction = reduction;
            score = -negamax(-alpha - 1, -alpha, lmr_depth, ply + 1, true, td);
            td.search_stack[ply].reduction = 0;

            if (score > alpha && lmr_depth < new_depth) {
                new_depth += score > best_score + lmr_deeper_margin() + lmr_deeper_depth_factor() * new_depth;
                new_depth -= score < best_score + lmr_shallower_margin() + lmr_shallower_depth_factor() * new_depth;

                score = -negamax(-alpha - 1, -alpha, new_depth, ply + 1, !cutnode, td);
            }

            if (pv_node && score > alpha) {
                score = -negamax(-beta, -alpha, new_depth, ply + 1, false, td);
            }
        }

        unmake_move(td, move);
        assert(score >= -MAX_SCORE);
        td.node_table[move.from_and_to()] += td.nodes_searched - nodes_before_search;

        if (score > best_score) {
            best_score = score;

            if (score > alpha) {
                best_move = move;
                if (pv_node) {
                    node.pv_list.update(best_move, td.search_stack[ply + 1].pv_list);
                }

                if (score >= beta) { // Failed high
                    td.search_history.update_history(td, best_move, depth, ply, quiets_tried, tacticals_tried);
                    bound = LOWER;
                    break;
                }
                alpha = score; // Only update alpha if don't failed high
                bound = EXACT;
            }
        }
    }

    if (moves_searched == 0) { // handle positions under stalemate or checkmate
        return position.in_check() ? -MATE_SCORE + ply : 0;
    }

    if (!in_check                                        //
        && (best_move.is_none() || best_move.is_quiet()) //
        && (bound == EXACT || (bound == LOWER && best_score > node.static_eval) ||
            (bound == UPPER && best_score < node.static_eval)) //
    ) {
        td.correction_history.update(td, depth, ply, best_score - node.static_eval);
    }

    if (!stop_search(td) && !singular_search) {
        td.tt.store(position.hash(), depth, best_move, best_score, raw_eval, bound, ttpv, td.tt.age());
        td.best_move = best_move;
    }

    return best_score;
}

ScoreType quiescence(ScoreType alpha, ScoreType beta, CounterType ply, ThreadData &td) {
    ++td.nodes_searched;
    Position &position = td.position;
    if (stop_search(td))
        return -MAX_SCORE;
    else if (position.is_draw())
        return 0;
    else if (ply >= MAX_SEARCH_DEPTH - 1)
        return position.in_check() ? 0 : td.nnue.eval(position);

    const bool pv_node = alpha != beta - 1;
    SearchStackEntry &node = td.search_stack[ply];

    TTEntry tte;
    const bool tthit = td.tt.probe(position, tte);
    const Move ttmove = tthit ? tte.best_move() : Move::none();
    const ScoreType ttscore = tthit ? tte.score() : SCORE_NONE;
    const ScoreType tteval = tthit ? tte.eval() : SCORE_NONE;
    const IndexType ttbound = tthit ? tte.bound() : static_cast<IndexType>(BOUND_EMPTY);
    const bool ttpv = pv_node || (tthit && tte.was_pv());
    if (!pv_node                                      //
        && tthit                                      //
        && ttscore != SCORE_NONE                      //
        && (ttbound == EXACT                          //
            || (ttbound == UPPER && ttscore <= alpha) //
            || (ttbound == LOWER && ttscore >= beta)) //
    ) {
        return ttscore;
    }

    const bool in_check = position.in_check();
    ScoreType best_score, raw_eval;
    if (in_check) {
        node.static_eval = raw_eval = SCORE_NONE;
        best_score = -MAX_SCORE;
    } else if (tthit) {
        raw_eval = tteval != SCORE_NONE ? tteval : td.nnue.eval(position);
        best_score = node.static_eval = adjust_eval(position, raw_eval, td.correction_history.correction(td, ply));

        if (ttscore != SCORE_NONE                              //
            && (ttbound == EXACT                               //
                || (ttbound == UPPER && ttscore < best_score)  //
                || (ttbound == LOWER && ttscore > best_score)) //
        ) {
            best_score = ttscore;
        }

    } else {
        raw_eval = td.nnue.eval(position);
        best_score = node.static_eval = adjust_eval(position, raw_eval, td.correction_history.correction(td, ply));
        td.tt.store(position.hash(), 0, Move::none(), SCORE_NONE, raw_eval, BOUND_EMPTY, ttpv, td.tt.age());
    }

    // Stand-pat
    if (!in_check && best_score >= beta) {
        return best_score;
    }
    alpha = std::max(alpha, best_score);

    Move best_move = Move::none();
    MovePicker move_picker((tthit ? ttmove : Move::none()), td, ply, QSEARCH);
    int moves_searched = 0;
    BoundType bound = UPPER;
    while (true) { // iterate through all moves in move_picker
        const Move move = move_picker.next_move(!in_check);
        if (!move) { // no more moves
            break;
        }
        node.curr_pmove = {move, position.piece_at(move.from())};

        if (!is_mated(best_score)) {
            if (moves_searched >= 3) // late move pruning
                break;

            const ScoreType futility = node.static_eval + qs_futility_margin();
            if (!in_check                  //
                && futility <= alpha       //
                && !SEE(position, move, 1) //
            ) {
                best_score = std::max(best_score, futility);
                continue;
            }
        }
        make_move(td, move);
        td.tt.prefetch(position.hash());

        ++moves_searched;
        const ScoreType score = -quiescence(-beta, -alpha, ply + 1, td);

        unmake_move(td, move);

        if (score > best_score) {
            best_score = score;
            best_move = move;

            if (score > alpha) {
                if (score >= beta) {
                    bound = LOWER;
                    break;
                }

                alpha = score;
            }
        }
    }

    if (moves_searched == 0 && in_check) {
        return -MATE_SCORE + ply;
    }

    td.tt.store(position.hash(), 0, best_move, best_score, raw_eval, bound, ttpv, td.tt.age());

    return best_score;
}

bool SEE(Position &position, const Move &move, int threshold) {
    if (move.is_castle()) // Cannot win or lose material by castling
        return threshold <= 0;

    const Square from = move.from();
    const Square to = move.to();
    const Piece target = move.is_ep() ? WHITE_PAWN : position.piece_at(to); // piece color does not matter
    const Piece attacker = position.piece_at(from);

    int score = SEE_VALUES[target] - threshold;
    if (move.is_promotion())
        score += SEE_VALUES[move.promotee()] - SEE_VALUES[PAWN];
    if (score < 0) // Cannot beat threshold
        return false;

    score -= (move.is_promotion() ? SEE_VALUES[move.promotee()] : SEE_VALUES[attacker]);
    if (score >= 0) // Already surpassed threshold
        return true;

    Bitboard attackers = position.attackers(to);
    Bitboard occupancy = position.occ_bb() ^ Bitboard(from); // Removed already used attacker
    const Bitboard diagonal_attackers = position.piece_bb(BISHOP) | position.piece_bb(QUEEN);
    const Bitboard line_attackers = position.piece_bb(ROOK) | position.piece_bb(QUEEN);
    Color stm = static_cast<Color>(!position.stm());

    while (true) {
        attackers &= occupancy; // Remove used piece from attackers bitboard

        Bitboard my_attackers = attackers & position.occ_bb(static_cast<Color>(stm));
        if (!my_attackers) // There is no attacker from stm
            break;

        // Get cheapest attacker
        int cheapest_attacker;
        for (cheapest_attacker = PAWN; cheapest_attacker <= KING; ++cheapest_attacker) {
            if ((my_attackers = attackers & position.piece_bb(static_cast<PieceType>(cheapest_attacker), stm)))
                break;
        }
        stm = static_cast<Color>(!stm);

        score = -score - SEE_VALUES[cheapest_attacker] - 1; // Updating negamaxed score

        if (score >= 0) { // Score beats threshold
            if (cheapest_attacker == KING && (attackers & position.occ_bb(static_cast<Color>(!stm))))
                // King is the only attacker and square is still attacked by opponent, so we don't have a valid attacker
                stm = static_cast<Color>(!stm);
            break;
        }

        occupancy ^= my_attackers.isolate_lsb();

        // Add x-ray attackers, if there is any
        switch (cheapest_attacker) {
            case PAWN:
                [[fallthrough]];
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

    return stm != position.stm();
}
