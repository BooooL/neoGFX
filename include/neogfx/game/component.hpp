// component.hpp
/*
  neogfx C++ App/Game Engine
  Copyright (c) 2018, 2020 Leigh Johnston.  All Rights Reserved.
  
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
#include <vector>
#include <unordered_map>
#include <string>
#include <neolib/intrusive_sort.hpp>
#include <neogfx/game/ecs_ids.hpp>
#include <neogfx/game/i_component.hpp>

namespace neogfx::game
{
    class i_ecs;

    template <typename Data>
    struct shared;

    template <typename T>
    inline bool batchable(const std::optional<T>& lhs, const std::optional<T>& rhs)
    {
        return !!lhs == !!rhs && (lhs == std::nullopt || batchable(*lhs, *rhs));
    }

    template <typename Data>
    inline bool batchable(const shared<Data>& lhs, const shared<Data>& rhs)
    {
        if (!!lhs.ptr != !!rhs.ptr)
            return false;
        if (lhs.ptr == nullptr)
            return true;
        return batchable(*lhs.ptr, *rhs.ptr);
    }

    namespace detail
    {
        template <typename Data>
        struct crack_component_data
        {
            typedef ecs_data_type_t<Data> data_type;
            typedef data_type value_type;
            typedef std::vector<value_type> container_type;
            static constexpr bool optional = false;
        };

        template <typename Data>
        struct crack_component_data<std::optional<Data>>
        {
            typedef ecs_data_type_t<Data> data_type;
            typedef std::optional<data_type> value_type;
            typedef std::vector<value_type> container_type;
            static constexpr bool optional = true;
        };

        template <typename Data>
        struct crack_component_data<shared<Data>>
        {
            typedef ecs_data_type_t<Data> data_type;
            typedef data_type mapped_type;
            typedef std::pair<const std::string, mapped_type> value_type;
            typedef std::unordered_map<std::string, mapped_type> container_type;
            static constexpr bool optional = false;
        };

        template <typename Data>
        struct crack_component_data<shared<std::optional<Data>>>
        {
            typedef ecs_data_type_t<Data> data_type;
            typedef std::optional<data_type> mapped_type;
            typedef std::pair<const std::string, mapped_type> value_type;
            typedef std::unordered_map<std::string, mapped_type> container_type;
            static constexpr bool optional = true;
        };
    }

    template <typename Data, typename Base>
    class static_component_base : public Base
    {
        typedef static_component_base<ecs_data_type_t<Data>, Base> self_type;
    public:
        struct entity_record_not_found : std::logic_error { entity_record_not_found() : std::logic_error("neogfx::static_component::entity_record_not_found") {} };
        struct invalid_data : std::logic_error { invalid_data() : std::logic_error("neogfx::static_component::invalid_data") {} };
    public:
        typedef typename detail::crack_component_data<Data>::data_type data_type;
        typedef typename data_type::meta data_meta_type;
        typedef typename detail::crack_component_data<Data>::value_type value_type;
        typedef typename detail::crack_component_data<Data>::container_type component_data_t;
    public:
        static_component_base(game::i_ecs& aEcs) : 
            iEcs{ aEcs }
        {
        }
        static_component_base(const self_type& aOther) :
            iEcs{ aOther.iEcs },
            iComponentData{ aOther.iComponentData }
        {
        }
    public:
        self_type& operator=(const self_type& aRhs)
        {
            iComponentData = aRhs.iComponentData;
            return *this;
        }
    public:
        game::i_ecs& ecs() const override
        {
            return iEcs;
        }
        const component_id& id() const override
        {
            return data_meta_type::id();
        }
    public:
        neolib::i_lockable& mutex() const override
        {
            return ecs().mutex();
        }
    public:
        bool is_data_optional() const override
        {
            return detail::crack_component_data<Data>::optional;
        }
        const i_string& name() const override
        {
            return data_meta_type::name();
        }
        uint32_t field_count() const override
        {
            return data_meta_type::field_count();
        }
        component_data_field_type field_type(uint32_t aFieldIndex) const override
        {
            return data_meta_type::field_type(aFieldIndex);
        }
        neolib::uuid field_type_id(uint32_t aFieldIndex) const override
        {
            return data_meta_type::field_type_id(aFieldIndex);
        }
        const i_string& field_name(uint32_t aFieldIndex) const override
        {
            return data_meta_type::field_name(aFieldIndex);
        }
    public:
        const component_data_t& component_data() const
        {
            return iComponentData;
        }
        component_data_t& component_data()
        {
            return iComponentData;
        }
        const value_type& operator[](typename component_data_t::size_type aIndex) const
        {
            return *std::next(component_data().begin(), aIndex);
        }
        value_type& operator[](typename component_data_t::size_type aIndex)
        {
            return *std::next(component_data().begin(), aIndex);
        }
    private:
        game::i_ecs& iEcs;
        component_data_t iComponentData;
    };

    template <typename Data>
    class static_component : public static_component_base<Data, i_component>
    {
        typedef static_component<Data> self_type;
        typedef static_component_base<Data, i_component> base_type;
    public:
        using typename base_type::entity_record_not_found;
        using typename base_type::invalid_data;
    public:
        typedef typename base_type::data_type data_type;
        typedef typename base_type::data_meta_type data_meta_type;
        typedef typename base_type::value_type value_type;
        typedef typename base_type::component_data_t component_data_t;
        typedef std::vector<entity_id> component_data_entities_t;
        typedef typename component_data_t::size_type reverse_index_t;
        typedef std::vector<reverse_index_t> reverse_indices_t;
        typedef std::vector<reverse_index_t> free_indices_t;
    public:
        typedef std::unique_ptr<self_type> snapshot_ptr;
        class scoped_snapshot
        {
        public:
            scoped_snapshot(self_type& aOwner) :
                iOwner{ aOwner }
            {
                std::scoped_lock<neolib::i_lockable> lock{ iOwner.mutex() };
                ++iOwner.iUsingSnapshot;
            }
            scoped_snapshot(const scoped_snapshot& aOther) :
                iOwner{ aOther.iOwner }
            {
                std::scoped_lock<neolib::i_lockable> lock{ iOwner.mutex() };
                ++iOwner.iUsingSnapshot;
            }
            ~scoped_snapshot()
            {
                std::scoped_lock<neolib::i_lockable> lock{ iOwner.mutex() };
                --iOwner.iUsingSnapshot;
            }
        public:
            self_type& data() const
            {
                return *iOwner.iSnapshot;
            }
        private:
            self_type& iOwner;
        };
    private:
        static constexpr reverse_index_t invalid = ~reverse_index_t{};
    public:
        static_component(game::i_ecs& aEcs) : 
            base_type{ aEcs },
            iHaveSnapshot{ false },
            iUsingSnapshot{ 0u }
        {
        }
        static_component(const self_type& aOther) :
            base_type{ aOther },
            iEntities{ aOther.iEntities },
            iFreeIndices{ aOther.iFreeIndices },
            iReverseIndices{ aOther.iReverseIndices },
            iHaveSnapshot{ false },
            iUsingSnapshot{ 0u }
        {
        }
    public:
        self_type& operator=(const self_type& aRhs)
        {
            base_type::operator=(aRhs);
            iEntities = aRhs.iEntities;    
            iFreeIndices = aRhs.iFreeIndices;
            iReverseIndices = aRhs.iReverseIndices;
            return *this;
        }
    public:
        using base_type::ecs;
        using base_type::id;
        using base_type::mutex;
    public:
        using base_type::is_data_optional;
        using base_type::name;
        using base_type::field_count;
        using base_type::field_type;
        using base_type::field_type_id;
        using base_type::field_name;
    public:
        using base_type::component_data;
        using base_type::operator[];
    public:
        entity_id entity(const data_type& aData) const
        {
            const data_type* lhs = &aData;
            const data_type* rhs = &base_type::component_data()[0];
            auto index = lhs - rhs;
            return entities()[index];
        }
        const component_data_entities_t& entities() const
        {
            return iEntities;
        }
        component_data_entities_t& entities()
        {
            return iEntities;
        }
        const reverse_indices_t& reverse_indices() const
        {
            return iReverseIndices;
        }
        reverse_indices_t& reverse_indices()
        {
            return iReverseIndices;
        }
        reverse_index_t reverse_index(entity_id aEntity) const
        {
            if (reverse_indices().size() > aEntity)
                return reverse_indices()[aEntity];
            return invalid;
        }
        bool has_entity_record(entity_id aEntity) const override
        {
            return reverse_index(aEntity) != invalid;
        }
        const data_type& entity_record(entity_id aEntity) const
        {
            auto reverseIndex = reverse_index(aEntity);
            if (reverseIndex == invalid)
                   throw entity_record_not_found();
            return base_type::component_data()[reverseIndex];
        }
        value_type& entity_record(entity_id aEntity)
        {
            return const_cast<value_type&>(to_const(*this).entity_record(aEntity));
        }
        void destroy_entity_record(entity_id aEntity) override
        {
            auto reverseIndex = reverse_index(aEntity);
            if (reverseIndex == invalid)
                throw entity_record_not_found();
            if constexpr (data_meta_type::has_handles)
                data_meta_type::free_handles(base_type::component_data()[reverseIndex], ecs());
            entities()[reverseIndex] = null_entity;
            reverse_indices()[aEntity] = invalid;
            free_indices().push_back(reverseIndex);
            if (have_snapshot())
            {
                std::scoped_lock<neolib::i_lockable> lock{ mutex() };
                if (have_snapshot())
                {
                    auto ss = snapshot();
                    ss.data().destroy_entity_record(aEntity);
                }
            }

            // todo: sort/remove component data of dead entities
        }
        value_type& populate(entity_id aEntity, const value_type& aData)
        {
            return do_populate(aEntity, aData);
        }
        value_type& populate(entity_id aEntity, value_type&& aData)
        {
            return do_populate(aEntity, aData);
        }
        const void* populate(entity_id aEntity, const void* aComponentData, std::size_t aComponentDataSize) override
        {
            if ((aComponentData == nullptr && !is_data_optional()) || aComponentDataSize != sizeof(data_type))
                throw invalid_data();
            if (aComponentData != nullptr)
                return &do_populate(aEntity, *static_cast<const data_type*>(aComponentData));
            else
                return &do_populate(aEntity, value_type{}); // empty optional
        }
    public:
        bool have_snapshot() const
        {
            return iHaveSnapshot;
        }
        void take_snapshot()
        {
            std::scoped_lock<neolib::i_lockable> lock{ mutex() };
            if (!iUsingSnapshot)
            {
                if (iSnapshot == nullptr)
                    iSnapshot = snapshot_ptr{ new self_type{*this} };
                else
                    *iSnapshot = *this;
                iHaveSnapshot = true;
            }
        }
        scoped_snapshot snapshot()
        {
            std::scoped_lock<neolib::i_lockable> lock{ mutex() };
            return scoped_snapshot{ *this };
        }
        template <typename Compare>
        void sort(Compare aComparator)
        {
            neolib::intrusive_sort(base_type::component_data().begin(), base_type::component_data().end(),
                [this](auto lhs, auto rhs) 
                { 
                    std::swap(*lhs, *rhs);
                    auto lhsIndex = lhs - base_type::component_data().begin();
                    auto rhsIndex = rhs - base_type::component_data().begin();
                    auto& lhsEntity = entities()[lhsIndex];
                    auto& rhsEntity = entities()[rhsIndex];
                    std::swap(lhsEntity, rhsEntity);
                    if (lhsEntity != invalid)
                        reverse_indices()[lhsEntity] = lhsIndex;
                    if (rhsEntity != invalid)
                        reverse_indices()[rhsEntity] = rhsIndex;
                }, aComparator);
        }
    private:
        free_indices_t& free_indices()
        {
            return iFreeIndices;
        }
        template <typename T>
        value_type& do_populate(entity_id aEntity, T&& aComponentData)
        {
            if (has_entity_record(aEntity))
                return do_update(aEntity, aComponentData);
            reverse_index_t reverseIndex = invalid;
            if (!free_indices().empty())
            {
                reverseIndex = free_indices().back();
                free_indices().pop_back();
                base_type::component_data()[reverseIndex] = std::forward<T>(aComponentData);
                entities()[reverseIndex] = aEntity;
            }
            else
            {
                reverseIndex = base_type::component_data().size();
                base_type::component_data().push_back(std::forward<T>(aComponentData));
                try
                {
                    entities().push_back(aEntity);
                }
                catch (...)
                {
                    base_type::component_data().pop_back();
                    throw;
                }
            }
            try
            {
                if (reverse_indices().size() <= aEntity)
                    reverse_indices().resize(aEntity + 1, invalid);
                reverse_indices()[aEntity] = reverseIndex;
            }
            catch (...)
            {
                entities()[reverseIndex] = null_entity;
                throw;
            }
            return base_type::component_data()[reverseIndex];
        }
        template <typename T>
        value_type& do_update(entity_id aEntity, T&& aComponentData)
        {
            auto& record = entity_record(aEntity);
            record = aComponentData;
            return record;
        }
    private:
        component_data_entities_t iEntities;
        free_indices_t iFreeIndices;
        reverse_indices_t iReverseIndices;
        mutable std::atomic<bool> iHaveSnapshot;
        mutable std::atomic<uint32_t> iUsingSnapshot;
        mutable snapshot_ptr iSnapshot;
    };

    template <typename Data>
    struct shared
    {
        typedef typename detail::crack_component_data<shared<Data>>::mapped_type mapped_type;

        const mapped_type* ptr;

        shared() :
            ptr{ nullptr }
        {
        }
        shared(const mapped_type* aData) :
            ptr{ aData }
        {
        }
        shared(const mapped_type& aData) :
            ptr { &aData }
        {
        }
    };

    template <typename Data>
    class static_shared_component : public static_component_base<shared<ecs_data_type_t<Data>>, i_shared_component>
    {
        typedef static_shared_component<Data> self_type;
        typedef static_component_base<shared<ecs_data_type_t<Data>>, i_shared_component> base_type;
    public:
        using typename base_type::entity_record_not_found;
        using typename base_type::invalid_data;
    public:
        typedef typename base_type::data_type data_type;
        typedef typename base_type::data_meta_type data_meta_type;
        typedef typename base_type::value_type value_type;
        typedef typename base_type::component_data_t component_data_t;
        typedef typename component_data_t::mapped_type mapped_type;
    public:
        static_shared_component(game::i_ecs& aEcs) :
            base_type{ aEcs }
        {
        }
    public:
        using base_type::ecs;
        using base_type::id;
        using base_type::mutex;
    public:
        using base_type::is_data_optional;
        using base_type::name;
        using base_type::field_count;
        using base_type::field_type;
        using base_type::field_type_id;
        using base_type::field_name;
    public:
        using base_type::component_data;
    public:
        const mapped_type& operator[](typename component_data_t::size_type aIndex) const
        {
            return std::next(component_data().begin(), aIndex)->second;
        }
        mapped_type& operator[](typename component_data_t::size_type aIndex)
        {
            return std::next(component_data().begin(), aIndex)->second;
        }
        const mapped_type& operator[](const std::string& aName) const
        {
            return component_data()[aName];
        }
        mapped_type& operator[](const std::string& aName)
        {
            return component_data()[aName];
        }
    public:
        shared<mapped_type> populate(const std::string& aName, const mapped_type& aData)
        {
            base_type::component_data()[aName] = aData;
            auto& result = base_type::component_data()[aName];
            if constexpr (mapped_type::meta::has_updater)
                mapped_type::meta::update(result, ecs(), null_entity);
            return shared<mapped_type> { &result };
        }
        shared<mapped_type> populate(const std::string& aName, mapped_type&& aData)
        {
            base_type::component_data()[aName] = std::move(aData);
            auto& result = base_type::component_data()[aName];
            if constexpr (mapped_type::meta::has_updater)
                mapped_type::meta::update(result, ecs(), null_entity);
            return shared<mapped_type> { &result };
        }
        const void* populate(const std::string& aName, const void* aComponentData, std::size_t aComponentDataSize) override
        {
            if ((aComponentData == nullptr && !is_data_optional()) || aComponentDataSize != sizeof(mapped_type))
                throw invalid_data();
            if (aComponentData != nullptr)
                return populate(aName, *static_cast<const mapped_type*>(aComponentData)).ptr;
            else
                return populate(aName, mapped_type{}).ptr; // empty optional
        }
    };
}