// scrollable_widget.cpp
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
#include <neolib/core/scoped.hpp>
#include <neogfx/gui/layout/i_layout.hpp>
#include <neogfx/gui/widget/scrollable_widget.hpp>

namespace neogfx
{
    scrollable_widget::scrollable_widget(frame_style aFrameStyle, scrollbar_style aScrollbarStyle) :
        framed_widget{ aFrameStyle },
        iVerticalScrollbar{ *this, scrollbar_type::Vertical, aScrollbarStyle },
        iHorizontalScrollbar{ *this, scrollbar_type::Horizontal, aScrollbarStyle },
        iIgnoreScrollbarUpdates{ 0 }
    {
        if (has_surface())
            init_scrollbars();
    }
    
    scrollable_widget::scrollable_widget(i_widget& aParent, frame_style aFrameStyle, scrollbar_style aScrollbarStyle) :
        framed_widget{ aParent, aFrameStyle },
        iVerticalScrollbar{ *this, scrollbar_type::Vertical, aScrollbarStyle },
        iHorizontalScrollbar{ *this, scrollbar_type::Horizontal, aScrollbarStyle },
        iIgnoreScrollbarUpdates{ 0 }
    {
        if (has_surface())
            init_scrollbars();
    }
    
    scrollable_widget::scrollable_widget(i_layout& aLayout, frame_style aFrameStyle, scrollbar_style aScrollbarStyle) :
        framed_widget{ aLayout, aFrameStyle },
        iVerticalScrollbar{ *this, scrollbar_type::Vertical, aScrollbarStyle },
        iHorizontalScrollbar{ *this, scrollbar_type::Horizontal, aScrollbarStyle },
        iIgnoreScrollbarUpdates{ 0 }
    {
        if (has_surface())
            init_scrollbars();
    }
    
    scrollable_widget::~scrollable_widget()
    {
    }

    void scrollable_widget::scroll_to(i_widget& aChild)
    {
        (void)aChild;
        /* todo */
    }

    void scrollable_widget::layout_items_started()
    {
        framed_widget::layout_items_started();
    }

    void scrollable_widget::layout_items_completed()
    {
        framed_widget::layout_items_completed();
        if (!layout_items_in_progress() && !iIgnoreScrollbarUpdates)
            update_scrollbar_visibility();
    }

    void scrollable_widget::resized()
    {
        framed_widget::resized();
        if (!layout_items_in_progress() && !iIgnoreScrollbarUpdates)
            update_scrollbar_visibility();
    }

    rect scrollable_widget::client_rect(bool aIncludePadding) const
    {
        rect result = framed_widget::client_rect(aIncludePadding);
        if (vertical_scrollbar().visible())
        {
            if (vertical_scrollbar().style() == scrollbar_style::Normal)
                result.cx -= vertical_scrollbar().width();
            else if (vertical_scrollbar().style() == scrollbar_style::Menu)
            {
                result.y += vertical_scrollbar().width();
                result.cy -= vertical_scrollbar().width() * 2.0;
            }
            else if (vertical_scrollbar().style() == scrollbar_style::Scroller)
                result.cy -= vertical_scrollbar().width() * 2.0;
        }
        if (horizontal_scrollbar().visible())
        {
            if (horizontal_scrollbar().style() == scrollbar_style::Normal)
                result.cy -= horizontal_scrollbar().width();
            else if (vertical_scrollbar().style() == scrollbar_style::Menu)
            {
                result.x += horizontal_scrollbar().width();
                result.cx -= horizontal_scrollbar().width() * 2.0;
            }
            else if (vertical_scrollbar().style() == scrollbar_style::Scroller)
                result.cx -= horizontal_scrollbar().width() * 2.0;
        }
        return result;
    }

