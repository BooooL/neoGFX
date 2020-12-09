// widget.inl
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
#include <unordered_map>
#include <neolib/core/scoped.hpp>
#include <neogfx/app/i_app.hpp>
#include <neogfx/gfx/graphics_context.hpp>
#include <neogfx/gui/widget/widget.hpp>
#include <neogfx/gui/layout/i_layout.hpp>
#include <neogfx/gui/layout/i_layout_item_proxy.hpp>
#include <neogfx/hid/i_surface_manager.hpp>
#include <neogfx/hid/i_surface_window.hpp>

namespace neogfx
{
    template <typename Interface>
    class widget<Interface>::layout_timer : public pause_rendering, neolib::callback_timer
    {
    public:
        layout_timer(i_window& aWindow, i_async_task& aIoTask, std::function<void(callback_timer&)> aCallback) :
            pause_rendering{ aWindow }, neolib::callback_timer{ aIoTask, aCallback, 0 }
        {
        }
        ~layout_timer()
        {
        }
    };

    template <typename Interface>
    widget<Interface>::widget() :
        iSingular{ false },
        iParent{ nullptr },
        iAddingChild{ false },
        iLinkBefore{ nullptr },
        iLinkAfter{ nullptr },
        iParentLayout{ nullptr },
        iLayoutInProgress{ 0 },
        iLayer{ 0 }
    {
        base_type::Position.Changed([this](const point&) { moved(); });
        base_type::set_alive();
    }
    
    template <typename Interface>
    widget<Interface>::widget(i_widget& aParent) :
        iSingular{ false },
        iParent{ nullptr },
        iAddingChild{ false },
        iLinkBefore{ nullptr },
        iLinkAfter{ nullptr },
        iParentLayout{ nullptr },
        iLayoutInProgress{ 0 },
        iLayer{ 0 }
    {
        base_type::Position.Changed([this](const point&) { moved(); });
        aParent.add(*this);
        base_type::set_alive();
    }

    template <typename Interface>
    widget<Interface>::widget(i_layout& aLayout) :
        iSingular{ false },
        iParent{ nullptr },
        iAddingChild{ false },
        iLinkBefore{ nullptr },
        iLinkAfter{ nullptr },
        iParentLayout{ nullptr },
        iLayoutInProgress{ 0 },
        iLayer{ 0 }
    {
        base_type::Position.Changed([this](const point&) { moved(); });
        aLayout.add(*this);
        base_type::set_alive();
    }

    template <typename Interface>
    widget<Interface>::~widget()
    {
        unlink();
        if (service<i_keyboard>().is_keyboard_grabbed_by(*this))
            service<i_keyboard>().ungrab_keyboard(*this);
        remove_all();
        {
            auto layout = iLayout;
            iLayout.reset();
        }
        if (has_parent())
            parent().remove(*this);
        if (has_parent_layout())
            parent_layout().remove(*this);
    }

    template <typename Interface>
    void widget<Interface>::property_changed(i_property& aProperty)
    {
        static auto invalidate_layout = [](i_widget& self) { if (self.has_parent_layout()) self.parent_layout().invalidate(); };
        static auto invalidate_canvas = [](i_widget& self) { self.update(true); };
        static auto invalidate_window_canvas = [](i_widget& self) { self.root().as_widget().update(true); };
        static auto ignore = [](i_widget&) {};
        static const std::unordered_map<std::type_index, std::function<void(i_widget&)>> sActions =
        {
            { std::type_index{ typeid(property_category::hard_geometry) }, invalidate_layout },
            { std::type_index{ typeid(property_category::soft_geometry) }, invalidate_window_canvas },
            { std::type_index{ typeid(property_category::font) }, invalidate_layout },
            { std::type_index{ typeid(property_category::color) }, invalidate_canvas },
            { std::type_index{ typeid(property_category::other_appearance) }, invalidate_canvas },
            { std::type_index{ typeid(property_category::other) }, ignore }
        };
        auto iterAction = sActions.find(std::type_index{ aProperty.category() });
        if (iterAction != sActions.end())
            iterAction->second(*this);
    }

    template <typename Interface>
    bool widget<Interface>::device_metrics_available() const
    {
        return has_root() && root().has_native_window();
    }

    template <typename Interface>
    const i_device_metrics& widget<Interface>::device_metrics() const
    {
        if (device_metrics_available())
            return surface();
        throw no_device_metrics();
    }

    template <typename Interface>
    bool widget<Interface>::is_singular() const
    {
        return iSingular;
    }

    template <typename Interface>
    void widget<Interface>::set_singular(bool aSingular)
    {
        if (iSingular != aSingular)
        {
            iSingular = aSingular;
            if (iSingular)
            {
                iParent = nullptr;
            }
        }
    }

    template <typename Interface>
    bool widget<Interface>::is_root() const
    {
        return false;
    }

    template <typename Interface>
    bool widget<Interface>::has_root() const
    {
        if (iRoot == std::nullopt)
        {
            const i_widget* w = this;
            while (!w->is_root() && w->has_parent())
                w = &w->parent();
            if (w->is_root())
                iRoot = &w->root();
        }
        return iRoot != std::nullopt;
    }

    template <typename Interface>
    const i_window& widget<Interface>::root() const
    {
        if (has_root())
            return **iRoot;
        throw no_root();
    }

    template <typename Interface>
    i_window& widget<Interface>::root()
    {
        return const_cast<i_window&>(to_const(*this).root());
    }

    template <typename Interface>
    bool widget<Interface>::has_parent() const
    {
        return iParent != nullptr;
    }

    template <typename Interface>
    const i_widget& widget<Interface>::parent() const
    {
        if (!has_parent())
            throw no_parent();
        return *iParent;
    }

    template <typename Interface>
    i_widget& widget<Interface>::parent()
    {
        return const_cast<i_widget&>(to_const(*this).parent());
    }

    template <typename Interface>
    void widget<Interface>::set_parent(i_widget& aParent)
    {
        if (iParent != &aParent)
        {
            if (is_root() || aParent.adding_child())
            {
                iParent = &aParent;
                parent_changed();
            }
            else
                aParent.add(*this);
        }
    }

    template <typename Interface>
    void widget<Interface>::parent_changed()
    {
        if (!is_root())
        {
            iOrigin = std::nullopt;
            as_widget().update_layout();
        }
    }

    template <typename Interface>
    bool widget<Interface>::adding_child() const
    {
        return iAddingChild;
    }

    template <typename Interface>
    i_widget& widget<Interface>::add(i_widget& aChild)
    {
        return add(ref_ptr<i_widget>{ ref_ptr<i_widget>{}, &aChild });
    }

    template <typename Interface>
    i_widget& widget<Interface>::add(const i_ref_ptr<i_widget>& aChild)
    {
        neolib::scoped_flag sf{ iAddingChild };
        if (aChild->has_parent() && &aChild->parent() == this)
            return *aChild;
        i_widget* oldParent = aChild->has_parent() ? &aChild->parent() : nullptr;
        ref_ptr<i_widget> child = aChild;
        if (oldParent != nullptr)
            oldParent->remove(*child, true);
        iChildren.push_back(child);
        child->set_parent(*this);
        child->set_singular(false);
        if (has_root())
            root().widget_added(*child);
        ChildAdded.trigger(*child);
        return *child;
    }

