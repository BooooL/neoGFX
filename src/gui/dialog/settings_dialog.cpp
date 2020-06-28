// settings_dialog.cpp
/*
  neogfx C++ App/Game Engine
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

#include <neogfx/neogfx.hpp>
#include <neolib/core/scoped.hpp>
#include <neogfx/app/i_app.hpp>
#include <neogfx/gui/widget/item_presentation_model.hpp>
#include <neogfx/gui/dialog/settings_dialog.hpp>
#include <neogfx/gui/widget/line_edit.hpp>
#include <neogfx/gui/widget/check_box.hpp>

namespace neogfx
{
    class default_setting_widget_factory : public reference_counted<i_setting_widget_factory>
    {
    public:
        default_setting_widget_factory(ref_ptr<i_setting_widget_factory> aUserFactory) :
            iUserFactory{ aUserFactory }
        {
        }
    public:
        std::shared_ptr<i_widget> create_widget(neolib::i_setting& aSetting, i_layout& aLayout) const
        {
            std::shared_ptr<i_widget> result;
            if (iUserFactory)
                result = iUserFactory->create_widget(aSetting, aLayout);
            if (!result)
            {
                switch (aSetting.value().type())
                {
                case neolib::setting_type::Boolean:
                    {
                        auto settingWidget = std::make_shared<check_box>(aLayout);
                        settingWidget->Checked([&]()
                        {
                            aSetting.set_value(true);
                        });
                        settingWidget->Unchecked([&]()
                        {
                            aSetting.set_value(false);
                        });
                        result = settingWidget;
                    }
                    break;
                case neolib::setting_type::Int8:
                case neolib::setting_type::Int16:
                case neolib::setting_type::Int32:
                case neolib::setting_type::Int64:
                case neolib::setting_type::Uint8:
                case neolib::setting_type::Uint16:
                case neolib::setting_type::Uint32:
                case neolib::setting_type::Uint64:
                    // todo
                    break;
                case neolib::setting_type::Float32:
                case neolib::setting_type::Float64:
                    // todo
                    break;
                case neolib::setting_type::String:
                    // todo
                    break;
                case neolib::setting_type::Enum:
                case neolib::setting_type::Custom:
                default:
                    break;
                }
            }
            if (!result)
                throw unsupported_setting_type();
            return result;
        }
    private:
        ref_ptr<i_setting_widget_factory> iUserFactory;
    };

    settings_dialog::settings_dialog(neolib::i_settings& aSettings, ref_ptr<i_setting_widget_factory> aWidgetFactory) :
        dialog{ "Settings", window_style::DefaultDialog },
        iSettings{ aSettings },
        iLayout{ client_layout() },
        iTree{ iLayout },
        iDetails{ iLayout },
        iDetailLayout{ iDetails }
    {
        init();
    }

    settings_dialog::settings_dialog(i_widget& aParent, neolib::i_settings& aSettings, ref_ptr<i_setting_widget_factory> aWidgetFactory) :
        dialog{ aParent, "Settings", window_style::DefaultDialog },
        iSettings{ aSettings },
        iLayout{ client_layout() },
        iTree{ iLayout },
        iDetails{ iLayout },
        iDetailLayout{ iDetails }
    {
        init();
    }

    settings_dialog::~settings_dialog()
    {
    }

    class settings_tree_presentation_model : public item_tree_presentation_model
    {
    public:
        settings_tree_presentation_model() : 
            iIcon{ image{ ":/neogfx/resources/icons/settings.png" } }
        {
        }
    public:
        optional_font cell_font(const item_presentation_model_index& aIndex) const override
        {
            if (!item_model().has_parent(to_item_model_index(aIndex)))
                return default_font();
            return default_font().with_size(9).with_style(font_style::Normal);
        }
        optional_texture cell_image(const item_presentation_model_index& aIndex) const override
        {
            if (!item_model().has_parent(to_item_model_index(aIndex)))
                return iIcon;
            return {};
        }
        optional_size cell_image_size(const item_presentation_model_index& aIndex) const override
        {
            if (!item_model().has_parent(to_item_model_index(aIndex)))
                return size{ 32.0_dip, 32.0_dip };
            return {};
        }
    private:
        texture iIcon;
    };

    void settings_dialog::init()
    {
        set_minimum_size(size{ 656_dip, 446_dip });
        iLayout.set_size_policy(size_constraint::Expanding);
        iTree.set_weight(size{ 1.0, 1.0 });
        iDetails.set_size_policy(size_constraint::Expanding);
        iDetails.set_weight(size{ 2.0, 1.0 });
        iDetailLayout.set_size_policy(size_constraint::Expanding);

        auto treeModel = std::make_shared<item_tree_model>();
        auto treePresentationModel = std::make_shared<settings_tree_presentation_model>();
        iTree.set_model(treeModel);
        iTree.set_presentation_model(treePresentationModel);

        for (auto const& category : iSettings.all_categories())
        {
            auto c = treeModel->insert_item(treeModel->send(), category.second().to_std_string());
            for (auto const& group : iSettings.all_groups().find(category.first())->second())
            {
                treeModel->append_item(c, group.second().to_std_string());
            }
        }
        treePresentationModel->set_default_font(service<i_app>().current_style().font().with_size(14).with_style(font_style::Bold));
        treePresentationModel->set_column_read_only(0);
        iTree.selection_model().set_mode(item_selection_mode::SingleSelection);

        button_box().add_button(standard_button::Ok);
        button_box().add_button(standard_button::Cancel);
        button_box().add_button(standard_button::Discard);
        button_box().add_button(standard_button::Apply);

        TryAccept([&](bool& aCanAccept, bool)
        {
            aCanAccept = iSettings.dirty();
        });

        auto update_buttons = [&]()
        {
            if (iSettings.dirty())
            {
                button_box().enable_role(button_role::Apply);
                button_box().enable_role(button_role::Destructive);
            }
            else
            {
                button_box().disable_role(button_role::Apply);
                button_box().disable_role(button_role::Destructive);
            }
        };
        update_buttons();

        iSink += iSettings.setting_changed([&](const neolib::i_setting&)
        {
            update_buttons();
        });

        HaveResult([&](dialog_result aResult)
        {
            switch (aResult)
            {
            case dialog_result::Accepted:
                iSettings.apply_changes();
                break;
            case dialog_result::Rejected:
                iSettings.discard_changes();
                break;
            }
        });

        button_box().Clicked([&](standard_button aButton)
        {
            switch (aButton)
            {
            case standard_button::Apply:
                iSettings.apply_changes();
                break;
            case standard_button::Discard:
                iSettings.discard_changes();
                break;
            }
        });

        center_on_parent();

        iTree.set_frame_style(frame_style::NoFrame);
        iDetails.set_frame_color();

        auto update_colors = [&](style_aspect aspect)
        {
            if (aspect == style_aspect::Color)
            {
                iTree.set_background_color(container_background_color());
                iDetails.set_background_color(container_background_color());
                iDetails.set_frame_color(container_background_color().dark() ? color::Black : color::White);
            }
        };
        update_colors(style_aspect::Color);
        iSink += service<i_app>().current_style_changed(update_colors);

        set_ready_to_render(true);
    }
}