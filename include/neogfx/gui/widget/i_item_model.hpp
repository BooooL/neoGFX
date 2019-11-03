// i_item_model.hpp
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

#include <neogfx/neogfx.hpp>
#include <boost/format.hpp>

#include <neolib/variant.hpp>
#include <neolib/generic_iterator.hpp>

#include <neogfx/core/i_object.hpp>
#include <neogfx/core/i_property.hpp>
#include <neogfx/gui/widget/item_index.hpp>

namespace neogfx
{
    class i_item_model;

    struct item_model_index : item_index<item_model_index> { using item_index::item_index; };
    typedef std::optional<item_model_index> optional_item_model_index;

    enum item_cell_data_type
    {
        Unknown,
        Bool,
        Int32,
        UInt32,
        Int64,
        UInt64,
        Float,
        Double,
        String,
        Pointer,
        CustomType,
        ChoiceBool,
        ChoiceInt32,
        ChoiceUInt32,
        ChoiceInt64,
        ChoiceUInt16,
        ChoiceFloat,
        ChoiceDouble,
        ChoiceString,
        ChoicePointer,
        ChoiceCustomType
    };

    template <typename T>
    struct item_cell_choice_type
    {
        typedef T value_type;
        typedef std::pair<value_type, std::string> option;
        typedef std::vector<option> type;
    };

    typedef neolib::variant<
        bool,
        int32_t,
        uint32_t,
        int64_t,
        uint64_t,
        float,
        double,
        std::string,
        void*,
        custom_type,
        item_cell_choice_type<bool>::type::const_iterator,
        item_cell_choice_type<int32_t>::type::const_iterator,
        item_cell_choice_type<uint32_t>::type::const_iterator,
        item_cell_choice_type<int64_t>::type::const_iterator,
        item_cell_choice_type<uint64_t>::type::const_iterator,
        item_cell_choice_type<float>::type::const_iterator,
        item_cell_choice_type<double>::type::const_iterator,
        item_cell_choice_type<std::string>::type::const_iterator,
        item_cell_choice_type<void*>::type::const_iterator,
        item_cell_choice_type<custom_type>::type::const_iterator> item_cell_data_variant;

    enum class item_cell_data_category
    {
        Invalid,
        Value,
        Pointer,
        CustomType,
        ChooseValue,
        ChoosePointer,
        ChooseCustomType
    };
    template <typename T> struct classify_item_call_data { static constexpr item_cell_data_category category = item_cell_data_category::Invalid; };
    template <> struct classify_item_call_data<bool> { static constexpr item_cell_data_category category = item_cell_data_category::Value; };
    template <> struct classify_item_call_data<int32_t> { static constexpr item_cell_data_category category = item_cell_data_category::Value; };
    template <> struct classify_item_call_data<uint32_t> { static constexpr item_cell_data_category category = item_cell_data_category::Value; };
    template <> struct classify_item_call_data<int64_t> { static constexpr item_cell_data_category category = item_cell_data_category::Value; };
    template <> struct classify_item_call_data<uint64_t> { static constexpr item_cell_data_category category = item_cell_data_category::Value; };
    template <> struct classify_item_call_data<float> { static constexpr item_cell_data_category category = item_cell_data_category::Value; };
    template <> struct classify_item_call_data<double> { static constexpr item_cell_data_category category = item_cell_data_category::Value; };
    template <> struct classify_item_call_data<std::string> { static constexpr item_cell_data_category category = item_cell_data_category::Value; };
    template <> struct classify_item_call_data<void*> { static constexpr item_cell_data_category category = item_cell_data_category::Pointer; };
    template <> struct classify_item_call_data<custom_type> { static constexpr item_cell_data_category category = item_cell_data_category::CustomType; };
    template <> struct classify_item_call_data<item_cell_choice_type<bool>::type::const_iterator> { static constexpr item_cell_data_category category = item_cell_data_category::ChooseValue; };
    template <> struct classify_item_call_data<item_cell_choice_type<int32_t>::type::const_iterator> { static constexpr item_cell_data_category category = item_cell_data_category::ChooseValue; };
    template <> struct classify_item_call_data<item_cell_choice_type<uint32_t>::type::const_iterator> { static constexpr item_cell_data_category category = item_cell_data_category::ChooseValue; };
    template <> struct classify_item_call_data<item_cell_choice_type<int64_t>::type::const_iterator> { static constexpr item_cell_data_category category = item_cell_data_category::ChooseValue; };
    template <> struct classify_item_call_data<item_cell_choice_type<uint64_t>::type::const_iterator> { static constexpr item_cell_data_category category = item_cell_data_category::ChooseValue; };
    template <> struct classify_item_call_data<item_cell_choice_type<float>::type::const_iterator> { static constexpr item_cell_data_category category = item_cell_data_category::ChooseValue; };
    template <> struct classify_item_call_data<item_cell_choice_type<double>::type::const_iterator> { static constexpr item_cell_data_category category = item_cell_data_category::ChooseValue; };
    template <> struct classify_item_call_data<item_cell_choice_type<std::string>::type::const_iterator> { static constexpr item_cell_data_category category = item_cell_data_category::ChooseValue; };
    template <> struct classify_item_call_data<item_cell_choice_type<void*>::type::const_iterator> { static constexpr item_cell_data_category category = item_cell_data_category::ChoosePointer; };
    template <> struct classify_item_call_data<item_cell_choice_type<custom_type>::type::const_iterator> { static constexpr item_cell_data_category category = item_cell_data_category::ChooseCustomType; };

