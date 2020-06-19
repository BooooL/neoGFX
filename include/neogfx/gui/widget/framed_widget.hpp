// framed_widget.hpp
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
#include "widget.hpp"

namespace neogfx
{
    enum class frame_style
    {
        NoFrame,
        DottedFrame,
        DashedFrame,
        SolidFrame,
        ContainerFrame,
        DoubleFrame,
        GrooveFrame,
        RidgeFrame,
        InsetFrame,
        OutsetFrame,
        HiddenFrame,
        WindowFrame
    };

    class framed_widget : public widget
    {
    public:
        framed_widget(frame_style aStyle = frame_style::SolidFrame, dimension aLineWidth = 1.0);
        framed_widget(const framed_widget&) = delete;
        framed_widget(i_widget& aParent, frame_style aStyle = frame_style::SolidFrame, dimension aLineWidth = 1.0);
        framed_widget(i_layout& aLayout, frame_style aStyle = frame_style::SolidFrame, dimension aLineWidth = 1.0);
        ~framed_widget();
    public:
        rect client_rect(bool aIncludePadding = true) const override;
    public:
        size minimum_size(const optional_size& aAvailableSpace = optional_size()) const override;
        size maximum_size(const optional_size& aAvailableSpace = optional_size()) const override;
    public:
        bool transparent_background() const override;
        void paint_non_client(i_graphics_context& aGc) const override;
        void paint(i_graphics_context& aGc) const override;
    public:
        void set_frame_style(frame_style aStyle);
    public:
        virtual bool has_frame_color() const;
        virtual color frame_color() const;
        virtual void set_frame_color(const optional_color& aFrameColor = optional_color{});
        virtual color inner_frame_color() const;
    public:
        dimension line_width() const;
        dimension effective_frame_width() const;
    private:
        frame_style iStyle;
        dimension iLineWidth;
        optional_color iFrameColor;
    };
}