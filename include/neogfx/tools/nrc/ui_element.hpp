// ui_element.hpp
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
#include <algorithm>
#include <boost/format.hpp>
#include <neolib/reference_counted.hpp>
#include <neolib/optional.hpp>
#include <neolib/vector.hpp>
#include <neolib/string.hpp>
#include <neogfx/core/units.hpp>
#include <neogfx/gui/layout/i_geometry.hpp>
#include <neogfx/gui/widget/i_widget.hpp>
#include <neogfx/gui/widget/label.hpp>
#include <neogfx/gui/widget/image_widget.hpp>
#include <neogfx/gui/widget/text_field.hpp>
#include <neogfx/tools/nrc/i_ui_element.hpp>

namespace neogfx::nrc
{
    template <typename Base = i_ui_element>
    class ui_element : public neolib::reference_counted<Base>
    {
        typedef ui_element<Base> self_type;
        typedef neolib::reference_counted<Base> base_type;
    public:
        using i_ui_element::no_parent;
        using i_ui_element::wrong_type;
    public:
        typedef neolib::ref_ptr<i_ui_element> element_ptr_t;
        typedef neolib::vector<element_ptr_t> children_t;
        using i_ui_element::data_t;
        using i_ui_element::array_data_t;
    protected:
        typedef std::set<std::string> data_names_t;
    public:
        ui_element(const i_ui_element_parser& aParser, ui_element_type aType) :
            iParser{ aParser }, iParent{ nullptr }, iMemberElement{ false }, iId{ aParser.get_optional<neolib::string>("id") }, iAnonymousIdCounter{ 0u }, iType{ aType }
        {
            init();
        }
        ui_element(const i_ui_element_parser& aParser, ui_element_type aType, const neolib::optional<neolib::string>& aId) :
            iParser{ aParser }, iParent{ nullptr }, iMemberElement{ false }, iId{ aId }, iAnonymousIdCounter{ 0u }, iType{ aType }
        {
            init();
        }
        ui_element(const i_ui_element_parser& aParser, i_ui_element& aParent, ui_element_type aType) :
            iParser{ aParser }, iParent{ &aParent }, iMemberElement{ false }, iAnonymousIdCounter{ 0u }, iId{ aParser.get_optional<neolib::string>("id") }, iType{ aType }
        {
            init();
            parent().children().push_back(neolib::ref_ptr<i_ui_element>{ this });
        }
        ui_element(const i_ui_element_parser& aParser, i_ui_element& aParent, ui_element_type aType, const neolib::optional<neolib::string>& aId) :
            iParser{ aParser }, iParent{ &aParent }, iMemberElement{ false }, iAnonymousIdCounter{ 0u }, iId{ aId }, iType{ aType }
        {
            init();
            parent().children().push_back(neolib::ref_ptr<i_ui_element>{ this });
        }
        ui_element(const i_ui_element_parser& aParser, i_ui_element& aParent, member_element_t, ui_element_type aType) :
            iParser{ aParser }, iParent{ &aParent }, iMemberElement{ true }, iAnonymousIdCounter{ 0u }, iId{}, iType{ aType }
        {
            init();
            parent().children().push_back(neolib::ref_ptr<i_ui_element>{ this });
        }
        ~ui_element()
        {
        }
    public:
        const i_ui_element_parser& parser() const override
        {
            return iParser;
        }
    public:
        bool is_member_element() const override
        {
            return iMemberElement;
        }
        bool anonymous() const override
        {
            return !iId;
        }
        const neolib::i_string& id() const override
        {
            if (!anonymous())
                return *iId;
            return anonymous_id();
        }
        const neolib::i_string& anonymous_id() const override
        {
            if (!iAnonymousId)
            {
                if (!is_member_element())
                {
                    if (has_parent())
                        iAnonymousId = parent().generate_anonymous_id();
                    else
                        iAnonymousId = parser().generate_anonymous_id();
                }
                else
                {
                    switch (type() & ui_element_type::MASK_RESERVED_SPECIFIC)
                    {
                    case ui_element_type::Label:
                        iAnonymousId = parent().id() + ".label()";
                        break;
                    case ui_element_type::ImageWidget:
                        iAnonymousId = parent().id() + ".image_widget()";
                        break;
                    default:
                        throw unsupported_member_element();
                    }
                }
            }
            return *iAnonymousId;
        }
        using base_type::generate_anonymous_id;
        void generate_anonymous_id(neolib::i_string& aNewAnonymousId) const override
        {
            aNewAnonymousId = neolib::string{ id() + "_" + boost::lexical_cast<std::string>(++iAnonymousIdCounter) };
        }
        ui_element_type type() const override
        {
            return iType;
        }
    public:
        bool has_parent() const override
        {
            return iParent != nullptr;
        }
        const i_ui_element& parent() const override
        {
            if (has_parent())
                return *iParent;
            throw no_parent();
        }
        i_ui_element& parent() override
        {
            return const_cast<i_ui_element&>(to_const(*this).parent());
        }
        const children_t& children() const override
        {
            return iChildren;
        }
        children_t& children() override
        {
            return iChildren;
        }
    public:
        void instantiate(i_app& aApp) override
        {
        }
        void instantiate(i_widget& aWidget) override
        {
        }
        void instantiate(i_layout& aLayout) override
        {
        }
    protected:
        using i_ui_element::enum_to_string;
        using i_ui_element::get_enum;
        using i_ui_element::get_scalar;
        using i_ui_element::get_scalars;
        using i_ui_element::emplace_2;
        using i_ui_element::emplace_4;
        using i_ui_element::get_color;
    protected:
        const data_names_t& data_names() const
        {
            return iDataNames;
        }
        void add_data_names(data_names_t aNames)
        {
            iDataNames.merge(aNames);
        }
        void parse(const neolib::i_string& aName, const data_t& aData) override
        {
            if (data_names().find(aName) == data_names().end())
            {
                std::cerr << parser().source_location() << ": warning: nrc: Unknown element key '" << aName << "' in element '" << id() << "'." << std::endl;
                return;
            }
            if (aName == "weight")
                iWeight.emplace(get_scalar<double>(aData));
            else if (aName == "size_policy")
                iSizePolicy = get_enum<size_constraint>(aData);
            else if (aName == "alignment")
                iAlignment = get_enum<alignment>(aData);
            else if (aName == "size")
                iFixedSize.emplace(get_scalar<length>(aData));
            else if (aName == "minimum_size")
                iMinimumSize.emplace(get_scalar<length>(aData));
            else if (aName == "minimum_width")
                iMinimumWidth.emplace(get_scalar<length>(aData));
            else if (aName == "minimum_height")
                iMinimumHeight.emplace(get_scalar<length>(aData));
            else if (aName == "maximum_size")
                iMaximumSize.emplace(get_scalar<length>(aData));
            else if (aName == "maximum_width")
                iMaximumWidth.emplace(get_scalar<length>(aData));
            else if (aName == "maximum_height")
                iMaximumHeight.emplace(get_scalar<length>(aData));
            else if (aName == "margin")
                iMargin.emplace(get_scalar<length>(aData));
            else if (aName == "enabled")
                iEnabled = aData.get<bool>();
            else if (aName == "disabled")
                iEnabled = !aData.get<bool>();
            else if (aName == "focus_policy")
                iFocusPolicy.first = get_enum<focus_policy>(aData);
            else if (aName == "text")
                iText = aData.get<neolib::i_string>();
            else if (aName == "label")
                iLabelText = aData.get<neolib::i_string>();
            else if (aName == "image" || (aName == "uri" && (type() & ui_element_type::MASK_RESERVED_SPECIFIC) == ui_element_type::ImageWidget))
                iImage = aData.get<neolib::i_string>();
            else if (aName == "aspect_ratio")
                iAspectRatio = get_enum<aspect_ratio>(aData);
            else if (aName == "placement")
            {
                if ((type() & ui_element_type::MASK_RESERVED_SPECIFIC) != ui_element_type::TextField)
                {
                    if ((type() & ui_element_type::MASK_RESERVED_SPECIFIC) == ui_element_type::Label || (type() & ui_element_type::HasLabel) == ui_element_type::HasLabel)
                        iLabelPlacement = get_enum<label_placement>(aData);
                    else if ((type() & ui_element_type::MASK_RESERVED_SPECIFIC) == ui_element_type::ImageWidget || (type() & ui_element_type::HasImage) == ui_element_type::HasImage)
                        iImagePlacement = get_enum<cardinal>(aData);
                }
                else
                    iTextFieldPlacement = get_enum<text_field_placement>(aData);
            }
            else if (aName == "foreground_color")
                iForegroundColor = get_color(aData);
            else if (aName == "background_color")
                iBackgroundColor = get_color(aData);
            else if (aName == "opacity")
                iOpacity = aData.get<double>();
            else if (aName == "transparency")
                iOpacity = 1.0 - aData.get<double>();
        }
        void parse(const neolib::i_string& aName, const array_data_t& aArrayData) override
        {
            if (data_names().find(aName) == data_names().end())
            {
                std::cerr << parser().source_location() << ": warning: nrc: Unknown element key '" << aName << "' in element '" << id() << "'." << std::endl;
                return;
            }
            if (aName == "size_policy" && !aArrayData.empty())
                iSizePolicy = size_policy::from_string(
                    aArrayData[0u].get<neolib::i_string>().to_std_string(),
                    aArrayData[std::min<std::size_t>(1u, aArrayData.size() - 1u)].get<neolib::i_string>().to_std_string());
            else if (aName == "alignment")
                iAlignment = get_enum<alignment>(aArrayData);
            else if (aName == "size")
                emplace_2<length>("size", iFixedSize);
            else if (aName == "minimum_size")
                emplace_2<length>("minimum_size", iMinimumSize);
            else if (aName == "maximum_size")
                emplace_2<length>("maximum_size", iMaximumSize);
            else if (aName == "margin")
                emplace_4<length>("margin", iMargin);
            else if (aName == "weight")
                emplace_2<double>("weight", iWeight);
            else if (aName == "focus_policy")
                iFocusPolicy.first = get_enum<focus_policy>(aArrayData, iFocusPolicy.second, "Default");
            else if (aName == "foreground_color")
                iForegroundColor = get_color(aArrayData);
            else if (aName == "background_color")
                iBackgroundColor = get_color(aArrayData);
        }
        void emit_preamble() const override
        {
            for (auto const& child : children())
                child->emit_preamble();
        }
        void emit_ctor() const override
        {
            for (auto const& child : children())
                child->emit_ctor();
        }
        void emit_body() const override
        {
            if (iSizePolicy)
                emit("   %1%.set_size_policy(%2%);\n", id(), *iSizePolicy);
            if (iAlignment)
                emit("   %1%.set_alignment(%2%);\n", id(), enum_to_string("alignment", *iAlignment));
            if (iFixedSize)
                emit("   %1%.set_fixed_size(size{ %2%, %3% });\n", id(), iFixedSize->cx, iFixedSize->cy);
            if (iMinimumSize)
                emit("   %1%.set_minimum_size(size{ %2%, %3% });\n", id(), iMinimumSize->cx, iMinimumSize->cy);
            if (iMinimumWidth)
                emit("   %1%.set_minimum_width(%2%);\n", id(), *iMinimumWidth);
            if (iMinimumHeight)
                emit("   %1%.set_minimum_height(%2%);\n", id(), *iMinimumHeight);
            if (iMaximumSize)
                emit("   %1%.set_maximum_size(size{ %2%, %3% });\n", id(), iMaximumSize->cx, iMaximumSize->cy);
            if (iMaximumWidth)
                emit("   %1%.set_maximum_width(%2%);\n", id(), *iMaximumWidth);
            if (iMaximumHeight)
                emit("   %1%.set_maximum_height(%2%);\n", id(), *iMaximumHeight);
            if (iWeight)
                emit("   %1%.set_weight(size{ %2%, %3% });\n", id(), iWeight->cx, iWeight->cy);
            if (iMargin)
            {
                auto const& margin = *iMargin;
                if (margin.left == margin.right && margin.top == margin.bottom)
                {
                    if (margin.left == margin.top)
                        emit("   %1%.set_margins(margins{ %2% });\n", id(), margin.left);
                    else
                        emit("   %1%.set_margins(margins{ %2%, %3% });\n", id(), margin.left, margin.top);
                }
                else 
                    emit("   %1%.set_margins(margins{ %2%, %3%, %4%, %5% });\n", id(), margin.left, margin.top, margin.right, margin.bottom);
            }
            if (iEnabled)
                emit("   %1%.%2%();\n", id(), *iEnabled ? "enable" : "disable");
            if (iFocusPolicy.first)
            {
                if (!iFocusPolicy.second)
                    emit("   %1%.set_focus_policy(%2%);\n", id(), enum_to_string("focus_policy", *iFocusPolicy.first));
                else
                    emit("   %1%.set_focus_policy(%1%.focus_policy() | %2%);\n", id(), enum_to_string("focus_policy", *iFocusPolicy.first));
            }
            if (iLabelPlacement)
                emit("   %1%.set_placement(%2%);\n", id(), enum_to_string("label_placement", *iLabelPlacement));
            if (iTextFieldPlacement)
                emit("   %1%.set_placement(%2%);\n", id(), enum_to_string("text_field_placement", *iTextFieldPlacement));
            if (iImage)
                emit("   %1%.set_image(image{ \"%2%\" });\n", id(), *iImage);
            if (iAspectRatio)
                emit("   %1%.set_aspect_ratio(%2%);\n", id(), enum_to_string("aspect_ratio", *iAspectRatio));
            if (iImagePlacement)
                emit("   %1%.set_placement(%2%);\n", id(), enum_to_string("cardinal", *iImagePlacement));
            if (iText)
                emit("   %1%.set_text(\"%2%\"_t);\n", id(), *iText);
            if (iLabelText)
                emit("   %1%.label().set_text(\"%2%\"_t);\n", id(), *iLabelText);
            if (iOpacity)
                emit("   %1%.set_opacity(%2%);\n", id(), *iOpacity);
            if (iForegroundColor)
                emit("   %1%.set_foreground_color(color{ %2% });\n", id(), *iForegroundColor);
            if (iBackgroundColor)
                emit("   %1%.set_background_color(color{ %2% });\n", id(), *iBackgroundColor);
            for (auto const& child : children())
                child->emit_body();
        }
    protected:
        neolib::string layout() const
        {
            if ((parent().type() & ui_element_type::MASK_RESERVED_GENERIC) == ui_element_type::Window)
            {
                switch (type() & ui_element_type::MASK_RESERVED_SPECIFIC)
                {
                case ui_element_type::MenuBar:
                    return ".menu_layout()";
                case ui_element_type::Toolbar:
                    return ".toolbar_layout()";
                case ui_element_type::StatusBar:
                    return ".status_bar_layout()";
                default:
                    return ".client_layout()";
                }
            }
            else if ((parent().type() & ui_element_type::MASK_RESERVED_SPECIFIC) == ui_element_type::GroupBox)
                return ".item_layout()";
            else
                return "";
        }
        void emit_generic_ctor() const
        {
            if (is_member_element())
                return;
            emit(",\n"
                "   %1%{ %2%%3% }", id(), parent().id(), layout());
        }
        void emit_generic_ctor(const std::optional<neolib::string>& aText) const
        {
            if (is_member_element())
                return;
            emit(",\n"
                "   %1%{ %2%%3%, \"%4%\"_t }", id(), parent().id(), layout(), *aText);
        }
        template <typename Enum>
        std::enable_if_t<std::is_enum_v<Enum>, void> emit_generic_ctor(const std::string& aEnumName, Enum aEnum) const
        {
            if (is_member_element())
                return;
            emit(",\n"
                "   %1%{ %2%%3%, %4% }", id(), parent().id(), layout(), enum_to_string(aEnumName, aEnum));
        }
        template <typename T>
        void emit_generic_ctor(const T& aArgument) const
        {
            if (is_member_element())
                return;
            emit(",\n"
                "   %1%{ %2%%3%, %4% }", id(), parent().id(), layout(), aArgument);
        }
    protected:
        void emit(const std::string& aArgument) const
        {
            parser().emit(aArgument);
        }
        template <typename... Args>
        void emit(const std::string& aFormat, const Args&... aArguments) const
        {
            parser().emit(aFormat, base_type::convert_emit_argument(aArguments)...);
        }
    protected:
    private:
        void init()
        {
            if ((type() & ui_element_type::Widget) == ui_element_type::Widget)
                add_data_names({ "enabled", "disabled", "focus_policy" });
            if ((type() & ui_element_type::HasGeometry) == ui_element_type::HasGeometry)
                add_data_names({ "size_policy", "margin", "minimum_size", "maximum_size", "size", "minimum_width", "minimum_height", "maximum_width", "maximum_height", "weight" });
            if ((type() & ui_element_type::HasAlignment) == ui_element_type::HasAlignment)
                add_data_names({ "alignment" });
            if ((type() & (ui_element_type::HasText | ui_element_type::HasLabel)) != ui_element_type::None)
                add_data_names({ "text" });
            if ((type() & (ui_element_type::HasImage | ui_element_type::HasLabel)) != ui_element_type::None)
                add_data_names({ "image", "aspect_ratio", "placement" });
            if ((type() & ui_element_type::MASK_RESERVED_SPECIFIC) == ui_element_type::ImageWidget)
                add_data_names({ "uri" });
            if ((type() & ui_element_type::HasColor) == ui_element_type::HasColor)
                add_data_names({ "foreground_color", "background_color", "opacity", "transparency" });
        }
    private:
        const i_ui_element_parser& iParser;
        i_ui_element* iParent;
        bool iMemberElement;
        data_names_t iDataNames;
        neolib::optional<neolib::string> iId;
        mutable neolib::optional<neolib::string> iAnonymousId;
        mutable uint32_t iAnonymousIdCounter;
        ui_element_type iType;
        children_t iChildren;
        std::optional<size_policy> iSizePolicy;
        std::optional<alignment> iAlignment;
        std::optional<basic_size<length>> iFixedSize;
        std::optional<basic_size<length>> iMinimumSize;
        std::optional<basic_size<length>> iMaximumSize;
        std::optional<length> iMinimumWidth;
        std::optional<length> iMinimumHeight;
        std::optional<length> iMaximumWidth;
        std::optional<length> iMaximumHeight;
        std::optional<size> iWeight;
        std::optional<basic_margins<length>> iMargin;
        std::optional<bool> iEnabled;
        std::pair<std::optional<focus_policy>, bool> iFocusPolicy;
        std::optional<label_placement> iLabelPlacement;
        std::optional<text_field_placement> iTextFieldPlacement;
        std::optional<string> iText;
        std::optional<string> iLabelText;
        std::optional<string> iImage;
        std::optional<aspect_ratio> iAspectRatio;
        std::optional<cardinal> iImagePlacement;
        std::optional<double> iOpacity;
        std::optional<color> iForegroundColor;
        std::optional<color> iBackgroundColor;
    };
}
