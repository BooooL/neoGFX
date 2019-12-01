﻿// push_button.cpp
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
#include <neogfx/gfx/graphics_context.hpp>
#include <neogfx/gfx/pen.hpp>
#include <neogfx/gui/widget/push_button.hpp>

namespace neogfx
{
    push_button::push_button(push_button_style aStyle) :
        button{ (aStyle == push_button_style::Normal || aStyle == push_button_style::ButtonBox || aStyle == push_button_style::SpinBox ? alignment::Centre : alignment::Left) | alignment::VCentre },
        iAnimator{ service<neolib::async_task>(), [this](neolib::callback_timer&) { animate(); }, 20, false },
        iAnimationFrame{ 0 },
        iStyle{ aStyle }
    {
        init();
    }

    push_button::push_button(const std::string& aText, push_button_style aStyle) :
        button{ aText, (aStyle == push_button_style::Normal || aStyle == push_button_style::ButtonBox || aStyle == push_button_style::SpinBox ? alignment::Centre : alignment::Left) | alignment::VCentre },
        iAnimator{ service<neolib::async_task>(), [this](neolib::callback_timer&) { animate(); }, 20, false },
        iAnimationFrame{ 0 },
        iStyle{ aStyle }
    {
        init();
    }

    push_button::push_button(const i_texture& aTexture, push_button_style aStyle) :
        button{ aTexture, (aStyle == push_button_style::Normal || aStyle == push_button_style::ButtonBox || aStyle == push_button_style::SpinBox ? alignment::Centre : alignment::Left) | alignment::VCentre },
        iAnimator{ service<neolib::async_task>(), [this](neolib::callback_timer&) { animate(); }, 20, false },
        iAnimationFrame{ 0 },
        iStyle{ aStyle }
    {
        init();
    }

    push_button::push_button(const i_image& aImage, push_button_style aStyle) :
        button{ aImage, (aStyle == push_button_style::Normal || aStyle == push_button_style::ButtonBox || aStyle == push_button_style::SpinBox ? alignment::Centre : alignment::Left) | alignment::VCentre },
        iAnimator{ service<neolib::async_task>(), [this](neolib::callback_timer&) { animate(); }, 20, false },
        iAnimationFrame{ 0 },
        iStyle{ aStyle }
    {
        init();
    }
    
    push_button::push_button(i_widget& aParent, push_button_style aStyle) :
        button{ aParent, (aStyle == push_button_style::Normal || aStyle == push_button_style::ButtonBox || aStyle == push_button_style::SpinBox ? alignment::Centre : alignment::Left) | alignment::VCentre },
        iAnimator{ service<neolib::async_task>(), [this](neolib::callback_timer&) { animate(); }, 20, false },
        iAnimationFrame{ 0 },
        iStyle{ aStyle }
    {
        init();
    }

    push_button::push_button(i_widget& aParent, const std::string& aText, push_button_style aStyle) :
        button{ aParent, aText, (aStyle == push_button_style::Normal || aStyle == push_button_style::ButtonBox || aStyle == push_button_style::SpinBox ? alignment::Centre : alignment::Left) | alignment::VCentre },
        iAnimator{ service<neolib::async_task>(), [this](neolib::callback_timer&) { animate(); }, 20, false },
        iAnimationFrame{ 0 },
        iStyle{ aStyle }
    {
        init();
    }

    push_button::push_button(i_widget& aParent, const i_texture& aTexture, push_button_style aStyle) :
        button{ aParent, aTexture, (aStyle == push_button_style::Normal || aStyle == push_button_style::ButtonBox || aStyle == push_button_style::SpinBox ? alignment::Centre : alignment::Left) | alignment::VCentre },
        iAnimator{ service<neolib::async_task>(), [this](neolib::callback_timer&) { animate(); }, 20, false },
        iAnimationFrame{ 0 },
        iStyle{ aStyle }
    {
        init();
    }

    push_button::push_button(i_widget& aParent, const i_image& aImage, push_button_style aStyle) :
        button{ aParent, aImage, (aStyle == push_button_style::Normal || aStyle == push_button_style::ButtonBox || aStyle == push_button_style::SpinBox ? alignment::Centre : alignment::Left) | alignment::VCentre },
        iAnimator{ service<neolib::async_task>(), [this](neolib::callback_timer&) { animate(); }, 20, false },
        iAnimationFrame{ 0 },
        iStyle{ aStyle }
    {
        init();
    }

