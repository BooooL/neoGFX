// chrono.hpp
/*
  neogfx C++ App/Game Engine
  Copyright (c) 2015, 2020 Leigh Johnston.  All Rights Reserved.
  
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
#include <neogfx/core/numerical.hpp>
#include <neogfx/game/3rdparty/facebook/flicks.h>

namespace neogfx::game
{
    namespace chrono
    {
        using namespace facebook::util;

        inline constexpr double to_milliseconds(const flicks ns)
        {
            return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(ns)).count());
        }
    }

    typedef scalar time_interval;
    typedef std::optional<time_interval> optional_time_interval;
    typedef int64_t step_time_interval;
    typedef std::optional<step_time_interval> optional_step_time_interval;
    typedef step_time_interval step_time;
    typedef std::optional<step_time> optional_step_time;

    inline step_time_interval to_step_time(time_interval aTime, step_time_interval aStepInterval)
    {
        auto fs = chrono::to_flicks(aTime).count();
        return fs - (fs % aStepInterval);
    }

    inline step_time_interval to_step_time(optional_time_interval& aTime, step_time_interval aStepInterval)
    {
        if (aTime)
            return to_step_time(*aTime, aStepInterval);
        else
            return 0;
    }

    inline time_interval from_step_time(step_time_interval aStepTime)
    {
        return chrono::to_seconds(chrono::flicks{ aStepTime });
    }
}