// font_manager.cpp
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

#include <neogfx/neogfx.hpp>
#include <neolib/string_utils.hpp>
#include <boost/filesystem.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
#include <neogfx/app/i_app.hpp>
#include <neogfx/gfx/text/font_manager.hpp>
#include "../../gfx/text/native/native_font_face.hpp"
#include "../../gfx/text/native/native_font.hpp"

namespace neogfx
{
    neolib::small_cookie item_cookie(const font_manager::id_cache_entry& aEntry)
    {
        return aEntry.first.id();
    }

    namespace detail
    {
        class native_font_face_wrapper : public i_native_font_face
        {
        public:
            native_font_face_wrapper(i_native_font_face& aFontFace) : iFontFace(aFontFace) { add_ref(); }
            ~native_font_face_wrapper() { release(); }
        public:
            font_id id() const override { return iFontFace.id(); }
            i_native_font& native_font() override { return iFontFace.native_font(); }
            const std::string& family_name() const override { return iFontFace.family_name(); }
            font_style style() const override { return iFontFace.style(); }
            const std::string& style_name() const override { return iFontFace.style_name(); }
            font::point_size size() const override { return iFontFace.size(); }
            dimension horizontal_dpi() const override { return iFontFace.horizontal_dpi(); }
            dimension vertical_dpi() const override { return iFontFace.vertical_dpi(); }
            dimension height() const override { return iFontFace.height(); }
            dimension descender() const override { return iFontFace.descender(); }
            dimension underline_position() const override { return iFontFace.underline_position(); }
            dimension underline_thickness() const override { return iFontFace.underline_thickness(); }
            dimension line_spacing() const override { return iFontFace.line_spacing(); }
            dimension kerning(uint32_t aLeftGlyphIndex, uint32_t aRightGlyphIndex) const override { return iFontFace.kerning(aLeftGlyphIndex, aRightGlyphIndex); }
            bool is_bitmap_font() const override { return iFontFace.is_bitmap_font(); }
            uint32_t num_fixed_sizes() const override { return iFontFace.num_fixed_sizes(); }
            font::point_size fixed_size(uint32_t aFixedSizeIndex) const override { return iFontFace.fixed_size(aFixedSizeIndex); }
            bool has_fallback() const override { return iFontFace.has_fallback(); }
            bool fallback_cached() const override { return iFontFace.fallback_cached(); }
            i_native_font_face& fallback() const override { return iFontFace.fallback(); }
            void* handle() const override { return iFontFace.handle(); }
            void update_handle(void* aHandle) override { iFontFace.update_handle(aHandle); }
            void* aux_handle() const override { return iFontFace.aux_handle(); }
            uint32_t glyph_index(char32_t aCodePoint) const override { return iFontFace.glyph_index(aCodePoint); }
            i_glyph_texture& glyph_texture(const glyph& aGlyph) const override { return iFontFace.glyph_texture(aGlyph); }
        public:
            void add_ref() override { iFontFace.add_ref(); }
            void release() override { iFontFace.release(); }
        private:
            i_native_font_face& iFontFace;
        };

        namespace platform_specific
        {
            font_info default_system_font_info()
            {
#ifdef WIN32
                std::wstring defaultFontFaceName = L"Microsoft Sans Serif";
                HKEY hkeyDefaultFont;
                if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes",
                    0, KEY_READ, &hkeyDefaultFont) == ERROR_SUCCESS)
                {
                    DWORD dwType;
                    wchar_t byteBuffer[LF_FACESIZE + 1];
                    DWORD dwBufferSize = sizeof(byteBuffer);
                    if (RegQueryValueEx(hkeyDefaultFont, L"MS Shell Dlg 2", NULL, &dwType,
                        (LPBYTE)&byteBuffer, &dwBufferSize) == ERROR_SUCCESS)
                    {
                        defaultFontFaceName = (LPCTSTR)byteBuffer;
                    }
                    else if (RegQueryValueEx(hkeyDefaultFont, L"MS Shell Dlg", NULL, &dwType,
                        (LPBYTE)&byteBuffer, &dwBufferSize) == ERROR_SUCCESS)
                    {
                        defaultFontFaceName = (LPCTSTR)byteBuffer;
                    }
                    ::RegCloseKey(hkeyDefaultFont);
                }
                return font_info(neolib::utf16_to_utf8(reinterpret_cast<const char16_t*>(defaultFontFaceName.c_str())), font_style::Normal, 8);
#else
                throw std::logic_error("neogfx::detail::platform_specific::default_system_font_info: Unknown system");
#endif
            }

