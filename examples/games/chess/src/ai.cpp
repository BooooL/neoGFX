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

#include <chess/ai.hpp>

namespace chess
{
    template <typename Representation, player Player>
    ai<Representation, Player>::ai() :
        async_thread{ "chess::ai" },
        iMoveTables{ generate_move_tables<representation_type>() },
        iBoard{ chess::setup_position<representation_type>() },
        iThreads{ std::max(1u, std::thread::hardware_concurrency()) }
    {
        start();
        Decided([&](move const& aBestMove)
        {
            play(aBestMove);
        });
    }

    template <typename Representation, player Player>
    ai<Representation, Player>::~ai()
    {
        {
            std::lock_guard<std::mutex> lk{ iSignalMutex };
            iFinished = true;
        }
        iSignal.notify_one();
    }
        
    template <typename Representation, player Player>
    player_type ai<Representation, Player>::type() const
    {
        return player_type::AI;
    }

    template <typename Representation, player Player>
    player ai<Representation, Player>::player() const
    {
        return Player;
    }

    template <typename Representation, player Player>
    void ai<Representation, Player>::greet(i_player& aOpponent)
    {
        iSink = aOpponent.moved([&](move const& aMove)
        {
            std::lock_guard<std::recursive_mutex> lk{ iBoardMutex };
            move_piece(iBoard, aMove);
        });
    }

    template <typename Representation, player Player>
    void ai<Representation, Player>::play()
    {
        {
            std::unique_lock<std::mutex> lk{ iSignalMutex };
            iPlaying = true;
        }
        iSignal.notify_one();
    }

    template <typename Representation, player Player>
    void ai<Representation, Player>::stop()
    {
        // todo
    }

    template <typename Representation, player Player>
    bool ai<Representation, Player>::play(move const& aMove)
    {
        std::lock_guard<std::recursive_mutex> lk{ iBoardMutex };
        move_piece(iBoard, aMove);
        Moved.trigger(aMove);
        return true;
    }

    template <typename Representation, player Player>
    void ai<Representation, Player>::undo()
    {
        chess::undo(iBoard);
    }

    template <typename Representation, player Player>
    bool ai<Representation, Player>::do_work(neolib::yield_type aYieldType)
    {
        bool didWork = async_task::do_work(aYieldType);

        std::unique_lock<std::mutex> lk{ iSignalMutex };
        if (!iFinished)
        {
            iSignal.wait_for(lk, std::chrono::seconds{ 1 }, [&]() { return iPlaying || iFinished; });
            if (iPlaying)
            {
                auto bestMove = execute();
                iPlaying = false;
                if (bestMove)
                    Decided.trigger(bestMove->move);
            }
        }

        return didWork;
    }

    template <typename Representation, player Player>
    std::optional<best_move> ai<Representation, Player>::execute()
    {
        thread_local std::vector<move> tValidMoves;
        std::optional<std::lock_guard<std::recursive_mutex>> lk{ iBoardMutex };
        valid_moves<Player>(iMoveTables, iBoard, tValidMoves, true);
        // todo: opening book and/or sensible white first move...
        thread_local std::random_device tEntropy;
        thread_local std::mt19937 tGenerator{ tEntropy() };
        if (tValidMoves.size() > 0u)
        {
            ai_thread<representation_type, Player> thread;
            std::vector<std::future<best_move>> futures;
            futures.reserve(tValidMoves.size());
            auto iterThread = iThreads.begin();
            for (auto const& m : tValidMoves)
            {
                futures.emplace_back(iterThread->eval(iBoard, m).get_future());
                if (++iterThread == iThreads.end())
                    iterThread = iThreads.begin();
            }
            lk = std::nullopt;
            for (auto& t : iThreads)
                t.start();
            std::vector<best_move> bestMoves;
            for (auto& future : futures)
            {
                auto const& nextMove = future.get();
                bestMoves.push_back(nextMove);
            }
            std::sort(bestMoves.begin(), bestMoves.end(),
                [](auto const& m1, auto const& m2)
                {
                    return m1.value >= m2.value;
                });
            auto const bestMove = bestMoves[0];
            auto const decimator = 0.125 * (iBoard.moveHistory.size() + 1); // todo: involve difficulty level?
            auto similarEnd = std::remove_if(bestMoves.begin(), bestMoves.end(),
                [decimator, bestMove](auto const& m)
                {
                    return static_cast<int64_t>(m.value * decimator) != static_cast<int64_t>(bestMove.value * decimator);
                });
            std::uniform_int_distribution<std::ptrdiff_t> options{ 0, std::distance(bestMoves.begin(), similarEnd) };
            return bestMoves[options(tGenerator)];
        }
        return {};
    }

    template <typename Representation, player Player>
    bool ai<Representation, Player>::playing() const
    {
        return iPlaying;
    }

    template <typename Representation, player Player>
    void ai<Representation, Player>::setup(matrix_board const& aSetup)
    {
        if constexpr (std::is_same_v<representation_type, matrix>)
        {
            std::lock_guard<std::recursive_mutex> lk{ iBoardMutex };
            iBoard = aSetup;
        }
        else
            ; // todo (convert to bitboard representation)
    }

    template class ai<matrix, player::White>;
    template class ai<matrix, player::Black>;
    template class ai<bitboard, player::White>;
    template class ai<bitboard, player::Black>;
}