    template <typename Interface>
    void widget<Interface>::remove(i_widget& aChild, bool aSingular, i_ref_ptr<i_widget>& aChildRef)
    {
        auto existing = find(aChild, false);
        if (existing == iChildren.end())
            return;
        ref_ptr<i_widget> keep = *existing;
        iChildren.erase(existing);
        if (aSingular)
            keep->set_singular(true);
        if (has_layout())
            layout().remove(aChild);
        if (has_root())
            root().widget_removed(aChild);
        ChildRemoved.trigger(*keep);
        aChildRef = keep;
    }

    template <typename Interface>
    void widget<Interface>::remove_all()
    {
        while (!iChildren.empty())
            remove(*iChildren.back(), true);
    }

    template <typename Interface>
    bool widget<Interface>::has_children() const
    {
        return !iChildren.empty();
    }

    template <typename Interface>
    const typename widget<Interface>::widget_list& widget<Interface>::children() const
    {
        return iChildren;
    }

    template <typename Interface>
    typename widget<Interface>::widget_list::const_iterator widget<Interface>::last() const
    {
        if (!has_children())
        {
            if (has_parent())
                return parent().find(*this);
            else
                throw no_children();
        }
        else
            return iChildren.back()->last();
    }

    template <typename Interface>
    typename widget<Interface>::widget_list::iterator widget<Interface>::last()
    {
        if (!has_children())
        {
            if (has_parent())
                return parent().find(*this);
            else
                throw no_children();
        }
        else
            return iChildren.back()->last();
    }

    template <typename Interface>
    typename widget<Interface>::widget_list::const_iterator widget<Interface>::find(const i_widget& aChild, bool aThrowIfNotFound) const
    {
        for (auto i = iChildren.begin(); i != iChildren.end(); ++i)
            if (&**i == &aChild)
                return i;
        if (aThrowIfNotFound)
            throw not_child();
        else
            return iChildren.end();
    }

    template <typename Interface>
    typename widget<Interface>::widget_list::iterator widget<Interface>::find(const i_widget& aChild, bool aThrowIfNotFound)
    {
        for (auto i = iChildren.begin(); i != iChildren.end(); ++i)
            if (&**i == &aChild)
                return i;
        if (aThrowIfNotFound)
            throw not_child();
        else
            return iChildren.end();
    }

    template <typename Interface>
    const i_widget& widget<Interface>::before() const
    {
        if (iLinkBefore != nullptr)
            return *iLinkBefore;
        if (has_parent())
        {
            auto me = parent().find(*this);
            if (me != parent().children().begin())
                return **(*(me - 1))->last();
            else
                return parent();
        }
        else if (has_children())
            return **last();
        else
            return *this;
    }

    template <typename Interface>
    void widget<Interface>::bring_child_to_front(const i_widget& aChild)
    {
        auto existing = std::find_if(iChildren.begin(), iChildren.end(), [&](auto&& c) { return &*c == &aChild; });
        if (existing != iChildren.end())
        {
            ref_ptr<i_widget> child = *existing;
            iChildren.erase(existing);
            iChildren.insert(iChildren.begin(), child);
        }
    }

    template <typename Interface>
    void widget<Interface>::send_child_to_back(const i_widget& aChild)
    {
        auto existing = std::find_if(iChildren.begin(), iChildren.end(), [&](auto&& c) { return &*c == &aChild; });
        if (existing != iChildren.end())
        {
            ref_ptr<i_widget> child = *existing;
            iChildren.erase(existing);
            iChildren.insert(iChildren.end(), child);
        }
    }

    template <typename Interface>
    int32_t widget<Interface>::layer() const
    {
        return iLayer;
    }

    template <typename Interface>
    void widget<Interface>::set_layer(int32_t aLayer)
    {
        if (iLayer != aLayer)
        {
            iLayer = aLayer;
            update(true);
        }
    }

    template <typename Interface>
    i_widget& widget<Interface>::before()
    {
        return const_cast<i_widget&>(to_const(*this).before());
    }

    template <typename Interface>
    const i_widget& widget<Interface>::after() const
    {
        if (iLinkAfter != nullptr)
            return *iLinkAfter;
        if (has_children())
            return *iChildren.front();
        if (has_parent())
        {
            auto me = parent().find(*this);
            if (me + 1 != parent().children().end())
                return *(*(me + 1));
            else if (parent().has_parent())
            {
                auto myParent = parent().parent().find(parent());
                while ((*myParent)->has_parent() && (*myParent)->parent().has_parent() &&
                    myParent + 1 == (*myParent)->parent().children().end())
                    myParent = (*myParent)->parent().parent().find((*myParent)->parent());
                if ((*myParent)->has_parent() && myParent + 1 != (*myParent)->parent().children().end())
                    return **(myParent + 1);
                else if ((*(myParent))->has_parent())
                    return (*(myParent))->parent();
                else
                    return **myParent;
            }
            else
                return parent();
        }
        else
            return *this;
    }

    template <typename Interface>
    i_widget& widget<Interface>::after()
    {
        return const_cast<i_widget&>(to_const(*this).after());
    }

    template <typename Interface>
    void widget<Interface>::link_before(i_widget* aPreviousWidget)
    {
        iLinkBefore = aPreviousWidget;
    }

    template <typename Interface>
    void widget<Interface>::link_after(i_widget* aNextWidget)
    {
        iLinkAfter = aNextWidget;
    }

    template <typename Interface>
    void widget<Interface>::unlink()
    {
        if (iLinkBefore != nullptr)
            iLinkBefore->link_after(iLinkAfter);
        if (iLinkAfter != nullptr)
            iLinkAfter->link_before(iLinkBefore);
        iLinkBefore = nullptr;
        iLinkAfter = nullptr;
    }

    template <typename Interface>
    bool widget<Interface>::has_layout() const
    {
        return iLayout != nullptr;
    }

    template <typename Interface>
    void widget<Interface>::set_layout(i_layout& aLayout, bool aMoveExistingItems)
    {
        set_layout(ref_ptr<i_layout>{ ref_ptr<i_layout>{}, &aLayout }, aMoveExistingItems);
    }

    template <typename Interface>
    void widget<Interface>::set_layout(const i_ref_ptr<i_layout>& aLayout, bool aMoveExistingItems)
    {
        if (iLayout == aLayout)
            throw layout_already_set();
        auto oldLayout = iLayout;
        iLayout = aLayout;
        if (iLayout != nullptr)
        {
            if (has_parent_layout())
                iLayout->set_parent_layout(&parent_layout());
            iLayout->set_layout_owner(this);
            if (aMoveExistingItems)
            {
                if (oldLayout == nullptr)
                {
                    for (auto& child : iChildren)
                        if (child->has_parent_layout() && &child->parent_layout() == nullptr)
                            iLayout->add(child);
                }
                else
                    oldLayout->move_all_to(*iLayout);
            }
        }
    }

    template <typename Interface>
    const i_layout& widget<Interface>::layout() const
    {
        if (!iLayout)
            throw no_layout();
        return *iLayout;
    }
    
    template <typename Interface>
    i_layout& widget<Interface>::layout()
    {
        if (!iLayout)
            throw no_layout();
        return *iLayout;
    }

    template <typename Interface>
    bool widget<Interface>::can_defer_layout() const
    {
        return is_managing_layout();
    }

    template <typename Interface>
    bool widget<Interface>::is_managing_layout() const
    {
        return false;
    }