    widget_part scrollable_widget::hit_test(const point& aPosition) const
    {
        if (vertical_scrollbar().visible() && vertical_scrollbar().element_at(aPosition + origin()) != scrollbar_element::None)
            return widget_part::VerticalScrollbar;
        else if (horizontal_scrollbar().visible() && horizontal_scrollbar().element_at(aPosition + origin()) != scrollbar_element::None)
            return widget_part::HorizontalScrollbar;
        else
            return widget::hit_test(aPosition);
    }

    void scrollable_widget::paint_non_client_after(i_graphics_context& aGc) const
    {
        framed_widget::paint_non_client_after(aGc);
        if (vertical_scrollbar().visible())
            vertical_scrollbar().render(aGc);
        if (horizontal_scrollbar().visible())
            horizontal_scrollbar().render(aGc);
        if (vertical_scrollbar().visible() && horizontal_scrollbar().visible() && vertical_scrollbar().style() == horizontal_scrollbar().style() && vertical_scrollbar().style() == scrollbar_style::Normal)
        {
            point oldOrigin = aGc.origin();
            aGc.set_origin(point(0.0, 0.0));
            auto scrollbarColor = background_color();
            aGc.fill_rect(
                rect{
                    point{ scrollbar_geometry(horizontal_scrollbar()).right(), scrollbar_geometry(iVerticalScrollbar).bottom() },
                    size{ scrollbar_geometry(iVerticalScrollbar).width(), scrollbar_geometry(horizontal_scrollbar()).height() } },
                scrollbarColor.shade(0x40));
            aGc.set_origin(oldOrigin);
        }
    }

    void scrollable_widget::mouse_wheel_scrolled(mouse_wheel aWheel, const point& aPosition, delta aDelta, key_modifiers_e aKeyModifiers)
    {
        bool handledVertical = false;
        bool handledHorizontal = false;
        mouse_wheel verticalSense = mouse_wheel::Vertical;
        mouse_wheel horizontalSense = mouse_wheel::Horizontal;
        if (service<i_keyboard>().is_key_pressed(ScanCode_LSHIFT) || service<i_keyboard>().is_key_pressed(ScanCode_RSHIFT))
            std::swap(verticalSense, horizontalSense);
        if ((aWheel & verticalSense) != mouse_wheel::None && vertical_scrollbar().visible())
            handledVertical = vertical_scrollbar().set_position(vertical_scrollbar().position() + (((verticalSense == mouse_wheel::Vertical ? aDelta.dy : aDelta.dx) >= 0.0 ? -1.0 : 1.0) * vertical_scrollbar().step()));
        if ((aWheel & horizontalSense) != mouse_wheel::None && horizontal_scrollbar().visible())
            handledHorizontal = horizontal_scrollbar().set_position(horizontal_scrollbar().position() + (((horizontalSense == mouse_wheel::Horizontal ? aDelta.dx : aDelta.dy) >= 0.0 ? -1.0 : 1.0) * horizontal_scrollbar().step()));
        mouse_wheel passOn = static_cast<mouse_wheel>(
            aWheel & ((handledVertical ? ~verticalSense : verticalSense) | (handledHorizontal ? ~horizontalSense : horizontalSense)));
        if (passOn != mouse_wheel::None)
            framed_widget::mouse_wheel_scrolled(passOn, aPosition, aDelta, aKeyModifiers);
    }

    void scrollable_widget::mouse_button_pressed(mouse_button aButton, const point& aPosition, key_modifiers_e aKeyModifiers)
    {
        if (aButton == mouse_button::Middle)
        {
            bool handled = false;
            if (vertical_scrollbar().visible())
            {
                vertical_scrollbar().track();
                handled = true;
            }
            if (horizontal_scrollbar().visible())
            {
                horizontal_scrollbar().track();
                handled = true;
            }
            if (handled)
                set_capture();
            else
                framed_widget::mouse_button_pressed(aButton, aPosition, aKeyModifiers);
        }
        else
        {
            framed_widget::mouse_button_pressed(aButton, aPosition, aKeyModifiers);
            if (aButton == mouse_button::Left)
            {
                if (vertical_scrollbar().clicked_element() == scrollbar_element::None && horizontal_scrollbar().clicked_element() == scrollbar_element::None)
                {
                    if (vertical_scrollbar().visible() && vertical_scrollbar().element_at(aPosition + origin()) != scrollbar_element::None)
                    {
                        update(true);
                        vertical_scrollbar().click_element(vertical_scrollbar().element_at(aPosition + origin()));
                    }
                    else if (horizontal_scrollbar().visible() && horizontal_scrollbar().element_at(aPosition + origin()) != scrollbar_element::None)
                    {
                        update(true);
                        horizontal_scrollbar().click_element(horizontal_scrollbar().element_at(aPosition + origin()));
                    }
                }
            }
        }
    }

