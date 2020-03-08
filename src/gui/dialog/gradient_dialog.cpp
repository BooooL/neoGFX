// gradient_dialog.cpp
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

#include <neogfx/gui/dialog/gradient_dialog.hpp>
#include <neolib/string_utils.hpp>
#include <neolib/thread.hpp>
#include <neogfx/core/numerical.hpp>
#include <neogfx/gui/dialog/message_box.hpp>
#include <neogfx/app/file_dialog.hpp>

namespace neogfx
{
    namespace
    {
        inline gradient convert_gimp_gradient(const gradient& aGradient, const std::string& aPath)
        {
            // todo: midpoint support
            std::ifstream gimpGradient{ aPath };
            std::string line;
            std::getline(gimpGradient, line);
            std::getline(gimpGradient, line);
            std::getline(gimpGradient, line);
            struct segment
            {
                double start;
                double mid;
                double end;
                vec4 startRgba;
                vec4 endRgba;
            };
            std::optional<segment> s;
            gradient::color_stop_list colorStops;
            gradient::alpha_stop_list alphaStops;
            while (std::getline(gimpGradient, line))
            {
                neolib::vecarray<std::string, 13> bits;
                neolib::tokens(line, " "s, bits);
                if (bits.size() != 13)
                    continue;
                s = segment
                {
                    boost::lexical_cast<double>(bits[0]),
                    boost::lexical_cast<double>(bits[1]),
                    boost::lexical_cast<double>(bits[2]),
                    vec4{
                        boost::lexical_cast<double>(bits[3]),
                        boost::lexical_cast<double>(bits[4]),
                        boost::lexical_cast<double>(bits[5]),
                        boost::lexical_cast<double>(bits[6]),
                    },
                    vec4{
                        boost::lexical_cast<double>(bits[7]),
                        boost::lexical_cast<double>(bits[8]),
                        boost::lexical_cast<double>(bits[9]),
                        boost::lexical_cast<double>(bits[10]),
                    }
                };
                colorStops.emplace_back(s->start, vec3{ s->startRgba.xyz });
                alphaStops.emplace_back(s->start, s->startRgba[3] * 0xFF);
                colorStops.emplace_back(s->mid, lerp(vec3{ s->startRgba.xyz }, vec3{ s->endRgba.xyz }, 0.5));
                alphaStops.emplace_back(s->mid, lerp(s->startRgba[3], s->endRgba[3], 0.5) * 0xFF);
            }
            colorStops.emplace_back(s->end, vec3{ s->endRgba.xyz });
            alphaStops.emplace_back(s->end, s->endRgba[3] * 0xFF);
            return gradient{ aGradient, colorStops, alphaStops };
        }
    }

    void draw_alpha_background(i_graphics_context& aGraphicsContext, const rect& aRect, dimension aAlphaPatternSize = 4.0);

