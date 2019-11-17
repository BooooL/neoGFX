// i_ui_element.hpp
/*
neoGFX Resource Compiler
Copyright(C) 2019 Leigh Johnston

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
#include <neolib/neolib.hpp>
#include <neolib/i_reference_counted.hpp>
#include <neolib/i_string.hpp>
#include <neolib/i_vector.hpp>
#include <neogfx/app/i_app.hpp>
#include <neogfx/gui/widget/i_widget.hpp>
#include <neogfx/gui/layout/i_layout.hpp>
#include <neogfx/tools/nrc/i_ui_element_parser.hpp>

namespace neogfx::nrc
{
    enum class ui_element_type : uint32_t
    {
        MASK_CATEGORY   = 0x000000FF,
        MASK_TYPE       = 0xFFFFFF00,

        App             = 0x00000001,
        Action          = 0x00000002,
        Widget          = 0x00000003,
        Layout          = 0x00000004,

        Window          = 0x00000100 | Widget,
    };

    inline constexpr ui_element_type operator|(ui_element_type aLhs, ui_element_type aRhs)
    {
        return static_cast<ui_element_type>(static_cast<uint32_t>(aLhs) | static_cast<uint32_t>(aRhs));
    }

    inline constexpr ui_element_type operator&(ui_element_type aLhs, ui_element_type aRhs)
    {
        return static_cast<ui_element_type>(static_cast<uint32_t>(aLhs)& static_cast<uint32_t>(aRhs));
    }

    inline constexpr ui_element_type category(ui_element_type aType)
    {
        return aType & ui_element_type::MASK_CATEGORY;
    }

    inline constexpr bool is_widget_or_layout(ui_element_type aType)
    {
        switch (category(aType))
        {
        case ui_element_type::Widget:
        case ui_element_type::Layout:
            return true;
        default:
            return false;
        }
    }

    class i_ui_element : public neolib::i_reference_counted
    {
    public:
        struct no_parent : std::logic_error { no_parent() : std::logic_error{ "neogfx::nrc::i_ui_element::no_parent" } {} };
        struct wrong_type : std::logic_error { wrong_type() : std::logic_error{ "neogfx::nrc::i_ui_element::wrong_type" } {} };
        struct ui_element_not_found : std::runtime_error { ui_element_not_found() : std::runtime_error{ "neogfx::nrc::i_ui_element::ui_element_not_found" } {} };
    public:
        typedef neolib::i_vector<neolib::i_ref_ptr<i_ui_element>> children_t;
        typedef i_ui_element_parser::data_t data_t;
        typedef i_ui_element_parser::array_data_t array_data_t;
    public:
        virtual ~i_ui_element() {}
    public:
        virtual const i_ui_element_parser& parser() const = 0;
    public:
        virtual const neolib::i_string& id() const = 0;
        virtual ui_element_type type() const = 0;
    public:
        virtual bool has_parent() const = 0;
        virtual const i_ui_element& parent() const = 0;
        virtual i_ui_element& parent() = 0;
        virtual const children_t& children() const = 0;
        virtual children_t& children() = 0;
    public:
        virtual void parse(const neolib::i_string& aType, const data_t& aData) = 0;
        virtual void parse(const neolib::i_string& aType, const array_data_t& aData) = 0;
        virtual void emit() const = 0;
        virtual void emit_preamble() const = 0;
        virtual void emit_ctor() const = 0;
        virtual void emit_body() const = 0;
    public:
        virtual void instantiate(i_app& aApp) = 0;
        virtual void instantiate(i_widget& aWidget) = 0;
        virtual void instantiate(i_layout& aLayout) = 0;
    public:
        const i_ui_element& find(const neolib::i_string& aId) const
        {
            if (id() == aId)
                return *this;
            if (has_parent())
                return parent().find(aId);
            throw ui_element_not_found();
        }
        i_ui_element& find(const neolib::i_string& aId)
        {
            return const_cast<i_ui_element&>(to_const(*this).find(aId));
        }
    };
}