    void scrollable_widget::mouse_button_double_clicked(mouse_button aButton, const point& aPosition, key_modifiers_e aKeyModifiers)
    {
        framed_widget::mouse_button_double_clicked(aButton, aPosition, aKeyModifiers);
        if (aButton == mouse_button::Left)
        {
            if (vertical_scrollbar().clicked_element() == scrollbar_element::None && horizontal_scrollbar().clicked_element() == scrollbar_element::None)
            {
                if (vertical_scrollbar().visible() && vertical_scrollbar().element_at(aPosition + origin()) != scrollbar_element::None)
                {
                    update(true);
                    vertical_scrollbar().click_element(vertical_scrollbar().element_at(aPosition + origin()));
                }
                else if (horizontal_scrollbar().visible() && horizontal_scrollbar().element_at(aPosition + origin()) != scrollbar_element::None)
                {
                    update(true);
                    horizontal_scrollbar().click_element(horizontal_scrollbar().element_at(aPosition + origin()));
                }
            }
        }
    }

    void scrollable_widget::mouse_button_released(mouse_button aButton, const point& aPosition)
    {
        framed_widget::mouse_button_released(aButton, aPosition);
        if (aButton == mouse_button::Left)
        {
            if (vertical_scrollbar().clicked_element() != scrollbar_element::None)
            {
                update(true);
                vertical_scrollbar().unclick_element();
            }
            else if (horizontal_scrollbar().clicked_element() != scrollbar_element::None)
            {
                update(true);
                horizontal_scrollbar().unclick_element();
            }
        }
        else if (aButton == mouse_button::Middle)
        {
            vertical_scrollbar().untrack();
            horizontal_scrollbar().untrack();
        }
    }

    void scrollable_widget::mouse_moved(const point& aPosition, key_modifiers_e aKeyModifiers)
    {
        framed_widget::mouse_moved(aPosition, aKeyModifiers);
        vertical_scrollbar().update(aPosition + origin());
        horizontal_scrollbar().update(aPosition + origin());
    }

    void scrollable_widget::mouse_entered(const point& aPosition)
    {
        framed_widget::mouse_entered(aPosition);
        vertical_scrollbar().update();
        horizontal_scrollbar().update();
    }

    void scrollable_widget::mouse_left()
    {
        framed_widget::mouse_left();
        vertical_scrollbar().update();
        horizontal_scrollbar().update();
    }

