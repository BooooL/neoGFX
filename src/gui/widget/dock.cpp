// dock.cpp
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
#include <neogfx/gui/widget/dock.hpp>
#include <neogfx/gui/widget/i_dockable.hpp>
#include <neogfx/gui/layout/horizontal_layout.hpp>
#include <neogfx/gui/layout/vertical_layout.hpp>

namespace neogfx
{
    dock::dock(i_widget& aParent, dock_area aArea, const optional_size& aInitialSize) :
        base_type{ neogfx::decoration_style::Dock | neogfx::decoration_style::Resizable, aParent, (aArea & dock_area::Vertical) != dock_area::None ? splitter_type::Vertical : splitter_type::Horizontal }, iArea { aArea  }
    {
        if (aInitialSize)
            parent_layout().set_fixed_size(aInitialSize);
        parent_layout().set_weight(size{ 1.0 });
        set_padding(neogfx::padding{ 1.5_mm });
        update_layout();
    }

    dock::dock(i_layout& aLayout, dock_area aArea, const optional_size& aInitialSize) :
        base_type{ neogfx::decoration_style::Dock | neogfx::decoration_style::Resizable, aLayout, (aArea & dock_area::Vertical) != dock_area::None ? splitter_type::Vertical : splitter_type::Horizontal }, iArea{ aArea }
    {
        if (aInitialSize)
            parent_layout().set_fixed_size(aInitialSize);
        parent_layout().set_weight(size{ 1.0 });
        set_padding(neogfx::padding{ 1.5_mm });
        update_layout();
    }

    dock_area dock::area() const
    {
        return iArea;
    }

    void dock::set_area(dock_area aArea)
    {
        if (iArea != aArea)
        {
            iArea = aArea;
            update_layout();
        }
    }

    const dock::item_list& dock::items() const
    {
        return iItems;
    }

    void dock::add(const abstract_item& aItem)
    {
        iItems.push_back(aItem);
        try
        {
            layout().add(aItem->as_widget());
        }
        catch (...)
        {
            iItems.pop_back();
            throw;
        }
    }

    void dock::remove(const abstract_item& aItem)
    {
        auto existing = std::find(iItems.begin(), iItems.end(), aItem);
        if (existing == iItems.end())
            throw item_not_found();
        layout().remove(aItem->as_widget());
        iItems.erase(existing);
    }

    void dock::update_layout()
    {
        layout().set_padding(neogfx::padding{});
        layout().set_spacing(padding().top_left().to_vec2());
    }

    neogfx::size_policy dock::size_policy() const
    {
        if (has_size_policy() || !has_weight())
            return base_type::size_policy();
        return size_constraint::Expanding;
    }

    bool dock::part_active(widget_part aPart) const
    {
        switch (aPart.part)
        {
        case widget_part::BorderLeft:
            return area() == dock_area::Right;
        case widget_part::BorderTop:
            return area() == dock_area::Bottom;
        case widget_part::BorderRight:
            return area() == dock_area::Left;
        case widget_part::BorderBottom:
            return area() == dock_area::Top;
        case widget_part::Border:
        case widget_part::BorderTopLeft:
        case widget_part::BorderTopRight:
        case widget_part::BorderBottomRight:
        case widget_part::BorderBottomLeft:
            return false;
        }
        return base_type::part_active(aPart);
    }

    bool dock::transparent_background() const
    {
        return false;
    }

    color dock::palette_color(color_role aColorRole) const
    {
        if (has_palette_color(aColorRole))
            return widget::palette_color(aColorRole);
        if (aColorRole == color_role::Background)
                return container_background_color();
        return widget::palette_color(aColorRole);
    }

    bool dock::show(bool aVisible)
    {
        bool result = widget::show(aVisible);
        // todo: multiple docks in the same layout?
        if (!visible())
        {
            parent_layout().parent_layout().clear_weightings(true);
            parent_layout().disable();
            parent_layout().parent_layout().fix_weightings();
        }
        else
        {
            parent_layout().parent_layout().clear_weightings(true);
            parent_layout().enable();
            parent_layout().parent_layout().fix_weightings();
        }
        return result;
    }
}