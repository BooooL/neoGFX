// i_project.hpp
/*
  neogfx C++ App/Game Engine
  Copyright(C) 2020 Leigh Johnston
  
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

#include <DesignStudio/DesignStudio.hpp>
#include <neogfx/gui/view/i_model.hpp>

namespace design_studio
{
    class i_project : public ng::i_model, public ng::i_reference_counted
    {
    public:
        typedef i_project abstract_type;
    public:
        virtual const ng::i_string& name() const = 0;
        virtual const ng::i_string& namespace_() const = 0;
    };
}