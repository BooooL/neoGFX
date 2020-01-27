// slider.cpp
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
#include <neogfx/gui/widget/slider.hpp>
#include <neogfx/app/i_app.hpp>

namespace neogfx
{
    slider_impl::slider_impl(slider_orientation aOrientation) :
        iOrientation(aOrientation),
        iNormalizedValue{ 0.0 }
    {
        init();
    }

    slider_impl::slider_impl(i_widget& aParent, slider_orientation aOrientation) :
        widget(aParent),
        iOrientation(aOrientation),
        iNormalizedValue{ 0.0 }
    {
        init();
    }

    slider_impl::slider_impl(i_layout& aLayout, slider_orientation aOrientation) :
        widget(aLayout),
        iOrientation(aOrientation),
        iNormalizedValue{ 0.0 }
    {
        init();
    }

    slider_impl::~slider_impl()
    {
    }

    size slider_impl::minimum_size(const optional_size& aAvailableSpace) const
    {
        if (has_minimum_size())
            return widget::minimum_size(aAvailableSpace);
        return iOrientation == slider_orientation::Horizontal ? size{ 96_spx, 22_spx } : size{ 22_spx, 96_spx };
    }

    void slider_impl::paint(i_graphics_context& aGraphicsContext) const
    {
        scoped_units su{ *this, units::Pixels };
        rect rectBarBox = bar_box();
        color ink = background_color().light(0x80) ? background_color().darker(0x80) : background_color().lighter(0x80);
        aGraphicsContext.fill_rounded_rect(rectBarBox, 2.0, ink);
        rectBarBox.inflate(size{ 1.0, 1.0 });
        aGraphicsContext.fill_rounded_rect(rectBarBox, 2.0, ink.mid(background_color()));
        rectBarBox.deflate(size{ 1.0, 1.0 });
        point selection = normalized_value_to_position(normalized_value());
        rect selectionRect = rectBarBox;
        if (iOrientation == slider_orientation::Horizontal)
            selectionRect.cx = selection.x - selectionRect.x;
        else
        {
            selectionRect.cy = selectionRect.bottom() - selection.y;
            selectionRect.y = selection.y;
        }
        if (normalized_value() > 0.0)
            aGraphicsContext.fill_rounded_rect(selectionRect, 2.0, service<i_app>().current_style().palette().selection_color());
        rect rectIndicator = indicator_box();
        color indicatorColor = foreground_color();
        if (iDragOffset != std::nullopt)
        {
            if (indicatorColor.light(0x40))
                indicatorColor.darken(0x40);
            else
                indicatorColor.lighten(0x40);
        }
        color indicatorBorderColor = indicatorColor.darker(0x40);
        indicatorColor.lighten(0x40);
        aGraphicsContext.fill_circle(rectIndicator.centre(), rectIndicator.width() / 2.0, indicatorBorderColor);
        aGraphicsContext.fill_circle(rectIndicator.centre(), rectIndicator.width() / 2.0 - 1.0, indicatorColor);
    }

    void slider_impl::mouse_button_pressed(mouse_button aButton, const point& aPosition, key_modifiers_e aKeyModifiers)
    {
        widget::mouse_button_pressed(aButton, aPosition, aKeyModifiers);
        if (aButton == mouse_button::Left)
        {
            if (indicator_box().contains(aPosition))
            {
                iDragOffset = aPosition - indicator_box().centre();
                if (iOrientation == slider_orientation::Horizontal)
                    set_normalized_value(normalized_value_from_position(point{ aPosition.x - iDragOffset->x, aPosition.y }));
                else
                    set_normalized_value(normalized_value_from_position(point{ aPosition.x, aPosition.y - iDragOffset->y }));
            }
            else
                set_normalized_value(normalized_value_from_position(aPosition));
        }
    }

    void slider_impl::mouse_button_double_clicked(mouse_button aButton, const point& aPosition, key_modifiers_e aKeyModifiers)
    {
        widget::mouse_button_double_clicked(aButton, aPosition, aKeyModifiers);
        if (aButton == mouse_button::Left)
            set_normalized_value(0.5);
    }

    void slider_impl::mouse_button_released(mouse_button aButton, const point& aPosition)
    {
        widget::mouse_button_released(aButton, aPosition);
        if (aButton == mouse_button::Left)
        {
            iDragOffset = std::nullopt;
            update();
        }
    }

    void slider_impl::mouse_wheel_scrolled(mouse_wheel aWheel, delta aDelta)
    {
        if (aWheel == mouse_wheel::Vertical)
            set_normalized_value(std::max(0.0, std::min(1.0, normalized_value() + (aDelta.dy * normalized_step_value()))));
        else
            widget::mouse_wheel_scrolled(aWheel, aDelta);
    }

    void slider_impl::mouse_moved(const point& aPosition)
    {
        widget::mouse_moved(aPosition);
        if (iDragOffset != std::nullopt)
        {
            if (iOrientation == slider_orientation::Horizontal)
                set_normalized_value(normalized_value_from_position(point{ aPosition.x - iDragOffset->x, aPosition.y }));
            else
                set_normalized_value(normalized_value_from_position(point{ aPosition.x, aPosition.y - iDragOffset->y }));
        }
    }

    void slider_impl::set_normalized_value(double aValue)
    {
        aValue = std::max(0.0, std::min(1.0, aValue));
        if (iNormalizedValue != aValue)
        {
            iNormalizedValue = aValue;
            update();
        }
    }

    void slider_impl::init()
    {
        set_margins(neogfx::margins{});
        if (iOrientation == slider_orientation::Horizontal)
            set_size_policy(neogfx::size_policy{ size_constraint::Expanding, size_constraint::Minimum });
        else
            set_size_policy(neogfx::size_policy{ size_constraint::Minimum, size_constraint::Expanding });

        auto step_up = [this]()
        {
            set_normalized_value(std::max(0.0, std::min(1.0, normalized_value() + normalized_step_value())));
        };
        auto step_down = [this]()
        {
            set_normalized_value(std::max(0.0, std::min(1.0, normalized_value() - normalized_step_value())));
        };
    }

    rect slider_impl::bar_box() const
    {
        rect result = client_rect(false);
        result.deflate(size{ std::ceil((iOrientation == slider_orientation::Horizontal ? result.height() : result.width()) / 2.5) });
        result.deflate(size{ 1.0, 1.0 });
        return result;
    }

    rect slider_impl::indicator_box() const
    {
        rect result{ normalized_value_to_position(normalized_value()), size{} };
        result.inflate(size{ std::ceil((iOrientation == slider_orientation::Horizontal ? client_rect(false).height() : client_rect(false).width()) / 3.0) });
        return result;
    }

    double slider_impl::normalized_value_from_position(const point& aPosition) const
    {
        if (iOrientation == slider_orientation::Horizontal)
            return std::max(0.0, std::min(aPosition.x - bar_box().x, bar_box().right())) / bar_box().cx;
        else
            return 1.0 - std::max(0.0, std::min(aPosition.y - bar_box().y, bar_box().bottom())) / bar_box().cy;
    }

    point slider_impl::normalized_value_to_position(double aValue) const
    {
        const rect rectBarBox = bar_box();
        if (iOrientation == slider_orientation::Horizontal)
            return point{ rectBarBox.x + rectBarBox.cx * aValue, rectBarBox.centre().y };
        else
            return point{ rectBarBox.centre().x, rectBarBox.y + rectBarBox.cy * (1.0 - aValue) };
    }
}

