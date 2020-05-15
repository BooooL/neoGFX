// i_action.hpp
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
#include <optional>
#include <neogfx/core/event.hpp>
#include <neogfx/hid/keyboard.hpp>
#include <neogfx/gfx/i_texture.hpp>
#include <neogfx/gfx/i_image.hpp>

namespace neogfx
{
    class i_action
    {
    public:
        declare_event(triggered)
        declare_event(hovered)
        declare_event(enabled)
        declare_event(disabled)
        declare_event(checked)
        declare_event(unchecked)
        declare_event(changed)
    public:
        typedef std::optional<std::string> optional_text;
    public:
        virtual bool is_enabled() const = 0;
        virtual bool is_disabled() const = 0;
        virtual bool is_checkable() const = 0;
        virtual bool is_checked() const = 0;
        virtual bool is_unchecked() const = 0;
        virtual uint32_t group() const = 0;
        virtual bool is_separator() const = 0;
        virtual std::string text() const = 0;
        virtual std::string menu_text() const = 0;
        virtual std::string button_text() const = 0;
        virtual std::string tool_tip_text() const = 0;
        virtual std::string help_text() const = 0;
        virtual const i_texture& image() const = 0;
        virtual const i_texture& checked_image() const = 0;
        virtual const optional_key_sequence& shortcut() const = 0;
        virtual i_action& set_enabled(bool aEnabled) = 0;
        virtual i_action& set_checkable(bool aCheckable) = 0;
        virtual i_action& set_checked(bool aChecked) = 0;
        virtual i_action& set_group(uint32_t aGroup) = 0;
        virtual i_action& set_separator(bool aIsSeparator) = 0;
        virtual i_action& set_text(const optional_text& aText = optional_text()) = 0;
        virtual i_action& set_menu_text(const optional_text& aMenuText = optional_text()) = 0;
        virtual i_action& set_button_text(const optional_text& aButtonText = optional_text()) = 0;
        virtual i_action& set_tool_tip_text(const optional_text& aToolTipText = optional_text()) = 0;
        virtual i_action& set_help_text(const optional_text& aHelpText = optional_text()) = 0;
        virtual i_action& set_image(const std::string& aUri, dimension aDpiScaleFactor = 1.0, texture_sampling aSampling = texture_sampling::Scaled) = 0;
        virtual i_action& set_image(const i_image& aImage) = 0;
        virtual i_action& set_image(const i_texture& aTexture) = 0;
        virtual i_action& set_checked_image(const std::string& aUri, dimension aDpiScaleFactor = 1.0, texture_sampling aSampling = texture_sampling::Scaled) = 0;
        virtual i_action& set_checked_image(const i_image& aImage) = 0;
        virtual i_action& set_checked_image(const i_texture& aTexture) = 0;
        virtual i_action& set_shortcut(const optional_key_sequence& aShortcut) = 0;
        virtual i_action& set_shortcut(const std::string& aShortcut) = 0;
    public:
        i_action& enable(bool aEnabled = true)
        {
            return set_enabled(aEnabled);
        }
        i_action& disable(bool aDisabled = true)
        {
            return set_enabled(!aDisabled);
        }
        i_action& check(bool aChecked = true)
        {
            return set_checked(aChecked);
        }
        i_action& uncheck(bool aUnchecked = true)
        {
            return set_checked(!aUnchecked);
        }
        i_action& toggle()
        {
            if (is_checked())
                return uncheck();
            else
                return check();
        }
    };

    class i_action_container
    {
    public:
        virtual i_action& add_action(i_action& aAction) = 0;
        virtual i_action& add_action(std::shared_ptr<i_action> aAction) = 0;
    };
}