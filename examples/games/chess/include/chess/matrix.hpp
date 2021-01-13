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

#include <limits>
#include <chrono>

#include <chess/primitives.hpp>

namespace chess
{
    template<>
    struct move_tables<matrix>
    {
        typedef neogfx::point_i32 move_coordinates;
        typedef std::array<std::array<std::vector<move_coordinates>, static_cast<std::size_t>(piece_cardinal::COUNT)>, static_cast<std::size_t>(piece_color_cardinal::COUNT)> unit_moves;
        typedef std::array<bool, static_cast<std::size_t>(piece_cardinal::COUNT)> can_move_multiple;
        typedef std::array<std::array<std::array<std::array<std::array<std::array<bool, 8u>, 8u>, 8u>, 8u>, static_cast<std::size_t>(piece_cardinal::COUNT)>, static_cast<std::size_t>(piece_color_cardinal::COUNT)> valid_moves;
        unit_moves unitMoves;
        unit_moves unitCaptureMoves;
        can_move_multiple canMoveMultiple;
        valid_moves validMoves;
        valid_moves validCaptureMoves;
    };

    move_tables<matrix> generate_matrix_move_tables();

    inline bool in_check(move_tables<matrix> const& aTables, player aPlayer, matrix_board const& aBoard);
        
    inline bool can_move(move_tables<matrix> const& aTables, player aTurn, matrix_board const& aBoard, move const& aMove, bool aCheckTest = false)
    {
        auto const movingPiece = piece_at(aBoard, aMove.from);
        auto const targetPiece = piece_at(aBoard, aMove.to);
        if (piece_color(movingPiece) != static_cast<piece>(aTurn) ||
            piece_color(targetPiece) == static_cast<piece>(aTurn))
            return false;
        if (piece_type(targetPiece) == piece::King && !aCheckTest)
            return false;
        // non-capturing move...
        bool enPassant = false;
        bool castle = false;
        if (piece_type(targetPiece) == piece::None && !aTables.validMoves[as_color_cardinal<>(movingPiece)][as_cardinal<>(movingPiece)][aMove.from.y][aMove.from.x][aMove.to.y][aMove.to.x])
        {
            if (piece_type(movingPiece) == piece::Pawn &&
                aTables.validCaptureMoves[as_color_cardinal<>(movingPiece)][as_cardinal<>(movingPiece)][aMove.from.y][aMove.from.x][aMove.to.y][aMove.to.x] &&
                aBoard.lastMove)
            {
                auto const pieceLastMoved = piece_at(aBoard, aBoard.lastMove->to);
                if (piece_type(pieceLastMoved) == piece::Pawn && piece_color(pieceLastMoved) != static_cast<piece>(aTurn))
                {
                    auto const delta = aBoard.lastMove->to.as<int32_t>() - aBoard.lastMove->from.as<int32_t>();
                    if (std::abs(delta.dy) == 2)
                    {
                        auto const& deltaUnity = neogfx::delta_i32{ delta.dx != 0 ? delta.dx / std::abs(delta.dx) : 0, delta.dy != 0 ? delta.dy / std::abs(delta.dy) : 0 };
                        if (aBoard.lastMove->to.as<int32_t>() - deltaUnity == aMove.to.as<int32_t>())
                            enPassant = true;
                    }
                }
            }
            else if (piece_type(movingPiece) == piece::King && aBoard.lastMove && !aBoard.lastMove->castlingState[as_color_cardinal<>(movingPiece)][static_cast<std::size_t>(move::castling_piece_index::King)])
            {
                if (aMove.to.y == aMove.from.y)
                {
                    if ((aMove.to.x == 2 && !aBoard.lastMove->castlingState[as_color_cardinal<>(movingPiece)][static_cast<std::size_t>(move::castling_piece_index::QueensRook)]) ||
                        (aMove.to.x == 6 && !aBoard.lastMove->castlingState[as_color_cardinal<>(movingPiece)][static_cast<std::size_t>(move::castling_piece_index::KingsRook)]))
                        castle = !in_check(aTables, aTurn, aBoard);
                }
            }
            if (!enPassant && !castle)
                return false;
        }
        // capturing move...
        if (piece_type(targetPiece) != piece::None && !aTables.validCaptureMoves[as_color_cardinal<>(movingPiece)][as_cardinal<>(movingPiece)][aMove.from.y][aMove.from.x][aMove.to.y][aMove.to.x])
            return false;
        // any pieces in the way?
        if (aTables.canMoveMultiple[as_cardinal<>(movingPiece)] || castle)
        {
            // todo: would a move array be faster than calculating/using deltas? profile and see.
            auto const delta = move_tables<matrix>::move_coordinates{ static_cast<int32_t>(aMove.to.x), static_cast<int32_t>(aMove.to.y) } - move_tables<matrix>::move_coordinates{ static_cast<int32_t>(aMove.from.x), static_cast<int32_t>(aMove.from.y) };
            auto const& deltaUnity = neogfx::delta_i32{ delta.dx != 0 ? delta.dx / std::abs(delta.dx) : 0, delta.dy != 0 ? delta.dy / std::abs(delta.dy) : 0 };
            auto const start = aMove.from.as<int32_t>() + deltaUnity;
            auto const end = (!castle ? aMove.to.as<int32_t>() : aMove.to.as<int32_t>().with_x(aMove.to.x == 2 ? 0 : 7));
            for (move_tables<matrix>::move_coordinates pos = start; pos != end; pos += deltaUnity)
            {
                auto const inbetweenPiece = piece_at(aBoard, pos);
                if (piece_type(inbetweenPiece) != piece::None)
                    return false;
                if (castle)
                {
                    aBoard.checkTest = move{ aMove.from, pos };
                    bool inCheck = in_check(aTables, aTurn, aBoard);
                    aBoard.checkTest = std::nullopt;
                    if (inCheck)
                        return false;
                }
            }
        }
        if (!aCheckTest)
        {
            aBoard.checkTest = aMove;
            bool inCheck = in_check(aTables, aTurn, aBoard);
            aBoard.checkTest = std::nullopt;
            if (inCheck)
                return false;
        }
        // todo: corner cases (check, en passant, castling)
        return true;
    }