    template <typename Interface>
    bool widget<Interface>::is_layout() const
    {
        return false;
    }

    template <typename Interface>
    const i_layout& widget<Interface>::as_layout() const
    {
        throw not_a_layout();
    }

    template <typename Interface>
    i_layout& widget<Interface>::as_layout()
    {
        throw not_a_layout();
    }

    template <typename Interface>
    bool widget<Interface>::is_widget() const
    {
        return true;
    }

    template <typename Interface>
    const i_widget& widget<Interface>::as_widget() const
    {
        return *this;
    }

    template <typename Interface>
    i_widget& widget<Interface>::as_widget()
    {
        return *this;
    }

    template <typename Interface>
    rect widget<Interface>::element_rect(skin_element) const
    {
        return client_rect();
    }

    template <typename Interface>
    bool widget<Interface>::has_parent_layout() const
    {
        return iParentLayout != nullptr;
    }
    
    template <typename Interface>
    const i_layout& widget<Interface>::parent_layout() const
    {
        if (has_parent_layout())
            return *iParentLayout;
        throw no_parent_layout();
    }

    template <typename Interface>
    i_layout& widget<Interface>::parent_layout()
    {
        return const_cast<i_layout&>(to_const(*this).parent_layout());
    }

    template <typename Interface>
    void widget<Interface>::set_parent_layout(i_layout* aParentLayout)
    {
        if (has_layout() && layout().has_parent_layout() && &layout().parent_layout() == iParentLayout)
            layout().set_parent_layout(aParentLayout);
        iParentLayout = aParentLayout;
    }

    template <typename Interface>
    bool widget<Interface>::has_layout_owner() const
    {
        return has_parent_layout() && parent_layout().has_layout_owner();
    }

    template <typename Interface>
    const i_widget& widget<Interface>::layout_owner() const
    {
        if (has_layout_owner())
            return parent_layout().layout_owner();
        throw no_layout_owner();
    }

    template <typename Interface>
    i_widget& widget<Interface>::layout_owner()
    {
        return const_cast<i_widget&>(to_const(*this).layout_owner());
    }

    template <typename Interface>
    void widget<Interface>::set_layout_owner(i_widget* aOwner)
    {
        if (aOwner != nullptr && !has_parent())
        {
            auto itemIndex = parent_layout().find(*this);
            if (itemIndex == std::nullopt)
                throw i_layout::item_not_found();
            aOwner->add(dynamic_pointer_cast<i_widget>(proxy_for_layout().subject_ptr()));
        }
    }

    template <typename Interface>
    bool widget<Interface>::is_proxy() const
    {
        return false;
    }

    template <typename Interface>
    const i_layout_item_proxy& widget<Interface>::proxy_for_layout() const
    {
        return parent_layout().find_proxy(*this);
    }

    template <typename Interface>
    i_layout_item_proxy& widget<Interface>::proxy_for_layout()
    {
        return parent_layout().find_proxy(*this);
    }

    template <typename Interface>
    void widget<Interface>::layout_items(bool aDefer)
    {
        if (layout_items_in_progress())
            return;
        if (!aDefer)
        {
            if (iLayoutTimer != nullptr)
                iLayoutTimer.reset();
            if (has_layout())
            {
                layout_items_started();
                if (is_root() && size_policy() != size_constraint::Manual)
                {
                    size desiredSize = extents();
                    switch (size_policy().horizontal_size_policy())
                    {
                    case size_constraint::Fixed:
                        desiredSize.cx = as_widget().has_fixed_size() ? as_widget().fixed_size().cx : minimum_size(extents()).cx;
                        break;
                    case size_constraint::Minimum:
                        desiredSize.cx = minimum_size(extents()).cx;
                        break;
                    case size_constraint::Maximum:
                        desiredSize.cx = maximum_size(extents()).cx;
                        break;
                    default:
                        break;
                    }
                    switch (size_policy().vertical_size_policy())
                    {
                    case size_constraint::Fixed:
                        desiredSize.cy = as_widget().has_fixed_size() ? as_widget().fixed_size().cy : minimum_size(extents()).cy;
                        break;
                    case size_constraint::Minimum:
                        desiredSize.cy = minimum_size(extents()).cy;
                        break;
                    case size_constraint::Maximum:
                        desiredSize.cy = maximum_size(extents()).cy;
                        break;
                    default:
                        break;
                    }
                    resize(desiredSize);
                }
                layout().layout_items(client_rect(false).top_left(), client_rect(false).extents());
                layout_items_completed();
            }
        }
        else if (can_defer_layout())
        {
            if (has_root() && !iLayoutTimer)
            {
                iLayoutTimer = std::make_unique<layout_timer>(root(), service<i_async_task>(), [this](neolib::callback_timer&)
                {
                    if (root().has_native_window())
                    {
                        auto t = std::move(iLayoutTimer);
                        layout_items();
                        update();
                    }
                });
            }
        }
        else if (as_widget().has_layout_manager())
        {
            throw widget_cannot_defer_layout();
        }
    }

    template <typename Interface>
    void widget<Interface>::layout_items_started()
    {
#ifdef NEOGFX_DEBUG
        if (debug::layoutItem == this)
            service<debug::logger>() << "widget:layout_items_started()" << endl;
#endif // NEOGFX_DEBUG
        ++iLayoutInProgress;
    }

    template <typename Interface>
    bool widget<Interface>::layout_items_in_progress() const
    {
        return iLayoutInProgress != 0;
    }

    template <typename Interface>
    void widget<Interface>::layout_items_completed()
    {
#ifdef NEOGFX_DEBUG
        if (debug::layoutItem == this)
            service<debug::logger>() << "widget:layout_items_completed()" << endl;
#endif // NEOGFX_DEBUG
        if (--iLayoutInProgress == 0)
        {
            LayoutCompleted.trigger();
            update();
        }
    }

    template <typename Interface>
    bool widget<Interface>::high_dpi() const
    {
        return has_root() && root().has_surface() ? 
            root().surface().ppi() >= 150.0 : 
            service<i_surface_manager>().display().metrics().ppi() >= 150.0;
    }

    template <typename Interface>
    dimension widget<Interface>::dpi_scale_factor() const
    {
        return has_root() && root().has_surface() ?
            default_dpi_scale_factor(root().surface().ppi()) :
            service<i_app>().default_dpi_scale_factor();
    }

    template <typename Interface>
    bool widget<Interface>::has_logical_coordinate_system() const
    {
        return LogicalCoordinateSystem != std::nullopt;
    }

    template <typename Interface>
    logical_coordinate_system widget<Interface>::logical_coordinate_system() const
    {
        if (has_logical_coordinate_system())
            return *LogicalCoordinateSystem;
        return neogfx::logical_coordinate_system::AutomaticGui;
    }

    template <typename Interface>
    void widget<Interface>::set_logical_coordinate_system(const optional_logical_coordinate_system& aLogicalCoordinateSystem)
    {
        LogicalCoordinateSystem = aLogicalCoordinateSystem;
    }

    template <typename Interface>
    point widget<Interface>::position() const
    {
        return units_converter(*this).from_device_units(base_type::Position);
    }

    template <typename Interface>
    void widget<Interface>::set_position(const point& aPosition)
    {
        move(aPosition);
    }

    template <typename Interface>
    point widget<Interface>::origin() const
    {
        if (iOrigin == std::nullopt)
        {
            if ((!is_root() || root().is_nested()))
            {
                if (has_parent())
                    iOrigin = position() + parent().origin();
                else
                    iOrigin = position();
            }
            else
                iOrigin = point{};
        }
        return *iOrigin;
    }

