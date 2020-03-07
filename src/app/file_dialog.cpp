// file_dialog.cpp
/*
  neogfx C++ GUI Library
  Copyright (c) 2020 Leigh Johnston.  All Rights Reserved.

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

#include <neogfx/app/file_dialog.hpp>
#include <neolib/string_utils.hpp>
#include <neogfx/app/i_app.hpp>
#include <neogfx/gui/widget/i_widget.hpp>
#include "native/3rdparty/tinyfiledialogs.h"

namespace neogfx
{
    inline std::vector<const char*> convert_patterns(filter_patterns const& aPatterns)
    {
        std::vector<char const*> result;
        for (auto const& p : aPatterns)
            result.push_back(p.c_str());
        return result;
    }

    optional_file_path do_save_file_dialog(file_dialog_spec const& aSpec)
    {
        auto const patterns = convert_patterns(aSpec.filterPatterns);
        auto result = tinyfd_saveFileDialog(
            aSpec.title ? aSpec.title->c_str() : nullptr,
            aSpec.defaultPathAndFile ? aSpec.defaultPathAndFile->c_str() : nullptr,
            static_cast<int>(patterns.size()),
            !patterns.empty() ? &patterns[0] : nullptr,
            aSpec.filterPatternDescription ? aSpec.filterPatternDescription->c_str() : nullptr);
        return result ? file_path{ result } : optional_file_path{};
    }

    optional_file_paths do_open_file_dialog(file_dialog_spec const& aSpec, bool aAllowMultipleSelection)
    {
        auto const patterns = convert_patterns(aSpec.filterPatterns);
        auto const result = tinyfd_openFileDialog(
            aSpec.title ? aSpec.title->c_str() : nullptr,
            aSpec.defaultPathAndFile ? aSpec.defaultPathAndFile->c_str() : nullptr,
            static_cast<int>(patterns.size()),
            !patterns.empty() ? &patterns[0] : nullptr,
            aSpec.filterPatternDescription ? aSpec.filterPatternDescription->c_str() : nullptr,
            aAllowMultipleSelection);
        return result ? neolib::tokens(std::string{ result }, "|"s) : optional_file_paths{};
    }

    optional_file_path do_select_folder_dialog(std::optional<std::string> const& aTitle, optional_file_path const& aDefaultPath)
    {
        auto result = tinyfd_selectFolderDialog(
            aTitle ? aTitle->c_str() : nullptr,
            aDefaultPath ? aDefaultPath->c_str() : nullptr);
        return result ? file_path{ result } : optional_file_path{};
    }

    optional_file_path save_file_dialog(file_dialog_spec const& aSpec)
    {
        return do_save_file_dialog(aSpec);
    }

    optional_file_paths open_file_dialog(file_dialog_spec const& aSpec, bool aAllowMultipleSelection)
    {
        return do_open_file_dialog(aSpec, aAllowMultipleSelection);
    }

    optional_file_path select_folder_dialog(std::optional<std::string> const& aTitle, optional_file_path const& aDefaultPath)
    {
        return do_select_folder_dialog(aTitle, aDefaultPath);
    }

    optional_file_path save_file_dialog(i_widget& aParent, file_dialog_spec const& aSpec)
    {
        aParent.root().modal_enable(false);
        auto result = std::async(do_save_file_dialog, aSpec);
        while (result.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready)
            service<i_app>().process_events();
        aParent.root().modal_enable(true);
        return result.get();
    }

    optional_file_paths open_file_dialog(i_widget& aParent, file_dialog_spec const& aSpec, bool aAllowMultipleSelection)
    {
        aParent.root().modal_enable(false);
        auto result = std::async(do_open_file_dialog, aSpec, aAllowMultipleSelection);
        while (result.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready)
            service<i_app>().process_events();
        aParent.root().modal_enable(true);
        return result.get();
    }

    optional_file_path select_folder_dialog(i_widget& aParent, std::optional<std::string> const& aTitle, optional_file_path const& aDefaultPath)
    {
        aParent.root().modal_enable(false);
        auto result = std::async(do_select_folder_dialog, aTitle, aDefaultPath);
        while (result.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready)
            service<i_app>().process_events();
        aParent.root().modal_enable(true);
        return result.get();
    }
}