            std::string get_system_font_directory()
            {
#ifdef WIN32
                std::string windowsDirectory;
                windowsDirectory.resize(MAX_PATH);
                GetWindowsDirectoryA(&windowsDirectory[0], static_cast<UINT>(windowsDirectory.size()));
                windowsDirectory.resize(std::strlen(windowsDirectory.c_str()));
                return windowsDirectory + "\\fonts";
#else
                throw std::logic_error("neogfx::detail::platform_specific::get_system_font_directory: Unknown system");
#endif
            }

            fallback_font_info default_fallback_font_info()
            {
#ifdef WIN32
                return fallback_font_info{ {"Segoe UI Symbol", "Arial Unicode MS" } };
#else
                throw std::logic_error("neogfx::detail::platform_specific::default_fallback_font_info: Unknown system");
#endif
            }
        }
    }

    fallback_font_info::fallback_font_info(std::vector<std::string> aFallbackFontFamilies) :
        iFallbackFontFamilies(std::move(aFallbackFontFamilies))
    {
    }

    bool fallback_font_info::has_fallback_for(const std::string& aFontFamilyName) const
    {
        if (iFallbackFontFamilies.empty())
            return false;
        return std::find(iFallbackFontFamilies.begin(), iFallbackFontFamilies.end(), aFontFamilyName) != std::prev(iFallbackFontFamilies.end());
    }

    const std::string& fallback_font_info::fallback_for(const std::string& aFontFamilyName) const
    {
        auto f = std::find(iFallbackFontFamilies.begin(), iFallbackFontFamilies.end(), aFontFamilyName);
        if (f == iFallbackFontFamilies.end())
            return *iFallbackFontFamilies.begin();
        ++f;
        if (f == iFallbackFontFamilies.end())
            throw no_fallback();
        return *f;
    }

    font_manager::font_manager() :
        iDefaultSystemFontInfo{ detail::platform_specific::default_system_font_info() },
        iDefaultFallbackFontInfo{ detail::platform_specific::default_fallback_font_info() },
        iGlyphAtlas{ size{1024.0, 1024.0} },
        iEmojiAtlas{}
    {
        FT_Error error = FT_Init_FreeType(&iFontLib);
        if (error)
        {
            throw error_initializing_font_library();
        }
        std::string fontsDirectory = detail::platform_specific::get_system_font_directory();
        for (boost::filesystem::directory_iterator file(fontsDirectory); file != boost::filesystem::directory_iterator(); ++file)
        {
            if (!boost::filesystem::is_regular_file(file->status())) 
                continue;
            try
            {
                if (is_font_file(file->path().string()))
                {
                    auto font = iNativeFonts.emplace(iNativeFonts.end(), iFontLib, file->path().string());
                    iFontFamilies[neolib::make_ci_string(font->family_name())].push_back(font);
                }
            }
            catch (native_font::failed_to_load_font&)
            {
            }
            catch (...)
            {
                throw;
            }
        }
        for (auto& famlily : iFontFamilies)
            std::sort(famlily.second.begin(), famlily.second.end(),
                [](auto const& f1, auto const& f2) { return f1->min_style() < f2->min_style() || (f1->min_style() == f2->min_style() && f1->min_weight() < f2->min_weight()); });
    }

    font_manager::~font_manager()
    {
        iIdCache.clear();
        iFontFamilies.clear();
        iNativeFonts.clear();
        FT_Done_FreeType(iFontLib);
    }

    void* font_manager::font_library_handle() const
    {
        return iFontLib;
    }

    const font_info& font_manager::default_system_font_info() const
    {
        return iDefaultSystemFontInfo;
    }

    const i_fallback_font_info& font_manager::default_fallback_font_info() const
    {
        return iDefaultFallbackFontInfo;
    }

    std::shared_ptr<i_native_font_face> font_manager::create_default_font(const i_device_resolution& aDevice)
    {
        return create_font(service<i_app>().current_style().font_info(), aDevice);
    }

    bool font_manager::has_fallback_font(const i_native_font_face& aExistingFont) const
    {
        return default_fallback_font_info().has_fallback_for(aExistingFont.family_name());
    }
        
    std::shared_ptr<i_native_font_face> font_manager::create_fallback_font(const i_native_font_face& aExistingFont)
    {
        if (!has_fallback_font(aExistingFont))
            throw no_fallback_font();
        if (aExistingFont.fallback_cached())
            return std::shared_ptr<i_native_font_face>(new detail::native_font_face_wrapper(aExistingFont.fallback()));
        struct : i_device_resolution
        {
            size iResolution;
            virtual dimension horizontal_dpi() const { return iResolution.cx; }
            virtual dimension vertical_dpi() const { return iResolution.cy; }
            virtual dimension ppi() const { return iResolution.magnitude() / std::sqrt(2.0); }
        } deviceResolution;
        deviceResolution.iResolution = size(aExistingFont.horizontal_dpi(), aExistingFont.vertical_dpi());
        auto fallbackFont = create_font(default_fallback_font_info().fallback_for(aExistingFont.family_name()), aExistingFont.style(), aExistingFont.size(), deviceResolution);
        fallbackFont->add_ref();
        return fallbackFont;
    }

    std::shared_ptr<i_native_font_face> font_manager::create_font(const std::string& aFamilyName, neogfx::font_style aStyle, font::point_size aSize, const i_device_resolution& aDevice)
    {
        return add_font(find_best_font(aFamilyName, aStyle, aSize).create_face(aStyle, aSize, aDevice));
    }

    std::shared_ptr<i_native_font_face> font_manager::create_font(const std::string& aFamilyName, const std::string& aStyleName, font::point_size aSize, const i_device_resolution& aDevice)
    {
        return add_font(find_font(aFamilyName, aStyleName, aSize).create_face(aStyleName, aSize, aDevice));
    }

    std::shared_ptr<i_native_font_face> font_manager::create_font(const font_info& aInfo, const i_device_resolution& aDevice)
    {
        if (aInfo.style_name_available())
            return create_font(aInfo.family_name(), aInfo.style_name(), aInfo.size(), aDevice);
        else
            return create_font(aInfo.family_name(), aInfo.style(), aInfo.size(), aDevice);
    }

    std::shared_ptr<i_native_font_face> font_manager::create_font(i_native_font& aFont, neogfx::font_style aStyle, font::point_size aSize, const i_device_resolution& aDevice)
    {
        return add_font(aFont.create_face(aStyle, aSize, aDevice));
    }

    std::shared_ptr<i_native_font_face> font_manager::create_font(i_native_font& aFont, const std::string& aStyleName, font::point_size aSize, const i_device_resolution& aDevice)
    {
        return add_font(aFont.create_face(aStyleName, aSize, aDevice));
    }

    bool font_manager::is_font_file(const std::string& aFileName) const
    {
        FT_Face face;
        FT_Error error = FT_New_Face(iFontLib, aFileName.c_str(), 0, &face);
        if (error)
            return false;
        FT_Done_Face(face);
        return true;
    }

    std::shared_ptr<i_native_font_face> font_manager::load_font_from_file(const std::string& aFileName, const i_device_resolution& aDevice)
    {
        throw std::logic_error("neogfx::font_manager::load_font_from_file function overload not yet implemented");
        (void)aFileName;
        (void)aDevice;
    }

    std::shared_ptr<i_native_font_face> font_manager::load_font_from_file(const std::string& aFileName, neogfx::font_style aStyle, font::point_size aSize, const i_device_resolution& aDevice)
    {
        throw std::logic_error("neogfx::font_manager::load_font_from_file function overload not yet implemented");
        (void)aFileName;
        (void)aStyle;
        (void)aSize;
        (void)aDevice;
    }

    std::shared_ptr<i_native_font_face> font_manager::load_font_from_file(const std::string& aFileName, const std::string& aStyleName, font::point_size aSize, const i_device_resolution& aDevice)
    {
        throw std::logic_error("neogfx::font_manager::load_font_from_file function overload not yet implemented");
        (void)aFileName;
        (void)aStyleName;
        (void)aSize;
        (void)aDevice;
    }

    std::shared_ptr<i_native_font_face> font_manager::load_font_from_memory(const void* aData, std::size_t aSizeInBytes, const i_device_resolution& aDevice)
    {
        throw std::logic_error("neogfx::font_manager::load_font_from_memory function overload not yet implemented");
        (void)aData;
        (void)aSizeInBytes;
        (void)aDevice;
    }

    std::shared_ptr<i_native_font_face> font_manager::load_font_from_memory(const void* aData, std::size_t aSizeInBytes, neogfx::font_style aStyle, font::point_size aSize, const i_device_resolution& aDevice)
    {
        throw std::logic_error("neogfx::font_manager::load_font_from_memory function overload not yet implemented");
        (void)aData;
        (void)aSizeInBytes;
        (void)aStyle;
        (void)aSize;
        (void)aDevice;
    }

    std::shared_ptr<i_native_font_face> font_manager::load_font_from_memory(const void* aData, std::size_t aSizeInBytes, const std::string& aStyleName, font::point_size aSize, const i_device_resolution& aDevice)
    {
        throw std::logic_error("neogfx::font_manager::load_font_from_memory function overload not yet implemented");
        (void)aData;
        (void)aSizeInBytes;
        (void)aStyleName;
        (void)aSize;
        (void)aDevice;
    }

    uint32_t font_manager::font_family_count() const
    {
        return static_cast<uint32_t>(iFontFamilies.size());
    }

    std::string font_manager::font_family(uint32_t aFamilyIndex) const
    {
        if (aFamilyIndex < font_family_count())
            return neolib::make_string(std::next(iFontFamilies.begin(), aFamilyIndex)->first);
        throw bad_font_family_index();
    }

    uint32_t font_manager::font_style_count(uint32_t aFamilyIndex) const
    {
        if (aFamilyIndex < font_family_count())
        {
            uint32_t styles = 0;
            for (auto& font : std::next(iFontFamilies.begin(), aFamilyIndex)->second)
                styles += font->style_count();
            return styles;
        }
        throw bad_font_family_index();
    }

    std::string font_manager::font_style(uint32_t aFamilyIndex, uint32_t aStyleIndex) const
    {
        if (aFamilyIndex < font_family_count() && aStyleIndex < font_style_count(aFamilyIndex))
        {
            for (auto& font : std::next(iFontFamilies.begin(), aFamilyIndex)->second)
            {
                if (aStyleIndex < font->style_count())
                    return font->style_name(aStyleIndex);
                aStyleIndex -= font->style_count();
            }
        }
        throw bad_font_family_index();
    }

    font_id font_manager::allocate_font_id()
    {
        return iIdCache.next_cookie();
    }

    const font& font_manager::font_from_id(font_id aId) const
    {
        return iIdCache[aId].first;
    }

    const i_texture_atlas& font_manager::glyph_atlas() const
    {
        return iGlyphAtlas;
    }

    i_texture_atlas& font_manager::glyph_atlas()
    {
        return iGlyphAtlas;
    }

    const i_emoji_atlas& font_manager::emoji_atlas() const
    {
        return iEmojiAtlas;
    }

    i_emoji_atlas& font_manager::emoji_atlas()
    {
        return iEmojiAtlas;
    }

    i_native_font& font_manager::find_font(const std::string& aFamilyName, const std::string& aStyleName, font::point_size aSize)
    {
        auto family = iFontFamilies.find(neolib::make_ci_string(aFamilyName));
        if (family == iFontFamilies.end())
            family = iFontFamilies.find(neolib::make_ci_string(default_system_font_info().family_name()));
        if (family == iFontFamilies.end())
            throw no_matching_font_found();
        std::multimap<uint32_t, native_font_list::iterator> matches;
        for (auto& f : family->second)
        {
            for (uint32_t s = 0; s < f->style_count(); ++s)
                if (neolib::make_ci_string(f->style_name(s)) == neolib::make_ci_string(aStyleName))
                    return *f;
        }
        return find_best_font(aFamilyName, font_style::Normal, aSize);
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

    i_native_font& font_manager::find_best_font(const std::string& aFamilyName, neogfx::font_style aStyle, font::point_size)
    {
        auto family = iFontFamilies.find(neolib::make_ci_string(aFamilyName));
        if (family == iFontFamilies.end())
            family = iFontFamilies.find(neolib::make_ci_string(default_system_font_info().family_name()));
        if (family == iFontFamilies.end())
            throw no_matching_font_found();
        std::optional<std::pair<std::pair<uint32_t, font_weight>, i_native_font*>> bestNormalFont;
        std::optional<std::pair<std::pair<uint32_t, font_weight>, i_native_font*>> bestBoldFont;
        std::optional<std::pair<std::pair<uint32_t, font_weight>, i_native_font*>> bestOtherFont;
        for (auto& f : family->second)
        {
            for (uint32_t s = 0; s < f->style_count(); ++s)
            {
                auto const matchingBits = matching_bits(static_cast<uint32_t>(f->style(s)), static_cast<uint32_t>(aStyle));
                auto weight = font::weight_from_style_name(f->style_name(s));
                if (weight <= font_weight::Normal && (
                    bestNormalFont == std::nullopt || 
                    bestNormalFont->first.first < matchingBits ||
                    (bestNormalFont->first.first == matchingBits && bestNormalFont->first.second < weight)))
                    bestNormalFont = std::make_pair(std::make_pair(matchingBits, weight), &*f);
                else if (weight >= font_weight::Bold && (
                    bestBoldFont == std::nullopt ||
                    bestBoldFont->first.first < matchingBits ||
                    (bestBoldFont->first.first == matchingBits && bestBoldFont->first.second > weight)))
                    bestBoldFont = std::make_pair(std::make_pair(matchingBits, weight), &*f);
                else if (bestOtherFont == std::nullopt ||
                    bestOtherFont->first.first < matchingBits ||
                    (bestOtherFont->first.first == matchingBits && bestOtherFont->first.second < weight))
                    bestOtherFont = std::make_pair(std::make_pair(matchingBits, weight), &*f);
            }
        }
        if ((aStyle & neogfx::font_style::Bold) != neogfx::font_style::Bold)
        {
            if (bestNormalFont != std::nullopt)
                return *bestNormalFont->second;
            else if (bestOtherFont != std::nullopt)
                return *bestOtherFont->second;
            else if (bestBoldFont != std::nullopt)
                return *bestBoldFont->second;
            else
                throw no_matching_font_found();
        }
        else
        {
            if (bestBoldFont != std::nullopt)
                return *bestBoldFont->second;
            else if (bestOtherFont != std::nullopt)
                return *bestOtherFont->second;
            else if (bestNormalFont != std::nullopt)
                return *bestNormalFont->second;
            else
                throw no_matching_font_found();
        }
    }

    void font_manager::add_ref(font_id aId)
    {
        ++iIdCache[aId].second;
    }

    void font_manager::release(font_id aId)
    {
        if (iIdCache[aId].second == 0u)
            throw invalid_release();
        if (--iIdCache[aId].second == 0u)
        {
            if (iIdCache[aId].first.use_count() == 1)
                iIdCache.remove(aId);
        }
    }

    long font_manager::use_count(font_id aId) const
    {
        return iIdCache[aId].second;
    }

    std::shared_ptr<i_native_font_face> font_manager::add_font(i_native_font_face& aNewFont)
    {
        std::shared_ptr<i_native_font_face> font;
        if (iIdCache.contains(aNewFont.id()))
            font = iIdCache[aNewFont.id()].first.native_font_face_ptr();
        else
        {
            font = std::shared_ptr<i_native_font_face>(new detail::native_font_face_wrapper(aNewFont));
            iIdCache.add(aNewFont.id(), id_cache_entry{ font, 0u });
        }
        // cleanup opportunity
        cleanup();
        return font;
    }

    void font_manager::cleanup()
    {
        for (auto i = iIdCache.begin(); i != iIdCache.end();)
        {
            auto& cacheEntry = *i;
            if (cacheEntry.first.use_count() == 1 && cacheEntry.second == 0u)
                i = iIdCache.erase(i);
            else
                ++i;
        }
    }
}