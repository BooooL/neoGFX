// title_bar.cpp
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

#include <neogfx/app/i_app.hpp>
#include <neogfx/hid/i_surface_manager.hpp>
#include <neogfx/gui/widget/title_bar.hpp>

namespace neogfx
{
    title_bar::title_bar(i_window& aWindow, const std::string& aTitle) :
        widget{ aWindow.as_widget() },
        iWindow{ aWindow },
        iLayout{ *this },
        iIcon{ iLayout, service<i_app>().default_window_icon() },
        iTitle{ iLayout, aTitle },
        iSpacer{ iLayout },
        iMinimizeButton{ iLayout, push_button_style::TitleBar },
        iMaximizeButton{ iLayout, push_button_style::TitleBar },
        iRestoreButton{ iLayout, push_button_style::TitleBar },
        iCloseButton{ iLayout, push_button_style::TitleBar }
    {
        init();
    }

    title_bar::title_bar(i_window& aWindow, const i_texture& aIcon, const std::string& aTitle) :
        widget{ aWindow.as_widget() },
        iWindow{ aWindow },
        iLayout{ *this },
        iIcon{ iLayout, aIcon },
        iTitle{ iLayout, aTitle },
        iSpacer{ iLayout },
        iMinimizeButton{ iLayout, push_button_style::TitleBar },
        iMaximizeButton{ iLayout, push_button_style::TitleBar },
        iRestoreButton{ iLayout, push_button_style::TitleBar },
        iCloseButton{ iLayout, push_button_style::TitleBar }
    {
        init();
    }

    title_bar::title_bar(i_window& aWindow, const i_image& aIcon, const std::string& aTitle) :
        widget{ aWindow.as_widget() },
        iWindow{ aWindow },
        iLayout{ *this },
        iIcon{ iLayout, aIcon },
        iTitle{ iLayout, aTitle },
        iSpacer{ iLayout },
        iMinimizeButton{ iLayout, push_button_style::TitleBar },
        iMaximizeButton{ iLayout, push_button_style::TitleBar },
        iRestoreButton{ iLayout, push_button_style::TitleBar },
        iCloseButton{ iLayout, push_button_style::TitleBar }
    {
        init();
    }

    title_bar::title_bar(i_window& aWindow, i_layout& aLayout, const std::string& aTitle) :
        widget{ aLayout },
        iWindow{ aWindow },
        iLayout{ *this },
        iIcon{ iLayout, service<i_app>().default_window_icon() },
        iTitle{ iLayout, aTitle },
        iSpacer{ iLayout },
        iMinimizeButton{ iLayout, push_button_style::TitleBar },
        iMaximizeButton{ iLayout, push_button_style::TitleBar },
        iRestoreButton{ iLayout, push_button_style::TitleBar },
        iCloseButton{ iLayout, push_button_style::TitleBar }
    {
        init();
    }

    title_bar::title_bar(i_window& aWindow, i_layout& aLayout, const i_texture& aIcon, const std::string& aTitle) :
        widget{ aLayout },
        iWindow{ aWindow },
        iLayout{ *this },
        iIcon{ iLayout, aIcon },
        iTitle{ iLayout, aTitle },
        iSpacer{ iLayout },
        iMinimizeButton{ iLayout, push_button_style::TitleBar },
        iMaximizeButton{ iLayout, push_button_style::TitleBar },
        iRestoreButton{ iLayout, push_button_style::TitleBar },
        iCloseButton{ iLayout, push_button_style::TitleBar }
    {
        init();
    }

    title_bar::title_bar(i_window& aWindow, i_layout& aLayout, const i_image& aIcon, const std::string& aTitle) :
        widget{ aLayout },
        iWindow{ aWindow },
        iLayout{ *this },
        iIcon{ iLayout, aIcon },
        iTitle{ iLayout, aTitle },
        iSpacer{ iLayout },
        iMinimizeButton{ iLayout, push_button_style::TitleBar },
        iMaximizeButton{ iLayout, push_button_style::TitleBar },
        iRestoreButton{ iLayout, push_button_style::TitleBar },
        iCloseButton{ iLayout, push_button_style::TitleBar }
    {
        init();
    }