    template <typename Interface>
    void widget<Interface>::move(const point& aPosition)
    {
#ifdef NEOGFX_DEBUG
        if (debug::layoutItem == this)
            service<debug::logger>() << "widget<Interface>::move(" << aPosition << ")" << endl;
#endif // NEOGFX_DEBUG
        if (base_type::Position != units_converter(*this).to_device_units(aPosition))
            base_type::Position.assign(units_converter(*this).to_device_units(aPosition), false);
    }

    template <typename Interface>
    void widget<Interface>::moved()
    {
        if (!is_root())
        {
            update(true);
            iOrigin = std::nullopt;
            update(true);
            for (auto& child : iChildren)
                child->parent_moved();
            if ((widget_type() & neogfx::widget_type::Floating) == neogfx::widget_type::Floating)
            {
                parent().layout_items_started();
                parent().layout_items_completed();
            }
        }
        PositionChanged.trigger();
    }

    template <typename Interface>
    void widget<Interface>::parent_moved()
    {
        iOrigin = std::nullopt;
        for (auto& child : iChildren)
            child->parent_moved();
    }
    
    template <typename Interface>
    size widget<Interface>::extents() const
    {
        return units_converter(*this).from_device_units(base_type::Size);
    }

    template <typename Interface>
    void widget<Interface>::set_extents(const size& aSize)
    {
        resize(aSize);
    }

    template <typename Interface>
    void widget<Interface>::resize(const size& aSize)
    {
#ifdef NEOGFX_DEBUG
        if (debug::layoutItem == this)
            service<debug::logger>() << "widget<Interface>::resize(" << aSize << ")" << endl;
#endif // NEOGFX_DEBUG
        if (base_type::Size != units_converter(*this).to_device_units(aSize))
        {
            update(true);
            base_type::Size.assign(units_converter(*this).to_device_units(aSize), false);
            update(true);
            resized();
        }
    }

    template <typename Interface>
    void widget<Interface>::resized()
    {
        SizeChanged.trigger();
        layout_items();
        if ((widget_type() & neogfx::widget_type::Floating) == neogfx::widget_type::Floating)
        {
            parent().layout_items_started();
            parent().layout_items_completed();
        }
    }

    template <typename Interface>
    rect widget<Interface>::non_client_rect() const
    {
        return rect{origin(), extents()};
    }

    template <typename Interface>
    rect widget<Interface>::client_rect(bool aIncludePadding) const
    {
        if (!aIncludePadding)
            return rect{ padding().top_left(), extents() - padding().size() };
        else
            return rect{ point{}, extents() };
    }

    template <typename Interface>
    const i_widget& widget<Interface>::get_widget_at(const point& aPosition) const
    {
        if (client_rect().contains(aPosition))
        {
            i_widget const* hitWidget = nullptr;
            for (auto const& child : children())
                if (child->visible() && to_client_coordinates(child->non_client_rect()).contains(aPosition))
                {
                    if (hitWidget == nullptr || child->layer() > hitWidget->layer())
                        hitWidget = &*child;
                }
            if (hitWidget)
                return hitWidget->get_widget_at(aPosition - hitWidget->position());
        }
        return *this;
    }

    template <typename Interface>
    i_widget& widget<Interface>::get_widget_at(const point& aPosition)
    {
        return const_cast<i_widget&>(to_const(*this).get_widget_at(aPosition));
    }

    template <typename Interface>
    widget_type widget<Interface>::widget_type() const
    {
        return neogfx::widget_type::Client;
    }

    template <typename Interface>
    bool widget<Interface>::part_active(widget_part aPart) const
    {
        return true;
    }

    template <typename Interface>
    widget_part widget<Interface>::part(const point& aPosition) const
    {
        if (client_rect().contains(aPosition))
            return widget_part{ *this, widget_part::Client };
        else if (to_client_coordinates(non_client_rect()).contains(aPosition))
            return widget_part{ *this, widget_part::NonClient };
        else
            return widget_part{ *this, widget_part::Nowhere };
    }

    template <typename Interface>
    widget_part widget<Interface>::hit_test(const point& aPosition) const
    {
        return part(aPosition);
    }

    template <typename Interface>
    size_policy widget<Interface>::size_policy() const
    {
#ifdef NEOGFX_DEBUG
        if (debug::layoutItem == this)
            service<debug::logger>() << typeid(*this).name() << "::size_policy()" << endl;
#endif // NEOGFX_DEBUG
        if (as_widget().has_size_policy())
            return base_type::size_policy();
        else if (as_widget().has_fixed_size())
            return size_constraint::Fixed;
        else
            return size_constraint::Expanding;
    }

    template <typename Interface>
    size widget<Interface>::minimum_size(optional_size const& aAvailableSpace) const
    {
#ifdef NEOGFX_DEBUG
        if (debug::layoutItem == this)
            service<debug::logger>() << typeid(*this).name() << "::minimum_size(" << aAvailableSpace << ")" << endl;
#endif // NEOGFX_DEBUG
        size result;
        if (as_widget().has_minimum_size())
            result = units_converter{ *this }.from_device_units(*base_type::MinimumSize);
        else if (has_layout())
        {
            result = layout().minimum_size(aAvailableSpace != std::nullopt ? *aAvailableSpace - padding().size() : aAvailableSpace);
            if (result.cx != 0.0)
                result.cx += padding().size().cx;
            if (result.cy != 0.0)
                result.cy += padding().size().cy;
        }
        else
            result = padding().size();
#ifdef NEOGFX_DEBUG
        if (debug::layoutItem == this)
            service<debug::logger>() << typeid(*this).name() << "::minimum_size(" << aAvailableSpace << ") --> " << result << endl;
#endif // NEOGFX_DEBUG
        return result;
    }

    template <typename Interface>
    size widget<Interface>::maximum_size(optional_size const& aAvailableSpace) const
    {
#ifdef NEOGFX_DEBUG
        if (debug::layoutItem == this)
            service<debug::logger>() << typeid(*this).name() << "::maximum_size(" << aAvailableSpace << ")" << endl;
#endif // NEOGFX_DEBUG
        size result;
        if (as_widget().has_maximum_size())
            result = units_converter(*this).from_device_units(*base_type::MaximumSize);
        else if (size_policy() == size_constraint::Minimum)
            result = minimum_size(aAvailableSpace);
        else if (has_layout())
        {
            result = layout().maximum_size(aAvailableSpace != std::nullopt ? *aAvailableSpace - padding().size() : aAvailableSpace);
            if (result.cx != 0.0)
                result.cx += padding().size().cx;
            if (result.cy != 0.0)
                result.cy += padding().size().cy;
        }
        else
            result = size::max_size();
        if (size_policy().horizontal_size_policy() == size_constraint::Maximum)
            result.cx = size::max_size().cx;
        if (size_policy().vertical_size_policy() == size_constraint::Maximum)
            result.cy = size::max_size().cy;
#ifdef NEOGFX_DEBUG
        if (debug::layoutItem == this)
            service<debug::logger>() << typeid(*this).name() << "::maximum_size(" << aAvailableSpace << ") --> " << result << endl;
#endif // NEOGFX_DEBUG
        return result;
    }