    inline bool in_check(move_tables<matrix> const& aTables, player aPlayer, matrix_board const& aBoard)
    {
        auto const opponent = next_player(aPlayer);
        auto const kingPosition = king_position(aBoard, static_cast<piece>(aPlayer));
        for (coordinate yFrom = 0u; yFrom <= 7u; ++yFrom)
            for (coordinate xFrom = 0u; xFrom <= 7u; ++xFrom)
                if (can_move(aTables, opponent, aBoard, move{ coordinates{ xFrom, yFrom }, kingPosition }, true))
                    return true;
        return false;
    }

    template <chess::player Player>
    constexpr chess::player opponent_v;
    template <>
    constexpr chess::player opponent_v<chess::player::White> = chess::player::Black;
    template <>
    constexpr chess::player opponent_v<chess::player::Black> = chess::player::White;

    template <chess::player Player>
    inline double eval(move_tables<matrix> const& aTables, player aPlayer, matrix_board const& aBoard)
    {
        // todo: remove debug timing code
        auto start = std::chrono::steady_clock::now();

        double constexpr scaleMaterial = 10.0; // todo
        double constexpr scalePromotion = 1.0; // todo
        double constexpr scaleMobility = 1.0; // todo
        double constexpr scaleAttack = 1.0; // todo
        double constexpr scaleDefend = 1.0; // todo
        double constexpr scaleCheck = 1000.0; // todo
        double material = 0.0;
        double mobility = 0.0;
        double attack = 0.0;
        double defend = 0.0;
        double result = 0.0;
        double kingPlayerChecked = 0.0;
        bool kingPlayerMobility = false;
        double kingOpponentChecked = 0.0;
        bool kingOpponentMobility = false;
        for (coordinate yFrom = 0u; yFrom <= 7u; ++yFrom)
            for (coordinate xFrom = 0u; xFrom <= 7u; ++xFrom)
            {
                auto const from = piece_at(aBoard, coordinates{ xFrom, yFrom });
                if (from == piece::None)
                    continue;
                auto const playerFrom = static_cast<chess::player>(piece_color(from));
                auto const valueFrom = piece_value<Player>(from);
                material += valueFrom;
                for (coordinate yTo = 0u; yTo <= 7u; ++yTo)
                    for (coordinate xTo = 0u; xTo <= 7u; ++xTo)
                    {
                        if (yFrom == yTo && xFrom == xTo)
                            continue;
                        auto const to = piece_at(aBoard, coordinates{ xTo, yTo });
                        auto const playerTo = static_cast<chess::player>(piece_color(to));
                        auto const valueTo = piece_value<Player>(to);
                        if (can_move(aTables, Player, aBoard, move{ { xFrom, yFrom }, { xTo, yTo } }, true))
                        {
                            if (from == (piece::King | static_cast<piece>(Player)))
                                kingPlayerMobility = true;
                            if (to == (piece::King | static_cast<piece>(Player)))
                                kingPlayerChecked = 1.0;
                            if (from == (piece::Pawn | static_cast<piece>(Player)) && yTo == promotion_rank_v<Player>)
                                material += (piece_value<Player>(piece::Queen) * scalePromotion);
                            mobility += 1.0;
                            if (playerTo == opponent_v<Player>)
                                attack -= valueTo;
                        }
                        else if (can_move(aTables, opponent_v<Player>, aBoard, move{ { xFrom, yFrom }, { xTo, yTo } }, true))
                        {
                            if (from == (piece::King | static_cast<piece>(opponent_v<Player>)))
                                kingOpponentMobility = true;
                            if (to == (piece::King | static_cast<piece>(opponent_v<Player>)))
                                kingOpponentChecked = 1.0;
                            if (from == (piece::Pawn | static_cast<piece>(opponent_v<Player>)) && yTo == promotion_rank_v<opponent_v<Player>>)
                                material -= (piece_value<Player>(piece::Queen) * scalePromotion);
                            mobility -= 1.0;
                            if (playerTo == Player)
                                attack -= valueTo;
                        }
                        else if (playerFrom == playerTo)
                            defend += valueTo;
                    }
            }
        material *= scaleMaterial;
        mobility *= scaleMobility;
        attack *= scaleAttack;
        defend *= scaleDefend;
        result = material + mobility + attack + defend;
        result -= (kingPlayerChecked * scaleCheck);
        result += (kingOpponentChecked * scaleCheck);
        if (kingPlayerChecked != 0.0 && !kingPlayerMobility)
            result = -std::numeric_limits<double>::infinity();
        if (kingOpponentChecked != 0.0 && !kingOpponentMobility)
            result = +std::numeric_limits<double>::infinity();

        auto end_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();

        // todo: remove the following debug info
        std::cerr << std::fixed;
        std::cerr << std::setprecision(2);
        std::cerr << "material: " << material << std::endl;
        std::cerr << "mobility: " << mobility << std::endl;
        std::cerr << "attack: " << attack << std::endl;
        std::cerr << "defend: " << defend << std::endl;
        std::cerr << "kingPlayerChecked: " << kingPlayerChecked << std::endl;
        std::cerr << "kingPlayerMobility: " << kingPlayerMobility << std::endl;
        std::cerr << "kingOpponentChecked: " << kingOpponentChecked << std::endl;
        std::cerr << "kingOpponentMobility: " << kingOpponentMobility << std::endl;
        std::cerr << "eval: " << result << std::endl;
        std::cerr << "eval time: " << end_us << " us " << std::endl;

        return result;
    }
}