    const image_widget& title_bar::icon() const
    {
        return iIcon;
    }

    image_widget& title_bar::icon()
    {
        return iIcon;
    }

    const text_widget& title_bar::title() const
    {
        return iTitle;
    }

    text_widget& title_bar::title()
    {
        return iTitle;
    }

    neogfx::size_policy title_bar::size_policy() const
    {
        return neogfx::size_policy{ size_constraint::Expanding, size_constraint::Minimum };
    }

    widget_part title_bar::hit_test(const point&) const
    {
        return widget_part::NonClientTitleBar;
    }

    void title_bar::init()
    {
        set_margins(neogfx::margins{});
        iLayout.set_margins(neogfx::margins{ 4.0, 4.0, 4.0, 4.0 });
        iLayout.set_spacing(size{ 8.0 });
        icon().set_ignore_mouse_events(false);
        size iconSize{ 24.0_dip };
        if (icon().image().is_empty())
            icon().set_fixed_size(iconSize);
        else
            icon().set_fixed_size(iconSize.min(icon().image().extents()));
        iMinimizeButton.set_size_policy(neogfx::size_policy{ size_constraint::Minimum, size_constraint::Minimum });
        iMaximizeButton.set_size_policy(neogfx::size_policy{ size_constraint::Minimum, size_constraint::Minimum });
        iRestoreButton.set_size_policy(neogfx::size_policy{ size_constraint::Minimum, size_constraint::Minimum });
        iCloseButton.set_size_policy(neogfx::size_policy{ size_constraint::Minimum, size_constraint::Minimum });
        iSink += service<i_surface_manager>().dpi_changed([this](i_surface&)
        {
            size iconSize{ 24.0_dip };
            if (icon().image().is_empty())
                icon().set_fixed_size(iconSize);
            else
                icon().set_fixed_size(iconSize.min(icon().image().extents()));
            update_textures();
            managing_layout().layout_items(true);
            update(true);
        });
        iSink += service<i_app>().current_style_changed([this](style_aspect aAspect) 
        { 
            if ((aAspect & style_aspect::Color) == style_aspect::Color) update_textures(); 
        });
        auto update_widgets = [this]()
        {
            bool isEnabled = iWindow.window_enabled();
            bool isActive = iWindow.is_active();
            bool isIconic = iWindow.is_iconic();
            bool isMaximized = iWindow.is_maximized();
            bool isRestored = iWindow.is_restored();
            icon().enable(isActive);
            title().enable(isActive);
            iMinimizeButton.enable(!isIconic && isEnabled);
            iMaximizeButton.enable(!isMaximized && isEnabled);
            iRestoreButton.enable(!isRestored && isEnabled);
            iCloseButton.enable(iWindow.can_close());
            bool layoutChanged = false;
            layoutChanged = iMinimizeButton.show(!isIconic && (iWindow.style() & window_style::MinimizeBox) == window_style::MinimizeBox) || layoutChanged;
            layoutChanged = iMaximizeButton.show(!isMaximized && (iWindow.style() & window_style::MaximizeBox) == window_style::MaximizeBox) || layoutChanged;
            layoutChanged = iRestoreButton.show(!isRestored && (iWindow.style() & (window_style::MinimizeBox | window_style::MaximizeBox)) != window_style::Invalid) || layoutChanged;
            layoutChanged = iCloseButton.show((iWindow.style() & window_style::Close) != window_style::Invalid) || layoutChanged;
            if (layoutChanged)
            {
                managing_layout().layout_items(true);
                update(true);
            }
        };
        iSink += service<i_app>().execution_started([this, update_widgets]()
        {
            update_widgets();
        });
        iSink += iWindow.window_event([this, update_widgets](neogfx::window_event& e)
        {
            switch (e.type())
            {
            case window_event_type::Enabled:
            case window_event_type::Disabled:
            case window_event_type::FocusGained:
            case window_event_type::FocusLost:
            case window_event_type::Iconized:
            case window_event_type::Maximized:
            case window_event_type::Restored:
                update_widgets();
                break;
            default:
                break;
            }
        });
        update_textures();
        update_widgets();
    }