    template <typename Interface>
    padding widget<Interface>::padding() const
    {
        auto const& adjustedPadding = (as_widget().has_padding() ? *base_type::Padding : service<i_app>().current_style().padding(is_root() ? padding_role::Window : padding_role::Widget) * 1.0_dip);
        return units_converter(*this).from_device_units(adjustedPadding);
    }

    template <typename Interface>
    void widget<Interface>::layout_as(const point& aPosition, const size& aSize)
    {
#ifdef NEOGFX_DEBUG
        if (debug::layoutItem == this)
            service<debug::logger>() << typeid(*this).name() << "::layout_as(" << aPosition << ", " << aSize << ")" << endl;
#endif // NEOGFX_DEBUG
        move(aPosition);
        if (extents() != aSize)
            resize(aSize);
        else if (has_layout() && layout().invalidated())
            layout_items();
    }

    template <typename Interface>
    int32_t widget<Interface>::render_layer() const
    {
        if (iRenderLayer != std::nullopt)
            return *iRenderLayer;
        return layer();
    }

    template <typename Interface>
    void widget<Interface>::set_render_layer(const std::optional<int32_t>& aLayer)
    {
        if (iRenderLayer != aLayer)
        {
            iRenderLayer = aLayer;
            update(true);
        }
    }

    template <typename Interface>
    bool widget<Interface>::update(const rect& aUpdateRect)
    {
#ifdef NEOGFX_DEBUG
        if (debug::renderItem == this)
            service<debug::logger>() << typeid(*this).name() << "::update(" << aUpdateRect << ")" << endl;
#endif // NEOGFX_DEBUG
        if (!can_update())
            return false;
        if (aUpdateRect.empty())
            return false;
        surface().invalidate_surface(to_window_coordinates(aUpdateRect));
        return true;
    }

    template <typename Interface>
    bool widget<Interface>::requires_update() const
    {
        return surface().has_invalidated_area() && !surface().invalidated_area().intersection(non_client_rect()).empty();
    }

    template <typename Interface>
    rect widget<Interface>::update_rect() const
    {
        if (!requires_update())
            throw no_update_rect();
        return to_client_coordinates(surface().invalidated_area().intersection(non_client_rect()));
    }

    template <typename Interface>
    rect widget<Interface>::default_clip_rect(bool aIncludeNonClient) const
    {
        auto& cachedRect = (aIncludeNonClient ? iDefaultClipRect.first : iDefaultClipRect.second);
        if (cachedRect != std::nullopt)
            return *cachedRect;
        rect clipRect = to_client_coordinates(non_client_rect());
        if (!aIncludeNonClient)
            clipRect = clipRect.intersection(client_rect());
        if (!is_root())
            clipRect = clipRect.intersection(to_client_coordinates(parent().to_window_coordinates(parent().default_clip_rect((widget_type() & neogfx::widget_type::NonClient) == neogfx::widget_type::NonClient))));
        else if (root().is_nested())
        {
            auto& parent = root().parent_window().as_widget();
            clipRect = clipRect.intersection(to_client_coordinates(parent.to_window_coordinates(parent.default_clip_rect((widget_type() & neogfx::widget_type::NonClient) == neogfx::widget_type::NonClient))));
        }
        return *(cachedRect = clipRect);
    }

    template <typename Interface>
    bool widget<Interface>::ready_to_render() const
    {
        return iLayoutTimer == nullptr;
    }

    template <typename Interface>
    void widget<Interface>::render(i_graphics_context& aGc) const
    {
        if (effectively_hidden())
            return;
        if (!requires_update())
            return;

        iDefaultClipRect = std::make_pair(std::nullopt, std::nullopt);

        const rect updateRect = update_rect();
        const rect nonClientClipRect = default_clip_rect(true).intersection(updateRect);

#ifdef NEOGFX_DEBUG
        if (debug::renderItem == this)
            service<debug::logger>() << typeid(*this).name() << "::render(...), updateRect: " << updateRect << ", nonClientClipRect: " << nonClientClipRect << endl;
#endif // NEOGFX_DEBUG

        aGc.set_extents(extents());
        aGc.set_origin(origin());

        scoped_snap_to_pixel snap{ aGc };
        scoped_opacity sc{ aGc, opacity() };

        {
            scoped_scissor scissor(aGc, nonClientClipRect);
            paint_non_client(aGc);

            for (auto i = iChildren.rbegin(); i != iChildren.rend(); ++i)
            {
                auto const& child = *i;
                if ((child->widget_type() & neogfx::widget_type::Client) == neogfx::widget_type::Client)
                    continue;
                rect intersection = nonClientClipRect.intersection(child->non_client_rect() - origin());
                if (!intersection.empty())
                    child->render(aGc);
            }
        }

        {
            const rect clipRect = default_clip_rect().intersection(updateRect);

            aGc.set_extents(client_rect().extents());
            aGc.set_origin(origin());

#ifdef NEOGFX_DEBUG
            if (debug::renderItem == this)
                service<debug::logger>() << typeid(*this).name() << "::render(...): client_rect: " << client_rect() << ", origin: " << origin() << endl;
#endif // NEOGFX_DEBUG

            scoped_scissor scissor(aGc, clipRect);

            scoped_coordinate_system scs1(aGc, origin(), extents(), logical_coordinate_system());

            Painting.trigger(aGc);

            paint(aGc);

            scoped_coordinate_system scs2(aGc, origin(), extents(), logical_coordinate_system());

            PaintingChildren.trigger(aGc);

            typedef std::map<int32_t, std::vector<i_widget const*>> widget_layers_t;
            thread_local std::vector<std::unique_ptr<widget_layers_t>> widgetLayersStack;

            thread_local std::size_t stack;
            neolib::scoped_counter<std::size_t> stackCounter{ stack };
            if (widgetLayersStack.size() < stack)
                widgetLayersStack.push_back(std::make_unique<widget_layers_t>());

            widget_layers_t& widgetLayers = *widgetLayersStack[stack - 1];

            for (auto& layer : widgetLayers)
                layer.second.clear();

            for (auto iterChild = iChildren.rbegin(); iterChild != iChildren.rend(); ++iterChild)
            {
                auto const& childWidget = **iterChild;
                if ((childWidget.widget_type() & neogfx::widget_type::NonClient) == neogfx::widget_type::NonClient)
                    continue;
                rect intersection = clipRect.intersection(to_client_coordinates(childWidget.non_client_rect()));
                if (intersection.empty())
                    continue;
                widgetLayers[childWidget.render_layer()].push_back(&childWidget);
            }
                
            for (auto const& layer : widgetLayers)
            {
                for (auto const& childWidgetPtr : layer.second)
                {
                    auto const& childWidget = *childWidgetPtr;
                    childWidget.render(aGc);
                }
            }

            aGc.set_extents(client_rect().extents());
            aGc.set_origin(origin());

            scoped_coordinate_system scs3(aGc, origin(), extents(), logical_coordinate_system());

            Painted.trigger(aGc);
        }

        aGc.set_extents(extents());
        aGc.set_origin(origin());
        {
            scoped_scissor scissor(aGc, nonClientClipRect);
            paint_non_client_after(aGc);
        }
    }

    template <typename Interface>
    void widget<Interface>::paint_non_client(i_graphics_context& aGc) const
    {
        if (as_widget().has_background_color() || !as_widget().background_is_transparent())
            aGc.fill_rect(update_rect(), as_widget().background_color().with_combined_alpha(has_background_opacity() ? background_opacity() : 1.0));
    }

