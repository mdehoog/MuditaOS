// Copyright (c) 2017-2023, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#include "PowerNapProgressWindow.hpp"
#include "application-bell-powernap/ApplicationBellPowerNap.hpp"
#include "data/PowerNapStyle.hpp"
#include "data/PowerNapSwitchData.hpp"
#include <apps-common/widgets/ProgressTimerWithBarGraphAndCounter.hpp>
#include <keymap/KeyMap.hpp>

namespace
{
    constexpr auto powerNapTimerName{"PowerNapTimer"};
    constexpr auto powerNapTimerPeriod{std::chrono::seconds{1}};
} // namespace

namespace gui
{
    PowerNapProgressWindow::PowerNapProgressWindow(
        app::ApplicationCommon *app,
        std::shared_ptr<app::powernap::PowerNapProgressContract::Presenter> &&windowPresenter)
        : AppWindow(app, gui::window::name::powerNapProgress), presenter{std::move(windowPresenter)}
    {
        presenter->attach(this);
        buildInterface();
    }

    void PowerNapProgressWindow::buildInterface()
    {
        AppWindow::buildInterface();

        statusBar->setVisible(false);
        header->setTitleVisibility(false);
        navBar->setVisible(false);

        buildLayout();
        configureTimer();
    }

    void PowerNapProgressWindow::buildLayout()
    {
        using namespace gui::powerNapStyle;
        const auto progressArcRadius = progressStyle::progress::radius;
        const auto progressArcWidth  = progressStyle::progress::penWidth;
        const auto arcStartAngle     = -90 - progressStyle::progress::verticalDeviationDegrees;
        const auto arcSweepAngle     = 360 - (2 * progressStyle::progress::verticalDeviationDegrees);
        const auto arcProgressSteps  = 1000;

        Arc::ShapeParams arcParams;
        arcParams.setCenterPoint(Point(getWidth() / 2, getHeight() / 2))
            .setRadius(progressArcRadius)
            .setStartAngle(arcStartAngle) // Start drawing the circle from top.
            .setSweepAngle(arcSweepAngle)
            .setPenWidth(progressArcWidth)
            .setBorderColor(ColorFullBlack);

        progress = new ArcProgressBar(this,
                                      arcParams,
                                      ArcProgressBar::ProgressDirection::CounterClockwise,
                                      ArcProgressBar::ProgressChange::DecrementFromFull);
        progress->setMaximum(arcProgressSteps);

        mainVBox = new VBox(this, 0, 0, style::window_width, style::window_height);

        clock = new BellStatusClock(mainVBox);
        clock->setMaximumSize(progressStyle::clock::maxSizeX, progressStyle::clock::maxSizeY);
        clock->setAlignment(Alignment(Alignment::Horizontal::Center, Alignment::Vertical::Center));
        clock->setMargins(gui::Margins(0, progressStyle::clock::marginTop, 0, 0));

        timer =
            new gui::TimeFixedWidget(mainVBox, 0, 0, progressStyle::timer::maxSizeX, progressStyle::timer::maxSizeY);
        timer->setFontAndDimensions(progressStyle::timer::font);
        timer->setMinimumSize(progressStyle::timer::maxSizeX, progressStyle::timer::maxSizeY);
        timer->setMargins(gui::Margins(0, progressStyle::timer::marginTop, 0, 0));
        timer->setAlignment(Alignment(Alignment::Horizontal::Center, Alignment::Vertical::Center));

        iconPause = new Icon(mainVBox, 0, 0, 0, 0, {}, {});
        iconPause->setMinimumSize(progressStyle::pauseIcon::maxSizeX, progressStyle::pauseIcon::maxSizeY);
        iconPause->setMargins(gui::Margins(0, progressStyle::pauseIcon::marginTop, 0, 0));
        iconPause->setAlignment(Alignment(Alignment::Horizontal::Center, Alignment::Vertical::Center));
        iconPause->image->set(progressStyle::pauseIcon::image, ImageTypeSpecifier::W_G);
        iconPause->setVisible(false);

        iconRing = new Icon(mainVBox, 0, 0, 0, 0, {}, {});
        iconRing->setMinimumSize(progressStyle::ringIcon::maxSizeX, progressStyle::ringIcon::maxSizeY);
        iconRing->setMargins(gui::Margins(0, progressStyle::ringIcon::marginTop, 0, 0));
        iconRing->setAlignment(Alignment(Alignment::Horizontal::Center, Alignment::Vertical::Center));
        iconRing->image->set(progressStyle::ringIcon::image, ImageTypeSpecifier::W_G);
        iconRing->setVisible(false);

        mainVBox->resizeItems();
    }

    void PowerNapProgressWindow::configureTimer()
    {
        auto progressTimer = std::make_unique<app::ProgressTimerWithBarGraphAndCounter>(
            application, *this, powerNapTimerName, powerNapTimerPeriod, app::ProgressCountdownMode::Increasing);
        progressTimer->attach(progress);
        progressTimer->attach(timer);
        presenter->setTimer(std::move(progressTimer));
    }

    bool PowerNapProgressWindow::onInput(const InputEvent &inputEvent)
    {
        const auto key = mapKey(inputEvent.getKeyCode());

        /* Prevent leaving by back long press */
        if (inputEvent.isLongRelease() && (key == KeyMap::Back)) {
            return true;
        }

        if (inputEvent.isShortRelease()) {
            if (presenter->isNapFinished() && (key == KeyMap::LightPress)) {
                presenter->endNap();
                return true;
            }
            else if (!presenter->isNapFinished() && (key == KeyMap::LightPress)) {
                if (presenter->isTimerStopped()) {
                    presenter->resume();
                }
                else {
                    presenter->pause();
                }
                return true;
            }
            else if (!presenter->isNapFinished() && (key == KeyMap::Back)) {
                static_cast<app::Application *>(application)->resumeIdleTimer();
                presenter->abortNap();
                application->returnToPreviousWindow();
                return true;
            }
            return false;
        }
        return AppWindow::onInput(inputEvent);
    }

    void PowerNapProgressWindow::napEnded()
    {
        application->switchWindow(gui::window::name::powerNapSessionEnded, std::make_unique<gui::PowerNapSwitchData>());
    }

    void PowerNapProgressWindow::setTime(std::time_t newTime)
    {
        clock->setTime(newTime);
        clock->setTimeFormatSpinnerVisibility(true);
    }

    void PowerNapProgressWindow::setTimeFormat(utils::time::Locale::TimeFormat fmt)
    {
        clock->setTimeFormat(fmt);
    }

    RefreshModes PowerNapProgressWindow::updateTime()
    {
        if (presenter) {
            presenter->handleUpdateTimeEvent();
        }
        return RefreshModes::GUI_REFRESH_FAST;
    }

    void PowerNapProgressWindow::pause()
    {
        timer->setVisible(false);
        iconPause->setVisible(true);
        mainVBox->resizeItems();
    }

    void PowerNapProgressWindow::resume()
    {
        timer->setVisible(true);
        iconPause->setVisible(false);
        mainVBox->resizeItems();
    }

    void PowerNapProgressWindow::progressFinished()
    {
        timer->setVisible(false);
        iconPause->setVisible(false);
        iconRing->setVisible(true);
        mainVBox->resizeItems();
    }

    void PowerNapProgressWindow::onBeforeShow(ShowMode mode, SwitchData *data)
    {
        AppWindow::onBeforeShow(mode, data);
        presenter->onBeforeShow();
        updateTime();

        if (mode == ShowMode::GUI_SHOW_INIT) {
            presenter->activate();
        }
    }
} // namespace gui
