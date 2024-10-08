/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <cstddef>
#include <vector>

#include "game_elements.h"
#include "nnue.h"
#include "position.h"

// TODO check if its better to have a stack with past positions or rely on
// undo_move and PastMove stack.
class GameState {
  public:
    GameState();
    ~GameState() = default;

    bool draw(const CounterType &depth_searched) const;
    bool make_move(const Move &move);
    void undo_move();
    bool reset(const std::string &fen);
    WeightType consult_history(const Move &move) const;
    WeightType &consult_history(const Move &move);
    void increment_history(const Move &move, const CounterType &depth);
    WeightType eval() const;
    const Position &top() const;
    Position &top();
    const Move &last_move() const;

  private:
    bool repetition_draw(const CounterType &depth_searched) const;
    bool draw_50_move() const;

    std::vector<Position> m_position_stack;
    Move m_last_move;
    WeightType m_move_history[NumberOfPieces * 2][0x80];
    Network m_net;
    std::vector<HashType> m_played_positions;
};

#endif // #ifndef GAME_STATE_H