    template <typename Interface>
    void widget<Interface>::paint_non_client_after(i_graphics_context& aGc) const
    {
#ifdef NEOGFX_DEBUG
        // todo: move to debug::layoutItem function/service
        if (debug::layoutItem == this || debug::layoutItem != nullptr && has_layout() && debug::layoutItem->is_layout() &&
            (debug::layoutItem == &layout() || static_cast<i_layout const*>(debug::layoutItem)->is_descendent_of(layout())))
        {
            neogfx::font debugFont1 = service<i_app>().current_style().font().with_size(16);
            neogfx::font debugFont2 = service<i_app>().current_style().font().with_size(8);
            if (debug::renderGeometryText)
            {
                if (debug::layoutItem == this)
                {
                    aGc.draw_text(position(), typeid(*this).name(), debugFont1, text_appearance{ color::Yellow.with_alpha(0.75), text_effect{ text_effect_type::Outline, color::Black.with_alpha(0.75), 2.0 } });
                    std::ostringstream oss;
                    oss << "sizepol: " << size_policy();
                    oss << " minsize: " << minimum_size() << " maxsize: " << maximum_size();
                    oss << " fixsize: " << (as_widget().has_fixed_size() ? as_widget().fixed_size() : optional_size{}) << " weight: " << as_widget().weight() << " extents: " << extents();
                    aGc.draw_text(position() + size{ 0.0, debugFont1.height() }, oss.str(), debugFont2, text_appearance{ color::Orange.with_alpha(0.75), text_effect{ text_effect_type::Outline, color::Black.with_alpha(0.75), 2.0 } });
                }
            }
            rect const nonClientRect = to_client_coordinates(non_client_rect());
            aGc.draw_rect(nonClientRect, pen{ color::White, 3.0 });
            aGc.line_stipple_on(1.0, 0x5555);
            aGc.draw_rect(nonClientRect, pen{ color::Green, 3.0 });
            aGc.line_stipple_off();
            if (debug::layoutItem != this || has_layout())
            {
                i_layout const& debugLayout = (debug::layoutItem == this ? layout() : *static_cast<i_layout const*>(debug::layoutItem));
                if (debug::renderGeometryText)
                {
                    if (debug::layoutItem != this)
                    {
                        aGc.draw_text(debugLayout.position(), typeid(debugLayout).name(), debugFont1, text_appearance{ color::Yellow.with_alpha(0.75), text_effect{ text_effect_type::Outline, color::Black.with_alpha(0.75), 2.0 } });
                        std::ostringstream oss;
                        oss << "sizepol: " << debugLayout.size_policy();
                        oss << " minsize: " << debugLayout.minimum_size() << " maxsize: " << debugLayout.maximum_size();
                        oss << " fixsize: " << (debugLayout.has_fixed_size() ? debugLayout.fixed_size() : optional_size{}) << " weight: " << debugLayout.weight() << " extents: " << debugLayout.extents();
                        aGc.draw_text(debugLayout.position() + size{ 0.0, debugFont1.height() }, oss.str(), debugFont2, text_appearance{ color::Orange.with_alpha(0.75), text_effect{ text_effect_type::Outline, color::Black.with_alpha(0.75), 2.0 } });
                    }
                }
                for (layout_item_index itemIndex = 0; itemIndex < debugLayout.count(); ++itemIndex)
                {
                    auto const& item = debugLayout.item_at(itemIndex);
                    if (debug::renderGeometryText)
                    {
                        std::string text = typeid(item).name();
                        auto* l = &item;
                        while (l->has_parent_layout())
                        {
                            l = &l->parent_layout();
                            text = typeid(*l).name() + " > "_s + text;
                        }
                        aGc.draw_text(item.position(), text, debugFont2, text_appearance{ color::White.with_alpha(0.5), text_effect{ text_effect_type::Outline, color::Black.with_alpha(0.5), 2.0 } });
                    }
                    rect const itemRect{ item.position(), item.extents() };
                    aGc.draw_rect(itemRect, color::White.with_alpha(0.5));
                    aGc.line_stipple_on(1.0, 0x5555);
                    aGc.draw_rect(itemRect, color::Black.with_alpha(0.5));
                    aGc.line_stipple_off();
                }
                rect const layoutRect{ debugLayout.position(), debugLayout.extents() };
                aGc.draw_rect(layoutRect, color::White);
                aGc.line_stipple_on(1.0, 0x5555);
                aGc.draw_rect(layoutRect, debug::layoutItem == &layout() ? color::Blue : color::Purple);
                aGc.line_stipple_off();
            }
        }
#endif // NEOGFX_DEBUG
    }

    template <typename Interface>
    void widget<Interface>::paint(i_graphics_context& aGc) const
    {
        // do nothing
    }

    template <typename Interface>
    double widget<Interface>::opacity() const
    {
        return Opacity;
    }

    template <typename Interface>
    void widget<Interface>::set_opacity(double aOpacity)
    {
        if (Opacity != aOpacity)
        {
            Opacity = aOpacity;
            update(true);
        }
    }

    template <typename Interface>
    bool widget<Interface>::has_background_opacity() const
    {
        return BackgroundOpacity != std::nullopt;
    }

    template <typename Interface>
    double widget<Interface>::background_opacity() const
    {
        if (has_background_opacity())
            return *BackgroundOpacity.value();
        return 0.0;
    }

    template <typename Interface>
    void widget<Interface>::set_background_opacity(double aOpacity)
    {
        if (BackgroundOpacity != aOpacity)
        {
            BackgroundOpacity = aOpacity;
            update(true);
        }
    }

    template <typename Interface>
    bool widget<Interface>::has_palette() const
    {
        return Palette != std::nullopt;
    }

    template <typename Interface>
    const i_palette& widget<Interface>::palette() const
    {
        if (has_palette())
            return *Palette.value();
        return service<i_app>().current_style().palette();
    }

    template <typename Interface>
    void widget<Interface>::set_palette(const i_palette& aPalette)
    {
        if (Palette != aPalette)
        {
            Palette = aPalette;
            update(true);
        }
    }

    template <typename Interface>
    bool widget<Interface>::has_palette_color(color_role aColorRole) const
    {
        return has_palette() && palette().has_color(aColorRole);
    }

    template <typename Interface>
    color widget<Interface>::palette_color(color_role aColorRole) const
    {
        return palette().color(aColorRole);
    }

    template <typename Interface>
    void widget<Interface>::set_palette_color(color_role aColorRole, const optional_color& aColor)
    {
        if (Palette == std::nullopt)
            Palette = neogfx::palette{ current_style_palette_proxy() };
        if (palette_color(aColorRole) != aColor)
        {
            auto existing = neogfx::palette{ palette() }; // todo: support indirectly changing and notifying a property so we don't have to make a copy?
            existing.set_color(aColorRole, aColor);
            Palette = existing;
            update(true);
        }
    }

    template <typename Interface>
    color widget<Interface>::container_background_color() const
    {
        const i_widget* w = this;
        while (w->background_is_transparent() && w->has_parent())
            w = &w->parent();
        if (!w->background_is_transparent() && w->has_background_color())
            return w->background_color();
        else
            return service<i_app>().current_style().palette().color(color_role::Theme);
    }

    template <typename Interface>
    bool widget<Interface>::has_font_role() const
    {
        return FontRole != std::nullopt;
    }

