// native_font.hpp
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

#include <neogfx/neogfx.hpp>
#include <boost/filesystem.hpp>
#include <neolib/core/string_ci.hpp>
#include <neogfx/gfx/text/i_font_manager.hpp>
#include "native_font.hpp"
#include "native_font_face.hpp"

namespace neogfx
{
    native_font::native_font(FT_Library aFontLib, const std::string aFileName) :
        iFontLib(aFontLib), iSource(filename_type(aFileName)), iCache{}, iFaceCount(0)
    {
        register_face(0);
        for (FT_Long f = 1; f < iFaceCount; ++f)
            register_face(f);
        iCache.clear();
        iCache.shrink_to_fit();
    }

    native_font::native_font(FT_Library aFontLib, const void* aData, std::size_t aSizeInBytes) :
        iFontLib(aFontLib), iSource(memory_block_type(aData, aSizeInBytes)), iCache{}, iFaceCount(0)
    {
        register_face(0);
        for (FT_Long f = 1; f < iFaceCount; ++f)
            register_face(f);
        iCache.clear();
        iCache.shrink_to_fit();
    }

    native_font::~native_font()
    {
    }

    std::string const& native_font::family_name() const
    {
        return iFamilyName;
    }

    bool native_font::has_style(font_style aStyle) const
    {
        auto result = std::find_if(iStyleMap.begin(), iStyleMap.end(), [aStyle](auto const& s) { return s.first.first == aStyle; });
        return result != iStyleMap.end();
    }

    uint32_t native_font::style_count() const
    {
        return static_cast<uint32_t>(iStyleMap.size());
    }

    font_style native_font::style(uint32_t aStyleIndex) const
    {
        return std::next(iStyleMap.begin(), aStyleIndex)->first.first;
    }

    std::string const& native_font::style_name(uint32_t aStyleIndex) const
    {
        return std::next(iStyleMap.begin(), aStyleIndex)->first.second;
    }

    namespace
    {
        uint32_t matching_bits(uint32_t lhs, uint32_t rhs)
        {
            if (lhs == rhs)
                return 32;
            uint32_t matches = 0;
            uint32_t test = 1;
            while (test != 0)
            {
                if ((lhs & rhs) & test)
                    ++matches;
                test <<= 1;
            }
            return matches;
        }
    }

    void native_font::create_face(font_style aStyle, font::point_size aSize, const i_device_resolution& aDevice, i_ref_ptr<i_native_font_face>& aResult)
    {
        std::multimap<font_style, style_map::value_type*> matches;
        for (auto& s : iStyleMap)
            matches.insert(std::make_pair(static_cast<font_style>(matching_bits(static_cast<uint32_t>(s.first.first), static_cast<uint32_t>(aStyle))), &s));
        if (matches.empty())
            throw no_matching_style_found();
        FT_Long faceIndex = matches.rbegin()->second->second;
        font_style faceStyle = matches.rbegin()->second->first.first;
        aResult = create_face(faceIndex, faceStyle, aSize, aDevice);
    }

    void native_font::create_face(std::string const& aStyleName, font::point_size aSize, const i_device_resolution& aDevice, i_ref_ptr<i_native_font_face>& aResult)
    {
        style_map::value_type* foundStyle = 0;
        for (auto& s : iStyleMap)
            if (neolib::make_ci_string(s.first.second) == neolib::make_ci_string(aStyleName))
            {
                foundStyle = &s;
                break;
            }
        if (foundStyle == nullptr)
        {
            create_face(font_style::Normal, aSize, aDevice, aResult);
            return;
        }
        FT_Long faceIndex = foundStyle->second;
        font_style faceStyle = foundStyle->first.first;
        aResult = create_face(faceIndex, faceStyle, aSize, aDevice);
    }

    void native_font::register_face(FT_Long aFaceIndex)
    {
        FT_Face face = open_face(aFaceIndex);
        try
        {
            if (aFaceIndex == 0)
            {
                iFaceCount = face->num_faces;
                iFamilyName = face->family_name;
            }
            font_style style = font_style::Invalid;
            if (face->style_flags & FT_STYLE_FLAG_ITALIC)
                style = static_cast<font_style>(style | font_style::Italic);
            if (face->style_flags & FT_STYLE_FLAG_BOLD)
                style = static_cast<font_style>(style | font_style::Bold);
            auto const searchKey = neolib::ci_string{ face->style_name };
            if (searchKey.find("italic") != neolib::ci_string::npos)
                style |= font_style::Italic;
            if (searchKey.find("bold") != neolib::ci_string::npos || searchKey.find("heavy") != neolib::ci_string::npos || searchKey.find("black") != neolib::ci_string::npos)
                style |= font_style::Bold;
            if (style == font_style::Invalid)
                style = font_style::Normal;
            iStyleMap.emplace(std::make_pair(style, face->style_name), aFaceIndex);
        }
        catch (...)
        {
            close_face(face);
            throw;
        }
        close_face(face);
    }

    FT_Face native_font::open_face(FT_Long aFaceIndex)
    {
        FT_Face face;
        if (std::holds_alternative<filename_type>(iSource))
        {
            if (iCache.empty())
            {
                std::size_t fileSize = static_cast<std::size_t>(boost::filesystem::file_size(static_variant_cast<const filename_type&>(iSource)));
                iCache.resize(fileSize);
                std::ifstream file{ static_variant_cast<const filename_type&>(iSource).c_str(), std::ios::in | std::ios::binary };
                file.read(reinterpret_cast<char*>(&iCache[0]), fileSize);
            }
            FT_Error error = FT_New_Memory_Face(
                iFontLib,
                static_cast<const FT_Byte*>(&iCache[0]),
                static_cast<FT_Long>(iCache.size()),
                aFaceIndex,
                &face);
            if (error)
                throw failed_to_load_font();
        }
        else
        {
            FT_Error error = FT_New_Memory_Face(
                iFontLib,
                static_cast<const FT_Byte*>(static_variant_cast<const memory_block_type&>(iSource).first),
                static_cast<FT_Long>(static_variant_cast<const memory_block_type&>(iSource).second),
                aFaceIndex,
                &face);
            if (error)
                throw failed_to_load_font();
        }
        return face;
    }

    void native_font::close_face(FT_Face aFace)
    {
        FT_Done_Face(aFace);
    }

    ref_ptr<i_native_font_face> native_font::create_face(FT_Long aFaceIndex, font_style aStyle, font::point_size aSize, const i_device_resolution& aDevice)
    {
        auto existingFace = iFaces.find(std::make_tuple(aFaceIndex, aSize, size(aDevice.horizontal_dpi(), aDevice.vertical_dpi())));
        if (existingFace != iFaces.end())
            return existingFace->second;
        FT_Face newFreetypeFace = open_face(aFaceIndex);
        try
        {
            auto newFontId = service<i_font_manager>().allocate_font_id();
            auto newFace = make_ref<native_font_face>(newFontId, *this, aStyle, aSize, size(aDevice.horizontal_dpi(), aDevice.vertical_dpi()), newFreetypeFace);
            iFaces.insert(std::make_pair(std::make_tuple(aFaceIndex, aSize, size(aDevice.horizontal_dpi(), aDevice.vertical_dpi())), newFace)).first;
            return newFace;
        }
        catch (...)
        {
            if (newFreetypeFace != 0)
                close_face(newFreetypeFace);
            throw;
        }
    }
}