    push_button::push_button(i_layout& aLayout, push_button_style aStyle) :
        button{ aLayout, (aStyle == push_button_style::Normal || aStyle == push_button_style::ButtonBox || aStyle == push_button_style::SpinBox ? alignment::Centre : alignment::Left) | alignment::VCentre },
        iAnimator{ service<neolib::async_task>(), [this](neolib::callback_timer&) { animate(); }, 20, false },
        iAnimationFrame{ 0 },
        iStyle{ aStyle }
    {
        init();
    }

    push_button::push_button(i_layout& aLayout, const std::string& aText, push_button_style aStyle) :
        button{ aLayout, aText, (aStyle == push_button_style::Normal || aStyle == push_button_style::ButtonBox || aStyle == push_button_style::SpinBox ? alignment::Centre : alignment::Left) | alignment::VCentre },
        iAnimator{ service<neolib::async_task>(), [this](neolib::callback_timer&) { animate(); }, 20, false },
        iAnimationFrame{ 0 },
        iStyle{ aStyle }
    {
        init();
    }

    push_button::push_button(i_layout& aLayout, const i_texture& aTexture, push_button_style aStyle) :
        button{ aLayout, aTexture, (aStyle == push_button_style::Normal || aStyle == push_button_style::ButtonBox || aStyle == push_button_style::SpinBox ? alignment::Centre : alignment::Left) | alignment::VCentre },
        iAnimator{ service<neolib::async_task>(), [this](neolib::callback_timer&) { animate(); }, 20, false },
        iAnimationFrame{ 0 },
        iStyle{ aStyle }
    {
        init();
    }

    push_button::push_button(i_layout& aLayout, const i_image& aImage, push_button_style aStyle) :
        button{ aLayout, aImage, (aStyle == push_button_style::Normal || aStyle == push_button_style::ButtonBox || aStyle == push_button_style::SpinBox ? alignment::Centre : alignment::Left) | alignment::VCentre },
        iAnimator{ service<neolib::async_task>(), [this](neolib::callback_timer&) { animate(); }, 20, false },
        iAnimationFrame{ 0 },
        iStyle{ aStyle }
    {
        init();
    }

    size push_button::minimum_size(const optional_size& aAvailableSpace) const
    {
        if (has_minimum_size())
            return button::minimum_size(aAvailableSpace);
        size result = button::minimum_size(aAvailableSpace);
        if (iStyle == push_button_style::ButtonBox)
        {
            if (iStandardButtonWidth == std::nullopt || iStandardButtonWidth->first != label().text_widget().font())
            {
                graphics_context gc{ *this, graphics_context::type::Unattached };
                iStandardButtonWidth.emplace(label().text_widget().font(), gc.text_extent("#StdButton", label().text_widget().font()));
                iStandardButtonWidth->second.cx += (result.cx - label().text_widget().minimum_size(aAvailableSpace).cx);
            }
            result.cx = std::max(result.cx, iStandardButtonWidth->second.cx);
        }
        return result;
    }

    size push_button::maximum_size(const optional_size& aAvailableSpace) const
    {
        if (has_maximum_size())
            return button::maximum_size(aAvailableSpace);
        if (iStyle == push_button_style::ButtonBox)
            return minimum_size(aAvailableSpace);
        return button::maximum_size(aAvailableSpace);
    }

    void push_button::paint_non_client(i_graphics_context& aGraphicsContext) const
    {
        button::paint_non_client(aGraphicsContext);
        if ((iStyle == push_button_style::Toolbar || iStyle == push_button_style::TitleBar) && enabled() && (entered() || capturing()))
        {
            colour background = (capturing() && entered() ? 
                service<i_app>().current_style().palette().selection_colour() : 
                background_colour().light() ? background_colour().darker(0x40) : background_colour().lighter(0x40));
            background.set_alpha(0x80);
            aGraphicsContext.fill_rect(client_rect(), background);
        }
    }

