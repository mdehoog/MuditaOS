// Copyright (c) 2017-2023, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#include "common/options/BellOptionWithDescriptionWindow.hpp"
#include "common/options/OptionBellMenu.hpp"
#include "common/options/BellOptionsNavigation.hpp"

#include <messages/OptionsWindow.hpp>
#include <TextFixedSize.hpp>

namespace gui
{
    namespace
    {
        constexpr auto one_option_height      = style::bell_options::h + 2U * style::bell_options::option_margin;
        constexpr auto option_layout_height   = 2U * one_option_height;
        constexpr auto description_height     = 175U;
        constexpr auto max_description_height = description_height + 16U;
        constexpr auto top_center_margin      = -14;
    } // namespace

    BellOptionWithDescriptionWindow::BellOptionWithDescriptionWindow(app::ApplicationCommon *app,
                                                                     const std::string &name)
        : AppWindow(app, name), OptionsList(std::make_shared<OptionsModel>(app, one_option_height))
    {
        buildInterface();

        auto storedCallback        = optionsList->inputCallback;
        optionsList->inputCallback = [&, storedCallback](Item &item, const InputEvent &event) {
            return storedCallback(item, invertNavigationDirection(event));
        };
    }

    void BellOptionWithDescriptionWindow::rebuild()
    {
        recreateOptions();
    }

    void BellOptionWithDescriptionWindow::buildInterface()
    {
        AppWindow::buildInterface();

        statusBar->setVisible(false);
        header->setTitleVisibility(false);
        navBar->setVisible(false);

        body = new BellBaseLayout(this, 0, 0, style::window_width, style::window_height, false);
        body->setEdges(RectangleEdge::None);

        optionsList = new gui::ListView(body->lastBox, 0, 0, 0, 0, optionsModel, listview::ScrollBarType::None);
        optionsList->setAlignment(Alignment(Alignment::Vertical::Bottom));
        optionsList->setMinimumSize(style::bell_options::default_text_width, option_layout_height);
        optionsList->setEdges(RectangleEdge::None);

        body->lastBox->setMaximumHeight(option_layout_height);
        body->resize();

        optionsList->prepareRebuildCallback = [this]() { recreateOptions(); };
        optionsModel->createData(options);

        setFocusItem(optionsList);
    }

    void BellOptionWithDescriptionWindow::setListTitle(const std::string &title)
    {
        auto titleBody = new TextFixedSize(body->firstBox);
        titleBody->drawUnderline(false);
        titleBody->setAlignment(Alignment(Alignment::Horizontal::Center, Alignment::Vertical::Top));
        titleBody->setFont(style::window::font::largelight);
        titleBody->setMinimumWidth(style::bell_base_layout::outer_layouts_w);
        titleBody->setEdges(RectangleEdge::None);
        titleBody->setEditMode(EditMode::Browse);
        titleBody->setRichText(title);
        titleBody->setMinimumHeightToFitText();

        /* Hack to make box's height equal to title height */
        body->firstBox->setMaximumHeight(titleBody->widgetMinimumArea.h);
        body->firstBox->resizeItems();
        body->resizeItems();
    }

    void BellOptionWithDescriptionWindow::setListDescription(const std::string &title)
    {
        auto descriptionBody = new TextFixedSize(body->centerBox);
        descriptionBody->drawUnderline(false);
        descriptionBody->setAlignment(Alignment(Alignment::Horizontal::Center, Alignment::Vertical::Center));
        descriptionBody->setFont(style::window::font::verybiglight);
        descriptionBody->setMinimumSize(style::window_width, description_height);
        descriptionBody->setEditMode(EditMode::Browse);
        descriptionBody->setRichText(title);

        body->centerBox->setMargins(gui::Margins{0, top_center_margin, 0, 0});
        body->centerBox->setMaximumHeight(max_description_height);
        body->centerBox->setMinimumHeight(description_height);
        body->centerBox->resizeItems();
        body->resizeItems();
    }

    void BellOptionWithDescriptionWindow::onClose([[maybe_unused]] CloseReason reason)
    {
        if (reason != CloseReason::Popup) {
            optionsList->onClose();
        }
    }

    void BellOptionWithDescriptionWindow::onBeforeShow(ShowMode mode, SwitchData *data)
    {
        if (auto message = dynamic_cast<gui::OptionsWindowOptions *>(data)) {
            options = message->takeOptions();
        }
        optionsList->rebuildList(listview::RebuildType::InPlace);
    }
} // namespace gui
