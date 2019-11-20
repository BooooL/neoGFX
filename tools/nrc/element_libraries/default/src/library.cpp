// library.hpp
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
#include "library.hpp"
#include "app.hpp"
#include "action.hpp"
#include "window.hpp"
#include "status_bar.hpp"
#include "menu_bar.hpp"
#include "menu.hpp"
#include "toolbar.hpp"
#include "tab_page_container.hpp"
#include "tab_page.hpp"
#include "toolbar.hpp"
#include "layout.hpp"

namespace neogfx::nrc
{
    default_ui_element_library::default_ui_element_library(neolib::i_application& aApplication, const std::string& aPluginPath) :
        iApplication{ aApplication },
        iPluginPath{ aPluginPath },
        iElements{ 
            "app", "action", "window", "status_bar", "menu_bar", "menu", "toolbar", "tab_page_container", "tab_page", 
            "vertical_layout", "horizontal_layout", "grid_layout", "flow_layout", "stack_layout", "border_layout" }
    {
    }

    const default_ui_element_library::elements_t& default_ui_element_library::elements() const
    {
        return iElements;
    }

    i_ui_element* default_ui_element_library::do_create_element(const i_ui_element_parser& aParser, const neolib::i_string& aElementType)
    {
        static const std::map<std::string, std::function<i_ui_element*(const i_ui_element_parser&)>> sFactoryMethods =
        {
            { "app", [](const i_ui_element_parser& aParser) -> i_ui_element* { return new app{ aParser }; } }
        };
        auto method = sFactoryMethods.find(aElementType);
        if (method != sFactoryMethods.end())
            return (method->second)(aParser);
        throw unknown_element_type();
    }

    i_ui_element* default_ui_element_library::do_create_element(i_ui_element& aParent, const neolib::i_string& aElementType)
    {
        static const std::map<std::string, std::function<i_ui_element*(i_ui_element&)>> sFactoryMethods =
        {
            { "action", [](i_ui_element& aParent) -> i_ui_element* { return new action{ aParent }; } },
            { "window", [](i_ui_element& aParent) -> i_ui_element* { return new window{ aParent }; } },
            { "status_bar", [](i_ui_element& aParent) -> i_ui_element* { return new status_bar{ aParent }; } },
            { "menu_bar", [](i_ui_element& aParent) -> i_ui_element* { return new menu_bar{ aParent }; } },
            { "menu", [](i_ui_element& aParent) -> i_ui_element* { return new menu{ aParent }; } },
            { "toolbar", [](i_ui_element& aParent) -> i_ui_element* { return new toolbar{ aParent }; } },
            { "tab_page_container", [](i_ui_element& aParent) -> i_ui_element* { return new tab_page_container{ aParent }; } },
            { "tab_page", [](i_ui_element& aParent) -> i_ui_element* { return new tab_page{ aParent }; } },
            { "vertical_layout", [](i_ui_element& aParent) -> i_ui_element* { return new vertical_layout{ aParent }; } },
            { "horizontal_layout", [](i_ui_element& aParent) -> i_ui_element* { return new horizontal_layout{ aParent }; } },
            { "grid_layout", [](i_ui_element& aParent) -> i_ui_element* { return new grid_layout{ aParent }; } },
            { "flow_layout", [](i_ui_element& aParent) -> i_ui_element* { return new flow_layout{ aParent }; } },
            { "stack_layout", [](i_ui_element& aParent) -> i_ui_element* { return new stack_layout{ aParent }; } },
            { "border_layout", [](i_ui_element& aParent) -> i_ui_element* { return new border_layout{ aParent }; } }
        };
        auto method = sFactoryMethods.find(aElementType);
        if (method != sFactoryMethods.end())
            return (method->second)(aParent);
        throw unknown_element_type();
    }
}
