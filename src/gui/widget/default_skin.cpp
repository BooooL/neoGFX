// default_skin.cpp
/*
  neogfx C++ App/Game Engine
  Copyright (c) 2020 Leigh Johnston.  All Rights Reserved.
  
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
#include <neogfx/app/i_app.hpp>
#include <neogfx/gui/widget/i_widget.hpp>
#include <neogfx/gui/widget/default_skin.hpp>

namespace neogfx
{
    const std::string& default_skin::name() const
    {
        static const std::string sName = "Default";
        return sName;
    }

    bool default_skin::has_fallback() const
    {
        return false;
    }

    i_skin& default_skin::fallback()
    {
        throw no_fallback();
    }

    void default_skin::activate()
    {
    }

    void default_skin::deactivate()
    {
    }

    size default_skin::preferred_size(skin_element aElement, const optional_size& aDesiredSize) const
    {
        switch (aElement)
        {
        case skin_element::CheckBox:
            return (aDesiredSize == std::nullopt ? size{ 3.5_mm, 3.5_mm } : size{ 3.5_mm, 3.5_mm }.max(*aDesiredSize)).ceil();
        case skin_element::RadioButton:
            return (aDesiredSize == std::nullopt ? size{ 3.5_mm, 3.5_mm } : size{ 3.5_mm, 3.5_mm }.max(*aDesiredSize)).ceil();
        default:
            return {};
        }
    }

    void default_skin::draw_scrollbar(i_graphics_context& aGc, const i_skinnable_item& aItem, const i_scrollbar& aScrollbar) const
    {
        auto const& widget = aItem.as_widget();
        scoped_units su(aGc, units::Pixels);
        rect const g = aItem.element_rect(skin_element::Scrollbar);
        auto const width = aScrollbar.width();
        point oldOrigin = aGc.origin();
        aGc.set_origin(point(0.0, 0.0));
        color baseColor = aScrollbar.container().scrollbar_color(aScrollbar);
        color backgroundColor = baseColor.shade(0x40);
        if (widget.transparent_background())
            backgroundColor = widget.container_background_color();
        color foregroundColor = baseColor.shade(0x80);
        if (aScrollbar.style() == scrollbar_style::Normal)
            aGc.fill_rect(g, backgroundColor);
        else if (aScrollbar.style() == scrollbar_style::Menu)
        {
            auto g1 = g;
            auto g2 = g;
            if (aScrollbar.type() == scrollbar_type::Vertical)
            {
                g1.cy = width;
                g2.y = g2.y + g2.cy - width;
                g2.cy = g1.cy;
            }
            else
            {
                g1.cx = width;
                g2.x = g2.x + g2.cx - width;
                g2.cx = g1.cx;
            }
            aGc.fill_rect(g1, backgroundColor);
            aGc.fill_rect(g2, backgroundColor);
        }
        else if (aScrollbar.style() == scrollbar_style::Scroller)
        {
            auto g1 = g;
            auto g2 = g;
            if (aScrollbar.type() == scrollbar_type::Vertical)
            {
                g1.y = g1.y + g1.cy - width * 2.0;
                g1.cy = width;
                g2.y = g2.y + g2.cy - width;
                g2.cy = g1.cy;
            }
            else
            {
                g1.x = g1.x + g1.cx - width * 2.0;
                g1.cx = width;
                g2.x = g2.x + g2.cx - width;
                g2.cx = g1.cx;
            }
            aGc.fill_rect(g1, backgroundColor);
            aGc.fill_rect(g2, backgroundColor);
            if (aScrollbar.position() != aScrollbar.minimum())
            {
                auto fadeRect = g;
                if (aScrollbar.type() == scrollbar_type::Vertical)
                    fadeRect.cy = width;
                else
                    fadeRect.cx = width;
                aGc.fill_rect(fadeRect, gradient
                    {
                        gradient::color_stop_list{ { 0.0, backgroundColor }, { 1.0, backgroundColor } },
                        gradient::alpha_stop_list{ { 0.0, 0xFF_u8 }, { 1.0, 0x00_u8 } },
                        aScrollbar.type() == scrollbar_type::Vertical ? gradient_direction::Vertical : gradient_direction::Horizontal
                    });
            }
            if (aScrollbar.position() != aScrollbar.maximum() - aScrollbar.page())
            {
                auto fadeRect = g;
                if (aScrollbar.type() == scrollbar_type::Vertical)
                {
                    fadeRect.y = fadeRect.y + fadeRect.cy - width * 3.0;
                    fadeRect.cy = width;
                }
                else
                {
                    fadeRect.x = fadeRect.x + fadeRect.cx - width * 3.0;
                    fadeRect.cx = width;
                }
                aGc.fill_rect(fadeRect, gradient
                    {
                        gradient::color_stop_list{ { 0.0, backgroundColor }, { 1.0, backgroundColor } },
                        gradient::alpha_stop_list{ { 0.0, 0x00_u8 },{ 1.0, 0xFF_u8 } },
                        aScrollbar.type() == scrollbar_type::Vertical ? gradient_direction::Vertical : gradient_direction::Horizontal
                    });
            }
        }

        const dimension margin = 3.0_dip;
        rect rectUpButton = aItem.element_rect(skin_element::ScrollbarUpArrow).deflate(margin, margin);
        rect rectDownButton = aItem.element_rect(skin_element::ScrollbarDownArrow).deflate(margin, margin);
        if (aScrollbar.type() == scrollbar_type::Vertical)
        {
            coordinate x = std::floor(rectUpButton.center().x);
            coordinate w = 1.0;
            for (coordinate y = 0.0; y < rectUpButton.height(); ++y)
            {
                aGc.fill_rect(rect(point(x, std::floor(y + rectUpButton.top())), size(w, 1.0)), baseColor.light() ?
                    foregroundColor.darker(aScrollbar.position() == aScrollbar.minimum() ? 0x00 : aScrollbar.clicked_element()== scrollbar_element::UpButton ? 0x60 : aScrollbar.hovering_element() == scrollbar_element::UpButton ? 0x30 : 0x00) :
                    foregroundColor.lighter(aScrollbar.position() == aScrollbar.minimum() ? 0x00 : aScrollbar.clicked_element() == scrollbar_element::UpButton ? 0x60 : aScrollbar.hovering_element() == scrollbar_element::UpButton ? 0x30 : 0x00));
                aGc.fill_rect(rect(point(x, std::floor(rectDownButton.bottom() - y)), size(w, 1.0)), baseColor.light() ?
                    foregroundColor.darker(aScrollbar.position() == aScrollbar.maximum() - aScrollbar.page() ? 0x00 : aScrollbar.clicked_element() == scrollbar_element::DownButton ? 0x60 : aScrollbar.hovering_element() == scrollbar_element::DownButton ? 0x30 : 0x00) :
                    foregroundColor.lighter(aScrollbar.position() == aScrollbar.maximum() - aScrollbar.page() ? 0x00 : aScrollbar.clicked_element() == scrollbar_element::DownButton ? 0x60 : aScrollbar.hovering_element() == scrollbar_element::DownButton ? 0x30 : 0x00));
                x -= 1.0;
                w += 2.0;
            }
        }
        else
        {
            coordinate y = std::floor(rectUpButton.center().y);
            coordinate h = 1.0;
            for (coordinate x = 0.0; x < rectUpButton.width(); ++x)
            {
                aGc.fill_rect(rect(point(std::floor(x + rectUpButton.left()), y), size(1.0, h)), baseColor.light() ?
                    foregroundColor.darker(aScrollbar.position() == aScrollbar.minimum() ? 0x00 : aScrollbar.clicked_element() == scrollbar_element::UpButton ? 0x60 : aScrollbar.hovering_element() == scrollbar_element::UpButton ? 0x30 : 0x00) :
                    foregroundColor.lighter(aScrollbar.position() == aScrollbar.minimum() ? 0x00 : aScrollbar.clicked_element() == scrollbar_element::UpButton ? 0x60 : aScrollbar.hovering_element() == scrollbar_element::UpButton ? 0x30 : 0x00));
                aGc.fill_rect(rect(point(std::floor(rectDownButton.right() - x), y), size(1.0, h)), baseColor.light() ?
                    foregroundColor.darker(aScrollbar.position() == aScrollbar.maximum() - aScrollbar.page() ? 0x00 : aScrollbar.clicked_element() == scrollbar_element::DownButton ? 0x60 : aScrollbar.hovering_element() == scrollbar_element::DownButton ? 0x30 : 0x00) :
                    foregroundColor.lighter(aScrollbar.position() == aScrollbar.maximum() - aScrollbar.page() ? 0x00 : aScrollbar.clicked_element() == scrollbar_element::DownButton ? 0x60 : aScrollbar.hovering_element() == scrollbar_element::DownButton ? 0x30 : 0x00));
                y -= 1.0;
                h += 2.0;
            }
        }
        if (aScrollbar.style() == scrollbar_style::Normal)
            aGc.fill_rect(aItem.element_rect(skin_element::ScrollbarThumb).deflate(aScrollbar.type() == scrollbar_type::Vertical ? margin : 0.0, aScrollbar.type() == scrollbar_type::Vertical ? 0.0 : margin), baseColor.light() ?
                foregroundColor.darker(aScrollbar.clicked_element() == scrollbar_element::Thumb ? 0x60 : aScrollbar.hovering_element() == scrollbar_element::Thumb ? 0x30 : 0x00) :
                foregroundColor.lighter(aScrollbar.clicked_element() == scrollbar_element::Thumb ? 0x60 : aScrollbar.hovering_element() == scrollbar_element::Thumb ? 0x30 : 0x00));
        aGc.set_origin(oldOrigin);
    }

    void default_skin::draw_check_box(i_graphics_context& aGc, const i_skinnable_item& aItem, const button_checked_state& aCheckedState) const
    {
        auto const& widget = aItem.as_widget();
        scoped_units su{ widget, units::Pixels };
        rect boxRect = aItem.element_rect(skin_element::CheckBox);
        auto enabledAlphaCoefficient = widget.effectively_enabled() ? 1.0 : 0.25;
        color hoverColor = service<i_app>().current_style().palette().color(color_role::Hover).same_lightness_as(
            widget.background_color().shade(0x20));
        if (widget.capturing() && widget.capture_position() && aItem.element_rect(skin_element::ClickableArea).contains(*widget.capture_position()))
            widget.background_color().dark() ? hoverColor.lighten(0x20) : hoverColor.darken(0x20);
        color fillColor = widget.enabled() && aItem.element_rect(skin_element::ClickableArea).contains(widget.capture_position() ? *widget.capture_position() : widget.root().mouse_position() - widget.origin()) ?
            hoverColor : widget.background_color();
        aGc.fill_rect(boxRect, fillColor.with_combined_alpha(enabledAlphaCoefficient));
        color borderColor1 = widget.container_background_color().mid(widget.container_background_color().mid(widget.background_color()));
        if (borderColor1.similar_intensity(widget.container_background_color(), 0.03125))
            borderColor1.dark() ? borderColor1.lighten(0x40) : borderColor1.darken(0x40);
        aGc.draw_rect(boxRect, pen{ borderColor1.with_combined_alpha(enabledAlphaCoefficient), 1.0 });
        boxRect.deflate(1.0, 1.0);
        aGc.draw_rect(boxRect, pen{ borderColor1.mid(widget.background_color()).with_combined_alpha(enabledAlphaCoefficient), 1.0 });
        boxRect.deflate(2.0, 2.0);
        if (aCheckedState != std::nullopt && *aCheckedState == true)
        {
            scoped_snap_to_pixel snap{ aGc, false };
            /* todo: draw tick image eye candy */
            aGc.draw_line(boxRect.top_left(), boxRect.bottom_right(), pen(service<i_app>().current_style().palette().color(color_role::PrimaryAccent).with_combined_alpha(enabledAlphaCoefficient), 2.0));
            aGc.draw_line(boxRect.bottom_left(), boxRect.top_right(), pen(service<i_app>().current_style().palette().color(color_role::PrimaryAccent).with_combined_alpha(enabledAlphaCoefficient), 2.0));
        }
        else if (aCheckedState == std::nullopt)
            aGc.fill_rect(boxRect, service<i_app>().current_style().palette().color(color_role::PrimaryAccent).with_combined_alpha(enabledAlphaCoefficient));
    }

    void default_skin::draw_radio_button(i_graphics_context& aGc, const i_skinnable_item& aItem, const button_checked_state& aCheckedState) const
    {
        auto const& widget = aItem.as_widget();
        scoped_units su{ widget, units::Pixels };
        rect discRect = aItem.element_rect(skin_element::RadioButton);
        auto enabledAlphaCoefficient = widget.effectively_enabled() ? 1.0 : 0.25;
        color borderColor1 = widget.container_background_color().mid(widget.container_background_color().mid(widget.background_color()));
        if (borderColor1.similar_intensity(widget.container_background_color(), 0.03125))
            borderColor1.dark() ? borderColor1.lighten(0x40) : borderColor1.darken(0x40);
        size const scaledPixel{ 1.0_dip, 1.0_dip };
        discRect.deflate(scaledPixel.cx, scaledPixel.cy);
        aGc.draw_circle(discRect.center(), discRect.width() / 2.0, pen{ borderColor1.with_combined_alpha(enabledAlphaCoefficient), scaledPixel.cx });
        discRect.deflate(scaledPixel.cx, scaledPixel.cy);
        aGc.draw_circle(discRect.center(), discRect.width() / 2.0, pen{ borderColor1.mid(widget.background_color()).with_combined_alpha(enabledAlphaCoefficient), scaledPixel.cx });
        discRect.deflate(scaledPixel.cx, scaledPixel.cy);
        color hoverColor = service<i_app>().current_style().palette().color(color_role::Hover).same_lightness_as(
            widget.background_color().dark() ?
                widget.background_color().lighter(0x20) :
                widget.background_color().darker(0x20));
        if (widget.capturing())
            widget.background_color().dark() ? hoverColor.lighten(0x20) : hoverColor.darken(0x20);
        color backgroundFillColor = widget.effectively_enabled() && aItem.element_rect(skin_element::ClickableArea).contains(widget.root().mouse_position() - widget.origin()) ? 
            hoverColor : widget.background_color();
        aGc.fill_circle(discRect.center(), discRect.width() / 2.0, backgroundFillColor.with_combined_alpha(enabledAlphaCoefficient));
        discRect.deflate(scaledPixel.cx * 2.0, scaledPixel.cy * 2.0);
        if (aCheckedState != std::nullopt && *aCheckedState == true)
            aGc.fill_circle(discRect.center(), discRect.width() / 2.0, service<i_app>().current_style().palette().color(color_role::PrimaryAccent).with_combined_alpha(enabledAlphaCoefficient));
    }

    void default_skin::draw_tree_expander(i_graphics_context& aGc, const i_skinnable_item& aItem, bool aExpandedState) const
    {
        auto const expanderRect = aItem.element_rect(skin_element::TreeExpander);
        auto const expanderColor = aItem.is_widget() && aItem.as_widget().has_foreground_color() ? aItem.as_widget().foreground_color() : service<i_app>().current_style().palette().color(color_role::Text);
        thread_local neogfx::game::mesh mesh{ {}, neogfx::vertices_2d{ 3, neogfx::vec2{} }, neogfx::game::default_faces(3) };
        auto const d1 = 3.0_dip;
        auto const d2 = 1.0_dip;
        if (!aExpandedState)
            mesh.vertices = { vec3{ -d1 + d2, -d1 - d2 }, vec3{ d1 - d2, 0.0 }, vec3{ -d1 + d2, d1 + d2 } };
        else
            mesh.vertices = { vec3{ -d1, d1 }, vec3{ d1, -d1 }, vec3{ d1, d1 } };
        if (!aExpandedState)
            aGc.draw_shape(mesh, expanderRect.center().to_vec3(), expanderColor);
        else
            aGc.fill_shape(mesh, expanderRect.center().to_vec3(), expanderColor);
    }
}