// gradient.hpp
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
#include <neolib/uuid.hpp>
#include <neolib/string.hpp>
#include <neogfx/game/i_component_data.hpp>
#include <neogfx/game/color.hpp>

namespace neogfx::game
{
    struct gradient
    {
        std::vector<color> colorStops;
        std::vector<scalar> colorStopPositions;
        std::vector<scalar> alphaStops;
        std::vector<scalar> alphaStopPositions;

        struct meta : i_component_data::meta
        {
            static const neolib::uuid& id()
            {
                static const neolib::uuid sId = { 0x7056b018, 0x15a1, 0x4bb1, 0x8b32, { 0xa2, 0x2e, 0x2a, 0x9f, 0xb7, 0xe7 } };
                return sId;
            }
            static const i_string& name()
            {
                static const string sName = "Gradient";
                return sName;
            }
            static uint32_t field_count()
            {
                return 4;
            }
            static component_data_field_type field_type(uint32_t aFieldIndex)
            {
                switch (aFieldIndex)
                {
                case 0:
                    return component_data_field_type::ComponentData | component_data_field_type::Array;
                case 1:
                case 2:
                case 3:
                    return component_data_field_type::Scalar | component_data_field_type::Array;
                default:
                    throw invalid_field_index();
                }
            }
            static neolib::uuid field_type_id(uint32_t aFieldIndex)
            {
                switch (aFieldIndex)
                {
                case 0:
                    return color::meta::id();
                case 1:
                case 2:
                case 3:
                    return neolib::uuid{};
                default:
                    throw invalid_field_index();
                }
            }
            static const i_string& field_name(uint32_t aFieldIndex)
            {
                static const string sFieldNames[] =
                {
                    "Color Stops",
                    "Color Stop Positions",
                    "Alpha Stops",
                    "Alpha Stop Positions"
                };
                return sFieldNames[aFieldIndex];
            }
        };
    };

    inline bool batchable(const gradient& lhs, const gradient& rhs)
    {
        // todo: hash gradient for faster equality operation?
        return lhs.colorStops == rhs.colorStops &&
            lhs.colorStopPositions == rhs.colorStopPositions &&
            lhs.alphaStops == rhs.alphaStops &&
            lhs.alphaStopPositions == rhs.alphaStopPositions;
    }
}