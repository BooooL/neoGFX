// mesh_renderer.hpp
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
#include <neogfx/core/colour.hpp>
#include <neogfx/game/ecs_ids.hpp>
#include <neogfx/game/material.hpp>
#include <neogfx/game/patch.hpp>

namespace neogfx::game
{
    struct mesh_renderer
    {
        material material;
        patches_t patches;
        bool destroyOnFustrumCull;

        struct meta : i_component_data::meta
        {
            static const neolib::uuid& id()
            {
                static const neolib::uuid sId = { 0x19dc8436, 0xfa0d, 0x437c, 0xa784, { 0xca, 0x6a, 0x7a, 0x35, 0x44, 0x8b } };
                return sId;
            }
            static const neolib::i_string& name()
            {
                static const neolib::string sName = "Mesh Renderer";
                return sName;
            }
            static uint32_t field_count()
            {
                return 2;
            }
            static component_data_field_type field_type(uint32_t aFieldIndex)
            {
                switch (aFieldIndex)
                {
                case 0:
                    return component_data_field_type::ComponentData;
                case 1:
                    return component_data_field_type::ComponentData | component_data_field_type::Array;
                case 2:
                    return component_data_field_type::Bool;
                default:
                    throw invalid_field_index();
                }
            }
            static neolib::uuid field_type_id(uint32_t aFieldIndex)
            {
                switch (aFieldIndex)
                {
                case 0:
                    return material::meta::id();
                case 1:
                    return patch::meta::id();
                default:
                    throw invalid_field_index();
                }
            }
            static const neolib::i_string& field_name(uint32_t aFieldIndex)
            {
                static const neolib::string sFieldNames[] =
                {
                    "Material",
                    "Patches",
                    "Destroy On Fustrum Cull"
                };
                return sFieldNames[aFieldIndex];
            }
            static constexpr bool has_updater = true;
            static void update(mesh_renderer& aData, i_ecs&, entity_id)
            {
                std::sort(aData.patches.begin(), aData.patches.end(), [](const patch& lhs, const patch& rhs)
                {
                    auto lhsTexture = lhs.material.texture != std::nullopt ? 
                        lhs.material.texture->id : lhs.material.sharedTexture != std::nullopt ? 
                            lhs.material.sharedTexture->ptr->id : neolib::cookie_ref_ptr{};
                    auto rhsTexture = rhs.material.texture != std::nullopt ?
                        rhs.material.texture->id : rhs.material.sharedTexture != std::nullopt ?
                        rhs.material.sharedTexture->ptr->id : neolib::cookie_ref_ptr{};
                    return lhsTexture < rhsTexture;
                });
            }
        };
    };
}