    template <typename Interface>
    font_role widget<Interface>::font_role() const
    {
        if (has_font_role())
            return *FontRole.value();
        return neogfx::font_role::Widget;
    }

    template <typename Interface>
    void widget<Interface>::set_font_role(const optional_font_role& aFontRole)
    {
        if (FontRole != aFontRole)
        {
            FontRole = aFontRole;
            update(true);
        }
    }

    template <typename Interface>
    bool widget<Interface>::has_font() const
    {
        return Font != std::nullopt;
    }

    template <typename Interface>
    const font& widget<Interface>::font() const
    {
        if (has_font())
            return *Font;
        else
            return service<i_app>().current_style().font(font_role());
    }

    template <typename Interface>
    void widget<Interface>::set_font(optional_font const& aFont)
    {
        if (Font != aFont)
        {
            Font = aFont;
            as_widget().update_layout();
            update(true);
        }
    }

    template <typename Interface>
    bool widget<Interface>::visible() const
    {
        return Visible && (base_type::MaximumSize == std::nullopt || (base_type::MaximumSize->cx != 0.0 && base_type::MaximumSize->cy != 0.0));
    }

    template <typename Interface>
    bool widget<Interface>::effectively_visible() const
    {
        return visible() && (is_root() || !has_parent() || parent().effectively_visible());
    }

    template <typename Interface>
    bool widget<Interface>::hidden() const
    {
        return !visible();
    }

    template <typename Interface>
    bool widget<Interface>::effectively_hidden() const
    {
        return !effectively_visible();
    }

    template <typename Interface>
    bool widget<Interface>::show(bool aVisible)
    {
        if (Visible != aVisible)
        {
            bool isEntered = entered();
            Visible = aVisible;
            if (!visible() && isEntered)
            {
                if (!is_root())
                    root().as_widget().mouse_entered(root().mouse_position());
                else
                    mouse_left();
            }
            VisibilityChanged.trigger();
            if (effectively_hidden())
            {
                if (has_root() && root().has_focused_widget() &&
                    (root().focused_widget().is_descendent_of(*this) || &root().focused_widget() == this))
                {
                    root().release_focused_widget(root().focused_widget());
                }
            }
            else
            {
                if (has_parent_layout())
                    parent_layout().invalidate();
                update(true);
            }
            return true;
        }
        return false;
    }

    template <typename Interface>
    bool widget<Interface>::enabled() const
    {
        return Enabled;
    }

    template <typename Interface>
    bool widget<Interface>::effectively_enabled() const
    {
        return enabled() && (is_root() || !has_parent() || parent().effectively_enabled());
    }
    
    template <typename Interface>
    bool widget<Interface>::disabled() const
    {
        return !enabled();
    }

    template <typename Interface>
    bool widget<Interface>::effectively_disabled() const
    {
        return !effectively_enabled();
    }

    template <typename Interface>
    bool widget<Interface>::enable(bool aEnable)
    {
        if (Enabled != aEnable)
        {
            bool isEntered = entered();
            Enabled = aEnable;
            if (!enabled() && isEntered)
            {
                if (!is_root())
                    root().as_widget().mouse_entered(root().mouse_position());
                else
                    mouse_left();
            }
            update(true);
            return true;
        }
        return false;
    }

    template <typename Interface>
    bool widget<Interface>::entered(bool aChildEntered) const
    {
        return has_root() && root().has_entered_widget() && (&root().entered_widget() == this || (aChildEntered && root().entered_widget().is_descendent_of(*this)));
    }

    template <typename Interface>
    bool widget<Interface>::can_capture() const
    {
        return true;
    }

    template <typename Interface>
    bool widget<Interface>::capturing() const
    {
        return surface().as_surface_window().has_capturing_widget() && &surface().as_surface_window().capturing_widget() == this;
    }

    template <typename Interface>
    const optional_point& widget<Interface>::capture_position() const
    {
        return iCapturePosition;
    }

    template <typename Interface>
    void widget<Interface>::set_capture(capture_reason aReason, const optional_point& aPosition)
    {
        if (can_capture())
        {
            switch (aReason)
            {
            case capture_reason::MouseEvent:
                if (!as_widget().mouse_event_is_non_client())
                    surface().as_surface_window().set_capture(*this);
                else
                    surface().as_surface_window().non_client_set_capture(*this);
                break;
            default:
                surface().as_surface_window().set_capture(*this);
                break;
            }
            iCapturePosition = aPosition;
        }
        else
            throw widget_cannot_capture();
    }

    template <typename Interface>
    void widget<Interface>::release_capture(capture_reason aReason)
    {
        switch (aReason)
        {
        case capture_reason::MouseEvent:
            if (!as_widget().mouse_event_is_non_client())
                surface().as_surface_window().release_capture(*this);
            else
                surface().as_surface_window().non_client_release_capture(*this);
            break;
        default:
            surface().as_surface_window().release_capture(*this);
            break;
        }
        iCapturePosition = std::nullopt;
    }

    template <typename Interface>
    void widget<Interface>::non_client_set_capture()
    {
        if (can_capture())
            surface().as_surface_window().non_client_set_capture(*this);
        else
            throw widget_cannot_capture();
    }

    template <typename Interface>
    void widget<Interface>::non_client_release_capture()
    {
        surface().as_surface_window().non_client_release_capture(*this);
    }

    template <typename Interface>
    void widget<Interface>::captured()
    {
    }

    template <typename Interface>
    void widget<Interface>::capture_released()
    {
    }

    template <typename Interface>
    focus_policy widget<Interface>::focus_policy() const
    {
        return FocusPolicy;
    }

    template <typename Interface>
    void widget<Interface>::set_focus_policy(neogfx::focus_policy aFocusPolicy)
    {
        FocusPolicy = aFocusPolicy;
    }

    template <typename Interface>
    bool widget<Interface>::can_set_focus(focus_reason aFocusReason) const
    {
        switch (aFocusReason)
        {
        case focus_reason::ClickNonClient:
            return (focus_policy() & (neogfx::focus_policy::ClickFocus | neogfx::focus_policy::IgnoreNonClient)) == neogfx::focus_policy::ClickFocus;
        case focus_reason::ClickClient:
            return (focus_policy() & neogfx::focus_policy::ClickFocus) == neogfx::focus_policy::ClickFocus;
        case focus_reason::Tab:
            return (focus_policy() & neogfx::focus_policy::TabFocus) == neogfx::focus_policy::TabFocus;
        case focus_reason::Wheel:
            return (focus_policy() & neogfx::focus_policy::WheelFocus) == neogfx::focus_policy::WheelFocus;
        case focus_reason::Pointer:
            return (focus_policy() & neogfx::focus_policy::PointerFocus) == neogfx::focus_policy::PointerFocus;
        case focus_reason::WindowActivation:
        case focus_reason::Other:
        default:
            return focus_policy() != neogfx::focus_policy::NoFocus;
        }
    }

    template <typename Interface>
    bool widget<Interface>::has_focus() const
    {
        return has_root() && root().is_active() && root().has_focused_widget() && &root().focused_widget() == this;
    }

    template <typename Interface>
    bool widget<Interface>::child_has_focus() const
    {
        return has_root() && root().is_active() && root().has_focused_widget() && root().focused_widget().is_descendent_of(*this);
    }

