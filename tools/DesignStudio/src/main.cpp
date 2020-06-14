// main.cpp
/*
neoGFX Design Studio
Copyright(C) 2017 Leigh Johnston

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

#include <neogfx/tools/DesignStudio/DesignStudio.hpp>
#include <fstream>
#include <neogfx/app/app.hpp>
#include <neogfx/hid/i_surface_manager.hpp>
#include <neogfx/gui/window/window.hpp>
#include <neogfx/gui/layout/vertical_layout.hpp>
#include <neogfx/gui/layout/horizontal_layout.hpp>
#include <neogfx/gui/widget/menu_bar.hpp>
#include <neogfx/gui/widget/toolbar.hpp>
#include <neogfx/gui/widget/status_bar.hpp>
#include <neogfx/gui/view/view_container.hpp>
#include <neogfx/gui/widget/dock.hpp>
#include <neogfx/gui/widget/dockable.hpp>
#include <neogfx/gui/widget/tree_view.hpp>
#include <neogfx/gui/widget/item_model.hpp>
#include <neogfx/gui/widget/item_presentation_model.hpp>
#include <neogfx/core/css.hpp>
#include <neogfx/tools/DesignStudio/project_manager.hpp>
#include <neogfx/tools/DesignStudio/project.hpp>
#include "new_project_dialog.hpp"
#include "DesignStudio.ui.hpp"

int main(int argc, char* argv[])
{
    DesignStudio::ui ui{ argc, argv };
    auto& app = ui.mainApp;

    try
    {
        ui.actionShowStatusBar.checked([&]() { ui.statusBar.show(); });
        ui.actionShowStatusBar.unchecked([&]() { ui.statusBar.hide(); });

        app.current_style().palette().set_color(ng::color_role::Theme, ng::color{ 48, 48, 48 });
        app.current_style().set_spacing(ng::size{ 4.0 });

        auto& mainWindow = ui.mainWindow;

        ng::dock leftDock{ mainWindow.dock_layout(ng::dock_area::Left), ng::dock_area::Left };
        ng::dock rightDock{ mainWindow.dock_layout(ng::dock_area::Right), ng::dock_area::Right };

        auto objects = ng::make_dockable<ng::tree_view>("Objects"_t, ng::dock_area::Right, true, ng::frame_style::NoFrame);
        auto properties = ng::make_dockable<ng::tree_view>("Properties"_t, ng::dock_area::Right, true, ng::frame_style::NoFrame);
        objects.dock(rightDock);
        properties.dock(rightDock);

        leftDock.hide();
        rightDock.hide();

        ng::i_layout& mainLayout = mainWindow.client_layout();
        mainLayout.set_margins(ng::margins{});
        mainLayout.set_spacing(ng::size{});

        ng::horizontal_layout workspaceLayout{ mainLayout };
        ng::view_container workspace{ workspaceLayout };

        ng::texture backgroundTexture1{ ng::image{ ":/DesignStudio/resources/neoGFX.png" } };
        ng::texture backgroundTexture2{ ng::image{ ":/DesignStudio/resources/logo_i42.png" } };

        workspace.view_stack().Painting([&](ng::i_graphics_context& aGc)
        {
            auto const& cr = workspace.view_stack().client_rect();
            aGc.draw_texture(
                ng::point{ (cr.extents() - backgroundTexture1.extents()) / 2.0 },
                backgroundTexture1,
                ng::color::White.with_alpha(0.25));
            aGc.draw_texture(
                ng::rect{ ng::point{ cr.bottom_right() - backgroundTexture2.extents() / 2.0 }, backgroundTexture2.extents() / 2.0 },
                backgroundTexture2,
                ng::color::White.with_alpha(0.25));
        });

        ds::project_manager pm;

        auto update_ui = [&]()
        {
            app.actionFileNew.enable(pm.projects().empty());
            app.actionFileOpen.enable(pm.projects().empty());
            app.actionFileClose.enable(pm.project_active());
            app.actionFileSave.enable(pm.project_active() && pm.active_project().dirty());
            leftDock.show(pm.project_active());
            rightDock.show(pm.project_active());
        };

        update_ui();

        auto project_updated = [&](ds::i_project&) { update_ui(); };

        ng::sink sink;
        sink += pm.ProjectAdded(project_updated);
        sink += pm.ProjectRemoved(project_updated);
        sink += pm.ProjectActivated(project_updated);
        sink += app.actionFileClose.triggered([&]() { if (pm.project_active()) pm.close_project(pm.active_project()); });

        app.action_file_new().triggered([&]()
        {
            ds::new_project_dialog dialog{ mainWindow };
            if (dialog.exec() == ng::dialog_result::Accepted)
                pm.create_project(dialog.name(), dialog.namespace_()).set_dirty();
        });

        //        ng::css css{"test.css"};

        return app.exec();
    }
    catch (std::exception& e)
    {
        app.halt();
        std::cerr << "neoGFX Design Studio: terminating with exception: " << e.what() << std::endl;
        ng::service<ng::i_surface_manager>().display_error_message(app.name().empty() ? "Abnormal Program Termination" : "Abnormal Program Termination - " + app.name(), std::string("main: terminating with exception: ") + e.what());
        std::exit(EXIT_FAILURE);
    }
    catch (...)
    {
        app.halt();
        std::cerr << "neoGFX Design Studio: terminating with unknown exception" << std::endl;
        ng::service<ng::i_surface_manager>().display_error_message(app.name().empty() ? "Abnormal Program Termination" : "Abnormal Program Termination - " + app.name(), "main: terminating with unknown exception");
        std::exit(EXIT_FAILURE);
    }
}

