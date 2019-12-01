// radio_button.cpp
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
#include <neogfx/gui/layout/spacer.hpp>
#include <neogfx/gui/widget/radio_button.hpp>

namespace neogfx
{
    radio_button::disc::disc(radio_button& aParent)
    {
        aParent.layout().add_at(0, *this);
        set_margins(neogfx::margins(0.0));
        set_ignore_mouse_events(true);
    }

    size radio_button::disc::minimum_size(const optional_size& aAvailableSpace) const
    {
        if (has_minimum_size())
            return widget::minimum_size(aAvailableSpace);
        dimension const length = std::ceil(units_converter(*this).from_device_units(static_cast<const radio_button&>(parent()).text_widget().font().height() * (2.0 / 3.0)));
        return rasterize(size{ std::max<dimension>(length, 3.5_mm) });
    }

    size radio_button::disc::maximum_size(const optional_size& aAvailableSpace) const
    {
        return minimum_size(aAvailableSpace);
    }
        
    void radio_button::disc::paint(i_graphics_context& aGraphicsContext) const
    {
        scoped_units su{ *this, units::Pixels };
        rect discRect = client_rect();
        auto enabledAlphaCoefficient = effectively_enabled() ? 1.0 : 0.25;
        colour borderColour1 = container_background_colour().mid(container_background_colour().mid(background_colour()));
        if (borderColour1.similar_intensity(container_background_colour(), 0.03125))
            borderColour1.dark() ? borderColour1.lighten(0x40) : borderColour1.darken(0x40);
        size const scaledPixel{ 1.0_spx, 1.0_spx };
        discRect.deflate(scaledPixel.cx, scaledPixel.cy);
        aGraphicsContext.draw_circle(discRect.centre(), discRect.width() / 2.0, pen{ borderColour1.with_combined_alpha(enabledAlphaCoefficient), scaledPixel.cx });
        discRect.deflate(scaledPixel.cx, scaledPixel.cy);
        aGraphicsContext.draw_circle(discRect.centre(), discRect.width() / 2.0, pen{ borderColour1.mid(background_colour()).with_combined_alpha(enabledAlphaCoefficient), scaledPixel.cx });
        discRect.deflate(scaledPixel.cx, scaledPixel.cy);
        colour hoverColour = service<i_app>().current_style().palette().hover_colour().same_lightness_as(
            background_colour().dark() ?
            background_colour().lighter(0x20) :
            background_colour().darker(0x20));
        if (parent().capturing())
            background_colour().dark() ? hoverColour.lighten(0x20) : hoverColour.darken(0x20);
        colour backgroundFillColour = effectively_enabled() && parent().client_rect().contains(root().mouse_position() - parent().origin()) ? hoverColour : background_colour();
        aGraphicsContext.fill_circle(discRect.centre(), discRect.width() / 2.0, backgroundFillColour.with_combined_alpha(enabledAlphaCoefficient));
        discRect.deflate(scaledPixel.cx * 2.0, scaledPixel.cy * 2.0);
        if (static_cast<const radio_button&>(parent()).is_on())
            aGraphicsContext.fill_circle(discRect.centre(), discRect.width() / 2.0, service<i_app>().current_style().palette().widget_detail_primary_colour().with_combined_alpha(enabledAlphaCoefficient));
    }

    radio_button::radio_button(const std::string& aText) :
        button(aText), iDisc(*this)
    {
        set_checkable(button_checkable::BiState);
        set_margins(neogfx::margins{ 0.0 });
        layout().set_margins(neogfx::margins{ 0.0 });
        layout().add_spacer();
        if (!any_siblings_on())
            set_on();
        text_widget().set_alignment(alignment::Left | alignment::VCentre);
    }

    radio_button::radio_button(i_widget& aParent, const std::string& aText) :
        button(aParent, aText), iDisc(*this)
    {
        set_checkable(button_checkable::BiState);
        set_margins(neogfx::margins{ 0.0 });
        layout().set_margins(neogfx::margins{ 0.0 });
        layout().add_spacer();
        if (!any_siblings_on())
            set_on();
        text_widget().set_alignment(alignment::Left | alignment::VCentre);
    }

    radio_button::radio_button(i_layout& aLayout, const std::string& aText) :
        button(aLayout, aText), iDisc(*this)
    {
        set_checkable(button_checkable::BiState);
        set_margins(neogfx::margins{ 0.0 });
        layout().set_margins(neogfx::margins{ 0.0 });
        layout().add_spacer();
        if (!any_siblings_on())
            set_on();
        text_widget().set_alignment(alignment::Left | alignment::VCentre);
    }

    bool radio_button::is_on() const
    {
        return is_checked();
    }

    bool radio_button::is_off() const
    {
        return !is_checked();
    }

    void radio_button::set_on()
    {
        check();
    }

    neogfx::size_policy radio_button::size_policy() const
    {
        if (widget::has_size_policy())
            return widget::size_policy();
        return size_constraint::Minimum;
    }

    void radio_button::paint(i_graphics_context& aGraphicsContext) const
    {
        if (has_focus())
        {
            rect focusRect = label().client_rect() + label().position();
            aGraphicsContext.draw_focus_rect(focusRect);
        }
    }

    void radio_button::mouse_entered(const point& aPosition)
    {
        button::mouse_entered(aPosition);
        update();
    }

    void radio_button::mouse_left()
    {
        button::mouse_left();
        update();
    }

    void radio_button::handle_clicked()
    {
        button::handle_clicked();
        set_on();
    }

    const radio_button* radio_button::next_radio_button() const
    {
        const i_widget* candidate = &after();
        while (candidate != this)
        {
            if (is_sibling_of(*candidate))
            {
                // Teh ghastly dynamic_cast! A simpler CLEAN solution which doesn't leak details everywhere doesn't immediately spring to mind.
                const radio_button* candidateRadioButton = dynamic_cast<const radio_button*>(candidate);
                if (candidateRadioButton != nullptr)
                    return candidateRadioButton;
            }
            candidate = &candidate->after();
        }
        return this;
    }

    radio_button* radio_button::next_radio_button()
    {
        return const_cast<radio_button*>(to_const(*this).next_radio_button());
    }

    bool radio_button::can_toggle() const
    {
        return false;
    }

    bool radio_button::set_checked_state(const std::optional<bool>& aCheckedState)
    {
        if (checked_state() == aCheckedState)
            return false;
        if (aCheckedState != std::nullopt && *aCheckedState)
            for (radio_button* nextRadioButton = next_radio_button(); nextRadioButton != this; nextRadioButton = nextRadioButton->next_radio_button())
                nextRadioButton->uncheck();
        button::set_checked_state(aCheckedState);
        if (is_on())
            On.trigger();
        else if (is_off())
            Off.trigger();
        return true;
    }

    bool radio_button::any_siblings_on() const
    {
        for (const radio_button* nextRadioButton = next_radio_button(); nextRadioButton != this; nextRadioButton = nextRadioButton->next_radio_button())
            if (nextRadioButton->is_on())
                return true;
        return false;
    }
}