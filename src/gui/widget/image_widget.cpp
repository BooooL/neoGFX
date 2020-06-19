// image_widget.cpp
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
#include <neogfx/gui/widget/image_widget.hpp>


namespace neogfx
{
    image_widget::image_widget(const i_texture& aTexture, aspect_ratio aAspectRatio, cardinal aPlacement) :
        iTexture{ aTexture }, iAspectRatio{ aAspectRatio }, iPlacement{ aPlacement }, iSnap{ 1.0 }, iDpiAutoScale{ false }
    {
        set_padding(neogfx::padding(0.0));
        set_ignore_mouse_events(true);
    }

    image_widget::image_widget(const i_image& aImage, aspect_ratio aAspectRatio, cardinal aPlacement) :
        iTexture{ aImage }, iAspectRatio{ aAspectRatio }, iPlacement{ aPlacement }, iSnap{ 1.0 }, iDpiAutoScale{ false }
    {
        set_padding(neogfx::padding(0.0));
        set_ignore_mouse_events(true);
    }

    image_widget::image_widget(i_widget& aParent, const i_texture& aTexture, aspect_ratio aAspectRatio, cardinal aPlacement) :
        widget{ aParent }, iTexture{ aTexture }, iAspectRatio{ aAspectRatio }, iPlacement{ aPlacement }, iSnap{ 1.0 }, iDpiAutoScale{ false }
    {
        set_padding(neogfx::padding(0.0));
        set_ignore_mouse_events(true);
    }

    image_widget::image_widget(i_widget& aParent, const i_image& aImage, aspect_ratio aAspectRatio, cardinal aPlacement) :
        widget{ aParent }, iTexture{ aImage }, iAspectRatio{ aAspectRatio }, iPlacement{ aPlacement }, iSnap{ 1.0 }, iDpiAutoScale{ false }
    {
        set_padding(neogfx::padding(0.0));
        set_ignore_mouse_events(true);
    }

    image_widget::image_widget(i_layout& aLayout, const i_texture& aTexture, aspect_ratio aAspectRatio, cardinal aPlacement) :
        widget{ aLayout }, iTexture{ aTexture }, iAspectRatio{ aAspectRatio }, iPlacement{ aPlacement }, iSnap{ 1.0 }, iDpiAutoScale{ false }
    {
        set_padding(neogfx::padding(0.0));
        set_ignore_mouse_events(true);
    }

    image_widget::image_widget(i_layout& aLayout, const i_image& aImage, aspect_ratio aAspectRatio, cardinal aPlacement) :
        widget{ aLayout }, iTexture{ aImage }, iAspectRatio{ aAspectRatio }, iPlacement{ aPlacement }, iSnap{ 1.0 }, iDpiAutoScale{ false }
    {
        set_padding(neogfx::padding(0.0));
        set_ignore_mouse_events(true);
    }

    neogfx::size_policy image_widget::size_policy() const
    {
        if (widget::has_size_policy())
            return widget::size_policy();
        return size_constraint::Minimum;
    }

    size image_widget::minimum_size(const optional_size& aAvailableSpace) const
    {
        if (has_minimum_size() || iTexture.is_empty())
            return widget::minimum_size(aAvailableSpace);
        size result = iTexture.extents();
        if (iDpiAutoScale)
            result *= (dpi_scale_factor() / iTexture.dpi_scale_factor());
        return to_units(*this, scoped_units::current_units(), result);
    }