    void push_button::paint(i_graphics_context& aGraphicsContext) const
    {
        colour faceColour = animation_colour();
        colour borderColour = border_mid_colour().darker(0x40);
        colour innerBorderColour = border_mid_colour().lighter(capturing() ? 0x20 : 0x40);
        scoped_units su{ *this, units::Pixels };
        neogfx::path outline = path();
        dimension penWidth = 1.0;
        switch (iStyle)
        {
        case push_button_style::Normal:
        case push_button_style::ButtonBox:
        case push_button_style::Tab:
        case push_button_style::DropList:
        case push_button_style::SpinBox:
            outline.deflate(penWidth * 2.0, penWidth * 2.0);
            break;
        }
        aGraphicsContext.clip_to(outline);
        const double colourOffset = 0.2;
        colour colourStart = faceColour;
        colour colourEnd = colourStart.to_hsl().lighter(-colourOffset).to_rgb();
        if (colourStart.to_hsl().lightness() - colourEnd.to_hsl().lightness() < colourOffset)
            colourStart = colourEnd.with_lightness(colourEnd.to_hsl().lightness() + colourOffset);
        switch(iStyle)
        {
//        case push_button_style::Normal:
//        case push_button_style::ButtonBox:
//        case push_button_style::ItemViewHeader:
        case push_button_style::Toolbar:
        case push_button_style::TitleBar:
            if (!spot_colour())
            {
                colourStart = colourStart.with_lightness(colourStart.to_hsl().lightness() + 0.1);
                const double transitionHeight = 0.04;
                const double transitionStart = 0.5 - transitionHeight / 2.0 +
                    (!capturing() ? 0.0 : (rasterize(1.0_mm) / outline.bounding_rect().height()));
                const double transitionEnd = transitionStart + transitionHeight;
                aGraphicsContext.fill_rect(outline.bounding_rect(),
                    gradient{ gradient::colour_stop_list{
                        gradient::colour_stop{ 0.0, colourStart },
                        gradient::colour_stop{ transitionStart, colourEnd.to_hsl().lighter(colourOffset * 0.6).to_rgb() },
                        gradient::colour_stop{ transitionEnd, colourEnd.to_hsl().lighter(colourOffset * 0.2).to_rgb() },
                        gradient::colour_stop{ 1.0, colourEnd } } });
            }
            else
            {
                aGraphicsContext.fill_rect(outline.bounding_rect(), faceColour);
            }
            break;
        case push_button_style::Normal:
        case push_button_style::ButtonBox:
        case push_button_style::ItemViewHeader:
        case push_button_style::Tab:
        case push_button_style::DropList:
        case push_button_style::SpinBox:
            if (!spot_colour())
            {
                aGraphicsContext.fill_rect(outline.bounding_rect(), gradient{ colourStart, colourEnd });
            }
            else
            {
                aGraphicsContext.fill_rect(outline.bounding_rect(), faceColour);
            }
            break;
        }
        aGraphicsContext.reset_clip();
        if (has_focus())
        {
            rect focusRect = outline.bounding_rect();
            focusRect.deflate(2.0, 2.0);
            aGraphicsContext.draw_focus_rect(focusRect);
        }
        switch(iStyle)
        {
        case push_button_style::Normal:
        case push_button_style::ButtonBox:
        case push_button_style::Tab:
        case push_button_style::DropList:
        case push_button_style::SpinBox:
            outline.inflate(penWidth, penWidth);
            aGraphicsContext.draw_path(outline, pen(innerBorderColour, penWidth));
            outline.inflate(penWidth, penWidth);
            aGraphicsContext.draw_path(outline, pen(borderColour, penWidth));
            break;
        }
    }

    void push_button::mouse_entered(const point& aPosition)
    {
        button::mouse_entered(aPosition);
        if (perform_hover_animation() || !finished_animation())
            iAnimator.again_if();
        update();
    }

    void push_button::mouse_left()
    {
        button::mouse_left();
        if (perform_hover_animation() || !finished_animation())
            iAnimator.again_if();
        update();
    }

    push_button_style push_button::style() const
    {
        return iStyle;
    }

    rect push_button::path_bounding_rect() const
    {
        return client_rect();
    }

