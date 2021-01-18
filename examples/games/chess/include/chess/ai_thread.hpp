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

#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

#include <chess/primitives.hpp>

namespace chess
{
    struct best_move
    {
        double value = 0.0;
        move move;
    };

    template <typename Representation, player Player>
    class ai_thread
    {
    public:
        typedef Representation representation_type;
        typedef basic_board<representation_type> board_type;
        struct work_item
        {
            board_type board;
            move move;
            std::promise<best_move> result;
        };
    public:
        ai_thread();
        ~ai_thread();
    public:
        std::promise<best_move>& eval(board_type const& aBoard, move const& aMove);
        void start();
    private:
        void process();
    private:
        move_tables<representation_type> const iMoveTables;
        std::deque<work_item> iQueue;
        std::mutex iMutex;
        std::condition_variable iSignal;
        std::thread iThread;
        bool iFinished = false;
    };
}