    bool scrollable_widget::key_pressed(scan_code_e aScanCode, key_code_e aKeyCode, key_modifiers_e aKeyModifiers)
    {
        bool handled = true;
        switch (aScanCode)
        {
        case ScanCode_LEFT:
            horizontal_scrollbar().set_position(horizontal_scrollbar().position() - horizontal_scrollbar().step());
            break;
        case ScanCode_RIGHT:
            horizontal_scrollbar().set_position(horizontal_scrollbar().position() + horizontal_scrollbar().step());
            break;
        case ScanCode_UP:
            vertical_scrollbar().set_position(vertical_scrollbar().position() - vertical_scrollbar().step());
            break;
        case ScanCode_DOWN:
            vertical_scrollbar().set_position(vertical_scrollbar().position() + vertical_scrollbar().step());
            break;
        case ScanCode_PAGEUP:
            vertical_scrollbar().set_position(vertical_scrollbar().position() - vertical_scrollbar().page());
            break;
        case ScanCode_PAGEDOWN:
            vertical_scrollbar().set_position(vertical_scrollbar().position() + vertical_scrollbar().page());
            break;
        case ScanCode_HOME:
            if (horizontal_scrollbar().visible() && !(aKeyModifiers & KeyModifier_CTRL))
                horizontal_scrollbar().set_position(horizontal_scrollbar().minimum());
            else
                vertical_scrollbar().set_position(vertical_scrollbar().minimum());
            break;
        case ScanCode_END:
            if (horizontal_scrollbar().visible() && !(aKeyModifiers & KeyModifier_CTRL))
                horizontal_scrollbar().set_position(horizontal_scrollbar().maximum());
            else
                vertical_scrollbar().set_position(vertical_scrollbar().maximum());
            break;
        default:
            handled = framed_widget::key_pressed(aScanCode, aKeyCode, aKeyModifiers);
            break;
        }
        return handled;
    }

    const i_scrollbar& scrollable_widget::vertical_scrollbar() const
    {
        return iVerticalScrollbar;
    }

    i_scrollbar& scrollable_widget::vertical_scrollbar()
    {
        return iVerticalScrollbar;
    }

    const i_scrollbar& scrollable_widget::horizontal_scrollbar() const
    {
        return iHorizontalScrollbar;
    }

    i_scrollbar& scrollable_widget::horizontal_scrollbar()
    {
        return iHorizontalScrollbar;
    }

    void scrollable_widget::init_scrollbars()
    {
        vertical_scrollbar().set_step(std::ceil(1.0_cm));
        horizontal_scrollbar().set_step(std::ceil(1.0_cm));
    }

    scrolling_disposition scrollable_widget::scrolling_disposition() const
    {
        return neogfx::scrolling_disposition::ScrollChildWidgetVertically | neogfx::scrolling_disposition::ScrollChildWidgetHorizontally;
    }

    scrolling_disposition scrollable_widget::scrolling_disposition(const i_widget&) const
    {
        return neogfx::scrolling_disposition::ScrollChildWidgetVertically | neogfx::scrolling_disposition::ScrollChildWidgetHorizontally;
    }

    rect scrollable_widget::scrollbar_geometry(const i_scrollbar& aScrollbar) const
    {
        switch (aScrollbar.type())
        {
        case scrollbar_type::Vertical:
            if (vertical_scrollbar().style() == scrollbar_style::Normal)
                return rect{ non_client_rect().top_right() - point{ aScrollbar.width() + effective_frame_width(), -effective_frame_width() },
                        size{ aScrollbar.width(), non_client_rect().cy - (horizontal_scrollbar().visible() ? horizontal_scrollbar().width() : 0.0) - effective_frame_width() * 2.0} };
            else // scrollbar_style::Menu
                return non_client_rect().deflate(size{ effective_frame_width() });
        case scrollbar_type::Horizontal:
            if (horizontal_scrollbar().style() == scrollbar_style::Normal)
                return rect{ non_client_rect().bottom_left() - point{ -effective_frame_width(), aScrollbar.width() + effective_frame_width()},
                        size{ non_client_rect().cx - (vertical_scrollbar().visible() ? vertical_scrollbar().width() : 0.0) - effective_frame_width() * 2.0, aScrollbar.width() } };
            else // scrollbar_style::Menu
                return non_client_rect().deflate(size{ effective_frame_width() });
        default:
            return rect{};
        }
    }