    path push_button::path() const
    {
        neogfx::path ret;
        size pixel = units_converter(*this).from_device_units(size(1.0, 1.0));
        size currentSize = path_bounding_rect().extents();
        switch (iStyle)
        {
        case push_button_style::Normal:
        case push_button_style::ButtonBox:
        case push_button_style::Tab:
        case push_button_style::DropList:
        case push_button_style::SpinBox:
            ret.move_to(pixel.cx, 0, 12);
            ret.line_to(currentSize.cx - pixel.cx, 0);
            ret.line_to(currentSize.cx - pixel.cx, pixel.cy);
            ret.line_to(currentSize.cx - 0, pixel.cy);
            ret.line_to(currentSize.cx - 0, currentSize.cy - pixel.cy);
            ret.line_to(currentSize.cx - pixel.cx, currentSize.cy - pixel.cy);
            ret.line_to(currentSize.cx - pixel.cx, currentSize.cy - 0);
            ret.line_to(pixel.cx, currentSize.cy - 0);
            ret.line_to(pixel.cx, currentSize.cy - pixel.cy);
            ret.line_to(0, currentSize.cy - pixel.cy);
            ret.line_to(0, pixel.cy);
            ret.line_to(pixel.cx, pixel.cy);
            ret.line_to(pixel.cx, 0);
            break;
        case push_button_style::ItemViewHeader:
            ret.move_to(0, 0, 4);
            ret.line_to(currentSize.cx, 0);
            ret.line_to(currentSize.cx, currentSize.cy);
            ret.line_to(0, currentSize.cy);
            ret.line_to(0, 0);
            break;
        }
        ret.set_position(path_bounding_rect().top_left());
        return ret;
    }

    bool push_button::spot_colour() const
    {
        return false;
    }

    colour push_button::border_mid_colour() const
    {
        return animation_colour().darker(0x20);
    }

    bool push_button::perform_hover_animation() const
    {
        return true;
    }

    bool push_button::has_hover_colour() const
    {
        return iHoverColour != std::nullopt;
    }

    colour push_button::hover_colour() const
    {
        colour hoverColour = (has_hover_colour() ? *iHoverColour : service<i_app>().current_style().palette().hover_colour());
        if (capturing())
            return hoverColour.light(0x40) ? hoverColour.darker(0x40) : hoverColour.lighter(0x40);
        else 
            return hoverColour;
    }

    void push_button::set_hover_colour(const optional_colour& aHoverColour)
    {
        iHoverColour = aHoverColour;
        update();
    }

    void push_button::animate()
    {
        if (!root().has_native_surface())
            return;

        if (entered() && enabled())
        {
            if (iAnimationFrame < kMaxAnimationFrame)
            {
                ++iAnimationFrame;
                iAnimator.again();
            }
        }
        else
        {
            if (iAnimationFrame > 0)
            {
                --iAnimationFrame;
                iAnimator.again();
            }
        }
        update();
    }

    bool push_button::finished_animation() const
    {
        return iAnimationFrame == 0;
    }

    colour push_button::animation_colour() const
    {
        return animation_colour(iAnimationFrame);
    }

    colour push_button::animation_colour(uint32_t aAnimationFrame) const
    {
        colour faceColour;
        faceColour = foreground_colour();
        if (capturing())
        {
            if (faceColour.light(0x40))
                faceColour.darken(0x40);
            else
                faceColour.lighten(0x40);
        }
        return (enabled() && entered() && perform_hover_animation()) || !finished_animation() ? gradient(faceColour, hover_colour()).at(static_cast<coordinate>(aAnimationFrame), 0, static_cast<coordinate>(kMaxAnimationFrame)) : faceColour;
    }

    void push_button::init()
    {
        if (iStyle == push_button_style::ItemViewHeader)
        {
            set_margins(neogfx::margins{ 1.0_spx, 2.0_spx });
            layout().set_margins(neogfx::margins{});
            label().set_margins(neogfx::margins{});
            label().text_widget().set_alignment(neogfx::alignment::Left | neogfx::alignment::VCentre);
        }
        else if (iStyle == push_button_style::Toolbar)
        {
            set_margins(neogfx::margins{ 2.0_spx, 2.0_spx });
            layout().set_margins(neogfx::margins{});
            label().set_margins(neogfx::margins{});
        }
    }
}