    class gradient_dialog::preview_box : public framed_widget
    {
    public:
        preview_box(gradient_dialog& aOwner) :
            framed_widget(aOwner.iPreviewGroupBox.item_layout()),
            iOwner(aOwner),
            iAnimationTimer{ service<neolib::async_task>(), [this](neolib::callback_timer& aTimer)
            {
                iSink += surface().closed([&aTimer]() { aTimer.cancel(); });
                aTimer.again();
                animate();
            }, 10, true },
            iTracking{ false }
        {
            set_margins(neogfx::margins{});
        }
    public:
        virtual void paint(i_graphics_context& aGc) const
        {
            framed_widget::paint(aGc);
            auto const& cr = client_rect();
            draw_alpha_background(aGc, cr, 16.0_dip);
            aGc.fill_rect(cr, iOwner.gradient());
            if (iOwner.gradient().direction() == gradient_direction::Radial && iOwner.gradient().centre() != optional_point{})
            {
                point const centre{ cr.centre().x + cr.width() / 2.0 * iOwner.gradient().centre()->x, cr.centre().y + cr.height() / 2.0 * iOwner.gradient().centre()->y };
                auto const radius = 6.0_dip;
                auto const circumference = 2.0 * math::pi<double>() * radius;
                aGc.draw_circle(centre, radius, pen{ color::White, 1.0_dip });
                aGc.line_stipple_on(8.0_dip, 0x5555, circumference * neolib::thread::program_elapsed_ms() / 1000.0);
                aGc.draw_circle(centre, radius, pen{ color::Black, 1.0_dip });
                aGc.line_stipple_off();
            }
        }
    public:
        virtual void mouse_button_pressed(mouse_button aButton, const point& aPosition, key_modifiers_e aKeyModifiers)
        {
            framed_widget::mouse_button_pressed(aButton, aPosition, aKeyModifiers);
            if (aButton == mouse_button::Left)
            {
                select(aPosition - client_rect(false).top_left());
                iTracking = true;
            }
        }
        virtual void mouse_button_released(mouse_button aButton, const point& aPosition)
        {
            framed_widget::mouse_button_released(aButton, aPosition);
            if (!capturing())
                iTracking = false;
        }
        virtual void mouse_moved(const point& aPosition)
        {
            if (iTracking)
                select(aPosition - client_rect(false).top_left());
        }
        void select(const point& aPosition)
        {
            auto cr = client_rect(false);
            point centre{ aPosition.x / cr.width() * 2.0 - 1.0, aPosition.y / cr.height() * 2.0 - 1.0 };
            centre = centre.max(point{ -1.0, -1.0 }).min(point{ 1.0, 1.0 });
            iOwner.gradient_selector().set_gradient(iOwner.gradient().with_centre(centre));
        }
    private:
        void animate()
        {
            if (iOwner.gradient().direction() == gradient_direction::Radial && iOwner.gradient().centre() != optional_point{})
            {
                rect cr = client_rect();
                point centre{ cr.centre().x + cr.width() / 2.0 * iOwner.gradient().centre()->x, cr.centre().y + cr.height() / 2.0 * iOwner.gradient().centre()->y };
                update(rect{ centre - point{ 10, 10 }, size{ 20, 20 } });
            }
        }
    private:
        gradient_dialog& iOwner;
        neolib::sink iSink;
        neolib::callback_timer iAnimationTimer;
        bool iTracking;
    };

