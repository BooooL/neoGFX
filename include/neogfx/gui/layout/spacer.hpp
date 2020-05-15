// spacer.hpp
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

#pragma once

#include <neogfx/neogfx.hpp>
#include <neogfx/core/object.hpp>
#include <neogfx/gui/layout/layout_item.hpp>
#include <neogfx/gui/layout/i_spacer.hpp>

namespace neogfx
{
    class i_layout;

    class spacer : public layout_item<object<i_spacer>>
    {
    public:
        struct no_parent : std::logic_error { no_parent() : std::logic_error("neogfx::spacer::no_parent") {} };
        struct margins_unsupported : std::logic_error { margins_unsupported() : std::logic_error("neogfx::spacer::margins_unsupported") {} };
    public:
        typedef i_spacer abstract_type;
    public:
        spacer(neogfx::expansion_policy aExpansionPolicy);
        spacer(i_layout& aParent, neogfx::expansion_policy aExpansionPolicy);
        virtual ~spacer();
    public:
        bool is_layout() const override;
        const i_layout& as_layout() const override;
        i_layout& as_layout() override;
        bool is_widget() const override;
        const i_widget& as_widget() const override;
        i_widget& as_widget() override;
    public:
        bool has_parent_layout() const override;
        const i_layout& parent_layout() const override;
        i_layout& parent_layout() override;
        void set_parent_layout(i_layout* aParentLayout) override;
        bool has_layout_owner() const override;
        const i_widget& layout_owner() const override;
        i_widget& layout_owner() override;
        void set_layout_owner(i_widget* aOwner) override;
        bool is_proxy() const override;
        const i_layout_item_proxy& proxy_for_layout() const override;
        i_layout_item_proxy& proxy_for_layout() override;
    public:
        neogfx::expansion_policy expansion_policy() const override;
        void set_expansion_policy(neogfx::expansion_policy aExpansionPolicy) override;
    public:
        bool high_dpi() const override;
        dimension dpi_scale_factor() const override;
    public:
        point position() const override;
        void set_position(const point& aPosition) override;
        size extents() const override;
        void set_extents(const size& aExtents) override;
        bool has_size_policy() const override;
        neogfx::size_policy size_policy() const override;
        using i_geometry::set_size_policy;
        void set_size_policy(const optional_size_policy& aSizePolicy, bool aUpdateLayout = true) override;
        bool has_weight() const override;
        size weight() const override;
        void set_weight(const optional_size& aWeight, bool aUpdateLayout = true) override;
        bool has_minimum_size() const override;
        size minimum_size(const optional_size& aAvailableSpace = optional_size()) const override;
        void set_minimum_size(const optional_size& aMinimumSize, bool aUpdateLayout = true) override;
        bool has_maximum_size() const override;
        size maximum_size(const optional_size& aAvailableSpace = optional_size()) const override;
        void set_maximum_size(const optional_size& aMaximumSize, bool aUpdateLayout = true) override;
        bool has_fixed_size() const override;
        size fixed_size() const override;
        void set_fixed_size(const optional_size& aFixedSize, bool aUpdateLayout = true) override;
    public:
        neogfx::margins margins() const override;
        bool has_margins() const override;
        void set_margins(const optional_margins& aMargins, bool aUpdateLayout = true) override;
    public:
        bool device_metrics_available() const override;
        const i_device_metrics& device_metrics() const override;
    public:
        void layout_as(const point& aPosition, const size& aSize) override;
    public:
        bool visible() const override;
    private:
        i_layout* iParentLayout;
        point iPosition;
        size iExtents;
        optional_size_policy iSizePolicy;
        optional_size iMinimumSize;
        optional_size iMaximumSize;
        optional_size iFixedSize;
        optional_margins iMargins;
        neogfx::expansion_policy iExpansionPolicy;
        optional_size iWeight;
    };

    class horizontal_spacer : public spacer
    {
    public:
        horizontal_spacer();
        horizontal_spacer(i_layout& aParent);
    };

    class vertical_spacer : public spacer
    {
    public:
        vertical_spacer();
        vertical_spacer(i_layout& aParent);
    };
}