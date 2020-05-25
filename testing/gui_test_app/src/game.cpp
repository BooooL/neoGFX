﻿#include <neogfx/neogfx.hpp>
#include <neogfx/neogfx.hpp>
#include <iomanip>
#include <boost/format.hpp>
#include <neolib/core/random.hpp>
#include <neolib/core/singleton.hpp>
#include <neolib/app/i_power.hpp>
#include <neogfx/app/i_app.hpp>
#include <neogfx/hid/i_game_controllers.hpp>
#include <neogfx/gui/layout/i_layout.hpp>
#include <neogfx/gfx/image.hpp>
#include <neogfx/game/chrono.hpp>
#include <neogfx/game/ecs.hpp>
#include <neogfx/game/entity_info.hpp>
#include <neogfx/game/canvas.hpp>
#include <neogfx/game/standard_archetypes.hpp>
#include <neogfx/game/game_world.hpp>
#include <neogfx/game/time.hpp>
#include <neogfx/game/collision_detector.hpp>
#include <neogfx/game/simple_physics.hpp>
#include <neogfx/game/rigid_body.hpp>
#include <neogfx/game/sprite.hpp>
#include <neogfx/game/text_mesh.hpp>
#include <neogfx/game/box_collider.hpp>
#include <neogfx/game/rectangle.hpp>
#include <neogfx/game/animation_filter.hpp>
#include <neogfx/game/box_collider.hpp>

namespace ng = neogfx;
using namespace neolib::stdint_suffix;

namespace archetypes
{
    ng::game::sprite_archetype const spaceship{ "Spaceship" };
    ng::game::sprite_archetype const asteroid{ "Asteroid" };
    ng::game::sprite_archetype const missile{ "Missile" };
    ng::game::animation_archetype const explosion{ "Explosion" };
}

