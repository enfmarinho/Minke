#include "pv_list.h"

void PvList::update(Move new_move, const PvList &list) {
    m_pv[0] = new_move;
    std::copy(list.m_pv.begin(), list.m_pv.begin() + list.m_size, m_pv.begin() + 1);

    m_size = list.m_size + 1;
}

void PvList::print() const {
    for (int i = 0; i < m_size; ++i) {
        std::cout << m_pv[i].get_algebraic_notation() << ' ';
    }
}

PvList &PvList::operator=(const PvList &other) {
    std::copy(other.m_pv.begin(), other.m_pv.begin() + other.m_size, m_pv.begin());
    m_size = other.m_size;

    return *this;
}