    class item_cell_data : public item_cell_data_variant
    {
    public:
        item_cell_data() 
        {
        }
        template <typename T, std::enable_if_t<!std::is_same_v<std::decay_t<T>, item_cell_data>, int> = 0>
        item_cell_data(T&& aValue) : 
            item_cell_data_variant{ std::forward<T>(aValue) }
        {
        }
        item_cell_data(const item_cell_data& aOther) : 
            item_cell_data_variant{ static_cast<const item_cell_data_variant&>(aOther) } 
        {
        }
        item_cell_data(item_cell_data&& aOther) : 
            item_cell_data_variant{ static_cast<item_cell_data_variant&&>(std::move(aOther)) } 
        {
        }
    public:
        item_cell_data(const char* aString) : item_cell_data_variant{ std::string{ aString } } 
        {
        }
    public:
        item_cell_data& operator=(const item_cell_data& aOther)
        {
            item_cell_data_variant::operator=(static_cast<const item_cell_data_variant&>(aOther));
            return *this;
        }
        item_cell_data& operator=(item_cell_data&& aOther)
        {
            item_cell_data_variant::operator=(static_cast<item_cell_data_variant&&>(std::move(aOther)));
            return *this;
        }
        template <typename T, std::enable_if_t<!std::is_same_v<std::decay_t<T>, item_cell_data>, int> = 0>
        item_cell_data& operator=(T&& aArgument)
        {
            item_cell_data_variant::operator=(std::forward<T>(aArgument));
            return *this;
        }
    public:
        item_cell_data& operator=(const char* aString)
        {
            item_cell_data_variant::operator=(std::string{ aString });
            return *this;
        }
    public:
        std::string to_string() const
        {
            return std::visit([](auto&& arg) -> std::string
            {
                typedef typename std::remove_cv<typename std::remove_reference<decltype(arg)>::type>::type type;
                if constexpr(!std::is_same_v<type, neolib::none_t> && classify_item_call_data<type>::category == item_cell_data_category::Value)
                    return (boost::basic_format<char>{"%1%"} % arg).str();
                else
                    return "";
            }, for_visitor());
        }
    };

    struct item_cell_data_info
    {
        bool unselectable;
        bool readOnly;
        item_cell_data_type type;
        item_cell_data min;
        item_cell_data max;
        item_cell_data step;
    };

    typedef std::optional<item_cell_data_info> optional_item_cell_data_info;

