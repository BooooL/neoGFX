// i_app.hpp
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
#include <boost/program_options.hpp>
#include <neogfx/core/event.hpp>
#include <neogfx/gfx/i_rendering_engine.hpp>
#include <neogfx/gfx/i_texture.hpp>
#include <neogfx/app/i_event_processing_context.hpp>
#include <neogfx/app/i_style.hpp>
#include <neogfx/app/i18n.hpp>

namespace neogfx
{
    class i_widget;
    class i_style;
    class i_action;
    class i_mnemonic;
    class i_image;
    class i_menu;

    class i_help;

    class event_processing_context;

    enum standard_menu
    {
        File,
        Edit
    };

    class i_program_options
    {
    public:
        struct invalid_options : std::runtime_error { invalid_options(const std::string& aReason) : std::runtime_error("Invalid program options: " + aReason) {} };
    public:
        virtual ~i_program_options() {}
    public:
        virtual const boost::program_options::variables_map& options() const = 0;
    public:
        virtual bool debug() const = 0;
        virtual neogfx::renderer renderer() const = 0;
        virtual std::optional<std::pair<uint32_t, uint32_t>> full_screen() const = 0;
        virtual bool double_buffering() const = 0;
        virtual bool nest() const = 0;
    };

    class i_app
    {
    public:
        event<style_aspect> current_style_changed;
    public:
        struct main_window_closed_prematurely : std::runtime_error { main_window_closed_prematurely() : std::runtime_error("Main window closed prematurely!") {} };
        struct unknown_standard_menu : std::logic_error { unknown_standard_menu() : std::logic_error("neogfx::i_app::unknown_standard_menu") {} };
    public:
        virtual const i_program_options& program_options() const = 0;
        virtual const std::string& name() const = 0;
        virtual int exec(bool aQuitWhenLastWindowClosed = true) = 0;
        virtual bool in_exec() const = 0;
        virtual void quit(int aResultCode) = 0;
    public:
        virtual dimension default_dpi_scale_factor() const = 0;
    public:
        virtual const i_texture& default_window_icon() const = 0;
        virtual void set_default_window_icon(const i_texture& aIcon) = 0;
        virtual void set_default_window_icon(const i_image& aIcon) = 0;
        virtual const i_style& current_style() const = 0;
        virtual i_style& current_style() = 0;
        virtual i_style& change_style(const std::string& aStyleName) = 0;
        virtual i_style& register_style(const i_style& aStyle) = 0;
    public:
        virtual const std::string& translate(const std::string& aTranslatableString, const std::string& aContext = std::string{}) const = 0;
    public:
        virtual i_action& action_file_new() = 0;
        virtual i_action& action_file_open() = 0;
        virtual i_action& action_file_close() = 0;
        virtual i_action& action_file_close_all() = 0;
        virtual i_action& action_file_save() = 0;
        virtual i_action& action_file_save_all() = 0;
        virtual i_action& action_file_exit() = 0;
        virtual i_action& action_undo() = 0;
        virtual i_action& action_redo() = 0;
        virtual i_action& action_cut() = 0;
        virtual i_action& action_copy() = 0;
        virtual i_action& action_paste() = 0;
        virtual i_action& action_delete() = 0;
        virtual i_action& action_select_all() = 0;
        virtual i_action& add_action(i_action& aAction) = 0;
        virtual i_action& add_action(const std::string& aText) = 0;
        virtual i_action& add_action(const std::string& aText, const std::string& aImageUri, dimension aDpiScaleFactor = 1.0, texture_sampling aSampling = texture_sampling::Normal) = 0;
        virtual i_action& add_action(const std::string& aText, const i_texture& aImage) = 0;
        virtual i_action& add_action(const std::string& aText, const i_image& aImage) = 0;
        virtual void remove_action(i_action& aAction) = 0;
        virtual i_action& find_action(const std::string& aText) = 0;
        virtual void add_mnemonic(i_mnemonic& aMnemonic) = 0;
        virtual void remove_mnemonic(i_mnemonic& aMnemonic) = 0;
    public:
        virtual i_menu& add_standard_menu(i_menu& aParentMenu, standard_menu aStandardMenu) = 0;
    public:
        virtual i_help& help() const = 0;
    public:
        virtual bool process_events() = 0;
        virtual bool process_events(i_event_processing_context& aContext) = 0;
        virtual i_event_processing_context& app_message_queue_context() = 0;
    };
}