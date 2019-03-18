// i_native_font.hpp
/*
  neogfx C++ GUI Library
  Copyright (c) 2015 Leigh Johnston.  All Rights Reserved.
  
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
#include <neogfx/core/geometrical.hpp>
#include <neogfx/core/device_metrics.hpp>
#include <neogfx/gfx/text/font.hpp>
#include "i_native_font_face.hpp"

namespace neogfx
{
    class i_native_font
    {
    public:
        virtual ~i_native_font() {}
    public:
        virtual const std::string& family_name() const = 0;
        virtual bool has_style(font_style aStyle) const = 0;
        virtual uint32_t style_count() const = 0;
        virtual font_style style(uint32_t aStyleIndex) const = 0;
        virtual const std::string& style_name(uint32_t aStyleIndex) const = 0;
        virtual i_native_font_face& create_face(font_style aStyle, font::point_size aSize, const i_device_resolution& aDevice) = 0;
        virtual i_native_font_face& create_face(const std::string& aStyleName, font::point_size aSize, const i_device_resolution& aDevice) = 0;
    public:
        virtual void add_ref(i_native_font_face& aFace) = 0;
        virtual void release(i_native_font_face& aFace) = 0;
    };
}