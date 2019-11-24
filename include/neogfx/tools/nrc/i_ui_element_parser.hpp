// i_ui_element_parser.hpp
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
#include <neolib/i_reference_counted.hpp>
#include <neolib/optional.hpp>
#include <neolib/string.hpp>
#include <neolib/i_vector.hpp>
#include <neolib/i_enum.hpp>
#include <neolib/simple_variant.hpp>

namespace neogfx::nrc
{
    class i_ui_element;

    class i_ui_element_parser : public neolib::i_reference_counted
    {
        // exceptions
    public:
        struct element_type_not_found : std::runtime_error 
        { 
            element_type_not_found(const std::string& aType) : std::runtime_error{ "Element type '" + aType + "' not found." } {} 
            element_type_not_found(const std::string& aType, const std::string& aParentElement) : std::runtime_error{ "Element type '" + aType + "' not found for element '" + aParentElement + "'." } {}
        };
        struct element_data_not_found : std::runtime_error 
        { 
            element_data_not_found(const std::string& aData) : std::runtime_error{ "Element data '" + aData + "' not found." } {} 
            element_data_not_found(const std::string& aData, const std::string& aParentElement) : std::runtime_error{ "Element data '" + aData + "' not found for element '" + aParentElement + "'." } {}
        };
        // types
    public:
        typedef neolib::i_simple_variant data_t;
        typedef neolib::i_vector<neolib::i_simple_variant> array_data_t;
        // operations
    public:
        virtual const neolib::i_string& element_namespace() const = 0;
        virtual void generate_anonymous_id(neolib::i_string& aNewAnonymousId) const = 0;
        virtual void indent(int32_t aLevel, neolib::i_string& aResult) const = 0;
        virtual void emit(const neolib::i_string& aText) const = 0;
        // implementation
    private:
        virtual void do_source_location(neolib::i_string& aLocation) const = 0;
        virtual bool do_data_exists(const neolib::i_string& aKey) const = 0;
        virtual bool do_array_data_exists(const neolib::i_string& aKey) const = 0;
        virtual const data_t& do_get_data(const neolib::i_string& aKey) const = 0;
        virtual data_t& do_get_data(const neolib::i_string& aKey) = 0;
        virtual const array_data_t& do_get_array_data(const neolib::i_string& aKey) const = 0;
        virtual array_data_t& do_get_array_data(const neolib::i_string& aKey) = 0;
        // helpers
    public:
        neolib::string generate_anonymous_id() const
        {
            neolib::string result;
            generate_anonymous_id(result);
            return result;
        }
        std::string indent(int32_t aLevel) const
        {
            neolib::string result;
            indent(aLevel, result);
            return result.to_std_string();
        }
    public:
        std::string source_location() const
        {
            neolib::string location;
            do_source_location(location);
            return location.to_std_string();
        }
        bool data_exists(const std::string& aKey) const
        {
            return do_data_exists(neolib::string{ aKey });
        }
        bool array_data_exists(const std::string& aKey) const
        {
            return do_array_data_exists(neolib::string{ aKey });
        }
        const data_t& get_data(const std::string& aKey) const
        {
            return do_get_data(neolib::string{ aKey });
        }
        data_t& get_data(const std::string& aKey)
        {
            return do_get_data(neolib::string{ aKey });
        }
        const array_data_t& get_array_data(const std::string& aKey) const
        {
            return do_get_array_data(neolib::string{ aKey });
        }
        array_data_t& get_array_data(const std::string& aKey)
        {
            return do_get_array_data(neolib::string{ aKey });
        }
        template <typename T>
        const abstract_t<T>& get(const std::string& aKey) const
        {
            return get_data(aKey).get<abstract_t<T>>();
        }
        template <typename T>
        abstract_t<T>& get(const std::string& aKey)
        {
            return get_data(aKey).get<abstract_t<T>>();
        }
        template <typename T>
        neolib::optional<T> get_optional(const std::string& aKey) const
        {
            if (data_exists(aKey))
                return get_data(aKey).get<abstract_t<T>>();
            else
                return {};
        }
        template <typename T>
        neolib::optional<T> get_optional_enum(const std::string& aKey) const
        {
            if (data_exists(aKey))
                return neolib::string_to_enum<T>(get_data(aKey).get<neolib::i_string>());
            else
                return {};
        }
        template <typename T, typename U>
        const abstract_t<T>& get(const std::string& aKey, const U& aDefault) const
        {
            if (data_exists(aKey))
                return get_data(aKey).get<abstract_t<T>>();
            else
                return aDefault;
        }
        template <typename T, typename U>
        abstract_t<T>& get(const std::string& aKey, U& aDefault)
        {
            if (data_exists(aKey))
                return get_data(aKey).get<abstract_t<T>>();
            else
                return aDefault;
        }
    public:
        void emit(const std::string& aArgument) const
        {
            emit(neolib::string{ aArgument });
        }
        template <typename T>
        void emit(const std::string& aFormat, const T& aArgument) const
        {
            emit(neolib::string{ (boost::format(aFormat) % aArgument).str() });
        }
        template <typename T1, typename T2>
        void emit(const std::string& aFormat, const T1& aArgument1, const T2& aArgument2) const
        {
            emit(neolib::string{ (boost::format(aFormat) % aArgument1 % aArgument2).str() });
        }
        template <typename T1, typename T2, typename T3>
        void emit(const std::string& aFormat, const T1& aArgument1, const T2& aArgument2, const T3& aArgument3) const
        {
            emit(neolib::string{ (boost::format(aFormat) % aArgument1 % aArgument2 % aArgument3).str() });
        }
        template <typename T1, typename T2, typename T3, typename T4>
        void emit(const std::string& aFormat, const T1& aArgument1, const T2& aArgument2, const T3& aArgument3, const T4& aArgument4) const
        {
            emit(neolib::string{ (boost::format(aFormat) % aArgument1 % aArgument2 % aArgument3 % aArgument4).str() });
        }
        template <typename T1, typename T2, typename T3, typename T4, typename T5>
        void emit(const std::string& aFormat, const T1& aArgument1, const T2& aArgument2, const T3& aArgument3, const T4& aArgument4, const T5& aArgument5) const
        {
            emit(neolib::string{ (boost::format(aFormat) % aArgument1 % aArgument2 % aArgument3 % aArgument4 % aArgument5).str() });
        }
    private:
    };
}