void create_game(ng::i_layout& aLayout)
{
    // Create an ECS and canvas to render game world on...
    auto& canvas = aLayout.add(std::make_shared<ng::game::canvas>(std::make_shared<ng::game::ecs>(ng::game::ecs_flags::Default | ng::game::ecs_flags::CreatePaused)));
    canvas.set_font(ng::font{ canvas.font(), ng::font_style::Bold, 16 });
    canvas.set_background_color(ng::color::Black);
    canvas.set_layers(4);

    auto& ecs = canvas.ecs();

    struct game_state
    {
        neolib::basic_random<ng::scalar> prng;
        uint32_t score = 0u;
        uint32_t asteroidsDestroyed = 0u;
        bool showAabbGrid = false;
    };
    auto gameState = std::make_shared<game_state>();

    // Background...
    for (int i = 0; i < 1000; ++i)
        ng::game::shape::rectangle
    {
        ecs,
        { gameState->prng(800), gameState->prng(800), -1.0 + 0.5 * (gameState->prng(32) / 32.0) },
        { gameState->prng(64), gameState->prng(64) },
        ng::color{ ng::vec4{ gameState->prng(0.25), gameState->prng(0.25), gameState->prng(0.25), 1.0 } }
    }.detach();

    // Spaceship...
    const char* spaceshipImage
    {
        "[9,9]"
        "{0,paper}"
        "{1,ink1}"
        "{2,ink2}"

        "000010000"
        "000121000"
        "000121000"
        "001222100"
        "001222100"
        "011222110"
        "010111010"
        "010000010"
        "010000010"
    };

    auto spaceship = ecs.create_entity(
        archetypes::spaceship,
        ng::to_ecs_component(ng::game_rect{ ng::size{ 36.0, 36.0} }.with_centred_origin()),
        ng::game::mesh_renderer{ 
            ng::game::material
            { 
                {}, {}, {}, ng::to_ecs_component(
                ng::image
                {
                    spaceshipImage,
                    {
                        { "paper", ng::color{} },
                        { "ink1", ng::color::LightGoldenrod },
                        { "ink2", ng::color::DeepSkyBlue }
                    },
                    1.0,
                    ng::texture_sampling::Nearest
                })
            }, {}, 2
        },
        ng::game::rigid_body
        {
            { 400.0, 18.0, 0.0 }, 1.0
        },
        ng::game::box_collider{ 0x1ull });

    // Asteroids...
    auto make_asteroid_mesh = [&ecs, gameState](ng::scalar w)
    {
        ng::game::mesh asteroidMesh;
        asteroidMesh.vertices.push_back(ng::vec3{ 0.0, 0.0, 0.0 });
        for (ng::scalar angle = 0.0; angle < 360.0; angle += (gameState->prng(30.0) + 30.0))
            asteroidMesh.vertices.push_back(ng::rotation_matrix(ng::vec3{ 0.0, 0.0, ng::to_rad(angle) }) * ng::vec3{ w + gameState->prng(10.0) - 5.0, 0.0, 0.0 });
        for (uint32_t i = 1; i < asteroidMesh.vertices.size() - 1; ++i)
            asteroidMesh.faces.push_back(ng::game::face{ 0u, i, i + 1u });
        asteroidMesh.faces.push_back(ng::game::face{ 0u, 1u, static_cast<uint32_t>(asteroidMesh.vertices.size()) - 1u });
        return asteroidMesh;
    };

    auto make_asteroid = [&ecs, gameState, spaceship, make_asteroid_mesh]()
    {
        ng::vec3 position;
        ng::scalar size;
        do
        {
            position = ng::vec3{ gameState->prng(800), gameState->prng(800), 0.0 };
            size = gameState->prng(20.0) + 10.0;
        } while (ng::aabb_intersects(
            ng::to_aabb(position, size),
            ng::to_aabb(ecs.component<ng::game::rigid_body>().entity_record(spaceship).position, 36.0 * 3.0)));
        auto asteroid = ecs.create_entity(
            archetypes::asteroid,
            ng::game::mesh_renderer
            {
                ng::game::material{ ng::to_ecs_component(ng::color::from_hsl(gameState->prng(360), 1.0, 0.75)) }, {}, 1
            },
            make_asteroid_mesh(size),
            ng::game::rigid_body
            {
                position, 1.0,
                ng::rotation_matrix(ng::vec3{ 0.0, 0.0, ng::to_rad(gameState->prng(360.0)) }) * ng::vec3{ gameState->prng(20.0), 0.0, 0.0 },
                {},
                {},
                { 0.0, 0.0, ng::to_rad(gameState->prng(90.0) + 45.0) * (std::rand() % 2 == 0 ? 1.0 : -1.0) }
            },
            ng::game::box_collider{ 0x2ull });
    };

    for (int i = 0; i < 75; ++i)
        make_asteroid();

    auto const explosionAnimation = ng::regular_sprite_sheet_to_renderable_animation(ecs, "explosion", ":/test/resources/explosion.png", { 4u, 4u }, 0.05);

    auto make_explosion = [&ecs, explosionAnimation](const ng::aabb& target)
    {
        auto explosion = ecs.create_entity(archetypes::explosion, explosionAnimation.material, explosionAnimation.filter);
        auto& explosionFilter = ecs.component<ng::game::animation_filter>().entity_record(explosion);
        explosionFilter.transformation = ng::mat44::identity();
        explosionFilter.autoDestroy = true;
        ng::apply_translation(ng::apply_scaling(*explosionFilter.transformation, ng::aabb_extents(target)), ng::aabb_origin(target));
    };

    ng::service<ng::i_game_controllers>().controller_for(ng::game_player::One).button_pressed([gameState](ng::game_controller_button aButton, ng::key_modifiers_e)
    {
        if (aButton == ng::game_controller_button::Y)
            gameState->showAabbGrid = !gameState->showAabbGrid;
    });

    ng::font clockFont{ "SnareDrum Two NBP", "Regular", 40.0 };
    ng::font scoreFont{ "SnareDrum Two NBP", "Regular", 60.0 };
    // Some information text...
    canvas.EntitiesRendered([&canvas, &ecs, gameState, clockFont, scoreFont](ng::i_graphics_context& gc, int32_t layer)
    {
        if (layer == 3)
        {
            if (gameState->showAabbGrid)
            {
                ecs.system<ng::game::collision_detector>().visit_aabbs([&gc](const ng::aabb& aabb)
                {
                    gc.draw_rect(ng::rect{ ng::point{ aabb.min }, ng::point{ aabb.max } }, ng::pen{ ng::color::Blue });
                });
            }

            std::ostringstream text;
            auto worldTime = static_cast<uint64_t>(ng::game::from_step_time(ecs.system<ng::game::time>().world_time()) * 1000.0);
            text.fill('0');
            text << std::setw(2) << worldTime / (1000 * 60 * 60) << " : " << std::setw(2) << worldTime / (1000 * 60) % 60 << " : " << std::setw(2) << worldTime / (1000) % 60 << " . " << std::setw(3) << worldTime % 1000;
            gc.draw_text(ng::point{ 0.0, 0.0 }, text.str(), clockFont, ng::text_appearance{ ng::color::White, ng::text_effect{ ng::text_effect_type::Outline, ng::color::Black, 2.0 } });
            text.str({});
            text.fill('0');
            text << std::setw(6) << gameState->score;
            gc.draw_text(ng::point{ canvas.client_rect().right() - 250.0, 0.0}, text.str(), scoreFont, ng::text_appearance{ ng::color::Goldenrod, ng::text_effect{ ng::text_effect_type::Outline, ng::color::Black, 2.0 } });
        }
    });

    ~~~~ecs.system<ng::game::collision_detector>().Collision([&ecs, gameState, make_explosion, spaceship](ng::game::entity_id e1, ng::game::entity_id e2)
    {
        auto id1 = ecs.component<ng::game::entity_info>().entity_record(e1).archetypeId;
        auto id2 = ecs.component<ng::game::entity_info>().entity_record(e2).archetypeId;
        if (id1 == archetypes::missile.id())
        {
            if (id2 == archetypes::asteroid.id())
            {
                make_explosion(*ecs.component<ng::game::box_collider>().entity_record(e2).currentAabb);
                ++gameState->asteroidsDestroyed;
                gameState->score += 250;
            }
            ecs.async_destroy_entity(e1, false);
            ecs.async_destroy_entity(e2, false);
        }
        else if (id2 == archetypes::missile.id())
        {
            if (id1 == archetypes::asteroid.id())
            {
                make_explosion(*ecs.component<ng::game::box_collider>().entity_record(e1).currentAabb);
                ++gameState->asteroidsDestroyed;
                gameState->score += 250;
            }
            ecs.async_destroy_entity(e1, false);
            ecs.async_destroy_entity(e2, false);
        }
        else if (id1 == archetypes::explosion.id())
        {
            if (id2 == archetypes::asteroid.id())
            {
                make_explosion(*ecs.component<ng::game::box_collider>().entity_record(e2).currentAabb);
                ++gameState->asteroidsDestroyed;
                gameState->score += 500;
            }
            ecs.async_destroy_entity(e1, false);
            if (e2 != spaceship)
                ecs.async_destroy_entity(e2, false);
        }
        else if (id2 == archetypes::explosion.id())
        {
            if (id1 == archetypes::asteroid.id())
            {
                make_explosion(*ecs.component<ng::game::box_collider>().entity_record(e1).currentAabb);
                ++gameState->asteroidsDestroyed;
                gameState->score += 500;
            }
            ecs.async_destroy_entity(e2, false);
            if (e1 != spaceship)
                ecs.async_destroy_entity(e2, false);
        }
    });
    
    // Instantiate physics...
    ecs.system<ng::game::simple_physics>();

    ~~~~ecs.system<ng::game::game_world>().ApplyingPhysics([&ecs, spaceship](ng::game::step_time aPhysicsStepTime)
    {
        auto const& keyboard = ng::service<ng::i_keyboard>();
        auto& spaceshipPhysics = ecs.component<ng::game::rigid_body>().entity_record(spaceship);
        spaceshipPhysics.acceleration =
            ng::vec3
        {
            keyboard.is_key_pressed(ng::ScanCode_RIGHT) ? 16.0 : keyboard.is_key_pressed(ng::ScanCode_LEFT) ? -16.0 : 0.0,
            keyboard.is_key_pressed(ng::ScanCode_UP) ? 16.0 : keyboard.is_key_pressed(ng::ScanCode_DOWN) ? -16.0 : 0.0
        };

        spaceshipPhysics.spin.z = 0.0;

        if (keyboard.is_key_pressed(ng::ScanCode_Z))
            spaceshipPhysics.spin.z = ng::to_rad(30.0);
        else if (keyboard.is_key_pressed(ng::ScanCode_X))
            spaceshipPhysics.spin.z = ng::to_rad(-30.0);

        if (ng::service<ng::i_game_controllers>().have_controller_for(ng::game_player::One))
        {
            auto const& controller = ng::service<ng::i_game_controllers>().controller_for(ng::game_player::One);
            spaceshipPhysics.acceleration += ng::vec3{ 16.0, 16.0, 0.0 }.hadamard_product(ng::vec3{ controller.left_thumb_position() });
            spaceshipPhysics.acceleration += ng::vec3{ 0.0,
                controller.is_button_pressed(ng::game_controller_button::DirectionalPadUp) ? 16.0 : 
                    controller.is_button_pressed(ng::game_controller_button::DirectionalPadDown) ? -16.0 : 0.0 };
            if (controller.is_button_pressed(ng::game_controller_button::DirectionalPadLeft))
                spaceshipPhysics.spin.z = ng::to_rad(30.0);
            else if (controller.is_button_pressed(ng::game_controller_button::DirectionalPadRight))
                spaceshipPhysics.spin.z = ng::to_rad(-30.0);
        }
    });

    ecs.system<ng::game::game_world>().PhysicsApplied([&ecs, gameState, spaceship, make_asteroid](ng::game::step_time aPhysicsStepTime)
    {
        while (gameState->asteroidsDestroyed && ecs.component<ng::game::animation_filter>().entities().empty())
        {
            --gameState->asteroidsDestroyed;
            make_asteroid();
        }

        auto const& keyboard = ng::service<ng::i_keyboard>();
        auto& spaceshipPhysics = ecs.component<ng::game::rigid_body>().entity_record(spaceship);
        static bool sExtraFire = false;
        bool const fireMissile = sExtraFire || keyboard.is_key_pressed(ng::ScanCode_SPACE) ||
            (ng::service<ng::i_game_controllers>().have_controller_for(ng::game_player::One) &&
                ng::service<ng::i_game_controllers>().controller_for(ng::game_player::One).is_button_pressed(ng::game_controller_button::A));
        if (fireMissile)
        {
            auto stepTime_ms = static_cast<decltype(aPhysicsStepTime)>(ng::game::chrono::to_milliseconds(ng::game::chrono::flicks{ aPhysicsStepTime }));
            static auto sLastTime_ms = stepTime_ms;
            auto sinceLastTime_ms = stepTime_ms - sLastTime_ms;
            if (sinceLastTime_ms < 100)
            {
                if (sinceLastTime_ms / 10 % 2 == 0)
                {
                    sExtraFire = false;
                    auto make_missile = [&](double angle)
                    {
                        auto tm = ng::game::to_transformation_matrix(spaceshipPhysics, false);
                        auto missile = ecs.create_entity(
                            archetypes::missile,
                            ng::to_ecs_component(ng::rect{ ng::size{ 3.0, 3.0} }.with_centred_origin()),
                            ng::game::material{ ng::to_ecs_component(ng::color{ rand() % 160 + 96, rand() % 160 + 96, rand() % 160 + 96 }), {}, {}, {} },
                            ng::game::rigid_body
                            {
                                spaceshipPhysics.position + ~(tm * ng::vec4{ 0.0, 18.0, 0.0, 1.0 }).xyz,
                                0.016,
                                ~(tm * ng::affine_rotation_matrix(ng::vec3{0.0, 0.0, ng::to_rad(angle)}) * ng::vec4{ 0.0, 360.0, 0.0, 0.0 }).xyz + spaceshipPhysics.velocity,
                                {},
                                spaceshipPhysics.angle + ng::vec3{ 0.0, 0.0, ng::to_rad(angle) }
                            },
                            ng::game::box_collider{ 0x1ull });
                        ecs.component<ng::game::mesh_renderer>().entity_record(missile).destroyOnFustrumCull = true;
                    };
                    for (double angle = -30.0; angle <= 30.0; angle += 10.0)
                        make_missile(angle);
                }
            }
            else
                sExtraFire = true;
            if (sinceLastTime_ms > 200)
                sLastTime_ms = stepTime_ms;
        }
    });

    canvas.Mouse([&canvas, spaceship](const neogfx::mouse_event& e)
    {
        if ((e.type() == neogfx::mouse_event_type::ButtonClicked ||
            e.type() == neogfx::mouse_event_type::Moved) && (e.mouse_button() & neogfx::mouse_button::Left) == neogfx::mouse_button::Left)
        {
            auto newPos = ng::point{ e.position() - canvas.origin() };
            newPos.y = canvas.extents().cy - newPos.y;
            ng::game::scoped_component_lock<ng::game::rigid_body> lock{ canvas.ecs() };
            canvas.ecs().component<ng::game::rigid_body>().entity_record(spaceship).position = newPos.to_vec3();
        }
    });
}