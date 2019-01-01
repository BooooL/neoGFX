// opengl_graphics_context.cpp
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

#include <neogfx/neogfx.hpp>
#include <boost/math/constants/constants.hpp>
#include <neogfx/gfx/text/glyph.hpp>
#include <neogfx/gfx/text/i_emoji_atlas.hpp>
#include <neogfx/gfx/i_rendering_engine.hpp>
#include <neogfx/gfx/text/i_glyph_texture.hpp>
#include <neogfx/gfx/shapes.hpp>
#include <neogfx/gui/widget/i_widget.hpp>
#include <neogfx/game/rectangle.hpp>
#include <neogfx/game/ecs_helpers.hpp>
#include "../../hid/native/i_native_surface.hpp"
#include "i_native_texture.hpp"
#include "../text/native/i_native_font_face.hpp"
#include "opengl_graphics_context.hpp"
#include "opengl_vertex_arrays.hpp"

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
				iBegin{ aBegin }, iEnd{ aEnd }, iSkipAmount{ std::max(1u, std::min<std::size_t>(aEnd - aBegin, aSkipAmount)) }, iPasses{ aPasses }, iNext{ aBegin }, iSkipPass{ 1u }, iPass{ 1u }
			{
			}
			skip_iterator() :
				iBegin{ aBegin }, iEnd{ aEnd }, iSkipAmount{ 2u }, iPasses{ 1u }, iNext{ aBegin }, iSkipPass{ 1u }, iPass{ 1u }
			{
			}
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

		inline GLenum path_shape_to_gl_mode(path::shape_type_e aShape)
		{
			switch (aShape)
			{
			case path::Quads:
				return GL_QUADS;
			case path::Lines:
				return GL_LINES;
			case path::LineLoop:
				return GL_LINE_LOOP;
			case path::LineStrip:
				return GL_LINE_STRIP;
			case path::ConvexPolygon:
				return GL_TRIANGLE_FAN;
			default:
				return GL_POINTS;
			}
		}

		inline GLenum path_shape_to_gl_mode(const path& aPath)
		{
			return path_shape_to_gl_mode(aPath.shape());
		}

		inline double pixel_adjust(const dimension aWidth)
		{
			return static_cast<uint32_t>(aWidth) % 2 == 1 ? 0.5 : 0.0;
		}

		inline double pixel_adjust(const pen& aPen)
		{
			return pixel_adjust(aPen.width());
		}

		inline vertices_t line_loop_to_lines(const vertices_t& aLineLoop)
		{
			vertices_t result;
			result.reserve(aLineLoop.size() * 2);
			for (auto v = aLineLoop.begin(); v != aLineLoop.end(); ++v)
			{
				result.push_back(*v);
				if (v != aLineLoop.begin() && v != aLineLoop.end() - 1)
					result.push_back(*v);
			}
			return result;
		}
	}

	opengl_graphics_context::opengl_graphics_context(const i_render_target& aTarget) :
		iRenderingEngine{ service<i_rendering_engine>() },
		iTarget{aTarget}, 
		iWidget{ nullptr },
		iLogicalCoordinateSystem{ iTarget.logical_coordinate_system() },
		iLogicalCoordinates{ iTarget.logical_coordinates() },
		iBlendingMode{ neogfx::blending_mode::None },
		iSmoothingMode{ neogfx::smoothing_mode::None },
		iSubpixelRendering{ rendering_engine().is_subpixel_rendering_on() },
		iClipCounter{ 0 },
		iLineStippleActive{ false },
		iSrt{ iTarget }
	{
		rendering_engine().activate_shader_program(*this, rendering_engine().default_shader_program());
		set_smoothing_mode(neogfx::smoothing_mode::AntiAlias);
		iSink += render_target().target_deactivating([this]() 
		{ 
			flush(); 
		});
	}

	opengl_graphics_context::opengl_graphics_context(const i_render_target& aTarget, const i_widget& aWidget) :
		iRenderingEngine{ service<i_rendering_engine>() },
		iTarget{ aTarget },
		iWidget{ &aWidget },
		iLogicalCoordinateSystem{ aWidget.logical_coordinate_system() },
		iLogicalCoordinates{ iTarget.logical_coordinates() },
		iBlendingMode{ neogfx::blending_mode::None },
		iSmoothingMode{ neogfx::smoothing_mode::None },
		iSubpixelRendering{ rendering_engine().is_subpixel_rendering_on() },
		iClipCounter{ 0 },
		iLineStippleActive{ false },
		iSrt{ iTarget }
	{
		rendering_engine().activate_shader_program(*this, rendering_engine().default_shader_program());
		set_smoothing_mode(neogfx::smoothing_mode::AntiAlias);
		iSink += render_target().target_deactivating([this]()
		{
			flush();
		});
	}

	opengl_graphics_context::opengl_graphics_context(const opengl_graphics_context& aOther) :
		iRenderingEngine{ aOther.iRenderingEngine },
		iTarget{ aOther.iTarget },
		iWidget{ aOther.iWidget },
		iLogicalCoordinateSystem{ aOther.iLogicalCoordinateSystem },
		iLogicalCoordinates{ aOther.iLogicalCoordinates },
		iBlendingMode{ aOther.iBlendingMode },
		iSmoothingMode{ aOther.iSmoothingMode },
		iSubpixelRendering{ aOther.iSubpixelRendering },
		iClipCounter{ 0 },
		iLineStippleActive{ false },
		iSrt{ iTarget }
	{
		rendering_engine().activate_shader_program(*this, rendering_engine().default_shader_program());
		set_smoothing_mode(iSmoothingMode);
		iSink += render_target().target_deactivating([this]()
		{
			flush();
		});
	}

	opengl_graphics_context::~opengl_graphics_context()
	{
	}

	std::unique_ptr<i_graphics_context> opengl_graphics_context::clone() const
	{
		return std::unique_ptr<i_graphics_context>(new opengl_graphics_context(*this));
	}

	i_rendering_engine& opengl_graphics_context::rendering_engine()
	{
		return iRenderingEngine;
	}

	const i_render_target& opengl_graphics_context::render_target() const
	{
		return iTarget;
	}

	const i_render_target& opengl_graphics_context::render_target()
	{
		return iTarget;
	}

	rect opengl_graphics_context::rendering_area(bool aConsiderScissor) const
	{
		if (scissor_rect() == std::nullopt || !aConsiderScissor)
			return rect{ render_target().target_type() == render_target_type::Surface ? point{} : point{1.0, 1.0}, render_target().target_extents() };
		else
			return *scissor_rect();
	}

	neogfx::logical_coordinate_system opengl_graphics_context::logical_coordinate_system() const
	{
		return iLogicalCoordinateSystem;
	}

	void opengl_graphics_context::set_logical_coordinate_system(neogfx::logical_coordinate_system aSystem)
	{
		iLogicalCoordinateSystem = aSystem;
	}

	const neogfx::logical_coordinates& opengl_graphics_context::logical_coordinates() const
	{
		return to_logical_coordinates(render_target().target_extents(), iLogicalCoordinateSystem, iLogicalCoordinates);
	}

	void opengl_graphics_context::set_logical_coordinates(const neogfx::logical_coordinates& aCoordinates) const
	{
		iLogicalCoordinates = aCoordinates;
	}

	void opengl_graphics_context::enqueue(const graphics_operation::operation& aOperation)
	{
		scoped_render_target srt{ render_target() };

		if (iQueue.second.empty())
			iQueue.second.push_back(0);
		bool sameBatch = (iQueue.first.empty() || graphics_operation::batchable(iQueue.first.back(), aOperation)) && iQueue.first.size() - iQueue.second.back() < max_operations(aOperation);
		if (!sameBatch)
			iQueue.second.push_back(iQueue.first.size());
		iQueue.first.push_back(aOperation);
	}

	void opengl_graphics_context::flush()
	{
		if (iQueue.first.empty())
			return;

		iQueue.second.push_back(iQueue.first.size());
		auto endIndex = std::prev(iQueue.second.end());
		for (auto startIndex = iQueue.second.begin(); startIndex != endIndex; ++startIndex)
		{
			graphics_operation::batch opBatch{ &*iQueue.first.begin() + *startIndex, &*iQueue.first.begin() + *std::next(startIndex) };
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
			case graphics_operation::operation_type::ClipToRect:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
					clip_to(static_variant_cast<const graphics_operation::clip_to_rect&>(*op).rect);
				break;
			case graphics_operation::operation_type::ClipToPath:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
					clip_to(static_variant_cast<const graphics_operation::clip_to_path&>(*op).path, static_variant_cast<const graphics_operation::clip_to_path&>(*op).pathOutline);
				break;
			case graphics_operation::operation_type::ResetClip:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
				{
					(void)op;
					reset_clip();
				}
				break;
			case graphics_operation::operation_type::SetBlendingMode:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
					set_blending_mode(static_variant_cast<const graphics_operation::set_blending_mode&>(*op).blendingMode);
				break;
			case graphics_operation::operation_type::SetSmoothingMode:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
					set_smoothing_mode(static_variant_cast<const graphics_operation::set_smoothing_mode&>(*op).smoothingMode);
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
				for (auto op = opBatch.first; op != opBatch.second; ++op)
					line_stipple_on(static_variant_cast<const graphics_operation::line_stipple_on&>(*op).factor, static_variant_cast<const graphics_operation::line_stipple_on&>(*op).pattern);
				break;
			case graphics_operation::operation_type::LineStippleOff:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
				{
					(void)op;
					line_stipple_off();
				}
				break;
			case graphics_operation::operation_type::SubpixelRenderingOn:
				subpixel_rendering_on();
				break;
			case graphics_operation::operation_type::SubpixelRenderingOff:
				subpixel_rendering_off();
				break;
			case graphics_operation::operation_type::Clear:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
					clear(static_variant_cast<const graphics_operation::clear&>(*op).colour);
				break;
			case graphics_operation::operation_type::ClearDepthBuffer:
				clear_depth_buffer();
				break;
			case graphics_operation::operation_type::SetPixel:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
					set_pixel(static_variant_cast<const graphics_operation::set_pixel&>(*op).point, static_variant_cast<const graphics_operation::set_pixel&>(*op).colour);
				break;
			case graphics_operation::operation_type::DrawPixel:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
					draw_pixel(static_variant_cast<const graphics_operation::draw_pixel&>(*op).point, static_variant_cast<const graphics_operation::draw_pixel&>(*op).colour);
				break;
			case graphics_operation::operation_type::DrawLine:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
				{
					const auto& args = static_variant_cast<const graphics_operation::draw_line&>(*op);
					draw_line(args.from, args.to, args.pen);
				}
				break;
			case graphics_operation::operation_type::DrawRect:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
				{
					const auto& args = static_variant_cast<const graphics_operation::draw_rect&>(*op);
					draw_rect(args.rect, args.pen);
				}
				break;
			case graphics_operation::operation_type::DrawRoundedRect:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
				{
					const auto& args = static_variant_cast<const graphics_operation::draw_rounded_rect&>(*op);
					draw_rounded_rect(args.rect, args.radius, args.pen);
				}
				break;
			case graphics_operation::operation_type::DrawCircle:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
				{
					const auto& args = static_variant_cast<const graphics_operation::draw_circle&>(*op);
					draw_circle(args.centre, args.radius, args.pen, args.startAngle);
				}
				break;
			case graphics_operation::operation_type::DrawArc:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
				{
					const auto& args = static_variant_cast<const graphics_operation::draw_arc&>(*op);
					draw_arc(args.centre, args.radius, args.startAngle, args.endAngle, args.pen);
				}
				break;
			case graphics_operation::operation_type::DrawPath:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
				{
					const auto& args = static_variant_cast<const graphics_operation::draw_path&>(*op);
					draw_path(args.path, args.pen);
				}
				break;
			case graphics_operation::operation_type::DrawShape:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
				{
					const auto& args = static_variant_cast<const graphics_operation::draw_shape&>(*op);
					draw_shape(args.mesh, args.pen);
				}
				break;
			case graphics_operation::operation_type::DrawEntities:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
				{
					const auto& args = static_variant_cast<const graphics_operation::draw_entities&>(*op);
					draw_entities(args.ecs, args.transformation);
				}
				break;
			case graphics_operation::operation_type::FillRect:
				fill_rect(opBatch);
				break;
			case graphics_operation::operation_type::FillRoundedRect:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
				{
					const auto& args = static_variant_cast<const graphics_operation::fill_rounded_rect&>(*op);
					fill_rounded_rect(args.rect, args.radius, args.fill);
				}
				break;
			case graphics_operation::operation_type::FillCircle:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
				{
					const auto& args = static_variant_cast<const graphics_operation::fill_circle&>(*op);
					fill_circle(args.centre, args.radius, args.fill);
				}
				break;
			case graphics_operation::operation_type::FillArc:
				for (auto op = opBatch.first; op != opBatch.second; ++op)
				{
					const auto& args = static_variant_cast<const graphics_operation::fill_arc&>(*op);
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
				{
					use_shader_program usp{ *this, iRenderingEngine, rendering_engine().mesh_shader_program() };
					for (auto op = opBatch.first; op != opBatch.second; ++op)
					{
						const auto& args = static_variant_cast<const graphics_operation::draw_mesh&>(*op);
						draw_mesh(args.mesh, args.material, args.transformation);
					}
				}
				break;
			}
		}
		iQueue.first.clear();
		iQueue.second.clear();

		use_vertex_arrays uva{ *this, GL_TRIANGLES };
		uva.instance().execute();
	}

	void opengl_graphics_context::scissor_on(const rect& aRect)
	{
		if (iScissorRect == std::nullopt)
		{
			glCheck(glEnable(GL_SCISSOR_TEST));
			iScissorRect = aRect;
		}
		iScissorRects.push_back(*iScissorRect);
		iScissorRect = iScissorRect->intersection(aRect);
		apply_scissor();
	}

	void opengl_graphics_context::scissor_off()
	{
		auto previousScissorRect = iScissorRects.back();
		iScissorRects.pop_back();
		if (iScissorRects.empty())
		{
			glCheck(glDisable(GL_SCISSOR_TEST));
			iScissorRect = std::nullopt;
		}
		else
		{
			iScissorRect = previousScissorRect;
			apply_scissor();
		}
	}

	const optional_rect& opengl_graphics_context::scissor_rect() const
	{
		return iScissorRect;
	}

	void opengl_graphics_context::apply_scissor()
	{
		auto sr = *scissor_rect();
		GLint x = static_cast<GLint>(std::ceil(sr.x));
		GLint y = static_cast<GLint>(std::ceil(rendering_area(false).cy - sr.cy - sr.y));
		GLsizei cx = static_cast<GLsizei>(std::ceil(sr.cx));
		GLsizei cy = static_cast<GLsizei>(std::ceil(sr.cy));
		glCheck(glScissor(x, y, cx, cy));
	}

	void opengl_graphics_context::clip_to(const rect& aRect)
	{
		if (iClipCounter++ == 0)
		{
			glCheck(glClear(GL_STENCIL_BUFFER_BIT));
			glCheck(glEnable(GL_STENCIL_TEST));
		}
		glCheck(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));
		glCheck(glDepthMask(GL_FALSE));
		glCheck(glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP));  // draw 1s on test fail (always)
		glCheck(glStencilMask(static_cast<GLuint>(-1)));
		glCheck(glStencilFunc(GL_NEVER, 0, static_cast<GLuint>(-1)));
		fill_rect(rendering_area(), colour::White);
		glCheck(glStencilFunc(GL_NEVER, 1, static_cast<GLuint>(-1)));
		fill_rect(aRect, colour::White);
		glCheck(glStencilFunc(GL_NEVER, 1, static_cast<GLuint>(-1)));
		glCheck(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
		glCheck(glDepthMask(GL_TRUE));
		glCheck(glStencilMask(0x00));
		// draw only where stencil's value is 1
		glCheck(glStencilFunc(GL_EQUAL, 1, static_cast<GLuint>(-1)));
	}

	void opengl_graphics_context::clip_to(const path& aPath, dimension aPathOutline)
	{
		if (iClipCounter++ == 0)
		{
			glCheck(glClear(GL_STENCIL_BUFFER_BIT));
			glCheck(glEnable(GL_STENCIL_TEST));
		}
		glCheck(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));
		glCheck(glDepthMask(GL_FALSE));
		glCheck(glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP));  // draw 1s on test fail (always)
		glCheck(glStencilMask(static_cast<GLuint>(-1)));
		glCheck(glStencilFunc(GL_NEVER, 0, static_cast<GLuint>(-1)));
		fill_rect(rendering_area(), colour::White);
		glCheck(glStencilFunc(GL_EQUAL, 1, static_cast<GLuint>(-1)));
		for (std::size_t i = 0; i < aPath.paths().size(); ++i)
		{
			if (aPath.paths()[i].size() > 2)
			{
				auto vertices = aPath.to_vertices(aPath.paths()[i]);

				use_vertex_arrays vertexArrays{ *this, path_shape_to_gl_mode(aPath), vertices.size() };
				for (const auto& v : vertices)
				{
					vertexArrays.instance().push_back(opengl_standard_vertex_arrays::vertex{
						v, std::array<uint8_t, 4>{{0xFF, 0xFF, 0xFF, 0xFF}}
						});
				}
			}
		}
		if (aPathOutline != 0)
		{
			glCheck(glStencilFunc(GL_NEVER, 0, static_cast<GLuint>(-1)));
			path innerPath = aPath;
			innerPath.deflate(aPathOutline);
			for (std::size_t i = 0; i < innerPath.paths().size(); ++i)
			{
				if (innerPath.paths()[i].size() > 2)
				{
					auto vertices = aPath.to_vertices(innerPath.paths()[i]);

					use_vertex_arrays vertexArrays{ *this, path_shape_to_gl_mode(innerPath), vertices.size() };
					for (const auto& v : vertices)
					{
						vertexArrays.instance().push_back(opengl_standard_vertex_arrays::vertex{
							v, std::array<uint8_t, 4>{{0xFF, 0xFF, 0xFF, 0xFF}}
							});
					}
				}
			}
		}
		glCheck(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
		glCheck(glDepthMask(GL_TRUE));
		glCheck(glStencilMask(0x00));
		// draw only where stencil's value is 1
		glCheck(glStencilFunc(GL_EQUAL, 1, static_cast<GLuint>(-1)));
	}

	void opengl_graphics_context::reset_clip()
	{
		if (--iClipCounter == 0)
		{
			glCheck(glDisable(GL_STENCIL_TEST));
		}
	}

	neogfx::blending_mode opengl_graphics_context::blending_mode() const
	{
		return iBlendingMode;
	}

	void opengl_graphics_context::set_blending_mode(neogfx::blending_mode aBlendingMode)
		{
		iBlendingMode = aBlendingMode;
		switch (iBlendingMode)
		{
		case neogfx::blending_mode::None:
			glCheck(glDisable(GL_BLEND));
			break;
		case neogfx::blending_mode::Default:
			glCheck(glEnable(GL_BLEND));
			glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
			break;
		}
	}

	smoothing_mode opengl_graphics_context::smoothing_mode() const
	{
		return iSmoothingMode;
	}

	void opengl_graphics_context::set_smoothing_mode(neogfx::smoothing_mode aSmoothingMode)
	{
		iSmoothingMode = aSmoothingMode;
		if (iSmoothingMode == neogfx::smoothing_mode::AntiAlias)
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

	void opengl_graphics_context::push_logical_operation(logical_operation aLogicalOperation)
	{
		iLogicalOperationStack.push_back(aLogicalOperation);
		apply_logical_operation();
	}

	void opengl_graphics_context::pop_logical_operation()
	{
		if (!iLogicalOperationStack.empty())
			iLogicalOperationStack.pop_back();
		apply_logical_operation();
	}

	void opengl_graphics_context::apply_logical_operation()
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

	void opengl_graphics_context::gradient_on(const gradient& aGradient, const rect& aBoundingBox)
	{
		basic_rect<float> boundingBox{ aBoundingBox };
		iShaderProgramStack.emplace_back(*this, iRenderingEngine, rendering_engine().gradient_shader_program());
		rendering_engine().gradient_shader_program().set_uniform_variable("posViewportTop", static_cast<float>(logical_coordinates().first.y));
		rendering_engine().gradient_shader_program().set_uniform_variable("posTopLeft", boundingBox.top_left().x, boundingBox.top_left().y);
		rendering_engine().gradient_shader_program().set_uniform_variable("posBottomRight", boundingBox.bottom_right().x, boundingBox.bottom_right().y);
		rendering_engine().gradient_shader_program().set_uniform_variable("nGradientDirection", static_cast<int>(aGradient.direction()));
		rendering_engine().gradient_shader_program().set_uniform_variable("radGradientAngle", std::holds_alternative<double>(aGradient.orientation()) ? static_cast<float>(static_variant_cast<double>(aGradient.orientation())) : 0.0f);
		rendering_engine().gradient_shader_program().set_uniform_variable("nGradientStartFrom", std::holds_alternative<gradient::corner_e>(aGradient.orientation()) ? static_cast<int>(static_variant_cast<gradient::corner_e>(aGradient.orientation())) : -1);
		rendering_engine().gradient_shader_program().set_uniform_variable("nGradientSize", static_cast<int>(aGradient.size()));
		rendering_engine().gradient_shader_program().set_uniform_variable("nGradientShape", static_cast<int>(aGradient.shape()));
		basic_vector<float, 2> gradientExponents = (aGradient.exponents() != std::nullopt ? *aGradient.exponents() : vec2{2.0, 2.0});
		rendering_engine().gradient_shader_program().set_uniform_variable("exponents", gradientExponents.x, gradientExponents.y);
		basic_point<float> gradientCentre = (aGradient.centre() != std::nullopt ? *aGradient.centre() : point{});
		rendering_engine().gradient_shader_program().set_uniform_variable("posGradientCentre", gradientCentre.x, gradientCentre.y);
		auto combinedStops = aGradient.combined_stops();
		iGradientStopPositions.reserve(combinedStops.size());
		iGradientStopColours.reserve(combinedStops.size());
		iGradientStopPositions.clear();
		iGradientStopColours.clear();
		for (const auto& stop : combinedStops)
		{
			iGradientStopPositions.push_back(static_cast<float>(stop.first));
			iGradientStopColours.push_back(std::array<float, 4>{ {stop.second.red<float>(), stop.second.green<float>(), stop.second.blue<float>(), stop.second.alpha<float>()}});
		}
		rendering_engine().gradient_shader_program().set_uniform_variable("nStopCount", static_cast<int>(iGradientStopPositions.size()));
		rendering_engine().gradient_shader_program().set_uniform_variable("nFilterSize", static_cast<int>(GRADIENT_FILTER_SIZE));
		auto filter = static_gaussian_filter<float, GRADIENT_FILTER_SIZE>(static_cast<float>(aGradient.smoothness() * 10.0));
		auto& gradientArrays = iRenderingEngine.gradient_arrays(); 
		gradientArrays.stops.data().set_pixels(rect{ point{}, size_u32{ iGradientStopPositions.size(), 1 } }, &iGradientStopPositions[0]);
		gradientArrays.stopColours.data().set_pixels(rect{ point{}, size_u32{ iGradientStopColours.size(), 1 } }, &iGradientStopColours[0]);
		gradientArrays.filter.data().set_pixels(rect{ point(), size_u32{ GRADIENT_FILTER_SIZE, GRADIENT_FILTER_SIZE } }, &filter[0][0]);
		gradientArrays.stops.data().bind(2);
		gradientArrays.stopColours.data().bind(3);
		gradientArrays.filter.data().bind(4);
		rendering_engine().gradient_shader_program().set_uniform_variable("texStopPositions", 2);
		rendering_engine().gradient_shader_program().set_uniform_variable("texStopColours", 3);
		rendering_engine().gradient_shader_program().set_uniform_variable("texFilter", 4);
		glCheck(glActiveTexture(GL_TEXTURE1));
	}

	void opengl_graphics_context::gradient_off()
	{
		iShaderProgramStack.pop_back();
	}

	void opengl_graphics_context::line_stipple_on(uint32_t aFactor, uint16_t aPattern)
	{
		// TODO: glLineStipple unavailable in 3.2
		iLineStippleActive = true;
	}

	void opengl_graphics_context::line_stipple_off()
	{
		// TODO: glLineStipple unavailable in 3.2
		iLineStippleActive = false;
	}

	bool opengl_graphics_context::is_subpixel_rendering_on() const
	{
		return iSubpixelRendering;
	}

	void opengl_graphics_context::subpixel_rendering_on()
	{
		iSubpixelRendering = true;
	}

	void opengl_graphics_context::subpixel_rendering_off()
	{
		iSubpixelRendering = false;
	}

	void opengl_graphics_context::clear(const colour& aColour)
	{
		glCheck(glClearColor(aColour.red<GLclampf>(), aColour.green<GLclampf>(), aColour.blue<GLclampf>(), aColour.alpha<GLclampf>()));
		glCheck(glClear(GL_COLOR_BUFFER_BIT));
	}

	void opengl_graphics_context::clear_depth_buffer()
	{
		glCheck(glClear(GL_DEPTH_BUFFER_BIT));
	}

	void opengl_graphics_context::set_pixel(const point& aPoint, const colour& aColour)
	{
		/* todo: faster alternative to this... */
		disable_anti_alias daa{ *this };
		draw_pixel(aPoint, aColour.with_alpha(0xFF));
	}

	void opengl_graphics_context::draw_pixel(const point& aPoint, const colour& aColour)
	{
		/* todo: faster alternative to this... */
		fill_rect(rect{ aPoint, size{1.0, 1.0} }, aColour);
	}

	void opengl_graphics_context::draw_line(const point& aFrom, const point& aTo, const pen& aPen)
	{
		use_shader_program usp{ *this, iRenderingEngine, rendering_engine().default_shader_program() };

		if (std::holds_alternative<gradient>(aPen.colour()))
		{
			auto const& gradient = static_variant_cast<const neogfx::gradient&>(aPen.colour());
			gradient_on(gradient, rect{ aFrom, aTo });
		}

		double pixelAdjust = pixel_adjust(aPen);
		auto penColour = std::holds_alternative<colour>(aPen.colour()) ?
			std::array<uint8_t, 4>{{
				static_variant_cast<colour>(aPen.colour()).red(),
				static_variant_cast<colour>(aPen.colour()).green(),
				static_variant_cast<colour>(aPen.colour()).blue(),
				static_variant_cast<colour>(aPen.colour()).alpha()}} :
			std::array<uint8_t, 4>{};

		glCheck(glLineWidth(static_cast<GLfloat>(aPen.width())));
		{
			use_vertex_arrays vertexArrays{ *this, GL_LINES, 2u };
			vertexArrays.instance().push_back(opengl_standard_vertex_arrays::vertex{ xyz{aFrom.x + pixelAdjust, aFrom.y + pixelAdjust}, penColour });
			vertexArrays.instance().push_back(opengl_standard_vertex_arrays::vertex{ xyz{aTo.x + pixelAdjust, aTo.y + pixelAdjust}, penColour });
		}
		glCheck(glLineWidth(1.0f));

		if (std::holds_alternative<gradient>(aPen.colour()))
			gradient_off();
	}

	void opengl_graphics_context::draw_rect(const rect& aRect, const pen& aPen)
	{
		use_shader_program usp{ *this, iRenderingEngine, rendering_engine().default_shader_program() };

		if (std::holds_alternative<gradient>(aPen.colour()))
		{
			auto const& gradient = static_variant_cast<const neogfx::gradient&>(aPen.colour());
			gradient_on(gradient, aRect);
		}

		glCheck(glLineWidth(static_cast<GLfloat>(aPen.width())));
		{
			use_vertex_arrays vertexArrays{ *this, GL_LINES, 8u };
			back_insert_rect_vertices(vertexArrays.instance(), aRect, pixel_adjust(aPen), mesh_type::Outline);
			for (auto& v : vertexArrays.instance())
				v.rgba = colour_to_vec4f(std::holds_alternative<colour>(aPen.colour()) ?
					std::array <uint8_t, 4>{{
						static_variant_cast<colour>(aPen.colour()).red(),
						static_variant_cast<colour>(aPen.colour()).green(),
						static_variant_cast<colour>(aPen.colour()).blue(),
						static_variant_cast<colour>(aPen.colour()).alpha()}} :
					std::array <uint8_t, 4>{});
		}
		glCheck(glLineWidth(1.0f));

		if (std::holds_alternative<gradient>(aPen.colour()))
			gradient_off();
	}

	void opengl_graphics_context::draw_rounded_rect(const rect& aRect, dimension aRadius, const pen& aPen)
	{
		use_shader_program usp{ *this, iRenderingEngine, rendering_engine().default_shader_program() };

		if (std::holds_alternative<gradient>(aPen.colour()))
		{
			auto const& gradient = static_variant_cast<const neogfx::gradient&>(aPen.colour());
			gradient_on(gradient, aRect);
		}

		double pixelAdjust = pixel_adjust(aPen);
		auto vertices = rounded_rect_vertices(aRect + point{ pixelAdjust, pixelAdjust }, aRadius, mesh_type::Outline);

		glCheck(glLineWidth(static_cast<GLfloat>(aPen.width())));
		{
			use_vertex_arrays vertexArrays{ *this, GL_LINE_LOOP, vertices.size() };
			for (const auto& v : vertices)
				vertexArrays.instance().push_back({ v, std::holds_alternative<colour>(aPen.colour()) ?
					std::array <uint8_t, 4>{{
						static_variant_cast<colour>(aPen.colour()).red(),
						static_variant_cast<colour>(aPen.colour()).green(),
						static_variant_cast<colour>(aPen.colour()).blue(),
						static_variant_cast<colour>(aPen.colour()).alpha()}} :
					std::array <uint8_t, 4>{}});
		}
		glCheck(glLineWidth(1.0f));

		if (std::holds_alternative<gradient>(aPen.colour()))
			gradient_off();
	}

	void opengl_graphics_context::draw_circle(const point& aCentre, dimension aRadius, const pen& aPen, angle aStartAngle)
	{
		use_shader_program usp{ *this, iRenderingEngine, rendering_engine().default_shader_program() };

		if (std::holds_alternative<gradient>(aPen.colour()))
		{
			auto const& gradient = static_variant_cast<const neogfx::gradient&>(aPen.colour());
			gradient_on(gradient, rect{ aCentre - size{aRadius, aRadius}, size{aRadius * 2.0, aRadius * 2.0 } });
		}

		auto vertices = circle_vertices(aCentre, aRadius, aStartAngle, mesh_type::Outline);

		glCheck(glLineWidth(static_cast<GLfloat>(aPen.width())));
		{
			use_vertex_arrays vertexArrays{ *this, GL_LINE_LOOP, vertices.size() };
			for (const auto& v : vertices)
				vertexArrays.instance().push_back({ v, std::holds_alternative<colour>(aPen.colour()) ?
					std::array <uint8_t, 4>{{
						static_variant_cast<colour>(aPen.colour()).red(),
						static_variant_cast<colour>(aPen.colour()).green(),
						static_variant_cast<colour>(aPen.colour()).blue(),
						static_variant_cast<colour>(aPen.colour()).alpha()}} :
					std::array <uint8_t, 4>{}});
		}
		glCheck(glLineWidth(1.0f));

		if (std::holds_alternative<gradient>(aPen.colour()))
			gradient_off();
	}

	void opengl_graphics_context::draw_arc(const point& aCentre, dimension aRadius, angle aStartAngle, angle aEndAngle, const pen& aPen)
	{
		use_shader_program usp{ *this, iRenderingEngine, rendering_engine().default_shader_program() };

		if (std::holds_alternative<gradient>(aPen.colour()))
		{
			auto const& gradient = static_variant_cast<const neogfx::gradient&>(aPen.colour());
			gradient_on(gradient, rect{ aCentre - size{ aRadius, aRadius }, size{ aRadius * 2.0, aRadius * 2.0 } });
		}

		auto vertices = line_loop_to_lines(arc_vertices(aCentre, aRadius, aStartAngle, aEndAngle, aCentre, mesh_type::Outline));

		glCheck(glLineWidth(static_cast<GLfloat>(aPen.width())));
		{
			use_vertex_arrays vertexArrays{ *this, GL_LINES, vertices.size() };
			for (const auto& v : vertices)
				vertexArrays.instance().push_back({ v, std::holds_alternative<colour>(aPen.colour()) ?
					std::array <uint8_t, 4>{ {
						static_variant_cast<colour>(aPen.colour()).red(),
						static_variant_cast<colour>(aPen.colour()).green(),
						static_variant_cast<colour>(aPen.colour()).blue(),
						static_variant_cast<colour>(aPen.colour()).alpha()}} :
					std::array <uint8_t, 4>{} });
		}
		glCheck(glLineWidth(1.0f));

		if (std::holds_alternative<gradient>(aPen.colour()))
			gradient_off();
	}

	void opengl_graphics_context::draw_path(const path& aPath, const pen& aPen)
	{
		use_shader_program usp{ *this, iRenderingEngine, rendering_engine().default_shader_program() };

		if (std::holds_alternative<gradient>(aPen.colour()))
		{
			auto const& gradient = static_variant_cast<const neogfx::gradient&>(aPen.colour());
			gradient_on(gradient, aPath.bounding_rect());
		}

		for (std::size_t i = 0; i < aPath.paths().size(); ++i)
		{
			if (aPath.paths()[i].size() > 2)
			{
				if (aPath.shape() == path::ConvexPolygon)
					clip_to(aPath, aPen.width());

				auto vertices = aPath.to_vertices(aPath.paths()[i]);

				{
					use_vertex_arrays vertexArrays{ *this, path_shape_to_gl_mode(aPath.shape()), vertices.size() };
					for (const auto& v : vertices)
						vertexArrays.instance().push_back({ v, std::holds_alternative<colour>(aPen.colour()) ?
							std::array <uint8_t, 4>{{
								static_variant_cast<colour>(aPen.colour()).red(),
								static_variant_cast<colour>(aPen.colour()).green(),
								static_variant_cast<colour>(aPen.colour()).blue(),
								static_variant_cast<colour>(aPen.colour()).alpha()}} :
							std::array <uint8_t, 4>{}});
				}
				if (aPath.shape() == path::ConvexPolygon)
					reset_clip();
			}
		}

		if (std::holds_alternative<gradient>(aPen.colour()))
			gradient_off();
	}

	void opengl_graphics_context::draw_shape(const game::mesh& aMesh, const pen& aPen)
	{
		auto const& vertices = aMesh.vertices;

		use_shader_program usp{ *this, iRenderingEngine, rendering_engine().default_shader_program() };

		if (std::holds_alternative<gradient>(aPen.colour()))
		{
			auto const& gradient = static_variant_cast<const neogfx::gradient&>(aPen.colour());
			gradient_on(gradient, bounding_rect(aMesh));
		}

		{
			use_vertex_arrays vertexArrays{ *this, GL_LINE_LOOP, vertices.size() };
			for (const auto& v : vertices)
				vertexArrays.instance().push_back({ v, std::holds_alternative<colour>(aPen.colour()) ?
					std::array <uint8_t, 4>{{
						static_variant_cast<colour>(aPen.colour()).red(),
						static_variant_cast<colour>(aPen.colour()).green(),
						static_variant_cast<colour>(aPen.colour()).blue(),
						static_variant_cast<colour>(aPen.colour()).alpha()}} :
					std::array <uint8_t, 4>{}});
		}

		if (std::holds_alternative<gradient>(aPen.colour()))
			gradient_off();
	}

	void opengl_graphics_context::draw_entities(game::i_ecs& aEcs, const mat44& aTransformation)
	{
		iRenderingEngine.want_game_mode();
		aEcs.component<game::rigid_body>().take_snapshot();
		auto rigidBodiesSnapshot = aEcs.component<game::rigid_body>().snapshot();
		auto const& rigidBodies = rigidBodiesSnapshot.data();
		use_vertex_arrays uva{ *this, GL_TRIANGLES, with_textures };
		use_shader_program usp{ *this, iRenderingEngine, rendering_engine().mesh_shader_program() };
		texture_sampling withTextureSampling = texture_sampling::Normal;
		for (auto entity : aEcs.component<game::mesh_renderer>().entities())
		{
			if (entity == game::null_entity)
				continue; // todo: sort/remove and/or create skipping iterator
			auto const& meshFilter = aEcs.component<game::mesh_filter>().entity_record(entity);
			auto const& meshRenderer = aEcs.component<game::mesh_renderer>().entity_record(entity);
			auto transformation = rigidBodies.has_entity_record(entity) ? aTransformation * to_transformation_matrix(rigidBodies.entity_record(entity)) : aTransformation;
			bool renderTexture = (meshRenderer.material.texture != std::nullopt);
			bool drawn = draw_mesh(
				meshFilter,
				meshRenderer,
				transformation);
			if (!drawn && meshRenderer.destroyOnFustrumCull)
				aEcs.destroy_entity(entity);
		}
	}

	void opengl_graphics_context::fill_rect(const rect& aRect, const brush& aFill, scalar aZpos)
	{
		use_shader_program usp{ *this, iRenderingEngine, rendering_engine().default_shader_program() };

		graphics_operation::operation op{ graphics_operation::fill_rect{ aRect, aFill, aZpos } };
		fill_rect(graphics_operation::batch{ &op, &op + 1 });
	}

	void opengl_graphics_context::fill_rect(const graphics_operation::batch& aFillRectOps)
	{
		use_shader_program usp{ *this, iRenderingEngine, rendering_engine().default_shader_program() };

		auto& firstOp = static_variant_cast<const graphics_operation::fill_rect&>(*aFillRectOps.first);

		if (std::holds_alternative<gradient>(firstOp.fill))
			gradient_on(static_variant_cast<const gradient&>(firstOp.fill), firstOp.rect);

		{
			use_vertex_arrays vertexArrays{ *this, GL_TRIANGLES, 2u * 3u * (aFillRectOps.second - aFillRectOps.first)};

			for (auto op = aFillRectOps.first; op != aFillRectOps.second; ++op)
			{
				auto& drawOp = static_variant_cast<const graphics_operation::fill_rect&>(*op);
				auto newVertices = back_insert_rect_vertices(vertexArrays.instance(), drawOp.rect, 0.0, mesh_type::Triangles, drawOp.zpos);
				for (auto i = newVertices; i != vertexArrays.instance().end(); ++i)
					i->rgba = colour_to_vec4f(std::holds_alternative<colour>(drawOp.fill) ?
						std::array<uint8_t, 4>{{
							static_variant_cast<const colour&>(drawOp.fill).red(),
							static_variant_cast<const colour&>(drawOp.fill).green(),
							static_variant_cast<const colour&>(drawOp.fill).blue(),
							static_variant_cast<const colour&>(drawOp.fill).alpha()}} :
						std::array<uint8_t, 4>{});
			}
		}

		if (std::holds_alternative<gradient>(firstOp.fill))
			gradient_off();
	}

	void opengl_graphics_context::fill_rounded_rect(const rect& aRect, dimension aRadius, const brush& aFill)
	{
		if (aRect.empty())
			return;

		use_shader_program usp{ *this, iRenderingEngine, rendering_engine().default_shader_program() };

		if (std::holds_alternative<gradient>(aFill))
			gradient_on(static_variant_cast<const gradient&>(aFill), aRect);

		auto vertices = rounded_rect_vertices(aRect, aRadius, mesh_type::Triangles);
		
		{
			use_vertex_arrays vertexArrays{ *this, GL_TRIANGLES, vertices.size() };

			for (const auto& v : vertices)
			{
				vertexArrays.instance().push_back({v, std::holds_alternative<colour>(aFill) ?
					std::array <uint8_t, 4>{{
						static_variant_cast<const colour&>(aFill).red(),
						static_variant_cast<const colour&>(aFill).green(),
						static_variant_cast<const colour&>(aFill).blue(),
						static_variant_cast<const colour&>(aFill).alpha()}} :
					std::array <uint8_t, 4>{}}); 
			}
		}

		if (std::holds_alternative<gradient>(aFill))
			gradient_off();
	}

	void opengl_graphics_context::fill_circle(const point& aCentre, dimension aRadius, const brush& aFill)
	{
		if (std::holds_alternative<gradient>(aFill))
			gradient_on(static_variant_cast<const gradient&>(aFill), rect{ aCentre - point{ aRadius, aRadius }, size{ aRadius * 2.0 } });

		auto vertices = circle_vertices(aCentre, aRadius, 0.0, mesh_type::TriangleFan);

		{
			use_vertex_arrays vertexArrays{ *this, GL_TRIANGLE_FAN, vertices.size() };
			for (const auto& v : vertices)
			{
				vertexArrays.instance().push_back({v, std::holds_alternative<colour>(aFill) ?
					std::array <uint8_t, 4>{{
						static_variant_cast<const colour&>(aFill).red(),
						static_variant_cast<const colour&>(aFill).green(),
						static_variant_cast<const colour&>(aFill).blue(),
						static_variant_cast<const colour&>(aFill).alpha()}} :
					std::array <uint8_t, 4>{}});
			}
		}

		if (std::holds_alternative<gradient>(aFill))
			gradient_off();
	}

	void opengl_graphics_context::fill_arc(const point& aCentre, dimension aRadius, angle aStartAngle, angle aEndAngle, const brush& aFill)
	{
		if (std::holds_alternative<gradient>(aFill))
			gradient_on(static_variant_cast<const gradient&>(aFill), rect{ aCentre - point{ aRadius, aRadius }, size{ aRadius * 2.0 } });

		auto vertices = arc_vertices(aCentre, aRadius, aStartAngle, aEndAngle, aCentre, mesh_type::TriangleFan);

		{
			use_vertex_arrays vertexArrays{ *this, GL_TRIANGLE_FAN, vertices.size() };
			for (const auto& v : vertices)
			{
				vertexArrays.instance().push_back({v, std::holds_alternative<colour>(aFill) ?
					std::array <uint8_t, 4>{{
						static_variant_cast<const colour&>(aFill).red(),
						static_variant_cast<const colour&>(aFill).green(),
						static_variant_cast<const colour&>(aFill).blue(),
						static_variant_cast<const colour&>(aFill).alpha()}} :
					std::array <uint8_t, 4>{}});
			}
		}

		if (std::holds_alternative<gradient>(aFill))
			gradient_off();
	}

	void opengl_graphics_context::fill_path(const path& aPath, const brush& aFill)
	{
		for (std::size_t i = 0; i < aPath.paths().size(); ++i)
		{
			if (aPath.paths()[i].size() > 2)
			{
				clip_to(aPath, 0.0);
				point min = aPath.paths()[i][0];
				point max = min;
				for (auto const& pt : aPath.paths()[i])
				{
					min = min.min(pt);
					max = max.max(pt);
				}

				if (std::holds_alternative<gradient>(aFill))
					gradient_on(static_variant_cast<const gradient&>(aFill), rect{ point{ min.x, min.y }, size{ max.x - min.y, max.y - min.y } });

				auto vertices = aPath.to_vertices(aPath.paths()[i]);

				{
					use_vertex_arrays vertexArrays{ *this, path_shape_to_gl_mode(aPath.shape()), vertices.size() };
					for (const auto& v : vertices)
					{
						vertexArrays.instance().push_back({v, std::holds_alternative<colour>(aFill) ?
							std::array <uint8_t, 4>{{
								static_variant_cast<const colour&>(aFill).red(),
								static_variant_cast<const colour&>(aFill).green(),
								static_variant_cast<const colour&>(aFill).blue(),
								static_variant_cast<const colour&>(aFill).alpha()}} :
							std::array <uint8_t, 4>{}});
					}
				}

				reset_clip();

				if (std::holds_alternative<gradient>(aFill))
					gradient_off();
			}
		}
	}

	void opengl_graphics_context::fill_shape(const graphics_operation::batch& aFillShapeOps)
	{
		auto& firstOp = static_variant_cast<const graphics_operation::fill_shape&>(*aFillShapeOps.first);

		if (std::holds_alternative<gradient>(firstOp.fill))
		{
			auto const& vertices = firstOp.mesh.vertices;
			vec3 min = vertices[0].xyz;
			vec3 max = min;
			for (auto const& v : vertices)
			{
				min.x = std::min(min.x, v.x);
				max.x = std::max(max.x, v.x);
				min.y = std::min(min.y, v.y);
				max.y = std::max(max.y, v.y);
			}
			gradient_on(static_variant_cast<const gradient&>(firstOp.fill),
				rect{
					point{ min.x, min.y },
					size{ max.x - min.y, max.y - min.y } });
		}

		{
			use_vertex_arrays vertexArrays{ *this, GL_TRIANGLES };
			for (auto op = aFillShapeOps.first; op != aFillShapeOps.second; ++op)
			{
				auto& drawOp = static_variant_cast<const graphics_operation::fill_shape&>(*op);
				auto const& vertices = drawOp.mesh.vertices;
				auto const& uv = drawOp.mesh.uv;
				if (!vertexArrays.instance().room_for(drawOp.mesh.faces.size() * 3u))
					vertexArrays.instance().execute();
				for (auto const& f : drawOp.mesh.faces)
				{
					for (auto vi : f)
					{
						auto const& v = vertices[vi];
						vertexArrays.instance().push_back({
							v,
							std::holds_alternative<colour>(drawOp.fill) ?
								std::array <uint8_t, 4>{{
									static_variant_cast<const colour&>(drawOp.fill).red(),
									static_variant_cast<const colour&>(drawOp.fill).green(),
									static_variant_cast<const colour&>(drawOp.fill).blue(),
									static_variant_cast<const colour&>(drawOp.fill).alpha()}} :
								std::array <uint8_t, 4>{},
							uv[vi]});
					}
				}
			}
		}

		if (std::holds_alternative<gradient>(firstOp.fill))
			gradient_off();
	}

	namespace
	{
		void texture_vertices(const size& aTextureStorageSize, const rect& aTextureRect, const neogfx::logical_coordinates& aLogicalCoordinates, vertices_2d_t& aResult)
		{
			rect normalizedRect = aTextureRect / aTextureStorageSize;
			aResult.emplace_back(normalizedRect.top_left().x, normalizedRect.top_left().y);
			aResult.emplace_back(normalizedRect.top_right().x, normalizedRect.top_right().y);
			aResult.emplace_back(normalizedRect.bottom_right().x, normalizedRect.bottom_right().y);
			aResult.emplace_back(normalizedRect.bottom_left().x, normalizedRect.bottom_left().y);
			if (aLogicalCoordinates.first.y > aLogicalCoordinates.second.y)
			{
				std::swap(aResult[0][1], aResult[2][1]);
				std::swap(aResult[1][1], aResult[3][1]);
			}
		}
	}

	std::size_t opengl_graphics_context::max_operations(const graphics_operation::operation& aOperation)
	{
		auto need = 1u;
		if (std::holds_alternative<graphics_operation::draw_glyph>(aOperation))
		{
			need = 6u;
			auto& drawGlyphOp = static_variant_cast<const graphics_operation::draw_glyph&>(aOperation);
			if (drawGlyphOp.appearance.has_effect() && drawGlyphOp.appearance.effect().type() == text_effect_type::Outline)
				need += 6u * static_cast<uint32_t>(std::ceil((drawGlyphOp.appearance.effect().width() * 2 + 1) * (drawGlyphOp.appearance.effect().width() * 2 + 1)));
		}
		return rendering_engine().vertex_arrays().capacity() / need;
	}

	void opengl_graphics_context::draw_glyph(const graphics_operation::batch& aDrawGlyphOps)
	{
		auto& firstOp = static_variant_cast<const graphics_operation::draw_glyph&>(*aDrawGlyphOps.first);

		if (firstOp.glyph.is_emoji())
		{
			use_shader_program usp{ *this, iRenderingEngine, rendering_engine().default_shader_program() };
			if (firstOp.appearance.has_paper())
				fill_rect(rect{ point{ firstOp.point }, glyph_extents(firstOp) }, to_brush(firstOp.appearance.paper()), firstOp.point.z);
			auto const& emojiAtlas = rendering_engine().font_manager().emoji_atlas();
			auto const& emojiTexture = emojiAtlas.emoji_texture(firstOp.glyph.value()).as_sub_texture();
			draw_mesh(
				to_ecs_component(rect{ firstOp.point, glyph_extents(firstOp) }),
				game::material{ 
					{}, 
					{}, 
					{}, 
					to_ecs_component(emojiTexture)
				}, 
				mat44::identity());
			glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
			glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
			return;
		}

		const i_glyph_texture& firstGlyphTexture = glyph_texture(firstOp);

		bool renderEffects = !firstOp.appearance.only_calculate_effect() && firstOp.appearance.has_effect() && firstOp.appearance.effect().type() == text_effect_type::Outline;

		for (uint32_t pass = (renderEffects ? 1 : 2); pass <= 3; ++pass)
		{
			switch (pass)
			{
			case 1:
			case 3:
				{
					auto const glyphCount = std::count_if(aDrawGlyphOps.first, aDrawGlyphOps.second, [](const graphics_operation::operation& op) { return !static_variant_cast<const graphics_operation::draw_glyph&>(op).glyph.is_whitespace(); });
					auto const need = (pass == 3 ? 6u * glyphCount : 6u * static_cast<uint32_t>(std::ceil((firstOp.appearance.effect().width() * 2 + 1) * (firstOp.appearance.effect().width() * 2 + 1))) * glyphCount);

					bool const useTextureBarrier = firstOp.glyph.subpixel() && firstGlyphTexture.subpixel();
					use_vertex_arrays vertexArrays{ *this, GL_QUADS, with_textures, need, useTextureBarrier };

					auto const scanlineOffsets = (pass == 1 ? static_cast<uint32_t>(firstOp.appearance.effect().width()) * 2u + 1u : 1u);
					auto const offsets = scanlineOffsets * scanlineOffsets;
					point const offsetOrigin{ pass == 1 ? -firstOp.appearance.effect().width() : 0.0, pass == 1 ? -firstOp.appearance.effect().width() : 0.0 };
					for (auto op = skip_iterator<graphics_operation::operation>{ aDrawGlyphOps.first, aDrawGlyphOps.second, glyphCount / 2u, offsets }; op != aDrawGlyphOps.second; ++op)
					{
						auto& drawOp = static_variant_cast<const graphics_operation::draw_glyph&>(*op);

						if (drawOp.glyph.is_whitespace())
							continue;

						const font& glyphFont = service<i_font_manager>().font_from_id(drawOp.glyphFont);
						const i_glyph_texture& glyphTexture = glyph_texture(drawOp);

						vec3 glyphOrigin(
							drawOp.point.x + glyphTexture.placement().x,
							logical_coordinates().first.y < logical_coordinates().second.y ?
							drawOp.point.y + (glyphTexture.placement().y + -glyphFont.descender()) :
							drawOp.point.y + glyphFont.height() - (glyphTexture.placement().y + -glyphFont.descender()) - glyphTexture.texture().extents().cy,
							drawOp.point.z);

						iTempTextureCoords.clear();
						texture_vertices(glyphTexture.texture().atlas_texture().storage_extents(), rect{ glyphTexture.texture().atlas_location().top_left(), glyphTexture.texture().extents() }, logical_coordinates(), iTempTextureCoords);

						rect outputRect{ point{ glyphOrigin } +offsetOrigin + point{ static_cast<coordinate>((op.pass() - 1u) % scanlineOffsets), static_cast<coordinate>((op.pass() - 1u) / scanlineOffsets) }, glyphTexture.texture().extents() };
						vertices_t outputVertices = rect_vertices(outputRect, 0.0, mesh_type::Triangles, glyphOrigin.z);

						std::array<uint8_t, 4> passColour;

						if (pass == 1)
						{
							passColour = std::holds_alternative<colour>(drawOp.appearance.effect().colour()) ?
								std::array <uint8_t, 4>{ {
										static_variant_cast<const colour&>(drawOp.appearance.effect().colour()).red(),
											static_variant_cast<const colour&>(drawOp.appearance.effect().colour()).green(),
											static_variant_cast<const colour&>(drawOp.appearance.effect().colour()).blue(),
											static_variant_cast<const colour&>(drawOp.appearance.effect().colour()).alpha()}} :
								std::array <uint8_t, 4>{};
						}
						else
						{
							passColour = std::holds_alternative<colour>(drawOp.appearance.ink()) ?
								std::array <uint8_t, 4>{ {
										static_variant_cast<const colour&>(drawOp.appearance.ink()).red(),
											static_variant_cast<const colour&>(drawOp.appearance.ink()).green(),
											static_variant_cast<const colour&>(drawOp.appearance.ink()).blue(),
											static_variant_cast<const colour&>(drawOp.appearance.ink()).alpha()}} :
								std::array <uint8_t, 4>{};
						}

						vertexArrays.instance().push_back({ outputVertices[0], passColour, iTempTextureCoords[0] });
						vertexArrays.instance().push_back({ outputVertices[2], passColour, iTempTextureCoords[3] });
						vertexArrays.instance().push_back({ outputVertices[1], passColour, iTempTextureCoords[1] });
						vertexArrays.instance().push_back({ outputVertices[3], passColour, iTempTextureCoords[1] });
						vertexArrays.instance().push_back({ outputVertices[4], passColour, iTempTextureCoords[2] });
						vertexArrays.instance().push_back({ outputVertices[5], passColour, iTempTextureCoords[3] });
					}

					if (vertexArrays.instance().empty())
						continue;

					glCheck(glActiveTexture(GL_TEXTURE1));
					glCheck(glGetIntegerv(GL_TEXTURE_BINDING_2D, &iPreviousTexture));
					glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
					glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
					if (firstOp.glyph.subpixel() && firstGlyphTexture.subpixel())
					{
						glCheck(glActiveTexture(GL_TEXTURE2));
						glCheck(glBindTexture(
							render_target().target_texture().sampling() != texture_sampling::Multisample ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE,
							reinterpret_cast<GLuint>(render_target().target_texture().native_texture()->handle())));
						glCheck(glActiveTexture(GL_TEXTURE1));
					}

					if (std::holds_alternative<gradient>(firstOp.appearance.ink()))
						gradient_on(
							static_variant_cast<const gradient&>(firstOp.appearance.ink()),
							rect{
								point{
									vertexArrays.instance()[0].xyz[0],
									vertexArrays.instance()[0].xyz[1]},
								point{
									vertexArrays.instance()[2].xyz[0],
									vertexArrays.instance()[2].xyz[1]} });

					glCheck(glBindTexture(GL_TEXTURE_2D, reinterpret_cast<GLuint>(firstGlyphTexture.texture().native_texture()->handle())));

					disable_anti_alias daa{ *this };

					bool useSubpixelShader = render_target().target_type() == render_target_type::Surface && firstOp.glyph.subpixel() && firstGlyphTexture.subpixel();

					use_shader_program usp{ *this, iRenderingEngine, rendering_engine().glyph_shader_program(useSubpixelShader) };

					auto& shader = rendering_engine().active_shader_program();

					rendering_engine().vertex_arrays().instantiate_with_texture_coords(*this, shader);

					bool guiCoordinates = (logical_coordinates().first.y > logical_coordinates().second.y);
					shader.set_uniform_variable("guiCoordinates", guiCoordinates);
					shader.set_uniform_variable("outputExtents", static_cast<float>(render_target().target_extents().cx), static_cast<float>(render_target().target_extents().cy));

					shader.set_uniform_variable("glyphTexture", 1);

					if (useSubpixelShader)
						shader.set_uniform_variable("outputTexture", 2);

					shader.set_uniform_variable("subpixel", static_cast<int>(firstGlyphTexture.subpixel()));

					vertexArrays.instance().draw(need, glyphCount / 2u);
					vertexArrays.instance().execute();

					glCheck(glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(iPreviousTexture)));

					if (std::holds_alternative<gradient>(firstOp.appearance.ink()))
						gradient_off();
				}
				break;
			case 2:
				{
					thread_local std::vector<graphics_operation::operation> rects;
					rects.clear();
					rects.reserve(std::distance(aDrawGlyphOps.first, aDrawGlyphOps.second));
					for (auto op = aDrawGlyphOps.first; op != aDrawGlyphOps.second; ++op)
					{
						auto& drawOp = static_variant_cast<const graphics_operation::draw_glyph&>(*op);

						if (!drawOp.appearance.has_paper())
							continue;

						const font& glyphFont = service<i_font_manager>().font_from_id(drawOp.glyphFont);
						graphics_operation::fill_rect nextOp{ rect{ point{ drawOp.point }, size{ drawOp.glyph.advance().cx, glyphFont.height() } }, to_brush(drawOp.appearance.paper()), drawOp.point.z };
						if (!rects.empty() && !batchable(rects.back(), nextOp))
						{
							fill_rect(graphics_operation::batch{ &*rects.begin(), &*rects.begin() + rects.size() });
							rects.clear();
						}
						rects.push_back(nextOp);
					}
					if (!rects.empty())
						fill_rect(graphics_operation::batch{ &*rects.begin(), &*rects.begin() + rects.size() });
				}
				break;
			}
		}
	}

	bool opengl_graphics_context::draw_mesh(const game::mesh& aMesh, const game::material& aMaterial, const mat44& aTransformation)
	{
		return draw_mesh(game::mesh_filter{ { &aMesh }, {}, {} }, game::mesh_renderer{ aMaterial, {} }, aTransformation);
	}
	
	bool opengl_graphics_context::draw_mesh(const game::mesh_filter& aMeshFilter, const game::mesh_renderer& aMeshRenderer, const mat44& aTransformation)
	{
		auto const transformation = aTransformation * (aMeshFilter.transformation != std::nullopt ? *aMeshFilter.transformation : mat44::identity());

		auto const& vertices = transformation * (aMeshFilter.mesh != std::nullopt ? *aMeshFilter.mesh : *aMeshFilter.sharedMesh.ptr).vertices;
		auto const& uv = aMeshFilter.mesh != std::nullopt ? aMeshFilter.mesh->uv : aMeshFilter.sharedMesh.ptr->uv;
		auto const& faces = aMeshFilter.mesh != std::nullopt ? aMeshFilter.mesh->faces : aMeshFilter.sharedMesh.ptr->faces;

		auto const& material = aMeshRenderer.material;

		bool drawn = draw_patch(vertices, uv, material, faces);

		for (auto const& patch : aMeshRenderer.patches)
			drawn = (draw_patch(vertices, uv, patch.material, patch.faces) || drawn);

		return drawn;
	}


	bool opengl_graphics_context::draw_patch(const vertices_t& aVertices, const vertices_2d_t& aTextureVertices, const game::material& aMaterial, const game::faces_t& aFaces)
	{
		colour colourizationColour{ 0xFF, 0xFF, 0xFF, 0xFF };
		if (aMaterial.colour != std::nullopt)
			colourizationColour = aMaterial.colour->rgba;

		bool drawn = false;

		GLint previousTexture;

		{
			use_shader_program usp{ *this, iRenderingEngine, rendering_engine().mesh_shader_program() };
			rendering_engine().active_shader_program().set_uniform_variable("haveTexture", aMaterial.texture != std::nullopt);
			rendering_engine().active_shader_program().set_uniform_variable("effect", static_cast<int>(aMaterial.shaderEffect != std::nullopt ? *aMaterial.shaderEffect : shader_effect::None));

			if (aMaterial.texture != std::nullopt)
			{
				auto const& texture = *service<i_texture_manager>().find_texture(aMaterial.texture->id.cookie());

				glCheck(glActiveTexture(texture.sampling() != texture_sampling::Multisample ? GL_TEXTURE1 : GL_TEXTURE2));

				glCheck(glGetIntegerv(texture.sampling() != texture_sampling::Multisample ? GL_TEXTURE_BINDING_2D : GL_TEXTURE_BINDING_2D_MULTISAMPLE, &previousTexture));
				glCheck(glBindTexture(texture.sampling() != texture_sampling::Multisample ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE, reinterpret_cast<GLuint>(texture.native_texture()->handle())));
				if (texture.sampling() != texture_sampling::Multisample)
				{
					glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture.sampling() != texture_sampling::Nearest && texture.sampling() != texture_sampling::Data ? GL_LINEAR : GL_NEAREST));
					glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture.sampling() == texture_sampling::NormalMipmap ? GL_LINEAR_MIPMAP_LINEAR : texture.sampling() != texture_sampling::Nearest && texture.sampling() != texture_sampling::Data ? GL_LINEAR : GL_NEAREST));
				}
			}

			use_vertex_arrays vertexArrays{ *this, GL_TRIANGLES, with_textures, aFaces.size() * 3u };

			vec2 textureStorageExtents;
			vec2 uvFixupCoefficient;
			vec2 uvFixupOffset;

			if (aMaterial.texture != std::nullopt)
			{
				auto const& texture = *service<i_texture_manager>().find_texture(aMaterial.texture->id.cookie());

				uvFixupCoefficient = aMaterial.texture->extents;
				textureStorageExtents = texture.storage_extents().to_vec2();
				if (texture.type() == texture_type::Texture)
					uvFixupOffset = vec2{ 1.0, 1.0 };
				else if (aMaterial.texture->subTexture == std::nullopt)
					uvFixupOffset = texture.as_sub_texture().atlas_location().top_left().to_vec2();
				else
					uvFixupOffset = aMaterial.texture->subTexture->min;
				if (texture.sampling() != texture_sampling::Multisample)
				{
					rendering_engine().active_shader_program().set_uniform_variable("tex", 1);
					rendering_engine().active_shader_program().set_uniform_variable("multisample", false);
				}
				else
				{
					rendering_engine().active_shader_program().set_uniform_variable("texMS", 2);
					rendering_engine().active_shader_program().set_uniform_variable("multisample", true);
					rendering_engine().active_shader_program().set_uniform_variable("texSamples", static_cast<int>(texture.samples()));
					rendering_engine().active_shader_program().set_uniform_variable("texExtents", static_cast<float>(texture.storage_extents().cx), static_cast<float>(texture.storage_extents().cy));
				}
			}

			for (auto const& face : aFaces)
			{
				for (auto faceVertexIndex : face)
				{
					auto const& v = aVertices[faceVertexIndex];
					if (~v.x >= std::min(logical_coordinates().first.x, logical_coordinates().second.x) &&
						~v.x <= std::max(logical_coordinates().first.x, logical_coordinates().second.x) &&
						~v.y >= std::min(logical_coordinates().first.y, logical_coordinates().second.y) &&
						~v.y <= std::max(logical_coordinates().first.y, logical_coordinates().second.y))
						drawn = true;
					vec2 uv = {};
					if (aMaterial.texture != std::nullopt)
					{
						auto const& texture = *service<i_texture_manager>().find_texture(aMaterial.texture->id.cookie());
						uv = aTextureVertices[faceVertexIndex];
						if (texture.native_texture()->logical_coordinate_system() == neogfx::logical_coordinate_system::AutomaticGame && logical_coordinates().first.y > logical_coordinates().second.y)
							~uv.y = 1.0 - uv.y;
						uv = (uv * uvFixupCoefficient + uvFixupOffset) / textureStorageExtents;
					}
					vertexArrays.instance().emplace_back(
						v,
						std::array<uint8_t, 4>{ {
								colourizationColour.red(),
									colourizationColour.green(),
									colourizationColour.blue(),
									colourizationColour.alpha()}},
						uv);
				}
			}

			vertexArrays.instance().draw();
		}

		if (aMaterial.texture != std::nullopt)
		{
			auto const& texture = *service<i_texture_manager>().find_texture(aMaterial.texture->id.cookie());
			glCheck(glBindTexture(texture.sampling() != texture_sampling::Multisample ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE, static_cast<GLuint>(previousTexture)));
		}

		return drawn;
	}

	xyz opengl_graphics_context::to_shader_vertex(const point& aPoint, coordinate aZ) const
	{
		return xyz{{ aPoint.x, aPoint.y, aZ }};
	}
}