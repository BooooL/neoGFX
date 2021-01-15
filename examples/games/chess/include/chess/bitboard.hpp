﻿/*
neogfx C++ App/Game Engine - Examples - Games - Chess
Copyright(C) 2020 Leigh Johnston

This program is free software: you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <chess/primitives.hpp>

namespace chess
{
    template<>
    struct move_tables<bitboard>
    {
    };

    inline bool in_check(move_tables<bitboard> const& aTables, player aPlayer, bitboard_board const& aBoard);

    inline bool can_move(move_tables<bitboard> const& aTables, player aTurn, bitboard_board const& aBoard, move const& aMove, bool aCheckTest = false, bool aDefendTest = false)
    {
        // todo
        return false;
    }

    inline bool in_check(move_tables<bitboard> const& aTables, player aPlayer, bitboard_board const& aBoard)
    {
        // todo
        return false;
    }

    template <player Player, typename ResultContainer>
    inline void valid_moves(move_tables<bitboard> const& aTables, bitboard_board const& aBoard, ResultContainer& aResult)
    {
        // todo
        aResult.clear();
    }
}