    void image_widget::paint(i_graphics_context& aGc) const
    {
        if (iTexture.is_empty())
            return;
        scoped_units su{ *this, units::Pixels };
        auto imageExtents = iTexture.extents();
        if (iDpiAutoScale)
            imageExtents *= (dpi_scale_factor() / iTexture.dpi_scale_factor());
        rect placementRect{ point{}, imageExtents };
        if (iAspectRatio == aspect_ratio::Stretch)
        {
            placementRect.cx = client_rect().width();
            placementRect.cy = client_rect().height();
        }
        else if (placementRect.width() >= placementRect.height())
        {
            switch (iAspectRatio)
            {
            case aspect_ratio::Ignore:
                if (placementRect.width() > client_rect().width())
                    placementRect.cx = client_rect().width();
                if (placementRect.height() > client_rect().height())
                    placementRect.cy = client_rect().height();
                break;
            case aspect_ratio::Keep:
                if (placementRect.width() > client_rect().width())
                {
                    placementRect.cx = client_rect().width();
                    placementRect.cy = placementRect.cx * imageExtents.cy / imageExtents.cx;
                }
                if (placementRect.height() > client_rect().height())
                {
                    placementRect.cy = client_rect().height();
                    placementRect.cx = placementRect.cy * imageExtents.cx / imageExtents.cy;
                }
                break;
            case aspect_ratio::KeepExpanding:
                if (placementRect.height() != client_rect().height())
                {
                    placementRect.cy = client_rect().height();
                    placementRect.cx = placementRect.cy * imageExtents.cx / imageExtents.cy;
                }
                break;
            }
        }
        else
        {
            switch (iAspectRatio)
            {
            case aspect_ratio::Ignore:
                if (placementRect.width() > client_rect().width())
                    placementRect.cx = client_rect().width();
                if (placementRect.height() > client_rect().height())
                    placementRect.cy = client_rect().height();
                break;
            case aspect_ratio::Keep:
                if (placementRect.height() > client_rect().height())
                {
                    placementRect.cy = client_rect().height();
                    placementRect.cx = placementRect.cy * imageExtents.cx / imageExtents.cy;
                }
                if (placementRect.width() > client_rect().width())
                {
                    placementRect.cx = client_rect().width();
                    placementRect.cy = placementRect.cx * imageExtents.cy / imageExtents.cx;
                }
                break;
            case aspect_ratio::KeepExpanding:
                if (placementRect.width() != client_rect().width())
                {
                    placementRect.cx = client_rect().width();
                    placementRect.cy = placementRect.cx * imageExtents.cy / imageExtents.cx;
                }
                break;
            }
        }
        // "Snap" feature: after layout to avoid unsightly gaps between widgets some widgets will be 1 pixel larger than others with same content so a snap of 2.0 pixels will fix this (for e.g. see spin_box.cpp)...
        if (iSnap != 1.0) 
        {
            double f, i;
            f = std::fmod(placementRect.cx, iSnap);
            std::modf(placementRect.cx, &i);
            placementRect.cx = i + ((f < iSnap / 2.0) ? 0.0 : iSnap);
            f = std::fmod(placementRect.cy, iSnap);
            std::modf(placementRect.cy, &i);
            placementRect.cy = i + ((f < iSnap / 2.0) ? 0.0 : iSnap);
        }
        switch (iPlacement)
        {
        case cardinal::NorthWest:
            placementRect.position() = point{};
            break;
        case cardinal::North:
            placementRect.position() = point{ (client_rect().width() - placementRect.cx) / 2.0, 0.0 };
            break;
        case cardinal::NorthEast:
            placementRect.position() = point{ client_rect().width() - placementRect.width(), 0.0 };
            break;
        case cardinal::West:
            placementRect.position() = point{ 0.0, (client_rect().height() - placementRect.cy) / 2.0 };
            break;
        case cardinal::Center:
            placementRect.position() = point{ (client_rect().width() - placementRect.cx) / 2.0, (client_rect().height() - placementRect.cy) / 2.0 };
            break;
        case cardinal::East:
            placementRect.position() = point{ client_rect().width() - placementRect.width(), (client_rect().height() - placementRect.cy) / 2.0 };
            break;
        case cardinal::SouthWest:
            placementRect.position() = point{ 0.0, client_rect().height() - placementRect.height() };
            break;
        case cardinal::South:
            placementRect.position() = point{ (client_rect().width() - placementRect.cx) / 2.0, client_rect().height() - placementRect.height() };
            break;
        case cardinal::SouthEast:
            placementRect.position() = point{ client_rect().width() - placementRect.width(), client_rect().height() - placementRect.height() };
            break;
        }
        aGc.draw_texture(placementRect, iTexture, effectively_disabled() ? color(0xFF, 0xFF, 0xFF, 0x80) : iColor, effectively_disabled() ? shader_effect::Monochrome : iColor ? shader_effect::Colorize : shader_effect::None);
    }

    const texture& image_widget::image() const
    {
        return iTexture;
    }

    void image_widget::set_image(const std::string& aImageUri)
    {
        set_image(neogfx::image{ aImageUri });
    }

    void image_widget::set_image(const i_image& aImage)
    {
        set_image(texture{ aImage });
    }

    void image_widget::set_image(const i_texture& aTexture)
    {
        size oldSize = minimum_size();
        size oldTextureSize = image().extents();
        iTexture = aTexture;
        ImageChanged.trigger();
        if (oldSize != minimum_size() || oldTextureSize != image().extents())
        {
            ImageGeometryChanged.trigger();
            if (has_parent_layout() && (visible() || parent_layout().ignore_visibility()))
            {
                parent_layout().invalidate();
                if (has_layout_manager())
                    layout_manager().layout_items(true);
            }
        }
        update();
    }

    void image_widget::set_image_color(const optional_color& aImageColor)
    {
        iColor = aImageColor;
    }

    void image_widget::set_aspect_ratio(aspect_ratio aAspectRatio)
    {
        if (iAspectRatio != aAspectRatio)
        {
            iAspectRatio = aAspectRatio;
            update();
        }
    }

    void image_widget::set_placement(cardinal aPlacement)
    {
        if (iPlacement != aPlacement)
        {
            iPlacement = aPlacement;
            update();
        }
    }

    void image_widget::set_snap(dimension aSnap)
    {
        if (iSnap != aSnap)
        {
            iSnap = aSnap;
            update();
        }
    }

    void image_widget::set_dpi_auto_scale(bool aDpiAutoScale)
    {
        if (iDpiAutoScale != aDpiAutoScale)
        {
            iDpiAutoScale = aDpiAutoScale;
            update();
        }
    }
}