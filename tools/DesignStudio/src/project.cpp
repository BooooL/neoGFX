// project.cpp
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

#include <DesignStudio/DesignStudio.hpp>
#include <DesignStudio/project.hpp>

namespace design_studio
{
    project::project(const std::string& aName, const std::string& aNamespace) :
        iName{ aName }, iNamespace{ aNamespace }
    {
    }

    const ng::i_string& project::name() const
    {
        return iName;
    }

    const ng::i_string& project::namespace_() const
    {
        return iNamespace;
    }
}