    void title_bar::update_textures()
    {
        auto ink = service<i_app>().current_style().palette().text_color();
        auto paper = background_color();
        const char* sMinimizeTexturePattern
        {
            "[10,10]"
            "{0,paper}"
            "{1,ink}"
            "{2,ink_with_alpha}"

            "0000000000"
            "0000000000"
            "0000000000"
            "0000000000"
            "0000000000"
            "0000000000"
            "0000000000"
            "0000000000"
            "1111111111"
            "1111111111"
        };
        const char* sMaximizeTexturePattern
        {
            "[10,10]"
            "{0,paper}"
            "{1,ink}"
            "{2,ink_with_alpha}"

            "1111111111"
            "1111111111"
            "1000000001"
            "1000000001"
            "1000000001"
            "1000000001"
            "1000000001"
            "1000000001"
            "1000000001"
            "1111111111"
        };
        const char* sRestoreTexturePattern
        {
            "[10,10]"
            "{0,paper}"
            "{1,ink}"
            "{2,ink_with_alpha}"

            "0001111111"
            "0001111111"
            "0001000001"
            "1111111001"
            "1111111001"
            "1000001001"
            "1000001111"
            "1000001000"
            "1000001000"
            "1111111000"
        };
        const char* sCloseTexturePattern
        {
            "[10,10]"
            "{0,paper}"
            "{1,ink}"
            "{2,ink_with_alpha}"

            "1200000021"
            "2120000212"
            "0212002120"
            "0021221200"
            "0002112000"
            "0002112000"
            "0021221200"
            "0212002120"
            "2120000212"
            "1200000021"
        };
        const char* sMinimizeHighDpiTexturePattern
        {
            "[20,20]"
            "{0,paper}"
            "{1,ink}"
            "{2,ink_with_alpha}"

            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "00000000000000000000"
            "11111111111111111111"
            "11111111111111111111"
            "11111111111111111111"
            "11111111111111111111"
        };
        const char* sMaximizeHighDpiTexturePattern
        {
            "[20,20]"
            "{0,paper}"
            "{1,ink}"
            "{2,ink_with_alpha}"

            "11111111111111111111"
            "11111111111111111111"
            "11111111111111111111"
            "11111111111111111111"
            "11000000000000000011"
            "11000000000000000011"
            "11000000000000000011"
            "11000000000000000011"
            "11000000000000000011"
            "11000000000000000011"
            "11000000000000000011"
            "11000000000000000011"
            "11000000000000000011"
            "11000000000000000011"
            "11000000000000000011"
            "11000000000000000011"
            "11000000000000000011"
            "11000000000000000011"
            "11111111111111111111"
            "11111111111111111111"
        };
        const char* sRestoreHighDpiTexturePattern
        {
            "[20,20]"
            "{0,paper}"
            "{1,ink}"
            "{2,ink_with_alpha}"

            "00000011111111111111"
            "00000011111111111111"
            "00000011111111111111"
            "00000011111111111111"
            "00000011000000000011"
            "00000011000000000011"
            "11111111111111000011"
            "11111111111111000011"
            "11111111111111000011"
            "11111111111111000011"
            "11000000000011000011"
            "11000000000011000011"
            "11000000000011111111"
            "11000000000011111111"
            "11000000000011000000"
            "11000000000011000000"
            "11000000000011000000"
            "11000000000011000000"
            "11111111111111000000"
            "11111111111111000000"
        };
        const char* sCloseHighDpiTexturePattern
        {
            "[20,20]"
            "{0,paper}"
            "{1,ink}"
            "{2,ink_with_alpha}"

            "11200000000000000211"
            "11120000000000002111"
            "21112000000000021112"
            "02111200000000211120"
            "00211120000002111200"
            "00021112000021112000"
            "00002111200211120000"
            "00000211122111200000"
            "00000021111112000000"
            "00000002111120000000"
            "00000002111120000000"
            "00000021111112000000"
            "00000211122111200000"
            "00002111200211120000"
            "00021112000021112000"
            "00211120000002111200"
            "02111200000000211120"
            "21112000000000021112"
            "11120000000000002111"
            "11200000000000000211"
        };
        if (iTextures[TextureMinimize] == std::nullopt || iTextures[TextureMinimize]->first != ink)
        {
            iTextures[TextureMinimize].emplace(
                ink,
                !high_dpi() ? 
                    neogfx::image{
                        "neogfx::title_bar::iTextures[TextureMinimize]::" + ink.to_string(),
                        sMinimizeTexturePattern, { { "paper", color{} },{ "ink", ink }, { "ink_with_alpha", ink.with_alpha(0x80) } } } :
                    neogfx::image{
                        "neogfx::title_bar::iTextures[HighDpiTextureMinimize]::" + ink.to_string(),
                        sMinimizeHighDpiTexturePattern, { { "paper", color{} },{ "ink", ink }, { "ink_with_alpha", ink.with_alpha(0x80) } }, 2.0 });
        }
        if (iTextures[TextureMaximize] == std::nullopt || iTextures[TextureMaximize]->first != ink)
        {
            iTextures[TextureMaximize].emplace(
                ink,
                !high_dpi() ?
                    neogfx::image{
                        "neogfx::title_bar::iTextures[TextureMaximize]::" + ink.to_string(),
                        sMaximizeTexturePattern,{ { "paper", color{} },{ "ink", ink }, { "ink_with_alpha", ink.with_alpha(0x80) } } } :
                    neogfx::image{
                        "neogfx::title_bar::iTextures[HighDpiTextureMaximize]::" + ink.to_string(),
                        sMaximizeHighDpiTexturePattern,{ { "paper", color{} },{ "ink", ink }, { "ink_with_alpha", ink.with_alpha(0x80) } }, 2.0 });
        }
        if (iTextures[TextureRestore] == std::nullopt || iTextures[TextureRestore]->first != ink)
        {
            iTextures[TextureRestore].emplace(
                ink,
                !high_dpi() ?
                    neogfx::image{
                        "neogfx::title_bar::iTextures[TextureRestore]::" + ink.to_string(),
                        sRestoreTexturePattern,{ { "paper", color{} },{ "ink", ink }, { "ink_with_alpha", ink.with_alpha(0x80) } } } :
                    neogfx::image{
                        "neogfx::title_bar::iTextures[HighDpiTextureRestore]::" + ink.to_string(),
                        sRestoreHighDpiTexturePattern,{ { "paper", color{} },{ "ink", ink }, { "ink_with_alpha", ink.with_alpha(0x80) } }, 2.0 });
        }
        if (iTextures[TextureClose] == std::nullopt || iTextures[TextureClose]->first != ink)
        {
            iTextures[TextureClose].emplace(
                ink,
                !high_dpi() ?
                    neogfx::image{
                        "neogfx::title_bar::iTextures[TextureClose]::" + ink.to_string(),
                        sCloseTexturePattern, { { "paper", color{} },{ "ink", ink },{ "ink_with_alpha", ink.with_alpha(0x80) } } } :
                    neogfx::image{
                        "neogfx::title_bar::iTextures[HighDpiTextureClose]::" + ink.to_string(),
                        sCloseHighDpiTexturePattern, { { "paper", color{} }, { "ink", ink }, { "ink_with_alpha", ink.with_alpha(0x80) } }, 2.0 });
        }
        iMinimizeButton.set_image(iTextures[TextureMinimize]->second);
        iMaximizeButton.set_image(iTextures[TextureMaximize]->second);
        iRestoreButton.set_image(iTextures[TextureRestore]->second);
        iCloseButton.set_image(iTextures[TextureClose]->second);
    }
}