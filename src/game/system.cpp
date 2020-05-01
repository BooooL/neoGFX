// system.cpp
/*
  neogfx C++ GUI Library
  Copyright (c) 2018 Leigh Johnston.  All Rights Reserved.
  
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
#include <neolib/thread.hpp>
#include <neogfx/gfx/i_rendering_engine.hpp>
#include <neogfx/game/ecs.hpp>
#include <neogfx/game/system.hpp>

namespace neogfx::game
{
    system::system(game::i_ecs& aEcs) :
        iEcs{ aEcs }, iPaused{ 0u }
    {
    }

    system::system(const system& aOther) :
        iEcs{ aOther.iEcs }, iComponents{ aOther.iComponents }, iPaused{ 0u }
    {
    }

    system::system(system&& aOther) :
        iEcs{ aOther.iEcs }, iComponents{ std::move(aOther.iComponents) }, iPaused{ 0u }
    {
    }

    game::i_ecs& system::ecs() const
    {
        return iEcs;
    }

    const neolib::i_set<component_id>& system::components() const
    {
        return iComponents;
    }

    neolib::i_set<component_id>& system::components()
    {
        return iComponents;
    }

    const i_component& system::component(component_id aComponentId) const
    {
        return ecs().component(aComponentId);
    }

    i_component& system::component(component_id aComponentId)
    {
        return ecs().component(aComponentId);
    }

    bool system::paused() const
    {
        return iPaused != 0u;
    }

    void system::pause()
    {
        ++iPaused;
    }

    void system::resume()
    {
        --iPaused;
    }

    void system::terminate()
    {
        // do nothing
    }

    void system::yield()
    {
        if (service<i_rendering_engine>().turbo_mode())
            neolib::thread::yield();
        else
            neolib::thread::sleep(1);
    }
}