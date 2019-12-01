// check_box.cpp
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
#include <neogfx/gui/widget/check_box.hpp>

namespace neogfx
{
    check_box::box::box(check_box& aParent)
    {
        aParent.layout().add_at(0, *this);
        set_margins(neogfx::margins{});
        set_ignore_mouse_events(true);
    }

    size check_box::box::minimum_size(const optional_size& aAvailableSpace) const
    {
        if (has_minimum_size())
            return widget::minimum_size(aAvailableSpace);
        dimension const length = std::ceil(units_converter(*this).from_device_units(static_cast<const check_box&>(parent()).text_widget().font().height() * (2.0 / 3.0)));
        return rasterize(size{ std::max<dimension>(length, 3.5_mm) });
    }

    size check_box::box::maximum_size(const optional_size& aAvailableSpace) const
    {
        return minimum_size(aAvailableSpace);
    }
        
    void check_box::box::paint(i_graphics_context& aGraphicsContext) const
    {
        scoped_units su{ *this, units::Pixels };
        rect boxRect = client_rect();
        auto enabledAlphaCoefficient = effectively_enabled() ? 1.0 : 0.25;
        colour hoverColour = service<i_app>().current_style().palette().hover_colour().same_lightness_as(
            background_colour().dark() ?
                background_colour().lighter(0x20) :
                background_colour().darker(0x20));
        if (parent().capturing())
            background_colour().dark() ? hoverColour.lighten(0x20) : hoverColour.darken(0x20);
        colour fillColour = parent().enabled() && parent().client_rect().contains(root().mouse_position() - parent().origin()) ? hoverColour : background_colour();
        aGraphicsContext.fill_rect(boxRect, fillColour.with_combined_alpha(enabledAlphaCoefficient));
        colour borderColour1 = container_background_colour().mid(container_background_colour().mid(background_colour()));
        if (borderColour1.similar_intensity(container_background_colour(), 0.03125))
            borderColour1.dark() ? borderColour1.lighten(0x40) : borderColour1.darken(0x40);
        aGraphicsContext.draw_rect(boxRect, pen{ borderColour1.with_combined_alpha(enabledAlphaCoefficient), 1.0 });
        boxRect.deflate(1.0, 1.0);
        aGraphicsContext.draw_rect(boxRect, pen{ borderColour1.mid(background_colour()).with_combined_alpha(enabledAlphaCoefficient), 1.0 });
        boxRect.deflate(2.0, 2.0);
        if (static_cast<const check_box&>(parent()).is_checked())
        {
            /* todo: draw tick image eye candy */
            aGraphicsContext.draw_line(boxRect.top_left(), boxRect.bottom_right(), pen(service<i_app>().current_style().palette().widget_detail_primary_colour().with_combined_alpha(enabledAlphaCoefficient), 2.0));
            aGraphicsContext.draw_line(boxRect.bottom_left(), boxRect.top_right(), pen(service<i_app>().current_style().palette().widget_detail_primary_colour().with_combined_alpha(enabledAlphaCoefficient), 2.0));
        }
        else if (static_cast<const check_box&>(parent()).is_indeterminate())
        {
            aGraphicsContext.fill_rect(boxRect, service<i_app>().current_style().palette().widget_detail_primary_colour().with_combined_alpha(enabledAlphaCoefficient));
        }
    }

    check_box::check_box(const std::string& aText, button_checkable aCheckable) :
        button(aText), iBox(*this)
    {
        set_checkable(aCheckable);
        set_margins(neogfx::margins(0.0));
        layout().set_margins(neogfx::margins(0.0));
        layout().add_spacer();
        label().text_widget().set_alignment(alignment::Left | alignment::VCentre);
    }

    check_box::check_box(i_widget& aParent, const std::string& aText, button_checkable aCheckable) :
        button(aParent, aText), iBox(*this)
    {
        set_checkable(aCheckable);
        set_margins(neogfx::margins(0.0));
        layout().set_margins(neogfx::margins(0.0));
        layout().add_spacer();
        label().text_widget().set_alignment(alignment::Left | alignment::VCentre);
    }

    check_box::check_box(i_layout& aLayout, const std::string& aText, button_checkable aCheckable) :
        button(aLayout, aText), iBox(*this)
    {
        set_checkable(aCheckable);
        set_margins(neogfx::margins(0.0));
        layout().set_margins(neogfx::margins(0.0));
        layout().add_spacer();
        label().text_widget().set_alignment(alignment::Left | alignment::VCentre);
    }

    neogfx::size_policy check_box::size_policy() const
    {
        if (widget::has_size_policy())
            return widget::size_policy();
        return size_constraint::Minimum;
    }

    void check_box::paint(i_graphics_context& aGraphicsContext) const
    {
        if (has_focus())
        {
            rect focusRect = label().client_rect() + label().position();
            aGraphicsContext.draw_focus_rect(focusRect);
        }
    }

    void check_box::mouse_entered(const point& aPosition)
    {
        button::mouse_entered(aPosition);
        update();
    }

    void check_box::mouse_left()
    {
        button::mouse_left();
        update();
    }
}