    void scrollable_widget::scrollbar_updated(const i_scrollbar& aScrollbar, i_scrollbar::update_reason_e)
    {
        if (!iIgnoreScrollbarUpdates)
        {
            point scrollPosition = units_converter(*this).from_device_units(point(static_cast<coordinate>(horizontal_scrollbar().position()), static_cast<coordinate>(vertical_scrollbar().position())));
            if (iOldScrollPosition != scrollPosition)
            {
                for (auto& c : children())
                {
                    point delta = -(scrollPosition - iOldScrollPosition);
                    if (aScrollbar.type() == scrollbar_type::Horizontal || (scrolling_disposition(*c) & neogfx::scrolling_disposition::ScrollChildWidgetVertically) == neogfx::scrolling_disposition::DontScrollChildWidget)
                        delta.y = 0.0;
                    if (aScrollbar.type() == scrollbar_type::Vertical || (scrolling_disposition(*c) & neogfx::scrolling_disposition::ScrollChildWidgetHorizontally) == neogfx::scrolling_disposition::DontScrollChildWidget)
                        delta.x = 0.0;
                    c->move(c->position() + delta);
                }
                if (aScrollbar.type() == scrollbar_type::Vertical)
                {
                    iOldScrollPosition.y = scrollPosition.y;
                }
                else if (aScrollbar.type() == scrollbar_type::Horizontal)
                {
                    iOldScrollPosition.x = scrollPosition.x;
                }
            }
        }
        update(true);
    }

    color scrollable_widget::scrollbar_color(const i_scrollbar&) const
    {
        return background_color();
    }

    const i_widget& scrollable_widget::as_widget() const
    {
        return *this;
    }

    i_widget& scrollable_widget::as_widget()
    {
        return *this;
    }

    void scrollable_widget::update_scrollbar_visibility()
    {
        if ((scrolling_disposition() & neogfx::scrolling_disposition::ScrollChildWidgetVertically) == neogfx::scrolling_disposition::ScrollChildWidgetVertically)
            vertical_scrollbar().lock(0.0);
        if ((scrolling_disposition() & neogfx::scrolling_disposition::ScrollChildWidgetHorizontally) == neogfx::scrolling_disposition::ScrollChildWidgetHorizontally)
            horizontal_scrollbar().lock(0.0);
        {
            neolib::scoped_counter<uint32_t> sc(iIgnoreScrollbarUpdates);
            update_scrollbar_visibility(UsvStageInit);
            if (client_rect().cx > vertical_scrollbar().width() &&
                client_rect().cy > horizontal_scrollbar().width())
            {
                update_scrollbar_visibility(UsvStageCheckVertical1);
                update_scrollbar_visibility(UsvStageCheckHorizontal);
                update_scrollbar_visibility(UsvStageCheckVertical2);
            }
            update_scrollbar_visibility(UsvStageDone);
        }
        if ((scrolling_disposition() & neogfx::scrolling_disposition::ScrollChildWidgetVertically) == neogfx::scrolling_disposition::ScrollChildWidgetVertically)
            vertical_scrollbar().unlock();
        if ((scrolling_disposition() & neogfx::scrolling_disposition::ScrollChildWidgetHorizontally) == neogfx::scrolling_disposition::ScrollChildWidgetHorizontally)
            horizontal_scrollbar().unlock();
    }

