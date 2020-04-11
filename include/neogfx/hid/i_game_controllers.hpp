// i_game_controllers.hpp
/*
  neogfx C++ GUI Library
  Copyright (c) 2020 Leigh Johnston.  All Rights Reserved.
  
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

#include <neogfx/neogfx.hpp>
#include <neolib/i_vector.hpp>
#include <neogfx/hid/i_game_controller.hpp>

namespace neogfx
{
    class i_game_controllers
    {
    public:
        declare_event(controller_connected, i_game_controller&)
        declare_event(controller_disconnected, i_game_controller&)
    public:
        typedef neolib::i_vector<i_game_controller> controller_list;
    public:
        virtual ~i_game_controllers() = default;
    public:
        virtual const controller_list& controllers() const = 0;
        virtual controller_list& controllers() = 0;
    };
}