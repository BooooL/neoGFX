// radio_button.hpp
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
#include "button.hpp"

namespace neogfx
{
    class radio_button : public button
    {
    public:
        define_event(On, on)
        define_event(Off, off)
    private:
        class disc : public widget
        {
        public:
            disc(radio_button& aParent);
        public:
            size minimum_size(const optional_size& aAvailableSpace = optional_size()) const override;
            size maximum_size(const optional_size& aAvailableSpace = optional_size()) const override;
        public:
            void paint(i_graphics_context& aGraphicsContext) const override;
        };
    public:
        radio_button(const std::string& aText = std::string());
        radio_button(i_widget& aParent, const std::string& aText = std::string());
        radio_button(i_layout& aLayout, const std::string& aText = std::string());
    public:
        bool is_on() const;
        bool is_off() const;
        void set_on();
    public:
        neogfx::size_policy size_policy() const override;
    protected:
        void paint(i_graphics_context& aGraphicsContext) const override;
    protected:
        void mouse_entered(const point& aPosition) override;
        void mouse_left() override;
    protected:
        void handle_clicked() override;
    protected:
        bool can_toggle() const override;
        bool set_checked_state(const std::optional<bool>& aCheckedState) override;
    protected:
        virtual const radio_button* next_radio_button() const;
        virtual radio_button* next_radio_button();
        virtual bool any_siblings_on() const;
    private:
        disc iDisc;
    };
}