    void scrollable_widget::update_scrollbar_visibility(usv_stage_e aStage)
    {
        if ((scrolling_disposition() & neogfx::scrolling_disposition::DontConsiderChildWidgets) == neogfx::scrolling_disposition::DontConsiderChildWidgets)
            return;
        switch (aStage)
        {
        case UsvStageInit:
            if ((scrolling_disposition() & neogfx::scrolling_disposition::ScrollChildWidgetVertically) == neogfx::scrolling_disposition::ScrollChildWidgetVertically)
                vertical_scrollbar().hide();
            if ((scrolling_disposition() & neogfx::scrolling_disposition::ScrollChildWidgetHorizontally) == neogfx::scrolling_disposition::ScrollChildWidgetHorizontally)
                horizontal_scrollbar().hide();
            layout_items();
            break;
        case UsvStageCheckVertical1:
        case UsvStageCheckVertical2:
            if ((scrolling_disposition() & neogfx::scrolling_disposition::ScrollChildWidgetVertically) == neogfx::scrolling_disposition::ScrollChildWidgetVertically)
            {
                auto const& cr = client_rect();
                for (auto& child : children())
                {
                    auto const& childPos = child->position();
                    auto const& childExtents = child->extents();
                    if (child->hidden() || childExtents.cx == 0.0 || childExtents.cy == 0.0)
                        continue;
                    if ((scrolling_disposition(*child) & neogfx::scrolling_disposition::ScrollChildWidgetVertically) == neogfx::scrolling_disposition::DontScrollChildWidget)
                        continue;
                    if (childPos.y < cr.top() || childPos.y + childExtents.cy > cr.bottom())
                    {
                        vertical_scrollbar().show();
                        layout_items();
                        break;
                    }
                }
            }
            break;
        case UsvStageCheckHorizontal:
            if ((scrolling_disposition() & neogfx::scrolling_disposition::ScrollChildWidgetHorizontally) == neogfx::scrolling_disposition::ScrollChildWidgetHorizontally)
            {
                auto const& cr = client_rect();
                for (auto& child : children())
                {
                    auto const& childPos = child->position();
                    auto const& childExtents = child->extents();
                    if (child->hidden() || childExtents.cx == 0.0 || childExtents.cy == 0.0)
                        continue;
                    if ((scrolling_disposition(*child) & neogfx::scrolling_disposition::ScrollChildWidgetHorizontally) == neogfx::scrolling_disposition::DontScrollChildWidget)
                        continue;
                    if (childPos.x < cr.left() || childPos.x + childExtents.cx > cr.right())
                    {
                        horizontal_scrollbar().show();
                        layout_items();
                        break;
                    }
                }
            }
            break;
        case UsvStageDone:
            {
                point max;
                for (auto& c : children())
                {
                    if (c->hidden() || c->extents().cx == 0.0 || c->extents().cy == 0.0)
                        continue;
                    if ((scrolling_disposition(*c) & neogfx::scrolling_disposition::ScrollChildWidgetHorizontally) == neogfx::scrolling_disposition::ScrollChildWidgetHorizontally)
                        max.x = std::max(max.x, units_converter(*c).to_device_units(c->position().x + c->extents().cx));
                    if ((scrolling_disposition(*c) & neogfx::scrolling_disposition::ScrollChildWidgetVertically) == neogfx::scrolling_disposition::ScrollChildWidgetVertically)
                        max.y = std::max(max.y, units_converter(*c).to_device_units(c->position().y + c->extents().cy));
                }
                if (has_layout())
                {
                    if ((scrolling_disposition() & neogfx::scrolling_disposition::ScrollChildWidgetHorizontally) == neogfx::scrolling_disposition::ScrollChildWidgetHorizontally)
                        max.x += layout().padding().right;
                    if ((scrolling_disposition() & neogfx::scrolling_disposition::ScrollChildWidgetVertically) == neogfx::scrolling_disposition::ScrollChildWidgetVertically)
                        max.y += layout().padding().bottom;
                }
                if ((scrolling_disposition() & neogfx::scrolling_disposition::ScrollChildWidgetVertically) == neogfx::scrolling_disposition::ScrollChildWidgetVertically)
                {
                    vertical_scrollbar().set_minimum(0.0);
                    vertical_scrollbar().set_maximum(max.y);
                    vertical_scrollbar().set_page(units_converter(*this).to_device_units(client_rect()).height());
                }
                if ((scrolling_disposition() & neogfx::scrolling_disposition::ScrollChildWidgetHorizontally) == neogfx::scrolling_disposition::ScrollChildWidgetHorizontally)
                {
                    horizontal_scrollbar().set_minimum(0.0);
                    horizontal_scrollbar().set_maximum(max.x);
                    horizontal_scrollbar().set_page(units_converter(*this).to_device_units(client_rect()).width());
                }
            }
            break;
        default:
            break;
        }
    }
}