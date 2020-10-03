// drag_drop.cpp
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
#include <neogfx/app/drag_drop.hpp>

namespace neogfx
{
    drag_drop_object::drag_drop_object(drag_drop_object_type aType) : 
        iType{ aType }
    {
    }

    drag_drop_object_type drag_drop_object::ddo_type() const
    {
        return iType;
    }


    drag_drop_source::drag_drop_source()
    {
    }

    bool drag_drop_source::drag_drop_active() const
    {
        throw std::logic_error("not yet implemented");
    }

    void drag_drop_source::start_drag_drop()
    {
        throw std::logic_error("not yet implemented");
    }

    void drag_drop_source::end_drag_drop()
    {
        throw std::logic_error("not yet implemented");
    }


    drag_drop_target::drag_drop_target()
    {
    }

    bool drag_drop_target::can_accept(i_drag_drop_object const& aOobject) const
    {
        throw std::logic_error("not yet implemented");
    }

    void drag_drop_target::accept(i_drag_drop_object const& aOobject)
    {
        throw std::logic_error("not yet implemented");
    }

    bool drag_drop_target::is_widget() const
    {
        return false;
    }

    i_widget const& drag_drop_target::as_widget() const
    {
        throw drag_drop_target_not_a_widget();
    }

    i_widget& drag_drop_target::as_widget()
    {
        throw drag_drop_target_not_a_widget();
    }


    drag_drop::drag_drop()
    {
    }

    void drag_drop::register_source(i_drag_drop_source& aSource)
    {
        auto existing = std::find(iSources.begin(), iSources.end(), &aSource);
        if (existing == iSources.end())
        {
            iSources.push_back(&aSource);
            SourceRegistered.trigger(aSource);
        }
    }

    void drag_drop::unregister_source(i_drag_drop_source& aSource)
    {
        auto existing = std::find(iSources.begin(), iSources.end(), &aSource);
        if (existing != iSources.end())
        {
            iSources.erase(existing);
            SourceUnregistered.trigger(aSource);
        }
    }

    void drag_drop::register_target(i_drag_drop_target& aTarget)
    {
        auto existing = std::find(iTargets.begin(), iTargets.end(), &aTarget);
        if (existing == iTargets.end())
        {
            iTargets.push_back(&aTarget);
            TargetRegistered.trigger(aTarget);
        }
    }

    void drag_drop::unregister_target(i_drag_drop_target& aTarget)
    {
        auto existing = std::find(iTargets.begin(), iTargets.end(), &aTarget);
        if (existing != iTargets.end())
        {
            iTargets.erase(existing);
            TargetUnregistered.trigger(aTarget);
        }
    }

    bool drag_drop::is_target_for(i_drag_drop_object const& aObject) const
    {
        return find_target(aObject) != nullptr;
    }

    bool drag_drop::is_target_at(i_drag_drop_object const& aObject, i_surface const& aSurface, point const& aPosition) const
    {
        return find_target(aObject, aSurface, aPosition) != nullptr;
    }

    i_drag_drop_target& drag_drop::target_for(i_drag_drop_object const& aObject) const
    {
        auto existing = find_target(aObject);
        if (existing != nullptr)
            return *existing;
        throw drag_drop_target_not_found();
    }

    i_drag_drop_target& drag_drop::target_at(i_drag_drop_object const& aObject, i_surface const& aSurface, point const& aPosition) const
    {
        auto existing = find_target(aObject, aSurface, aPosition);
        if (existing != nullptr)
            return *existing;
        throw drag_drop_target_not_found();
    }

    i_drag_drop_target* drag_drop::find_target(i_drag_drop_object const& aObject) const
    {
        for (auto const& target : iTargets)
            if (target->can_accept(aObject))
                return target;
        return nullptr;
    }

    i_drag_drop_target* drag_drop::find_target(i_drag_drop_object const& aObject, i_surface const& aSurface, point const& aPosition) const
    {
        if (aSurface.is_window())
        {
            auto const& w = aSurface.as_surface_window().as_widget().get_widget_at(aPosition);
            for (auto const& target : iTargets)
                if (target->can_accept(aObject) &&
                    target->is_widget() &&
                    target->as_widget().same_surface(w) &&
                    (&target->as_widget() == &w || target->as_widget().is_ancestor_of(w)))
                    return target;
            return nullptr;
        }
        return find_target(aObject);
    }
}