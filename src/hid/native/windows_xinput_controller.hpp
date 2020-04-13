// windows_xinput_controller.hpp
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
#include <neogfx/hid/game_controller.hpp>

namespace neogfx
{
    namespace native::windows
    {
        class xinput_controller : public game_controller
        {
        public:
            xinput_controller(game_controller_port aPort, hid_device_subclass aSubclass = hid_device_subclass::Gamepad, const i_string& aName = string{ "Generic XInput Game Controller" });
            ~xinput_controller();
        };
    }
}