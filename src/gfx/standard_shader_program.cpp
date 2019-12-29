// standard_shader_program.cpp
/*
  neogfx C++ GUI Library
  Copyright (c) 2019 Leigh Johnston.  All Rights Reserved.
  
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

#include <neogfx/gfx/standard_shader_program.hpp>
#include <neogfx/gfx/vertex_shader.hpp>
#include <neogfx/gfx/fragment_shader.hpp>

namespace neogfx
{
    standard_shader_program::standard_shader_program(const std::string& aName) :
        shader_program<i_standard_shader_program>{ aName }
    {
        if (aName == "standard_shader_program")
        {
            add_shader(neolib::make_ref<standard_texture_vertex_shader>().as<i_shader>());
            iDefaultShader = static_cast<i_fragment_shader&>(add_shader(neolib::make_ref<standard_fragment_shader<>>().as<i_shader>()));
            iGradientShader = static_cast<i_gradient_shader&>(add_shader(neolib::make_ref<standard_gradient_shader>().as<i_shader>()));
            iTextureShader = static_cast<i_texture_shader&>(add_shader(neolib::make_ref<standard_texture_shader>().as<i_shader>()));
            iGlyphShader = static_cast<i_glyph_shader&>(add_shader(neolib::make_ref<standard_glyph_shader>().as<i_shader>()));
            iStippleShader = static_cast<i_stipple_shader&>(add_shader(neolib::make_ref<standard_stipple_shader>().as<i_shader>()));
        }
    }

    shader_program_type standard_shader_program::type() const
    {
        return shader_program_type::Standard;
    }

    const i_gradient_shader& standard_shader_program::gradient_shader() const
    {
        if (iGradientShader != nullptr)
            return *iGradientShader;
        throw no_gradient_shader();
    }

    i_gradient_shader& standard_shader_program::gradient_shader()
    {
        if (iGradientShader != nullptr)
            return *iGradientShader;
        throw no_gradient_shader();
    }

    const i_texture_shader& standard_shader_program::texture_shader() const
    {
        return *iTextureShader;
    }

    i_texture_shader& standard_shader_program::texture_shader()
    {
        return *iTextureShader;
    }

    const i_glyph_shader& standard_shader_program::glyph_shader() const
    {
        if (iGlyphShader != nullptr)
            return *iGlyphShader;
        throw no_glyph_shader();
    }

    i_glyph_shader& standard_shader_program::glyph_shader()
    {
        if (iGlyphShader != nullptr)
            return *iGlyphShader;
        throw no_glyph_shader();
    }

    const i_stipple_shader& standard_shader_program::stipple_shader() const
    {
        if (iStippleShader != nullptr)
            return *iStippleShader;
        throw no_stipple_shader();
    }

    i_stipple_shader& standard_shader_program::stipple_shader()
    {
        if (iStippleShader != nullptr)
            return *iStippleShader;
        throw no_stipple_shader();
    }
}