﻿#include <neolib/neolib.hpp>
#include <neogfx/app/app.hpp>
#include <neogfx/core/i_power.hpp>
#include <neogfx/gui/window/window.hpp>
#include <neogfx/gui/dialog/message_box.hpp>
#include <neogfx/hid/i_surface_manager.hpp>
#include <neogfx/game/canvas.hpp>
#include <neogfx/game/game_world.hpp>
#include <video_poker/poker.hpp>
#include <video_poker/table.hpp>

namespace ng = neogfx;
using namespace ng::unit_literals;

int main(int argc, char* argv[])
{
    ng::app app(argc, argv, "neoGFX Sample Application - Video Poker");

    try
    {
        app.set_default_window_icon(ng::image(":/video_poker/resources/icon.png"));
        app.change_style("Light").set_font_info(ng::font_info("Segoe UI", std::string("Regular"), 9));
        app.change_style("Dark").set_font_info(ng::font_info("Segoe UI", std::string("Regular"), 9));

        std::optional<ng::window> windowObject;
        if (!app.program_options().full_screen())
            windowObject.emplace(ng::size{ 1280_dip, 720_dip });
        else
            windowObject.emplace(ng::video_mode{ *app.program_options().full_screen() });

        auto& window = *windowObject;

        ng::game::canvas canvas{ window.client_layout(), std::make_shared<ng::game::ecs>() };
        canvas.set_logical_coordinate_system(neogfx::logical_coordinate_system::AutomaticGui);
        ng::vertical_layout canvasLayout{ canvas };

        video_poker::table table{ canvasLayout, canvas };

        return app.exec();
    }
    catch (std::exception& e)
    {
        app.halt();
        std::cerr << "neogfx::app::exec: terminating with exception: " << e.what() << std::endl;
        ng::service<ng::i_surface_manager>().display_error_message(app.name().empty() ? "Abnormal Program Termination" : "Abnormal Program Termination - " + app.name(), std::string("main: terminating with exception: ") + e.what());
        std::exit(EXIT_FAILURE);
    }
    catch (...)
    {
        app.halt();
        std::cerr << "neogfx::app::exec: terminating with unknown exception" << std::endl;
        ng::service<ng::i_surface_manager>().display_error_message(app.name().empty() ? "Abnormal Program Termination" : "Abnormal Program Termination - " + app.name(), "main: terminating with unknown exception");
        std::exit(EXIT_FAILURE);
    }
}

