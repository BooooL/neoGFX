// i18n.hpp
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

namespace neogfx
{
    using neolib::i_string;
    using neolib::string;

    class translation_context
    {
    public:
        translation_context(string const& aContext);
        ~translation_context();
    public:
        static string const& context();
    private:
        static std::vector<string>& context_stack();
    };

    string operator "" _t(const char* aTranslatableString, std::size_t aStringLength);
    string operator "" _t(const char8_t* aTranslatableString, std::size_t aStringLength);

    i_string const& translate(string const& aTranslatableString);

    i_string const& translate(string const& aTranslatableString, string const& aContext);
}

using neogfx::operator "" _t;