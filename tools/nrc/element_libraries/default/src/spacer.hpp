// spacer.hpp
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
#include <neogfx/tools/nrc/ui_element.hpp>
#include <neogfx/gui/layout/i_spacer.hpp>

namespace neogfx::nrc
{
    template <ui_element_type SpacerType>
    class basic_spacer : public ui_element<>
    {
    public:
        basic_spacer(const i_ui_element_parser& aParser, i_ui_element& aParent) :
            ui_element<>{ aParser, aParent, aParser.get_optional<neolib::string>("id"), SpacerType }
        {
        }
    public:
        const neolib::i_string& header() const override
        {
            static const neolib::string sHeader = "neogfx/gui/layout/spacer.hpp";
            return sHeader;
        }
    public:
        void parse(const neolib::i_string& aName, const data_t& aData) override
        {
            ui_element<>::parse(aName, aData);
        }
        void parse(const neolib::i_string& aName, const array_data_t& aData) override
        {
            ui_element<>::parse(aName, aData);
        }
    protected:
        void emit() const override
        {
        }
        void emit_preamble() const override
        {
            if constexpr (SpacerType == ui_element_type::Spacer)
                emit("  spacer %1%;\n", id());
            else if constexpr (SpacerType == ui_element_type::VerticalSpacer)
                emit("  vertical_spacer %1%;\n", id());
            else if constexpr (SpacerType == ui_element_type::HorizontalSpacer)
                emit("  horizontal_spacer %1%;\n", id());
            ui_element<>::emit_preamble();
        }
        void emit_ctor() const override
        {
            if constexpr (SpacerType != ui_element_type::Spacer)
            {
                if (iExpansionPolicy)
                    throw element_ill_formed(id());
                ui_element<>::emit_generic_ctor();
            }
            else
            {
                if (iExpansionPolicy)
                    emit(",\n"
                        "   %1%{ %2%, %3% }", id(), parent().id(), enum_to_string("expansion_policy", *iExpansionPolicy));
                else if ((parent().type() & ui_element_type::MASK_RESERVED_SPECIFIC) == ui_element_type::VerticalLayout)
                    emit(",\n"
                        "   %1%{ %2%, %3% }", id(), parent().id(), enum_to_string("expansion_policy", expansion_policy::ExpandVertically));
                else if ((parent().type() & ui_element_type::MASK_RESERVED_SPECIFIC) == ui_element_type::HorizontalLayout)
                    emit(",\n"
                        "   %1%{ %2%, %3% }", id(), parent().id(), enum_to_string("expansion_policy", expansion_policy::ExpandHorizontally));
                else if ((parent().type() & ui_element_type::MASK_RESERVED_SPECIFIC) == ui_element_type::GridLayout)
                    emit(",\n"
                        "   %1%{ %2%, %3% }", id(), parent().id(), enum_to_string("expansion_policy", expansion_policy::ExpandVertically | expansion_policy::ExpandHorizontally));
                else
                    throw element_ill_formed(id());
            }
            ui_element<>::emit_ctor();
        }
        void emit_body() const override
        {
            ui_element<>::emit_body();
        }
    protected:
        using ui_element<>::emit;
    private:
        std::optional<expansion_policy> iExpansionPolicy;
    };

    typedef basic_spacer<ui_element_type::Spacer> spacer;
    typedef basic_spacer<ui_element_type::VerticalSpacer> vertical_spacer;
    typedef basic_spacer<ui_element_type::HorizontalSpacer> horizontal_spacer;
}