    class i_item_model : public i_object
    {
    public:
        declare_event(column_info_changed, item_model_index::column_type)
        declare_event(item_added, const item_model_index&)
        declare_event(item_changed, const item_model_index&)
        declare_event(item_removed, const item_model_index&)
    public:
        typedef neolib::generic_iterator iterator;
        typedef neolib::generic_iterator const_iterator;
    public:
        struct bad_column_index : std::logic_error { bad_column_index() : std::logic_error("neogfx::i_item_model::bad_column_index") {} };
    public:
        virtual ~i_item_model() {}
    public:
        virtual uint32_t rows() const = 0;
        virtual uint32_t columns() const = 0;
        virtual uint32_t columns(const item_model_index& aIndex) const = 0;
        virtual const std::string& column_name(item_model_index::column_type aColumnIndex) const = 0;
        virtual void set_column_name(item_model_index::column_type aColumnIndex, const std::string& aName) = 0;
        virtual bool column_selectable(item_model_index::column_type aColumnIndex) const = 0;
        virtual void set_column_selectable(item_model_index::column_type aColumnIndex, bool aSelectable) = 0;
        virtual bool column_read_only(item_model_index::column_type aColumnIndex) const = 0;
        virtual void set_column_read_only(item_model_index::column_type aColumnIndex, bool aReadOnly) = 0;
        virtual item_cell_data_type column_data_type(item_model_index::column_type aColumnIndex) const = 0;
        virtual void set_column_data_type(item_model_index::column_type aColumnIndex, item_cell_data_type aType) = 0;
        virtual const item_cell_data& column_min_value(item_model_index::column_type aColumnIndex) const = 0;
        virtual void set_column_min_value(item_model_index::column_type aColumnIndex, const item_cell_data& aValue) = 0;
        virtual const item_cell_data& column_max_value(item_model_index::column_type aColumnIndex) const = 0;
        virtual void set_column_max_value(item_model_index::column_type aColumnIndex, const item_cell_data& aValue) = 0;
        virtual const item_cell_data& column_step_value(item_model_index::column_type aColumnIndex) const = 0;
        virtual void set_column_step_value(item_model_index::column_type aColumnIndex, const item_cell_data& aValue) = 0;
    public:
        virtual iterator index_to_iterator(const item_model_index& aIndex) = 0;
        virtual const_iterator index_to_iterator(const item_model_index& aIndex) const = 0;
        virtual item_model_index iterator_to_index(const_iterator aPosition) const = 0;
        virtual iterator begin() = 0;
        virtual const_iterator begin() const = 0;
        virtual iterator end() = 0;
        virtual const_iterator end() const = 0;
        virtual iterator sibling_begin() = 0;
        virtual const_iterator sibling_begin() const = 0;
        virtual iterator sibling_end() = 0;
        virtual const_iterator sibling_end() const = 0;
        virtual iterator parent(const_iterator aChild) = 0;
        virtual const_iterator parent(const_iterator aChild) const = 0;
        virtual iterator sibling_begin(const_iterator aParent) = 0;
        virtual const_iterator sibling_begin(const_iterator aParent) const = 0;
        virtual iterator sibling_end(const_iterator aParent) = 0;
        virtual const_iterator sibling_end(const_iterator aParent) const = 0;
    public:
        virtual bool empty() const = 0;
        virtual void reserve(uint32_t aItemCount) = 0;
        virtual uint32_t capacity() const = 0;
        virtual iterator insert_item(const_iterator aPosition, const item_cell_data& aCellData) = 0;
        virtual iterator insert_item(const item_model_index& aIndex, const item_cell_data& aCellData) = 0;
        virtual iterator append_item(const_iterator aParent, const item_cell_data& aCellData) = 0;
        virtual void clear() = 0;
        virtual void erase(const_iterator aPosition) = 0;
        virtual void insert_cell_data(const_iterator aItem, item_model_index::column_type aColumnIndex, const item_cell_data& aCellData) = 0;
        virtual void insert_cell_data(const item_model_index& aIndex, const item_cell_data& aCellData) = 0;
        virtual void update_cell_data(const item_model_index& aIndex, const item_cell_data& aCellData) = 0;
    public:
        virtual const item_cell_data_info& cell_data_info(const item_model_index& aIndex) const = 0;
        virtual const item_cell_data& cell_data(const item_model_index& aIndex) const = 0;
    };
}