    gradient_dialog::gradient_dialog(i_widget& aParent, const neogfx::gradient& aCurrentGradient) :
        dialog(aParent, "Select Gradient"_t, window_style::Modal | window_style::TitleBar | window_style::Close),
        iLayout{ client_layout() }, iLayout2{ iLayout }, iLayout3{ iLayout2 }, iLayout4{ iLayout2 },
        iSelectorGroupBox{ iLayout3 },
        iGradientSelector{ *this, iSelectorGroupBox.item_layout(), aCurrentGradient },
        iLayout3_1{ iSelectorGroupBox.item_layout() },
        iImportGradient{ iLayout3_1, image{ ":/neogfx/resources/icons.naa#open.png" } },
        iLayout3_2{ iLayout3, alignment::Top },
        iDirectionGroupBox{ iLayout3_2, "Direction"_t },
        iDirectionHorizontalRadioButton{ iDirectionGroupBox.item_layout(), "Horizontal"_t },
        iDirectionVerticalRadioButton{ iDirectionGroupBox.item_layout(), "Vertical"_t },
        iDirectionDiagonalRadioButton{ iDirectionGroupBox.item_layout(), "Diagonal"_t },
        iDirectionRectangularRadioButton{ iDirectionGroupBox.item_layout(), "Rectangular"_t },
        iDirectionRadialRadioButton{ iDirectionGroupBox.item_layout(), "Radial"_t },
        iSmoothnessGroupBox{ iLayout3_2, "Smoothness (%)"_t },
        iSmoothnessSpinBox{ iSmoothnessGroupBox.item_layout() },
        iSmoothnessSlider{ iSmoothnessGroupBox.item_layout() },
        iLayout5{ iLayout3 },
        iOrientationGroupBox{ iLayout5, "Orientation"_t },
        iStartingFromGroupBox{ iOrientationGroupBox.with_item_layout<horizontal_layout>(), "Starting From"_t },
        iTopLeftRadioButton{ iStartingFromGroupBox.item_layout(), "Top left corner"_t },
        iTopRightRadioButton{ iStartingFromGroupBox.item_layout(), "Top right corner"_t },
        iBottomRightRadioButton{ iStartingFromGroupBox.item_layout(), "Bottom right corner"_t },
        iBottomLeftRadioButton{ iStartingFromGroupBox.item_layout(), "Bottom left corner"_t },
        iAngleRadioButton{ iStartingFromGroupBox.item_layout(), "At a specific angle"_t },
        iLayout6 { iOrientationGroupBox.item_layout() },
        iAngleGroupBox{ iLayout6 },
        iAngle{ iAngleGroupBox.with_item_layout<grid_layout>(), "Angle:"_t },
        iAngleSpinBox{ iAngleGroupBox.item_layout() },
        iAngleSlider{ iAngleGroupBox.item_layout() },
        iSizeGroupBox{ iLayout5, "Size"_t },
        iSizeClosestSideRadioButton{ iSizeGroupBox.item_layout(), "Closest side"_t },
        iSizeFarthestSideRadioButton{ iSizeGroupBox.item_layout(), "Farthest side"_t },
        iSizeClosestCornerRadioButton{ iSizeGroupBox.item_layout(), "Closest corner"_t },
        iSizeFarthestCornerRadioButton{ iSizeGroupBox.item_layout(), "Farthest corner"_t },
        iShapeGroupBox{ iLayout5, "Shape"_t },
        iShapeEllipseRadioButton{ iShapeGroupBox.item_layout(), "Ellipse"_t },
        iShapeCircleRadioButton{ iShapeGroupBox.item_layout(), "Circle"_t },
        iExponentGroupBox{ iLayout5, "Exponents"_t },
        iLinkedExponents{ iExponentGroupBox.with_item_layout<grid_layout>(3, 2).add_span(grid_layout::cell_coordinates{0, 0}, grid_layout::cell_coordinates{1, 0}), "Linked"_t },
        iMExponent{ iExponentGroupBox.item_layout(), "m:"_t },
        iMExponentSpinBox{ iExponentGroupBox.item_layout() },
        iNExponent{ iExponentGroupBox.item_layout(), "n:"_t },
        iNExponentSpinBox{ iExponentGroupBox.item_layout() },
        iCentreGroupBox{ iLayout5, "Centre"_t },
        iXCentre{ iCentreGroupBox.with_item_layout<grid_layout>(2, 2), "X:"_t },
        iXCentreSpinBox { iCentreGroupBox.item_layout() },
        iYCentre{ iCentreGroupBox.item_layout(), "Y:"_t },
        iYCentreSpinBox { iCentreGroupBox.item_layout() },
        iSpacer2{ iLayout5 },
        iSpacer3{ iLayout3 },
        iPreviewGroupBox{ iLayout4, "Preview"_t },
        iPreview{ new preview_box{*this} },
        iSpacer4{ iLayout4 },
        iUpdatingWidgets(false)
    {
        init();
    }

    gradient_dialog::~gradient_dialog()
    {
    }

    const gradient& gradient_dialog::gradient() const
    {
        return gradient_selector().gradient();
    }

    void gradient_dialog::set_gradient(const neogfx::gradient& aGradient)
    {
        gradient_selector().set_gradient(aGradient);
    }
    
    const gradient_widget& gradient_dialog::gradient_selector() const
    {
        return iGradientSelector;
    }

    gradient_widget& gradient_dialog::gradient_selector()
    {
        return iGradientSelector;
    }

