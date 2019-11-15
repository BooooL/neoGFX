// neogfx.hpp
/*
  neogfx C++ GUI Library
  Copyright (c) 2015 Leigh Johnston.  All Rights Reserved.
  
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

#include <neolib/neolib.hpp>
#include <string>
#include <boost/multiprecision/cpp_int.hpp>
using namespace boost::multiprecision;

#include <neolib/stdint.hpp>
#include <neolib/reference_counted.hpp>
#include <neogfx/app/i18n.hpp>

namespace neogfx
{
    using namespace neolib::stdint_suffix;
    using namespace std::string_literals;

    using neolib::ref_ptr;

    template <typename Component>
    Component& service();

    template <typename Component>
    void teardown_service();

    template <typename CharT, typename Traits, typename Allocator>
    inline const std::string to_string(const std::basic_string<CharT, Traits, Allocator>& aString)
    {
        static_assert(sizeof(CharT) == sizeof(char));
        return std::string{ reinterpret_cast<const char*>(aString.c_str()), aString.size() };
    }

    template <typename CharT, std::size_t Size>
    inline const std::string to_string(const CharT (&aString)[Size])
    {
        static_assert(sizeof(CharT) == sizeof(char));
        return std::string{ reinterpret_cast<const char*>(aString), Size };
    }
}