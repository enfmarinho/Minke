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

#include "datagen/book.h"

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "core/position.h"
#include "core/types.h"

EpdBook::EpdBook() { m_book.push_back(START_FEN); }

EpdBook::EpdBook(const std::filesystem::path &path) {
    std::ifstream file_in(path);
    if (!file_in.is_open()) {
        std::cerr << "Warning: could not open EPD opening book " << path << std::endl;
        return;
    }

    std::string fen_oppening;
    while (std::getline(file_in, fen_oppening)) {
        Position pos;
        if (!fen_oppening.empty() && pos.set_fen(fen_oppening)) { // fen is valid
            m_book.push_back(fen_oppening);
        }
    }

    if (m_book.empty()) {
        std::cerr << "Warning: could not read any valid openenings from EPD book " << path << std::endl;
        m_book.push_back(START_FEN);
    } else {
        std::cout << m_book.size() << " openings read from " << path << std::endl;
    }
}