    void gradient_dialog::init()
    {
        auto standardSpacing = set_standard_layout(size{ 16.0 });

        iLayout.set_margins(neogfx::margins{});
        iLayout.set_spacing(standardSpacing);
        iLayout2.set_margins(neogfx::margins{});
        iLayout2.set_spacing(standardSpacing);
        iLayout3.set_margins(neogfx::margins{});
        iLayout3.set_spacing(standardSpacing);
        iLayout5.set_alignment(alignment::Top);
        iImportGradient.image_widget().set_fixed_size(size{ 16.0_dip });
        iSmoothnessSpinBox.set_minimum(0.0);
        iSmoothnessSpinBox.set_maximum(100.0);
        iSmoothnessSpinBox.set_step(0.1);
        iSmoothnessSpinBox.set_format("%.1f");
        iSmoothnessSlider.set_minimum(0.0);
        iSmoothnessSlider.set_maximum(100.0);
        iSmoothnessSlider.set_step(0.1);
        iOrientationGroupBox.item_layout().set_alignment(alignment::Top);
        iAngleSpinBox.set_minimum(-360.0);
        iAngleSpinBox.set_maximum(360.0);
        iAngleSpinBox.set_step(0.1);
        iAngleSpinBox.set_format("%.1f");
        iAngleSlider.set_minimum(-360.0);
        iAngleSlider.set_maximum(360.0);
        iAngleSlider.set_step(0.1);
        iExponentGroupBox.set_checkable(true);
        iExponentGroupBox.item_layout().set_alignment(alignment::Right);
        iLinkedExponents.set_size_policy(size_constraint::Expanding);
        iLinkedExponents.set_checked(true);
        iMExponentSpinBox.set_minimum(0.0);
        iMExponentSpinBox.set_maximum(std::numeric_limits<double>::max());
        iMExponentSpinBox.set_step(0.1);
        iMExponentSpinBox.set_format("%.2f");
        iMExponentSpinBox.text_box().set_alignment(alignment::Right);
        iMExponentSpinBox.text_box().set_size_hint(size_hint{ "00.00" });
        iNExponentSpinBox.set_minimum(0.0);
        iNExponentSpinBox.set_maximum(std::numeric_limits<double>::max());
        iNExponentSpinBox.set_step(0.1);
        iNExponentSpinBox.set_format("%.2f");
        iNExponentSpinBox.text_box().set_alignment(alignment::Right);
        iNExponentSpinBox.text_box().set_size_hint(size_hint{ "00.00" });
        iCentreGroupBox.set_checkable(true);
        iXCentreSpinBox.set_minimum(-1.0);
        iXCentreSpinBox.set_maximum(1.0);
        iXCentreSpinBox.set_step(0.001);
        iXCentreSpinBox.set_format("%.3f");
        iXCentreSpinBox.text_box().set_alignment(alignment::Right);
        iXCentreSpinBox.text_box().set_size_hint(size_hint{ "-0.000" });
        iYCentreSpinBox.set_minimum(-1.0);
        iYCentreSpinBox.set_maximum(1.0);
        iYCentreSpinBox.set_step(0.001);
        iYCentreSpinBox.set_format("%.3f");
        iYCentreSpinBox.text_box().set_alignment(alignment::Right);
        iYCentreSpinBox.text_box().set_size_hint(size_hint{ "-0.000" });

        iGradientSelector.set_fixed_size(size{ 256.0_dip, iGradientSelector.minimum_size().cy });

        iGradientSelector.GradientChanged([this]() { update_widgets(); });

        iImportGradient.Clicked([this]()
        {
            auto const imports = open_file_dialog(*this, file_dialog_spec{ "Import Gradients", {}, { "*.ggr" }, "Gradient Files (*.ggr)" }, true);
            if (imports)
            {
                // todo: populate swatch library
                try
                {
                    set_gradient(convert_gimp_gradient(gradient(), (*imports)[0]));
                }
                catch (...)
                {
                    message_box::error("Import Gradient", "Failed to import gradient(s)");
                }
            }
        });

        iSmoothnessSpinBox.ValueChanged([this]() { iGradientSelector.set_gradient(gradient().with_smoothness(iSmoothnessSpinBox.value() / 100.0)); });
        iSmoothnessSlider.ValueChanged([this]() { iGradientSelector.set_gradient(gradient().with_smoothness(iSmoothnessSlider.value() / 100.0)); });

        iDirectionHorizontalRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_direction(gradient_direction::Horizontal)); });
        iDirectionVerticalRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_direction(gradient_direction::Vertical)); });
        iDirectionDiagonalRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_direction(gradient_direction::Diagonal)); });
        iDirectionRectangularRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_direction(gradient_direction::Rectangular)); });
        iDirectionRadialRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_direction(gradient_direction::Radial)); });

        iTopLeftRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_orientation(corner::TopLeft)); });
        iTopRightRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_orientation(corner::TopRight)); });
        iBottomRightRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_orientation(corner::BottomRight)); });
        iBottomLeftRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_orientation(corner::BottomLeft)); });
        iAngleRadioButton.checked([this]() 
        { 
            if (!std::holds_alternative<double>(iGradientSelector.gradient().orientation())) 
                iGradientSelector.set_gradient(gradient().with_orientation(0.0)); 
        });

        iAngleSpinBox.ValueChanged([this]() { iGradientSelector.set_gradient(gradient().with_orientation(to_rad(iAngleSpinBox.value()))); });
        iAngleSlider.ValueChanged([this]() { iGradientSelector.set_gradient(gradient().with_orientation(to_rad(iAngleSlider.value()))); });

        iSizeClosestSideRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_size(gradient_size::ClosestSide)); });
        iSizeFarthestSideRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_size(gradient_size::FarthestSide)); });
        iSizeClosestCornerRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_size(gradient_size::ClosestCorner)); });
        iSizeFarthestCornerRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_size(gradient_size::FarthestCorner)); });

        iShapeEllipseRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_shape(gradient_shape::Ellipse)); });
        iShapeCircleRadioButton.checked([this]() { iGradientSelector.set_gradient(gradient().with_shape(gradient_shape::Circle)); });

        iExponentGroupBox.check_box().checked([this]() { iGradientSelector.set_gradient(gradient().with_exponents(vec2{2.0, 2.0})); });
        iExponentGroupBox.check_box().Unchecked([this]() { iGradientSelector.set_gradient(gradient().with_exponents(optional_vec2{})); });

        iLinkedExponents.checked([this]() { auto e = gradient().exponents(); if (e != std::nullopt) { iGradientSelector.set_gradient(gradient().with_exponents(vec2{ e->x, e->x })); } });

        iMExponentSpinBox.ValueChanged([this]()
        { 
            if (iUpdatingWidgets)
                return;
            auto e = gradient().exponents(); 
            if (e == std::nullopt) 
                e = vec2{}; 
            e->x = iMExponentSpinBox.value(); 
            if (iLinkedExponents.is_checked())
                e->y = e->x;
            iGradientSelector.set_gradient(gradient().with_exponents(e)); 
        });

        iNExponentSpinBox.ValueChanged([this]()
        {
            if (iUpdatingWidgets)
                return;
            auto e = gradient().exponents();
            if (e == std::nullopt)
                e = vec2{};
            e->y = iNExponentSpinBox.value();
            if (iLinkedExponents.is_checked())
                e->x = e->y;
            iGradientSelector.set_gradient(gradient().with_exponents(e));
        });

        iCentreGroupBox.check_box().checked([this]() { iGradientSelector.set_gradient(gradient().with_centre(point{})); });
        iCentreGroupBox.check_box().Unchecked([this]() { iGradientSelector.set_gradient(gradient().with_centre(optional_point{})); });

        iXCentreSpinBox.ValueChanged([this]() { auto c = gradient().centre(); if (c == std::nullopt) c = point{}; c->x = iXCentreSpinBox.value(); iGradientSelector.set_gradient(gradient().with_centre(c)); });
        iYCentreSpinBox.ValueChanged([this]() { auto c = gradient().centre(); if (c == std::nullopt) c = point{}; c->y = iYCentreSpinBox.value(); iGradientSelector.set_gradient(gradient().with_centre(c)); });

        iPreview->set_margins(neogfx::margins{});
        iPreview->set_fixed_size(size{ std::ceil(256.0_dip * 16.0 / 9.0), 256.0_dip });

        button_box().add_button(standard_button::Ok);
        button_box().add_button(standard_button::Cancel);
        
        update_widgets();

        layout().invalidate();
        centre_on_parent();
    }

    void gradient_dialog::update_widgets()
    {
        if (iUpdatingWidgets)
            return;
        neolib::scoped_flag sf{ iUpdatingWidgets };
        iSmoothnessSpinBox.set_value(gradient().smoothness() * 100.0);
        iSmoothnessSlider.set_value(gradient().smoothness() * 100.0);
        iDirectionHorizontalRadioButton.set_checked(gradient().direction() == gradient_direction::Horizontal);
        iDirectionVerticalRadioButton.set_checked(gradient().direction() == gradient_direction::Vertical);
        iDirectionDiagonalRadioButton.set_checked(gradient().direction() == gradient_direction::Diagonal);
        iDirectionRectangularRadioButton.set_checked(gradient().direction() == gradient_direction::Rectangular);
        iDirectionRadialRadioButton.set_checked(gradient().direction() == gradient_direction::Radial);
        iTopLeftRadioButton.set_checked(gradient().orientation() == gradient::orientation_type(corner::TopLeft));
        iTopRightRadioButton.set_checked(gradient().orientation() == gradient::orientation_type(corner::TopRight));
        iBottomRightRadioButton.set_checked(gradient().orientation() == gradient::orientation_type(corner::BottomRight));
        iBottomLeftRadioButton.set_checked(gradient().orientation() == gradient::orientation_type(corner::BottomLeft));
        iAngleRadioButton.set_checked(std::holds_alternative<double>(gradient().orientation()));
        iAngleSpinBox.set_value(std::holds_alternative<double>(gradient().orientation()) ? to_deg(static_variant_cast<double>(gradient().orientation())) : 0.0);
        iAngleSlider.set_value(std::holds_alternative<double>(gradient().orientation()) ? to_deg(static_variant_cast<double>(gradient().orientation())) : 0.0);
        iSizeClosestSideRadioButton.set_checked(gradient().size() == gradient_size::ClosestSide);
        iSizeFarthestSideRadioButton.set_checked(gradient().size() == gradient_size::FarthestSide);
        iSizeClosestCornerRadioButton.set_checked(gradient().size() == gradient_size::ClosestCorner);
        iSizeFarthestCornerRadioButton.set_checked(gradient().size() == gradient_size::FarthestCorner);
        iShapeEllipseRadioButton.set_checked(gradient().shape() == gradient_shape::Ellipse);
        iShapeCircleRadioButton.set_checked(gradient().shape() == gradient_shape::Circle);
        auto exponents = gradient().exponents();
        bool specifyExponents = (exponents != std::nullopt);
        iExponentGroupBox.check_box().set_checked(specifyExponents);
        if (specifyExponents)
        {
            iMExponentSpinBox.set_value(exponents->x, false);
            iNExponentSpinBox.set_value(exponents->y, false);
        }
        else
        {
            iMExponentSpinBox.text_box().set_text("");
            iNExponentSpinBox.text_box().set_text("");
        }
        iLinkedExponents.enable(specifyExponents);
        iMExponent.enable(specifyExponents);
        iMExponentSpinBox.enable(specifyExponents);
        iNExponent.enable(specifyExponents);
        iNExponentSpinBox.enable(specifyExponents);        
        auto centre = gradient().centre();
        bool specifyCentre = (centre != std::nullopt);
        iCentreGroupBox.check_box().set_checked(specifyCentre);
        if (specifyCentre)
        {
            iXCentreSpinBox.set_value(centre->x, false);
            iYCentreSpinBox.set_value(centre->y, false);
        }
        else
        {
            iXCentreSpinBox.text_box().set_text("");
            iYCentreSpinBox.text_box().set_text("");
        }
        iXCentre.enable(specifyCentre);
        iXCentreSpinBox.enable(specifyCentre);
        iYCentre.enable(specifyCentre);
        iYCentreSpinBox.enable(specifyCentre);
        switch (gradient().direction())
        {
        case gradient_direction::Vertical:
        case gradient_direction::Horizontal:
        case gradient_direction::Rectangular:
            iOrientationGroupBox.hide();
            iSizeGroupBox.hide();
            iShapeGroupBox.hide();
            iExponentGroupBox.hide();
            iCentreGroupBox.hide();
            break;
        case gradient_direction::Diagonal:
            iOrientationGroupBox.show();
            iAngleGroupBox.show(std::holds_alternative<double>(gradient().orientation()));
            iSizeGroupBox.hide();
            iShapeGroupBox.hide();
            iExponentGroupBox.hide();
            iCentreGroupBox.hide();
            break;
        case gradient_direction::Radial:
            iOrientationGroupBox.hide();
            iSizeGroupBox.show();
            iShapeGroupBox.show();
            iExponentGroupBox.show(gradient().shape() == gradient_shape::Ellipse);
            iCentreGroupBox.show();
            break;
        }
        iPreview->update();
    }
}