    template <typename Interface>
    void widget<Interface>::set_focus(focus_reason aFocusReason)
    {
        root().set_focused_widget(*this, aFocusReason);
    }

    template <typename Interface>
    void widget<Interface>::release_focus()
    {
        root().release_focused_widget(*this);
    }

    template <typename Interface>
    void widget<Interface>::focus_gained(focus_reason aReason)
    {
        update(true);
        Focus.trigger(focus_event::FocusGained, aReason);
    }

    template <typename Interface>
    void widget<Interface>::focus_lost(focus_reason aReason)
    {
        update(true);
        Focus.trigger(focus_event::FocusLost, aReason);
    }

    template <typename Interface>
    bool widget<Interface>::ignore_mouse_events(bool aConsiderAncestors) const
    {
        return IgnoreMouseEvents || (aConsiderAncestors && (has_parent() && parent().ignore_mouse_events()));
    }

    template <typename Interface>
    void widget<Interface>::set_ignore_mouse_events(bool aIgnoreMouseEvents)
    {
        IgnoreMouseEvents = aIgnoreMouseEvents;
    }

    template <typename Interface>
    bool widget<Interface>::ignore_non_client_mouse_events(bool aConsiderAncestors) const
    {
        return IgnoreNonClientMouseEvents || (aConsiderAncestors && (has_parent() && parent().ignore_non_client_mouse_events()));
    }

    template <typename Interface>
    void widget<Interface>::set_ignore_non_client_mouse_events(bool aIgnoreNonClientMouseEvents)
    {
        IgnoreNonClientMouseEvents = aIgnoreNonClientMouseEvents;
    }

    template <typename Interface>
    mouse_event_location widget<Interface>::mouse_event_location() const
    {
        if (has_root() && root().has_native_surface())
            return surface().as_surface_window().current_mouse_event_location();
        else
            return neogfx::mouse_event_location::None;
    }

    template <typename Interface>
    void widget<Interface>::mouse_wheel_scrolled(mouse_wheel aWheel, const point& aPosition, delta aDelta, key_modifiers_e aKeyModifiers)
    {
        if (has_parent())
            parent().mouse_wheel_scrolled(aWheel, aPosition + position(), aDelta, aKeyModifiers);
    }

    template <typename Interface>
    void widget<Interface>::mouse_button_pressed(mouse_button aButton, const point& aPosition, key_modifiers_e aKeyModifiers)
    {
        if (aButton == mouse_button::Middle && has_parent())
            parent().mouse_button_pressed(aButton, aPosition + position(), aKeyModifiers);
        else if (aButton == mouse_button::Left && capture_ok(hit_test(aPosition)) && can_capture())
            set_capture(capture_reason::MouseEvent, aPosition);
    }

    template <typename Interface>
    void widget<Interface>::mouse_button_double_clicked(mouse_button aButton, const point& aPosition, key_modifiers_e aKeyModifiers)
    {
        if (aButton == mouse_button::Middle && has_parent())
            parent().mouse_button_double_clicked(aButton, aPosition + position(), aKeyModifiers);
        else if (aButton == mouse_button::Left && capture_ok(hit_test(aPosition)) && can_capture())
            set_capture(capture_reason::MouseEvent, aPosition);
    }

    template <typename Interface>
    void widget<Interface>::mouse_button_released(mouse_button aButton, const point& aPosition)
    {
        if (aButton == mouse_button::Middle && has_parent())
            parent().mouse_button_released(aButton, aPosition + position());
        else if (capturing())
            release_capture(capture_reason::MouseEvent);
    }

    template <typename Interface>
    void widget<Interface>::mouse_moved(const point&, key_modifiers_e)
    {
        // do nothing
    }

    template <typename Interface>
    void widget<Interface>::mouse_entered(const point&)
    {
        // do nothing
    }

    template <typename Interface>
    void widget<Interface>::mouse_left()
    {
        // do nothing
    }

    template <typename Interface>
    neogfx::mouse_cursor widget<Interface>::mouse_cursor() const
    {
        auto const partUnderMouse = part(root().mouse_position() - origin());
        if (part_active(partUnderMouse))
        {
            switch (partUnderMouse.part)
            {
            case widget_part::Grab:
                return mouse_system_cursor::Hand;
            case widget_part::BorderLeft:
                return mouse_system_cursor::SizeWE;
            case widget_part::BorderTopLeft:
                return mouse_system_cursor::SizeNWSE;
            case widget_part::BorderTop:
                return mouse_system_cursor::SizeNS;
            case widget_part::BorderTopRight:
                return mouse_system_cursor::SizeNESW;
            case widget_part::BorderRight:
                return mouse_system_cursor::SizeWE;
            case widget_part::BorderBottomRight:
                return mouse_system_cursor::SizeNWSE;
            case widget_part::BorderBottom:
                return mouse_system_cursor::SizeNS;
            case widget_part::BorderBottomLeft:
                return mouse_system_cursor::SizeNESW;
            case widget_part::GrowBox:
                return mouse_system_cursor::SizeNWSE;
            case widget_part::VerticalScrollbar:
                return mouse_system_cursor::Arrow;
            case widget_part::HorizontalScrollbar:
                return mouse_system_cursor::Arrow;
            }
        }
        if (has_parent())
            return parent().mouse_cursor();
        return mouse_system_cursor::Arrow;
    }

    template <typename Interface>
    bool widget<Interface>::key_pressed(scan_code_e, key_code_e, key_modifiers_e)
    {
        return false;
    }

    template <typename Interface>
    bool widget<Interface>::key_released(scan_code_e, key_code_e, key_modifiers_e)
    {
        return false;
    }

    template <typename Interface>
    bool widget<Interface>::text_input(std::string const&)
    {
        return false;
    }

    template <typename Interface>
    bool widget<Interface>::sys_text_input(std::string const&)
    {
        return false;
    }

    template <typename Interface>
    const i_widget& widget<Interface>::widget_for_mouse_event(const point& aPosition, bool aForHitTest) const
    {
        if (is_root() && (root().style() & window_style::Resize) == window_style::Resize)
        {
            auto const outerRect = to_client_coordinates(non_client_rect());
            auto const innerRect = outerRect.deflated(root().border());
            if (outerRect.contains(aPosition) && !innerRect.contains(aPosition))
                return *this;
        }
        if (non_client_rect().contains(aPosition))
        {
            const i_widget* w = &get_widget_at(aPosition);
            for (; w != this ; w = &w->parent()) 
            {
                if (w->effectively_hidden() || (w->effectively_disabled() && !aForHitTest))
                    continue;
                if (!w->ignore_mouse_events() && mouse_event_location() != neogfx::mouse_event_location::NonClient)
                    break;
                if (!w->ignore_non_client_mouse_events() && mouse_event_location() == neogfx::mouse_event_location::NonClient)
                    break;
                if (mouse_event_location() != neogfx::mouse_event_location::NonClient &&
                    w->ignore_mouse_events() && !w->ignore_non_client_mouse_events(false) &&
                    w->non_client_rect().contains(aPosition) &&
                    !w->client_rect(false).contains(aPosition - w->origin()))
                    break;
            }
            return *w;
        }
        else
            return *this;
    }

    template <typename Interface>
    i_widget& widget<Interface>::widget_for_mouse_event(const point& aPosition, bool aForHitTest)
    {
        return const_cast<i_widget&>(to_const(*this).widget_for_mouse_event(aPosition, aForHitTest));
    }
}
