// opengl_rendering_context.cpp
/*
  neogfx C++ App/Game Engine
  Copyright (c) 2015, 2020 Leigh Johnston.  All Rights Reserved.
  
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

#include <neogfx/neogfx.hpp>
#include <boost/math/constants/constants.hpp>
#include <neolib/app/i_power.hpp>
#include <neogfx/app/i_basic_services.hpp>
#include <neogfx/hid/i_surface_manager.hpp>
#include <neogfx/gfx/text/glyph.hpp>
#include <neogfx/gfx/text/i_emoji_atlas.hpp>
#include <neogfx/gfx/i_rendering_engine.hpp>
#include <neogfx/gfx/text/i_glyph_texture.hpp>
#include <neogfx/gfx/shapes.hpp>
#include <neogfx/gui/widget/i_widget.hpp>
#include <neogfx/game/rectangle.hpp>
#include <neogfx/game/text_mesh.hpp>
#include <neogfx/game/animation_filter.hpp>
#include <neogfx/game/ecs_helpers.hpp>
#include <neogfx/game/entity_info.hpp>
#include "../../hid/native/i_native_surface.hpp"
#include "i_native_texture.hpp"
#include "../text/native/i_native_font_face.hpp"
#include "opengl_rendering_context.hpp"

namespace neogfx
{
    namespace 
    {
        template <typename T>
        class skip_iterator
        {
        public:
            typedef T value_type;
        public:
            skip_iterator(const value_type* aBegin, const value_type* aEnd, std::size_t aSkipAmount = 2u, std::size_t aPasses = 1) :
                iBegin{ aBegin }, iEnd{ aEnd }, iSkipAmount{ std::max<std::size_t>(1u, std::min<std::size_t>(aEnd - aBegin, aSkipAmount)) }, iPasses{ aPasses }, iNext{ aBegin }, iSkipPass{ 1u }, iPass{ 1u }
            {
            }
            skip_iterator() = delete;
        public:
            std::size_t skip_amount() const
            {
                return iSkipAmount;
            }
            std::size_t pass() const
            {
                return iPass;
            }
        public:
            skip_iterator& operator++()
            {
                if (static_cast<std::size_t>(iEnd - iNext) <= iSkipAmount)
                {
                    if (iSkipPass < iSkipAmount)
                        iNext = iBegin + iSkipPass;
                    else
                    {
                        if (iPass >= iPasses)
                            iNext = iEnd;
                        else
                        {
                            ++iPass;
                            iSkipPass = 0u;
                            iNext = iBegin;
                        }
                    }
                    ++iSkipPass;
                }
                else
                    iNext += iSkipAmount;
                return *this;
            }
            const value_type& operator*() const
            {
                return *iNext;
            }
            bool operator==(const graphics_operation::operation* aTest) const
            {
                return iNext == aTest;
            }
            bool operator!=(const graphics_operation::operation* aTest) const
            {
                return !operator==(aTest);
            }
        private:
            const value_type* iBegin;
            const value_type* iEnd;
            std::size_t iSkipAmount;
            std::size_t iPasses;
            const value_type* iNext;
            std::size_t iSkipPass;
            std::size_t iPass;
        };

        inline vertices line_loop_to_lines(const vertices& aLineLoop, bool aClosed = true)
        {
            vertices result;
            result.reserve(aLineLoop.size() * 2);
            for (auto v = aLineLoop.begin(); v != aLineLoop.end(); ++v)
            {
                result.push_back(*v);
                if (v != aLineLoop.begin() && (aClosed || v != std::prev(aLineLoop.end())))
                    result.push_back(*v);
            }
            if (aClosed)
                result.push_back(*aLineLoop.begin());
            return result;
        }

        inline quad line_to_quad(const vec3& aStart, const vec3& aEnd, double aLineWidth)
        {
            auto const vecLine = aEnd - aStart;
            auto const length = vecLine.magnitude();
            auto const halfWidth = aLineWidth / 2.0;
            auto const v1 = vec3{ -halfWidth, -halfWidth, 0.0 };
            auto const v2 = vec3{ -halfWidth, halfWidth, 0.0 };
            auto const v3 = vec3{ length + halfWidth, halfWidth, 0.0 };
            auto const v4 = vec3{ length + halfWidth, -halfWidth, 0.0 };
            auto const r = rotation_matrix(vec3{ 1.0, 0.0, 0.0 }, vecLine);
            return quad{ aStart + r * v1, aStart + r * v2, aStart + r * v3, aStart + r * v4 };
        }

        template <typename VerticesIn, typename VerticesOut>
        inline void lines_to_quads(const VerticesIn& aLines, double aLineWidth, VerticesOut& aQuads)
        {
            for (auto v = aLines.begin(); v != aLines.end(); v += 2)
            {
                quad const q = line_to_quad(v[0], v[1], aLineWidth);
                aQuads.insert(aQuads.end(), q.begin(), q.end());
            }
        }

        template <typename VerticesIn, typename VerticesOut>
        inline void quads_to_triangles(const VerticesIn& aQuads, VerticesOut& aTriangles)
        {
            for (auto v = aQuads.begin(); v != aQuads.end(); v += 4)
            {
                aTriangles.push_back(v[0]);
                aTriangles.push_back(v[1]);
                aTriangles.push_back(v[2]);
                aTriangles.push_back(v[0]);
                aTriangles.push_back(v[3]);
                aTriangles.push_back(v[2]);
            }
        }

        inline GLenum path_shape_to_gl_mode(path_shape aShape)
        {
            switch (aShape)
            {
            case path_shape::Quads:
                return GL_QUADS;
            case path_shape::Lines:
                return GL_LINES;
            case path_shape::LineLoop:
                return GL_LINE_LOOP;
            case path_shape::LineStrip:
                return GL_LINE_STRIP;
            case path_shape::ConvexPolygon:
                return GL_TRIANGLE_FAN;
            default:
                return GL_POINTS;
            }
        }

        inline vertices path_vertices(const path& aPath, const path::sub_path_type& aSubPath, double aLineWidth, GLenum& aMode)
        {
            aMode = path_shape_to_gl_mode(aPath.shape());
            neogfx::vertices vertices = aPath.to_vertices(aSubPath);
            if (aMode == GL_LINE_LOOP)
            {
                aMode = GL_LINES;
                vertices = line_loop_to_lines(vertices);
            }
            if (aMode == GL_LINES)
            {
                aMode = GL_QUADS;
                lines_to_quads(neogfx::vertices{ std::move(vertices) }, aLineWidth, vertices);
            }
            if (aMode == GL_QUADS)
            {
                aMode = GL_TRIANGLES;
                quads_to_triangles(neogfx::vertices{ std::move(vertices) }, vertices);
            }
            return vertices;
        }

        void emit_any_stipple(i_rendering_context& aContext, use_vertex_arrays& aInstance, bool aLoop = false)
        {
            // assumes vertices are quads (as two triangles) created with quads_to_triangles above.
            auto& stippleShader = aContext.rendering_engine().default_shader_program().stipple_shader();
            if (stippleShader.stipple_active())
            {
                auto start = midpoint(aInstance.begin()->xyz, std::next(aInstance.begin())->xyz);
                auto end = midpoint(std::next(aInstance.begin(), 4)->xyz, std::next(aInstance.begin(), 2)->xyz);
                stippleShader.start(aContext, start);
                aInstance.draw(6u);
                auto positionOffset = 0.0;
                while (!aInstance.empty())
                {
                    positionOffset += start.distance(end);
                    start = midpoint(aInstance.begin()->xyz, std::next(aInstance.begin())->xyz);
                    if (aLoop)
                        positionOffset += start.distance(end);
                    end = midpoint(std::next(aInstance.begin(), 4)->xyz, std::next(aInstance.begin(), 2)->xyz);
                    stippleShader.next(aContext, start, positionOffset);
                    aInstance.draw(6u);
                }
            }
        }
    }

    opengl_rendering_context::opengl_rendering_context(const i_render_target& aTarget, neogfx::blending_mode aBlendingMode) :
        iRenderingEngine{ service<i_rendering_engine>() },
        iTarget{aTarget}, 
        iWidget{ nullptr },
        iMultisample{ true },
        iOpacity{ 1.0 },
        iSubpixelRendering{ rendering_engine().is_subpixel_rendering_on() },
        iSrt{ iTarget },
        iUseDefaultShaderProgram{ *this, rendering_engine().default_shader_program() },
        iSnapToPixel{ false }
    {
        set_blending_mode(aBlendingMode);
        set_smoothing_mode(neogfx::smoothing_mode::AntiAlias);
        iSink += render_target().target_deactivating([this]() 
        { 
            flush(); 
        });
    }

    opengl_rendering_context::opengl_rendering_context(const i_render_target& aTarget, const i_widget& aWidget, neogfx::blending_mode aBlendingMode) :
        iRenderingEngine{ service<i_rendering_engine>() },
        iTarget{ aTarget },
        iWidget{ &aWidget },
        iLogicalCoordinateSystem{ aWidget.logical_coordinate_system() },        
        iMultisample{ true },
        iOpacity{ 1.0 },
        iSubpixelRendering{ rendering_engine().is_subpixel_rendering_on() },
        iSrt{ iTarget },
        iUseDefaultShaderProgram{ *this, rendering_engine().default_shader_program() },
        iSnapToPixel{ false }
    {
        set_blending_mode(aBlendingMode);
        set_smoothing_mode(neogfx::smoothing_mode::AntiAlias);
        iSink += render_target().target_deactivating([this]()
        {
            flush();
        });
    }

    opengl_rendering_context::opengl_rendering_context(const opengl_rendering_context& aOther) :
        iRenderingEngine{ aOther.iRenderingEngine },
        iTarget{ aOther.iTarget },
        iWidget{ aOther.iWidget },
        iLogicalCoordinateSystem{ aOther.iLogicalCoordinateSystem },
        iLogicalCoordinates{ aOther.iLogicalCoordinates },
        iMultisample{ true },
        iOpacity{ 1.0 },
        iSubpixelRendering{ aOther.iSubpixelRendering },
        iSrt{ iTarget },
        iUseDefaultShaderProgram{ *this, rendering_engine().default_shader_program() },
        iSnapToPixel{ false }
    {
        set_blending_mode(aOther.blending_mode());
        set_smoothing_mode(aOther.smoothing_mode());
        iSink += render_target().target_deactivating([this]()
        {
            flush();
        });
    }

    opengl_rendering_context::~opengl_rendering_context()
    {
    }

    std::unique_ptr<i_rendering_context> opengl_rendering_context::clone() const
    {
        return std::unique_ptr<i_rendering_context>(new opengl_rendering_context(*this));
    }

    i_rendering_engine& opengl_rendering_context::rendering_engine()
    {
        return iRenderingEngine;
    }

    const i_render_target& opengl_rendering_context::render_target() const
    {
        return iTarget;
    }

    const i_render_target& opengl_rendering_context::render_target()
    {
        return iTarget;
    }

    rect opengl_rendering_context::rendering_area(bool aConsiderScissor) const
    {
        if (scissor_rect() == std::nullopt || !aConsiderScissor)
            return rect{ point{}, render_target().target_extents() };
        else
            return *scissor_rect();
    }

    neogfx::logical_coordinate_system opengl_rendering_context::logical_coordinate_system() const
    {
        if (iLogicalCoordinateSystem != std::nullopt)
            return *iLogicalCoordinateSystem;
        return render_target().logical_coordinate_system();
    }

    void opengl_rendering_context::set_logical_coordinate_system(neogfx::logical_coordinate_system aSystem)
    {
        iLogicalCoordinateSystem = aSystem;
    }

    logical_coordinates opengl_rendering_context::logical_coordinates() const
    {
        if (iLogicalCoordinates != std::nullopt)
            return *iLogicalCoordinates;
        auto result = render_target().logical_coordinates();
        if (logical_coordinate_system() != render_target().logical_coordinate_system())
        {
            switch (logical_coordinate_system())
            {
            case neogfx::logical_coordinate_system::Specified:
                break;
            case neogfx::logical_coordinate_system::AutomaticGame:
                if (render_target().logical_coordinate_system() == neogfx::logical_coordinate_system::AutomaticGui)
                    std::swap(result.bottomLeft.y, result.topRight.y);
                break;
            case neogfx::logical_coordinate_system::AutomaticGui:
                std::swap(result.bottomLeft.y, result.topRight.y);
                break;
            }
        }
        return result;
    }

    void opengl_rendering_context::set_logical_coordinates(const neogfx::logical_coordinates& aCoordinates)
    {
        iLogicalCoordinates = aCoordinates;
    }

    vec2 opengl_rendering_context::offset() const
    {
        return (iOffset != std::nullopt ? *iOffset : vec2{}) + (snap_to_pixel() ? 0.5 : 0.0);
    }

    void opengl_rendering_context::set_offset(const optional_vec2& aOffset)
    {
        iOffset = aOffset;
    }

    bool opengl_rendering_context::snap_to_pixel() const
    {
        return iSnapToPixel;
    }

    void opengl_rendering_context::set_snap_to_pixel(bool aSnapToPixel)
    {
        iSnapToPixel = true;
    }

    const graphics_operation::queue& opengl_rendering_context::queue() const
    {
        thread_local graphics_operation::queue tQueue;
        return tQueue;
    }

    graphics_operation::queue& opengl_rendering_context::queue()
    {
        return const_cast<graphics_operation::queue&>(to_const(*this).queue());
    }

    void opengl_rendering_context::enqueue(const graphics_operation::operation& aOperation)
    {
        scoped_render_target srt{ render_target() };

        queue().push_back(aOperation);
    }

    void opengl_rendering_context::flush()
    {
        if (queue().empty())
            return;

        set_blending_mode(blending_mode());

        for (auto batchStart = queue().begin(); batchStart != queue().end();)
        {
            auto batchEnd = std::next(batchStart);
            while (batchEnd != queue().end() && graphics_operation::batchable(*batchStart, *batchEnd))
                ++batchEnd;
            graphics_operation::batch const opBatch{ &*batchStart, &*batchStart + (batchEnd - batchStart) };
            batchStart = batchEnd;
            switch (opBatch.first->index())
            {
            case graphics_operation::operation_type::SetLogicalCoordinateSystem:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                    set_logical_coordinate_system(static_variant_cast<const graphics_operation::set_logical_coordinate_system&>(*op).system);
                break;
            case graphics_operation::operation_type::SetLogicalCoordinates:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                    set_logical_coordinates(static_variant_cast<const graphics_operation::set_logical_coordinates&>(*op).coordinates);
                break;
            case graphics_operation::operation_type::ScissorOn:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                    scissor_on(static_variant_cast<const graphics_operation::scissor_on&>(*op).rect);
                break;
            case graphics_operation::operation_type::ScissorOff:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    (void)op;
                    scissor_off();
                }
                break;
            case graphics_operation::operation_type::SnapToPixelOn:
                set_snap_to_pixel(true);
                break;
            case graphics_operation::operation_type::SnapToPixelOff:
                set_snap_to_pixel(false);
                break;
            case graphics_operation::operation_type::SetOpacity:
                set_opacity(static_variant_cast<const graphics_operation::set_opacity&>(*(std::prev(opBatch.second))).opacity);
                break;
            case graphics_operation::operation_type::SetBlendingMode:
                set_blending_mode(static_variant_cast<const graphics_operation::set_blending_mode&>(*(std::prev(opBatch.second))).blendingMode);
                break;
            case graphics_operation::operation_type::SetSmoothingMode:
                set_smoothing_mode(static_variant_cast<const graphics_operation::set_smoothing_mode&>(*(std::prev(opBatch.second))).smoothingMode);
                break;
            case graphics_operation::operation_type::PushLogicalOperation:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                    push_logical_operation(static_variant_cast<const graphics_operation::push_logical_operation&>(*op).logicalOperation);
                break;
            case graphics_operation::operation_type::PopLogicalOperation:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    (void)op;
                    pop_logical_operation();
                }
                break;
            case graphics_operation::operation_type::LineStippleOn:
                {
                    auto const& lso = static_variant_cast<const graphics_operation::line_stipple_on&>(*(std::prev(opBatch.second)));
                    line_stipple_on(lso.factor, lso.pattern, lso.position);
                }
                break;
            case graphics_operation::operation_type::LineStippleOff:
                line_stipple_off();
                break;
            case graphics_operation::operation_type::SubpixelRenderingOn:
                subpixel_rendering_on();
                break;
            case graphics_operation::operation_type::SubpixelRenderingOff:
                subpixel_rendering_off();
                break;
            case graphics_operation::operation_type::Clear:
                clear(static_variant_cast<const graphics_operation::clear&>(*(std::prev(opBatch.second))).color);
                break;
            case graphics_operation::operation_type::ClearDepthBuffer:
                clear_depth_buffer();
                break;
            case graphics_operation::operation_type::ClearStencilBuffer:
                clear_stencil_buffer();
                break;
            case graphics_operation::operation_type::SetPixel:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                    set_pixel(static_variant_cast<const graphics_operation::set_pixel&>(*op).point, static_variant_cast<const graphics_operation::set_pixel&>(*op).color);
                break;
            case graphics_operation::operation_type::DrawPixel:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                    draw_pixel(static_variant_cast<const graphics_operation::draw_pixel&>(*op).point, static_variant_cast<const graphics_operation::draw_pixel&>(*op).color);
                break;
            case graphics_operation::operation_type::DrawLine:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    auto const& args = static_variant_cast<const graphics_operation::draw_line&>(*op);
                    draw_line(args.from, args.to, args.pen);
                }
                break;
            case graphics_operation::operation_type::DrawRect:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    auto const& args = static_variant_cast<const graphics_operation::draw_rect&>(*op);
                    draw_rect(args.rect, args.pen);
                }
                break;
            case graphics_operation::operation_type::DrawRoundedRect:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    auto const& args = static_variant_cast<const graphics_operation::draw_rounded_rect&>(*op);
                    draw_rounded_rect(args.rect, args.radius, args.pen);
                }
                break;
            case graphics_operation::operation_type::DrawCircle:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    auto const& args = static_variant_cast<const graphics_operation::draw_circle&>(*op);
                    draw_circle(args.centre, args.radius, args.pen, args.startAngle);
                }
                break;
            case graphics_operation::operation_type::DrawArc:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    auto const& args = static_variant_cast<const graphics_operation::draw_arc&>(*op);
                    draw_arc(args.centre, args.radius, args.startAngle, args.endAngle, args.pen);
                }
                break;
            case graphics_operation::operation_type::DrawPath:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    auto const& args = static_variant_cast<const graphics_operation::draw_path&>(*op);
                    draw_path(args.path, args.pen);
                }
                break;
            case graphics_operation::operation_type::DrawShape:
                // todo: batch
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    auto const& args = static_variant_cast<const graphics_operation::draw_shape&>(*op);
                    draw_shape(args.mesh, args.position, args.pen);
                }
                break;
            case graphics_operation::operation_type::DrawEntities:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    auto const& args = static_variant_cast<const graphics_operation::draw_entities&>(*op);
                    draw_entities(args.ecs, args.layer, args.transformation);
                }
                break;
            case graphics_operation::operation_type::FillRect:
                fill_rect(opBatch);
                break;
            case graphics_operation::operation_type::FillRoundedRect:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    auto const& args = static_variant_cast<const graphics_operation::fill_rounded_rect&>(*op);
                    fill_rounded_rect(args.rect, args.radius, args.fill);
                }
                break;
            case graphics_operation::operation_type::FillCircle:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    auto const& args = static_variant_cast<const graphics_operation::fill_circle&>(*op);
                    fill_circle(args.centre, args.radius, args.fill);
                }
                break;
            case graphics_operation::operation_type::FillArc:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    auto const& args = static_variant_cast<const graphics_operation::fill_arc&>(*op);
                    fill_arc(args.centre, args.radius, args.startAngle, args.endAngle, args.fill);
                }
                break;
            case graphics_operation::operation_type::FillPath:
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                    fill_path(static_variant_cast<const graphics_operation::fill_path&>(*op).path, static_variant_cast<const graphics_operation::fill_path&>(*op).fill);
                break;
            case graphics_operation::operation_type::FillShape:
                fill_shape(opBatch);
                break;
            case graphics_operation::operation_type::DrawGlyph:
                draw_glyph(opBatch);
                break;
            case graphics_operation::operation_type::DrawMesh:
                // todo: use draw_meshes
                for (auto op = opBatch.first; op != opBatch.second; ++op)
                {
                    auto const& args = static_variant_cast<const graphics_operation::draw_mesh&>(*op);
                    draw_mesh(args.mesh, args.material, args.transformation);
                }
                break;
            }
        }
        queue().clear();
    }

    void opengl_rendering_context::scissor_on(const rect& aRect)
    {
        iScissorRects.push_back(aRect);
        iScissorRect = std::nullopt;
        apply_scissor();
    }

    void opengl_rendering_context::scissor_off()
    {
        if (!iScissorRects.empty())
            iScissorRects.pop_back();
        iScissorRect = std::nullopt;
        apply_scissor();
    }

    const optional_rect& opengl_rendering_context::scissor_rect() const
    {
        if (iScissorRect == std::nullopt && !iScissorRects.empty())
        {
            for (auto const& rect : iScissorRects)
                if (iScissorRect != std::nullopt)
                    iScissorRect = iScissorRect->intersection(rect);
                else
                    iScissorRect = rect;
        }
        return iScissorRect;
    }

    void opengl_rendering_context::apply_scissor()
    {
        auto sr = scissor_rect();
        if (sr != std::nullopt)
        {
            glCheck(glEnable(GL_SCISSOR_TEST));
            GLint x = static_cast<GLint>(std::ceil(sr->x));
            GLint y = static_cast<GLint>(logical_coordinates().is_gui_orientation() ? std::ceil(rendering_area(false).cy - sr->cy - sr->y) : sr->y);
            GLsizei cx = static_cast<GLsizei>(std::ceil(sr->cx));
            GLsizei cy = static_cast<GLsizei>(std::ceil(sr->cy));
            glCheck(glScissor(x, y, cx, cy));
        }
        else
        {
            glCheck(glDisable(GL_SCISSOR_TEST));
        }
    }

    bool opengl_rendering_context::multisample() const
    {
        return iMultisample;
    }
    
    void opengl_rendering_context::set_multisample(bool aMultisample)
    {
        if (iMultisample != aMultisample)
        {
            iMultisample = aMultisample;
            if (multisample())
            {
                glCheck(glEnable(GL_MULTISAMPLE));
            }
            else
            {
                glCheck(glDisable(GL_MULTISAMPLE));
            }
        }
    }

    void opengl_rendering_context::enable_sample_shading(double aSampleShadingRate)
    {
        if (iSampleShadingRate == std::nullopt || iSampleShadingRate != aSampleShadingRate)
        {
            iSampleShadingRate = aSampleShadingRate;
            glCheck(glEnable(GL_SAMPLE_SHADING));
            glCheck(glMinSampleShading(1.0));
        }
    }

    void opengl_rendering_context::disable_sample_shading()
    {
        if (iSampleShadingRate != std::nullopt)
        {
            iSampleShadingRate = std::nullopt;
            glCheck(glDisable(GL_SAMPLE_SHADING));
        }
    }
    
    void opengl_rendering_context::set_opacity(double aOpacity)
    {
        iOpacity = aOpacity;
    }

    neogfx::blending_mode opengl_rendering_context::blending_mode() const
    {
        return *iBlendingMode;
    }

    void opengl_rendering_context::set_blending_mode(neogfx::blending_mode aBlendingMode)
    {
        if (iBlendingMode == std::nullopt || *iBlendingMode != aBlendingMode)
        {
            iBlendingMode = aBlendingMode;
            switch (*iBlendingMode)
            {
            case neogfx::blending_mode::None:
                glCheck(glDisable(GL_BLEND));
                break;
            case neogfx::blending_mode::Blit:
                glCheck(glEnable(GL_BLEND));
                glCheck(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
                break;
            case neogfx::blending_mode::Default:
                glCheck(glEnable(GL_BLEND));
                glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
                break;
            }
        }
    }

    smoothing_mode opengl_rendering_context::smoothing_mode() const
    {
        return *iSmoothingMode;
    }

    void opengl_rendering_context::set_smoothing_mode(neogfx::smoothing_mode aSmoothingMode)
    {
        if (iSmoothingMode == std::nullopt || *iSmoothingMode != aSmoothingMode)
        {
            iSmoothingMode = aSmoothingMode;
            if (*iSmoothingMode == neogfx::smoothing_mode::AntiAlias)
            {
                glCheck(glEnable(GL_LINE_SMOOTH));
                glCheck(glEnable(GL_POLYGON_SMOOTH));
            }
            else
            {
                glCheck(glDisable(GL_LINE_SMOOTH));
                glCheck(glDisable(GL_POLYGON_SMOOTH));
            }
        }
    }

    void opengl_rendering_context::push_logical_operation(logical_operation aLogicalOperation)
    {
        iLogicalOperationStack.push_back(aLogicalOperation);
        apply_logical_operation();
    }

    void opengl_rendering_context::pop_logical_operation()
    {
        if (!iLogicalOperationStack.empty())
            iLogicalOperationStack.pop_back();
        apply_logical_operation();
    }

    void opengl_rendering_context::apply_logical_operation()
    {
        if (iLogicalOperationStack.empty() || iLogicalOperationStack.back() == logical_operation::None)
        {
            glCheck(glDisable(GL_COLOR_LOGIC_OP));
        }
        else
        {
            glCheck(glEnable(GL_COLOR_LOGIC_OP));
            switch (iLogicalOperationStack.back())
            {
            case logical_operation::Xor:
                glCheck(glLogicOp(GL_XOR));
                break;
            }
        }    
    }

    void opengl_rendering_context::line_stipple_on(scalar aFactor, uint16_t aPattern, scalar aPosition)
    {
        rendering_engine().default_shader_program().stipple_shader().set_stipple(aFactor, aPattern, aPosition);
    }

    void opengl_rendering_context::line_stipple_off()
    {
        rendering_engine().default_shader_program().stipple_shader().clear_stipple();
    }

    bool opengl_rendering_context::is_subpixel_rendering_on() const
    {
        return iSubpixelRendering;
    }

    void opengl_rendering_context::subpixel_rendering_on()
    {
        iSubpixelRendering = true;
    }

    void opengl_rendering_context::subpixel_rendering_off()
    {
        iSubpixelRendering = false;
    }

    void opengl_rendering_context::clear(const color& aColor)
    {
        glCheck(glClearColor(aColor.red<GLclampf>(), aColor.green<GLclampf>(), aColor.blue<GLclampf>(), aColor.alpha<GLclampf>()));
        glCheck(glClear(GL_COLOR_BUFFER_BIT));
    }

    void opengl_rendering_context::clear_depth_buffer()
    {
        glCheck(glClearDepth(1.0));
        glCheck(glClear(GL_DEPTH_BUFFER_BIT));
    }

    void opengl_rendering_context::clear_stencil_buffer()
    {
        glCheck(glStencilMask(static_cast<GLuint>(-1)));
        glCheck(glClearStencil(0xFF));
        glCheck(glClear(GL_STENCIL_BUFFER_BIT));
    }

    void opengl_rendering_context::set_pixel(const point& aPoint, const color& aColor)
    {
        /* todo: faster alternative to this... */
        disable_anti_alias daa{ *this };
        draw_pixel(aPoint, aColor.with_alpha(1.0));
    }

    void opengl_rendering_context::draw_pixel(const point& aPoint, const color& aColor)
    {
        /* todo: faster alternative to this... */
        fill_rect(rect{ aPoint, size{1.0, 1.0} }, aColor);
    }

    void opengl_rendering_context::draw_line(const point& aFrom, const point& aTo, const pen& aPen)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        if (std::holds_alternative<gradient>(aPen.color()))
            rendering_engine().default_shader_program().gradient_shader().set_gradient(*this, static_variant_cast<const neogfx::gradient&>(aPen.color()), rect{ aFrom, aTo });

        auto v1 = aFrom.to_vec3();
        auto v2 = aTo.to_vec3();
        if (snap_to_pixel() && static_cast<int32_t>(aPen.width()) % 2 == 0)
        {
            v1 -= vec3{ 0.5, 0.5, 0.0 };
            v2 -= vec3{ 0.5, 0.5, 0.0 };
        }

        vec3_array<2> line = { v1, v2 };
        vec3_array<4> quad;
        lines_to_quads(line, aPen.width(), quad);
        vec3_array<6> triangles;
        quads_to_triangles(quad, triangles);

        use_vertex_arrays vertexArrays{ as_vertex_provider(), *this, GL_TRIANGLES, triangles.size() };

        for (auto const& v : triangles)
            vertexArrays.push_back({ v, std::holds_alternative<color>(aPen.color()) ?
                vec4f{{
                    static_variant_cast<color>(aPen.color()).red<float>(),
                    static_variant_cast<color>(aPen.color()).green<float>(),
                    static_variant_cast<color>(aPen.color()).blue<float>(),
                    static_variant_cast<color>(aPen.color()).alpha<float>() * static_cast<float>(iOpacity)}} :
                vec4f{} });

        emit_any_stipple(*this, vertexArrays);
    }

    void opengl_rendering_context::draw_rect(const rect& aRect, const pen& aPen)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        scoped_anti_alias saa{ *this, smoothing_mode::None };
        std::optional<disable_multisample> disableMultisample;

        if (std::holds_alternative<gradient>(aPen.color()))
            rendering_engine().default_shader_program().gradient_shader().set_gradient(*this, static_variant_cast<const neogfx::gradient&>(aPen.color()), aRect);

        auto adjustedRect = aRect;
        if (snap_to_pixel())
        {
            disableMultisample.emplace(*this);
            adjustedRect.position() -= size{ static_cast<int32_t>(aPen.width()) % 2 == 1 ? 0.0 : 0.5 };
            adjustedRect = adjustedRect.with_epsilon(size{ 1.0, 1.0 });
        }

        vec3_array<8> lines = rect_vertices(adjustedRect, mesh_type::Outline, 0.0);
        lines[1].x -= (aPen.width() + rect::default_epsilon);
        lines[3].y -= (aPen.width() + rect::default_epsilon);
        lines[5].x += (aPen.width() + rect::default_epsilon);
        lines[7].y += (aPen.width() + rect::default_epsilon);
        vec3_array<4 * 4> quads;
        lines_to_quads(lines, aPen.width(), quads);
        vec3_array<4 * 6> triangles;
        quads_to_triangles(quads, triangles);

        use_vertex_arrays vertexArrays{ as_vertex_provider(), *this, GL_TRIANGLES, triangles.size() };

        for (auto const& v : triangles)
            vertexArrays.push_back({ v, std::holds_alternative<color>(aPen.color()) ?
                vec4f{{
                    static_variant_cast<color>(aPen.color()).red<float>(),
                    static_variant_cast<color>(aPen.color()).green<float>(),
                    static_variant_cast<color>(aPen.color()).blue<float>(),
                    static_variant_cast<color>(aPen.color()).alpha<float>() * static_cast<float>(iOpacity)}} :
                vec4f{} });

        emit_any_stipple(*this, vertexArrays);
    }

    void opengl_rendering_context::draw_rounded_rect(const rect& aRect, dimension aRadius, const pen& aPen)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        if (std::holds_alternative<gradient>(aPen.color()))
            rendering_engine().default_shader_program().gradient_shader().set_gradient(*this, static_variant_cast<const neogfx::gradient&>(aPen.color()), aRect);

        auto adjustedRect = aRect;
        if (snap_to_pixel())
        {
            adjustedRect.position() -= size{ static_cast<int32_t>(aPen.width()) % 2 == 1 ? 0.0 : 0.5 };
            adjustedRect = adjustedRect.with_epsilon(size{ 1.0, 1.0 });
        }

        auto vertices = rounded_rect_vertices(adjustedRect, aRadius, mesh_type::Outline);

        auto lines = line_loop_to_lines(vertices);
        thread_local vec3_list quads;
        quads.clear();
        lines_to_quads(lines, aPen.width(), quads);
        thread_local vec3_list triangles;
        triangles.clear();
        quads_to_triangles(quads, triangles);

        use_vertex_arrays vertexArrays{ as_vertex_provider(), *this, GL_TRIANGLES, triangles.size() };

        for (auto const& v : triangles)
            vertexArrays.push_back({ v, std::holds_alternative<color>(aPen.color()) ?
                vec4f{{
                    static_variant_cast<color>(aPen.color()).red<float>(),
                    static_variant_cast<color>(aPen.color()).green<float>(),
                    static_variant_cast<color>(aPen.color()).blue<float>(),
                    static_variant_cast<color>(aPen.color()).alpha<float>() * static_cast<float>(iOpacity)}} :
                vec4f{} });

        emit_any_stipple(*this, vertexArrays);
    }

    void opengl_rendering_context::draw_circle(const point& aCentre, dimension aRadius, const pen& aPen, angle aStartAngle)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        neolib::scoped_flag snap{ iSnapToPixel, false };

        if (std::holds_alternative<gradient>(aPen.color()))
            rendering_engine().default_shader_program().gradient_shader().set_gradient(*this, static_variant_cast<const neogfx::gradient&>(aPen.color()), 
                rect{ aCentre - size{aRadius, aRadius}, size{aRadius * 2.0, aRadius * 2.0 } });

        auto vertices = circle_vertices(aCentre, aRadius, aStartAngle, mesh_type::Outline);

        auto lines = line_loop_to_lines(vertices);
        thread_local vec3_list quads;
        quads.clear();
        lines_to_quads(lines, aPen.width(), quads);
        thread_local vec3_list triangles;
        triangles.clear();
        quads_to_triangles(quads, triangles);

        use_vertex_arrays vertexArrays{ as_vertex_provider(), *this, GL_TRIANGLES, triangles.size() };

        for (auto const& v : triangles)
            vertexArrays.push_back({ v, std::holds_alternative<color>(aPen.color()) ?
                vec4f{{
                    static_variant_cast<color>(aPen.color()).red<float>(),
                    static_variant_cast<color>(aPen.color()).green<float>(),
                    static_variant_cast<color>(aPen.color()).blue<float>(),
                    static_variant_cast<color>(aPen.color()).alpha<float>() * static_cast<float>(iOpacity)}} :
                vec4f{} });

        emit_any_stipple(*this, vertexArrays);
    }

    void opengl_rendering_context::draw_arc(const point& aCentre, dimension aRadius, angle aStartAngle, angle aEndAngle, const pen& aPen)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        neolib::scoped_flag snap{ iSnapToPixel, false };

        if (std::holds_alternative<gradient>(aPen.color()))
            rendering_engine().default_shader_program().gradient_shader().set_gradient(*this, static_variant_cast<const neogfx::gradient&>(aPen.color()),
                rect{ aCentre - size{aRadius, aRadius}, size{aRadius * 2.0, aRadius * 2.0 } });

        auto vertices = arc_vertices(aCentre, aRadius, aStartAngle, aEndAngle, aCentre, mesh_type::Outline);

        auto lines = line_loop_to_lines(vertices, false);
        thread_local vec3_list quads;
        quads.clear();
        lines_to_quads(lines, aPen.width(), quads);
        thread_local vec3_list triangles;
        triangles.clear();
        quads_to_triangles(quads, triangles);

        use_vertex_arrays vertexArrays{ as_vertex_provider(), *this, GL_TRIANGLES, triangles.size() };

        for (auto const& v : triangles)
            vertexArrays.push_back({ v, std::holds_alternative<color>(aPen.color()) ?
                vec4f{{
                    static_variant_cast<color>(aPen.color()).red<float>(),
                    static_variant_cast<color>(aPen.color()).green<float>(),
                    static_variant_cast<color>(aPen.color()).blue<float>(),
                    static_variant_cast<color>(aPen.color()).alpha<float>() * static_cast<float>(iOpacity)}} :
                vec4f{} });

        emit_any_stipple(*this, vertexArrays);
    }

    void opengl_rendering_context::draw_path(const path& aPath, const pen& aPen)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        neolib::scoped_flag snap{ iSnapToPixel, false };

        if (std::holds_alternative<gradient>(aPen.color()))
            rendering_engine().default_shader_program().gradient_shader().set_gradient(*this, static_variant_cast<const neogfx::gradient&>(aPen.color()), aPath.bounding_rect());

        for (auto const& subPath : aPath.sub_paths())
        {
            if (subPath.size() > 2)
            {
                GLenum mode;
                auto vertices = path_vertices(aPath, subPath, aPen.width(), mode);

                {
                    use_vertex_arrays vertexArrays{ as_vertex_provider(), *this, mode, vertices.size() };
                    for (auto const& v : vertices)
                        vertexArrays.push_back({ v, std::holds_alternative<color>(aPen.color()) ?
                            vec4f{{
                                static_variant_cast<color>(aPen.color()).red<float>(),
                                static_variant_cast<color>(aPen.color()).green<float>(),
                                static_variant_cast<color>(aPen.color()).blue<float>(),
                                static_variant_cast<color>(aPen.color()).alpha<float>() * static_cast<float>(iOpacity)}} :
                            vec4f{} });
                }
            }
        }
    }

    void opengl_rendering_context::draw_shape(const game::mesh& aMesh, const vec3& aPosition, const pen& aPen)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        if (std::holds_alternative<gradient>(aPen.color()))
            rendering_engine().default_shader_program().gradient_shader().set_gradient(*this, static_variant_cast<const neogfx::gradient&>(aPen.color()), bounding_rect(aMesh));

        auto const& vertices = aMesh.vertices;

        auto lines = line_loop_to_lines(vertices);
        thread_local vec3_list quads;
        quads.clear();
        lines_to_quads(lines, aPen.width(), quads);
        thread_local vec3_list triangles;
        triangles.clear();
        quads_to_triangles(quads, triangles);

        use_vertex_arrays vertexArrays{ as_vertex_provider(), *this, GL_TRIANGLES, triangles.size() };

        for (auto const& v : triangles)
            vertexArrays.push_back({ v + aPosition, std::holds_alternative<color>(aPen.color()) ?
                vec4f{{
                    static_variant_cast<color>(aPen.color()).red<float>(),
                    static_variant_cast<color>(aPen.color()).green<float>(),
                    static_variant_cast<color>(aPen.color()).blue<float>(),
                    static_variant_cast<color>(aPen.color()).alpha<float>() * static_cast<float>(iOpacity)}} :
                vec4f{} });
    }

    void opengl_rendering_context::draw_entities(game::i_ecs& aEcs, int32_t aLayer, const mat44& aTransformation)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        neolib::scoped_flag snap{ iSnapToPixel, false };

        thread_local std::vector<std::vector<mesh_drawable>> drawables;
        thread_local int32_t maxLayer = 0;
        thread_local std::optional<game::scoped_component_lock<game::entity_info, game::mesh_renderer, game::mesh_filter, game::animation_filter, game::rigid_body>> lock;

        if (drawables.size() <= aLayer)
            drawables.resize(aLayer + 1);

        if (aLayer == 0)
        {
            for (auto& d : drawables)
                d.clear();
            lock.emplace(aEcs);
            aEcs.component<game::rigid_body>().take_snapshot();
            auto rigidBodiesSnapshot = aEcs.component<game::rigid_body>().snapshot();
            lock->mutex<game::rigid_body>().unlock();
            auto const& rigidBodies = rigidBodiesSnapshot.data();
            for (auto entity : aEcs.component<game::mesh_renderer>().entities())
            {
#ifndef NDEBUG
                if (aEcs.component<game::entity_info>().entity_record(entity).debug)
                    std::cerr << "Rendering debug entity..." << std::endl;
#endif
                auto const& info = aEcs.component<game::entity_info>().entity_record(entity);
                if (info.destroyed)
                    continue;
                auto const& meshRenderer = aEcs.component<game::mesh_renderer>().entity_record(entity);
                maxLayer = std::max(maxLayer, meshRenderer.layer);
                if (drawables.size() <= maxLayer)
                    drawables.resize(maxLayer + 1);
                auto const& meshFilter = aEcs.component<game::mesh_filter>().has_entity_record(entity) ?
                    aEcs.component<game::mesh_filter>().entity_record(entity) :
                    game::current_animation_frame(aEcs.component<game::animation_filter>().entity_record(entity));
                auto const& transformation = rigidBodies.has_entity_record(entity) ?
                    to_transformation_matrix(rigidBodies.entity_record(entity)) :
                    aEcs.component<game::animation_filter>().has_entity_record(entity) ?
                    to_transformation_matrix(aEcs.component<game::animation_filter>().entity_record(entity)) :
                    mat44::identity();
                drawables[meshRenderer.layer].emplace_back(
                    meshFilter,
                    meshRenderer,
                    transformation,
                    entity);
            }
        }
        if (!drawables[aLayer].empty())
            draw_meshes(dynamic_cast<i_vertex_provider&>(aEcs), &*drawables[aLayer].begin(), &*drawables[aLayer].begin() + drawables[aLayer].size(), aTransformation);
        for (auto const& d : drawables[aLayer])
            if (!d.drawn && d.renderer->destroyOnFustrumCull)
                aEcs.async_destroy_entity(d.entity);
        if (aLayer >= maxLayer)
        {
            maxLayer = 0;
            for (auto& d : drawables)
                d.clear();
            lock.reset();
        }
    }

    void opengl_rendering_context::fill_rect(const rect& aRect, const brush& aFill, scalar aZpos)
    {
        graphics_operation::operation op{ graphics_operation::fill_rect{ aRect, aFill, aZpos } };
        fill_rect(graphics_operation::batch{ &op, &op + 1 });
    }

    void opengl_rendering_context::fill_rect(const graphics_operation::batch& aFillRectOps)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        neolib::scoped_flag snap{ iSnapToPixel, false };
        scoped_anti_alias saa{ *this, smoothing_mode::None };
        disable_multisample disableMultisample{ *this };

        auto& firstOp = static_variant_cast<const graphics_operation::fill_rect&>(*aFillRectOps.first);

        if (std::holds_alternative<gradient>(firstOp.fill))
            rendering_engine().default_shader_program().gradient_shader().set_gradient(*this, static_variant_cast<const gradient&>(firstOp.fill), firstOp.rect);
        
        {
            use_vertex_arrays vertexArrays{ as_vertex_provider(), *this, GL_TRIANGLES, static_cast<std::size_t>(2u * 3u * (aFillRectOps.second - aFillRectOps.first))};

            for (auto op = aFillRectOps.first; op != aFillRectOps.second; ++op)
            {
                auto& drawOp = static_variant_cast<const graphics_operation::fill_rect&>(*op);
                auto rectVertices = rect_vertices(drawOp.rect, mesh_type::Triangles, drawOp.zpos);
                for (auto const& v : rectVertices)
                    vertexArrays.push_back({ v,
                        std::holds_alternative<color>(drawOp.fill) ?
                            vec4f{{
                                static_variant_cast<const color&>(drawOp.fill).red<float>(),
                                static_variant_cast<const color&>(drawOp.fill).green<float>(),
                                static_variant_cast<const color&>(drawOp.fill).blue<float>(),
                                static_variant_cast<const color&>(drawOp.fill).alpha<float>() * static_cast<float>(iOpacity)}} :
                            vec4f{} });
            }
        }
    }

    void opengl_rendering_context::fill_rounded_rect(const rect& aRect, dimension aRadius, const brush& aFill)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        neolib::scoped_flag snap{ iSnapToPixel, false };

        if (aRect.empty())
            return;

        if (std::holds_alternative<gradient>(aFill))
            rendering_engine().default_shader_program().gradient_shader().set_gradient(*this, static_variant_cast<const gradient&>(aFill), aRect);

        auto vertices = rounded_rect_vertices(aRect, aRadius, mesh_type::TriangleFan);
        
        {
            use_vertex_arrays vertexArrays{ as_vertex_provider(), *this, GL_TRIANGLE_FAN, vertices.size() };

            for (auto const& v : vertices)
            {
                vertexArrays.push_back({v, std::holds_alternative<color>(aFill) ?
                    vec4f{{
                        static_variant_cast<const color&>(aFill).red<float>(),
                        static_variant_cast<const color&>(aFill).green<float>(),
                        static_variant_cast<const color&>(aFill).blue<float>(),
                        static_variant_cast<const color&>(aFill).alpha<float>() * static_cast<float>(iOpacity)}} :
                    vec4f{}}); 
            }
        }
    }

    void opengl_rendering_context::fill_circle(const point& aCentre, dimension aRadius, const brush& aFill)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        neolib::scoped_flag snap{ iSnapToPixel, false };

        if (std::holds_alternative<gradient>(aFill))
            rendering_engine().default_shader_program().gradient_shader().set_gradient(*this, static_variant_cast<const gradient&>(aFill), 
                rect{ aCentre - point{ aRadius, aRadius }, size{ aRadius * 2.0 } });

        auto vertices = circle_vertices(aCentre, aRadius, 0.0, mesh_type::TriangleFan);

        {
            use_vertex_arrays vertexArrays{ as_vertex_provider(), *this, GL_TRIANGLE_FAN, vertices.size() };

            for (auto const& v : vertices)
            {
                vertexArrays.push_back({v, std::holds_alternative<color>(aFill) ?
                    vec4f{{
                        static_variant_cast<const color&>(aFill).red<float>(),
                        static_variant_cast<const color&>(aFill).green<float>(),
                        static_variant_cast<const color&>(aFill).blue<float>(),
                        static_variant_cast<const color&>(aFill).alpha<float>() * static_cast<float>(iOpacity)}} :
                    vec4f{}});
            }
        }
    }

    void opengl_rendering_context::fill_arc(const point& aCentre, dimension aRadius, angle aStartAngle, angle aEndAngle, const brush& aFill)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        neolib::scoped_flag snap{ iSnapToPixel, false };

        if (std::holds_alternative<gradient>(aFill))
            rendering_engine().default_shader_program().gradient_shader().set_gradient(*this, static_variant_cast<const gradient&>(aFill), 
                rect{ aCentre - point{ aRadius, aRadius }, size{ aRadius * 2.0 } });

        auto vertices = arc_vertices(aCentre, aRadius, aStartAngle, aEndAngle, aCentre, mesh_type::TriangleFan);

        {
            use_vertex_arrays vertexArrays{ as_vertex_provider(), *this, GL_TRIANGLE_FAN, vertices.size() };

            for (auto const& v : vertices)
            {
                vertexArrays.push_back({v, std::holds_alternative<color>(aFill) ?
                    vec4f{{
                        static_variant_cast<const color&>(aFill).red<float>(),
                        static_variant_cast<const color&>(aFill).green<float>(),
                        static_variant_cast<const color&>(aFill).blue<float>(),
                        static_variant_cast<const color&>(aFill).alpha<float>() * static_cast<float>(iOpacity)}} :
                    vec4f{}});
            }
        }
    }

    void opengl_rendering_context::fill_path(const path& aPath, const brush& aFill)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        neolib::scoped_flag snap{ iSnapToPixel, false };

        for (auto const& subPath : aPath.sub_paths())
        {
            if (subPath.size() > 2)
            {
                if (std::holds_alternative<gradient>(aFill))
                     rendering_engine().default_shader_program().gradient_shader().set_gradient(*this, static_variant_cast<const gradient&>(aFill), 
                        aPath.bounding_rect());

                GLenum mode;
                auto vertices = path_vertices(aPath, subPath, 0.0, mode);

                {
                    use_vertex_arrays vertexArrays{ as_vertex_provider(), *this, mode, vertices.size() };

                    for (auto const& v : vertices)
                    {
                        vertexArrays.push_back({v, std::holds_alternative<color>(aFill) ?
                            vec4f{{
                                static_variant_cast<const color&>(aFill).red<float>(),
                                static_variant_cast<const color&>(aFill).green<float>(),
                                static_variant_cast<const color&>(aFill).blue<float>(),
                                static_variant_cast<const color&>(aFill).alpha<float>() * static_cast<float>(iOpacity)}} :
                            vec4f{}});
                    }
                }
            }
        }
    }

    void opengl_rendering_context::fill_shape(const graphics_operation::batch& aFillShapeOps)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };

        neolib::scoped_flag snap{ iSnapToPixel, false };

        auto& firstOp = static_variant_cast<const graphics_operation::fill_shape&>(*aFillShapeOps.first);

        if (std::holds_alternative<gradient>(firstOp.fill))
        {
            auto const& vertices = firstOp.mesh.vertices;
            vec3 min = vertices[0].xyz;
            vec3 max = min;
            for (auto const& v : vertices)
            {
                min.x = std::min(min.x, v.x + firstOp.position.x);
                max.x = std::max(max.x, v.x + firstOp.position.x);
                min.y = std::min(min.y, v.y + firstOp.position.y);
                max.y = std::max(max.y, v.y + firstOp.position.y);
            }
            rendering_engine().default_shader_program().gradient_shader().set_gradient(*this, static_variant_cast<const gradient&>(firstOp.fill),
                rect{
                    point{ min.x, min.y },
                    size{ max.x - min.x, max.y - min.y } });
        }

        {
            use_vertex_arrays vertexArrays{ as_vertex_provider(), *this, GL_TRIANGLES };

            for (auto op = aFillShapeOps.first; op != aFillShapeOps.second; ++op)
            {
                auto& drawOp = static_variant_cast<const graphics_operation::fill_shape&>(*op);
                auto const& vertices = drawOp.mesh.vertices;
                auto const& uv = drawOp.mesh.uv;
                if (!vertexArrays.room_for(drawOp.mesh.faces.size() * 3u))
                    vertexArrays.execute();
                for (auto const& f : drawOp.mesh.faces)
                {
                    for (auto vi : f)
                    {
                        auto const& v = vertices[vi];
                        vertexArrays.push_back({
                            v + drawOp.position,
                            std::holds_alternative<color>(drawOp.fill) ?
                                vec4f{{
                                    static_variant_cast<const color&>(drawOp.fill).red<float>(),
                                    static_variant_cast<const color&>(drawOp.fill).green<float>(),
                                    static_variant_cast<const color&>(drawOp.fill).blue<float>(),
                                    static_variant_cast<const color&>(drawOp.fill).alpha<float>() * static_cast<float>(iOpacity)}} :
                                vec4f{},
                            uv[vi]});
                    }
                }
            }
        }
    }

    subpixel_format opengl_rendering_context::subpixel_format() const
    {
        if (render_target().target_type() == render_target_type::Texture)
            return neogfx::subpixel_format::None;
        else if (iWidget != nullptr)
            return service<i_surface_manager>().display(iWidget->surface()).subpixel_format();
        // todo: might not be monitor 0!
        return service<i_basic_services>().display(0).subpixel_format();
    }

    void opengl_rendering_context::draw_glyph(const graphics_operation::batch& aDrawGlyphOps)
    {
        disable_anti_alias daa{ *this };
        neolib::scoped_flag snap{ iSnapToPixel, false };

        thread_local std::vector<game::mesh_filter> meshFilters;
        thread_local std::vector<game::mesh_renderer> meshRenderers;
        thread_local std::vector<mesh_drawable> drawables;

        auto draw = [&]()
        {
            for (std::size_t i = 0; i < meshFilters.size(); ++i)
                drawables.emplace_back(meshFilters[i], meshRenderers[i]);
            if (!drawables.empty())
                draw_meshes(as_vertex_provider(), &*drawables.begin(), &*drawables.begin() + drawables.size(), mat44::identity());
            meshFilters.clear();
            meshRenderers.clear();
            drawables.clear();
        };

        std::size_t normalGlyphCount = 0;

        for (int32_t pass = 1; pass <= 3; ++pass)
        {
            switch (pass)
            {
            case 1: // Paper (glyph background) and emoji
                for (auto op = aDrawGlyphOps.first; op != aDrawGlyphOps.second; ++op)
                {
                    auto& drawOp = static_variant_cast<const graphics_operation::draw_glyph&>(*op);

                    if (!drawOp.glyph.is_whitespace() && !drawOp.glyph.is_emoji())
                        ++normalGlyphCount;

                    if (drawOp.appearance.paper() != std::nullopt)
                    {
                        font const& glyphFont = drawOp.glyph.font();

                        auto const& mesh = to_ecs_component(
                            rect{
                                point{ drawOp.point },
                                size{ drawOp.glyph.advance().cx, glyphFont.height() } },
                            mesh_type::Triangles,
                            drawOp.point.z);

                        meshFilters.push_back(game::mesh_filter{ {}, mesh });
                        meshRenderers.push_back(
                            game::mesh_renderer{
                                game::material{
                                    std::holds_alternative<color>(*drawOp.appearance.paper()) ?
                                        to_ecs_component(std::get<color>(*drawOp.appearance.paper())) : std::optional<game::color>{},
                                    std::holds_alternative<gradient>(*drawOp.appearance.paper()) ?
                                        to_ecs_component(std::get<gradient>(*drawOp.appearance.paper())) : std::optional<game::gradient>{} } });
                    }

                    if (drawOp.glyph.is_emoji())
                    {
                        font const& glyphFont = drawOp.glyph.font();

                        auto const& mesh = logical_coordinates().is_gui_orientation() ? 
                            to_ecs_component(
                                rect{
                                    point{ drawOp.point },
                                    size{ drawOp.glyph.advance().cx, glyphFont.height() } },
                                mesh_type::Triangles,
                                drawOp.point.z) :
                            to_ecs_component(
                                game_rect{
                                    point{ drawOp.point },
                                    size{ drawOp.glyph.advance().cx, glyphFont.height() } },
                                mesh_type::Triangles,
                                drawOp.point.z);

                        auto const& emojiAtlas = rendering_engine().font_manager().emoji_atlas();
                        auto const& emojiTexture = emojiAtlas.emoji_texture(drawOp.glyph.value()).as_sub_texture();
                        meshFilters.push_back(game::mesh_filter{ game::shared<game::mesh>{}, mesh });
                        meshRenderers.push_back(game::mesh_renderer{ game::material{ {}, {}, {}, to_ecs_component(emojiTexture) } });
                    }
                }
                break;
            case 2: // Special effects
            case 3: // Glyph render (final pass)
                {
                    draw();
                    bool updateGlyphShader = true;
                    for (auto op = aDrawGlyphOps.first; op != aDrawGlyphOps.second; ++op)
                    {
                        auto const& drawOp = static_variant_cast<const graphics_operation::draw_glyph&>(*op);

                        if (drawOp.glyph.is_whitespace() || drawOp.glyph.is_emoji())
                            continue;

                        auto const& glyphTexture = drawOp.glyph.glyph_texture();

                        bool const renderEffects = !drawOp.appearance.only_calculate_effect() && drawOp.appearance.effect() && drawOp.appearance.effect()->type() == text_effect_type::Outline;
                        if (!renderEffects && pass == 2)
                            continue;

                        if (updateGlyphShader)
                        {
                            updateGlyphShader = false;
                            rendering_engine().default_shader_program().glyph_shader().set_first_glyph(*this, drawOp.glyph);
                        }

                        bool const subpixelRender = drawOp.glyph.subpixel() && glyphTexture.subpixel();

                        auto const& glyphFont = drawOp.glyph.font();

                        vec3 glyphOrigin{
                            drawOp.point.x + glyphTexture.placement().x,
                            logical_coordinates().is_game_orientation() ?
                                drawOp.point.y + (glyphTexture.placement().y + -glyphFont.descender()) :
                                drawOp.point.y + glyphFont.height() - (glyphTexture.placement().y + -glyphFont.descender()) - glyphTexture.texture().extents().cy,
                            drawOp.point.z };

                        if (pass == 2)
                        {
                            auto const scanlineOffsets = (pass == 2 ? static_cast<uint32_t>(drawOp.appearance.effect()->width()) * 2u + 1u : 1u);
                            auto const offsets = scanlineOffsets * scanlineOffsets;
                            point const offsetOrigin{ pass == 2 ? -drawOp.appearance.effect()->width() : 0.0, pass == 2 ? -drawOp.appearance.effect()->width() : 0.0 };
                            for (uint32_t offset = 0; offset < offsets; ++offset)
                            {
                                rect const outputRect = {
                                        point{ glyphOrigin } + offsetOrigin + point{ static_cast<coordinate>(offset % scanlineOffsets), static_cast<coordinate>(offset / scanlineOffsets) },
                                        glyphTexture.texture().extents() };
                                bool haveGradient = drawOp.appearance.effect() && std::holds_alternative<gradient>(drawOp.appearance.effect()->color());
                                if (haveGradient)
                                {
                                    updateGlyphShader = true;
                                    draw();
                                    rendering_engine().default_shader_program().gradient_shader().set_gradient(
                                        *this, static_variant_cast<gradient>(drawOp.appearance.effect()->color()), outputRect);
                                }
                                auto mesh = logical_coordinates().is_gui_orientation() ? 
                                    to_ecs_component(
                                        outputRect,
                                        mesh_type::Triangles,
                                        drawOp.point.z) : 
                                    to_ecs_component(
                                        game_rect{ outputRect },
                                        mesh_type::Triangles,
                                        drawOp.point.z);
                                meshFilters.push_back(game::mesh_filter{ {}, mesh });
                                if (std::holds_alternative<color>(drawOp.appearance.effect()->color()))
                                    meshRenderers.push_back(
                                        game::mesh_renderer{
                                            game::material{ 
                                                to_ecs_component(static_variant_cast<const color&>(drawOp.appearance.effect()->color())),
                                                {},
                                                {},
                                                to_ecs_component(glyphTexture.texture()),
                                                shader_effect::Ignore
                                            },
                                            {}, false, subpixelRender });
                                else
                                    meshRenderers.push_back(
                                        game::mesh_renderer{
                                            game::material{ 
                                                {}, 
                                                {},
                                                {},
                                                to_ecs_component(glyphTexture.texture()),
                                                shader_effect::Ignore
                                            },
                                            {}, false, subpixelRender });
                                if (haveGradient)
                                {
                                    updateGlyphShader = true;
                                    draw();
                                    rendering_engine().default_shader_program().gradient_shader().clear_gradient();
                                }
                            }
                        }
                        else
                        {
                            rect const outputRect = { point{ glyphOrigin }, glyphTexture.texture().extents() };
                            bool haveGradient = std::holds_alternative<gradient>(drawOp.appearance.ink());
                            if (haveGradient)
                            {
                                updateGlyphShader = true;
                                draw();
                                rendering_engine().default_shader_program().gradient_shader().set_gradient(
                                    *this, static_variant_cast<gradient>(drawOp.appearance.ink()), outputRect);
                            }
                            auto mesh = logical_coordinates().is_gui_orientation() ? 
                                to_ecs_component(
                                    outputRect,
                                    mesh_type::Triangles,
                                    drawOp.point.z) : 
                                to_ecs_component(
                                    game_rect{ outputRect },
                                    mesh_type::Triangles,
                                    drawOp.point.z);
                            meshFilters.push_back(game::mesh_filter{ {}, mesh });
                            if (std::holds_alternative<color>(drawOp.appearance.ink()))
                                meshRenderers.push_back(
                                    game::mesh_renderer{
                                        game::material{ 
                                            to_ecs_component(static_variant_cast<const color&>(drawOp.appearance.ink())),
                                            {},
                                            {},
                                            to_ecs_component(glyphTexture.texture()),
                                            shader_effect::Ignore
                                        },
                                        {}, false, subpixelRender });
                            else
                                meshRenderers.push_back(
                                    game::mesh_renderer{
                                        game::material{ 
                                            {}, 
                                            {},
                                            {},
                                            to_ecs_component(glyphTexture.texture()),
                                            shader_effect::Ignore
                                        },
                                        {}, false, subpixelRender });
                            if (haveGradient)
                            {
                                updateGlyphShader = true;
                                draw();
                                rendering_engine().default_shader_program().gradient_shader().clear_gradient();
                            }
                        }
                    }
                }
            }
        }
        draw();
    }

    void opengl_rendering_context::draw_mesh(const game::mesh& aMesh, const game::material& aMaterial, const mat44& aTransformation)
    {
        draw_mesh(game::mesh_filter{ { &aMesh }, {}, {} }, game::mesh_renderer{ aMaterial, {} }, aTransformation);
    }
    
    void opengl_rendering_context::draw_mesh(const game::mesh_filter& aMeshFilter, const game::mesh_renderer& aMeshRenderer, const mat44& aTransformation)
    {
        mesh_drawable drawable
        {
            aMeshFilter,
            aMeshRenderer
        };
        draw_meshes(as_vertex_provider(), &drawable, &drawable + 1, aTransformation);
    }

    void opengl_rendering_context::draw_meshes(i_vertex_provider& aVertexProvider, mesh_drawable* aFirst, mesh_drawable* aLast, const mat44& aTransformation)
    {
        auto const logicalCoordinates = logical_coordinates();

        thread_local patch_drawable patch = {};
        patch.provider = &aVertexProvider;
        patch.items.clear();

        std::size_t vertexCount = 0;
        for (auto md = aFirst; md != aLast; ++md)
        {
            auto& meshDrawable = *md;
            auto& meshFilter = *meshDrawable.filter;
            auto& meshRenderer = *meshDrawable.renderer;
            auto& mesh = (meshFilter.mesh != std::nullopt ? *meshFilter.mesh : *meshFilter.sharedMesh.ptr);
            auto const& faces = mesh.faces;
            vertexCount += faces.size();
            for (auto const& meshPatch : meshRenderer.patches)
                vertexCount += meshPatch.faces.size();
        }

        auto& vertexBuffer = static_cast<opengl_vertex_buffer<>&>(service<i_rendering_engine>().vertex_buffer(aVertexProvider));
        auto& vertices = vertexBuffer.vertices();
        if (vertices.capacity() - vertices.size() < vertexCount)
        {
            vertexBuffer.execute();
            vertices.clear();
        }

        for (auto md = aFirst; md != aLast; ++md)
        {
            auto& meshDrawable = *md;
            auto& meshFilter = *meshDrawable.filter;
            auto& meshRenderer = *meshDrawable.renderer;
            auto& mesh = (meshFilter.mesh != std::nullopt ? *meshFilter.mesh : *meshFilter.sharedMesh.ptr);
            auto const transformation = meshDrawable.transformation * (meshFilter.transformation != std::nullopt ? *meshFilter.transformation : mat44::identity());
            auto const& faces = mesh.faces;
            auto const& material = meshRenderer.material;
            auto add_item = [&](auto const& mesh, auto const& material, auto const& faces)
            {
                vec2 textureStorageExtents;
                vec2 uvFixupCoefficient;
                vec2 uvFixupOffset;
                if (patch_drawable::has_texture(meshRenderer, material))
                {
                    auto const& materialTexture = patch_drawable::texture(meshRenderer, material);
                    auto const& texture = *service<i_texture_manager>().find_texture(materialTexture.id.cookie());
                    textureStorageExtents = texture.storage_extents().to_vec2();
                    uvFixupCoefficient = materialTexture.extents;
                    if (materialTexture.type == texture_type::Texture)
                        uvFixupOffset = vec2{ 1.0, 1.0 };
                    else if (materialTexture.subTexture == std::nullopt)
                        uvFixupOffset = texture.as_sub_texture().atlas_location().top_left().to_vec2();
                    else
                        uvFixupOffset = materialTexture.subTexture->min;
                }
                auto const vertexStartIndex = vertices.size();
                for (auto const& face : faces)
                {
                    for (auto faceVertexIndex : face)
                    {
                        auto const& xyz = transformation * mesh.vertices[faceVertexIndex];
                        auto const& rgba = (material.color != std::nullopt ? material.color->rgba.as<float>() : vec4f{ 1.0f, 1.0f, 1.0f, 1.0f });
                        auto const& uv = (patch_drawable::has_texture(meshRenderer, material) ?
                            (mesh.uv[faceVertexIndex].scale(uvFixupCoefficient) + uvFixupOffset).scale(1.0 / textureStorageExtents) : vec2{});
                        vertices.emplace_back(xyz, rgba, uv);
                        if (material.color != std::nullopt )
                            vertices.back().rgba[3] *= static_cast<float>(iOpacity);
                        auto const& v = vertices.back().xyz;
                        if (v.x >= std::min(logicalCoordinates.bottomLeft.x, logicalCoordinates.topRight.x) &&
                            v.x <= std::max(logicalCoordinates.bottomLeft.x, logicalCoordinates.topRight.x) &&
                            v.y >= std::min(logicalCoordinates.bottomLeft.y, logicalCoordinates.topRight.y) &&
                            v.y <= std::max(logicalCoordinates.bottomLeft.y, logicalCoordinates.topRight.y))
                            meshDrawable.drawn = true;
                    }
                }
                patch.items.emplace_back(meshDrawable, vertexStartIndex, material, faces);
            };
            if (!faces.empty())
                add_item(mesh, material, faces);
            for (auto const& meshPatch : meshRenderer.patches)
                add_item(mesh, meshPatch.material, meshPatch.faces);
        }

        std::optional<use_vertex_arrays> vertexArraysUsage;
        draw_patch(vertexArraysUsage, patch, aTransformation);
        if (vertexArraysUsage)
            vertexArraysUsage->execute();
    }

    void opengl_rendering_context::draw_patch(std::optional<use_vertex_arrays>& aVertexArrayUsage, patch_drawable& aPatch, const mat44& aTransformation)
    {
        use_shader_program usp{ *this, rendering_engine().default_shader_program() };
        neolib::scoped_flag snap{ iSnapToPixel, false };

        auto const logicalCoordinates = logical_coordinates();

        auto& vertexBuffer = static_cast<opengl_vertex_buffer<>&>(service<i_rendering_engine>().vertex_buffer(*aPatch.provider));
        auto& vertices = vertexBuffer.vertices();

        for (auto item = aPatch.items.begin(); item != aPatch.items.end();)
        {
            std::optional<GLint> previousTexture;

            auto const& batchRenderer = *item->meshDrawable->renderer;
            auto const& batchMaterial = *item->material;

            auto calc_bounding_rect = [&vertices, &aPatch](const patch_drawable::item& aItem) -> rect
            {
                return game::bounding_rect(vertices, *aItem.faces, mat44f::identity(), aItem.offsetVertices);
            };

            auto calc_sampling = [&aPatch, &calc_bounding_rect](const patch_drawable::item& aItem) -> texture_sampling
            {
                if (!aItem.has_texture())
                    return texture_sampling::Normal;
                auto const& texture = *service<i_texture_manager>().find_texture(aItem.texture().id.cookie());
                auto sampling = (aItem.texture().sampling != std::nullopt ? *aItem.texture().sampling : texture.sampling());
                if (sampling == texture_sampling::Scaled)
                {
                    auto const extents = size_u32{ texture.extents() };
                    auto const outputRect = calc_bounding_rect(aItem);
                    if (extents / 2u * 2u == extents && (outputRect.cx > extents.cx || outputRect.cy > extents.cy))
                        sampling = texture_sampling::Nearest;
                    else
                        sampling = texture_sampling::Normal;
                }
                return sampling;
            };

            std::size_t faceCount = item->faces->size();
            auto sampling = calc_sampling(*item);
            auto next = std::next(item);
            while (next != aPatch.items.end() && game::batchable(*item->material, *next->material) && sampling == calc_sampling(*next))
            {
                faceCount += next->faces->size();
                ++next;
            }

            {
                if (item->has_texture())
                {
                    auto const& texture = *service<i_texture_manager>().find_texture(item->texture().id.cookie());

                    glCheck(glActiveTexture(sampling != texture_sampling::Multisample ? GL_TEXTURE1 : GL_TEXTURE2));

                    previousTexture.emplace(0);
                    glCheck(glGetIntegerv(sampling != texture_sampling::Multisample ? GL_TEXTURE_BINDING_2D : GL_TEXTURE_BINDING_2D_MULTISAMPLE, &*previousTexture));
                    glCheck(glBindTexture(sampling != texture_sampling::Multisample ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE, static_cast<GLuint>(texture.native_handle())));
                    if (sampling != texture_sampling::Multisample)
                    {
                        glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampling != texture_sampling::Nearest && sampling != texture_sampling::Data ? 
                            GL_LINEAR : 
                            GL_NEAREST));
                        glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampling == texture_sampling::NormalMipmap ? 
                            GL_LINEAR_MIPMAP_LINEAR : 
                            sampling != texture_sampling::Nearest && sampling != texture_sampling::Data ? 
                                GL_LINEAR : 
                                GL_NEAREST));
                    }

                    rendering_engine().default_shader_program().texture_shader().set_texture(texture);
                    rendering_engine().default_shader_program().texture_shader().set_effect(batchMaterial.shaderEffect != std::nullopt ?
                        *batchMaterial.shaderEffect : shader_effect::None);
                    if (texture.sampling() == texture_sampling::Multisample && render_target().target_texture().sampling() == texture_sampling::Multisample)
                        enable_sample_shading(1.0);

                    if (aVertexArrayUsage == std::nullopt || !aVertexArrayUsage->with_textures())
                        aVertexArrayUsage.emplace(*aPatch.provider, *this, GL_TRIANGLES, aTransformation, with_textures, 0, batchRenderer.barrier);
                    aVertexArrayUsage->draw(item->offsetVertices, faceCount * 3);
                }
                else
                {
                    rendering_engine().default_shader_program().texture_shader().clear_texture();

                    if (aVertexArrayUsage == std::nullopt || aVertexArrayUsage->with_textures())
                        aVertexArrayUsage.emplace(*aPatch.provider, *this, GL_TRIANGLES, aTransformation, 0, batchRenderer.barrier);
                    aVertexArrayUsage->draw(item->offsetVertices, faceCount * 3);
                }

                item = next;
            }

            if (previousTexture != std::nullopt)
                glCheck(glBindTexture(sampling != texture_sampling::Multisample ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE, static_cast<GLuint>(*previousTexture)));

            disable_sample_shading();
        }
    }
}