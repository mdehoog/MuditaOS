// Copyright (c) 2017-2022, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#include "bsp/eink_frontlight/eink_frontlight.hpp"
#include "board/BoardDefinitions.hpp"
#include "drivers/pwm/DriverPWM.hpp"
#include <cmath>

namespace bsp::eink_frontlight
{
    namespace
    {
        constexpr auto minDutyCycle = 2U; // backlight on minimum brightness
        constexpr auto maxDutyCycle = 65535U;
        std::uint16_t dutyCycleToReloadValue(std::uint8_t dutyCyclePercent)
        {
            //return dutyCyclePercent == 0 ? 0U : minDutyCycle * 2 + (dutyCyclePercent - 1U) * (maxDutyCycle - minDutyCycle * 2) / 99U;
            return dutyCyclePercent < 50 ? 0U : dutyCyclePercent - 50U;
        }

        std::shared_ptr<drivers::DriverPWM> pwm;
        constexpr inline auto PWM_FREQUENCY_HZ = 528000000UL * minDutyCycle / maxDutyCycle;
        float gammaFactor                      = 2.5f;

        std::uint8_t gammaCorrection(BrightnessPercentage brightness)
        {
            std::clamp(brightness, 0.0f, 100.0f);
//            return static_cast<std::uint8_t>(100 * std::pow((brightness / 100.0f), gammaFactor));
            return static_cast<std::uint8_t>(brightness);
        }
        constexpr auto pwmChannel = static_cast<drivers::PWMChannel>(BoardDefinitions::EINK_FRONTLIGHT_PWM_CHANNEL);

    } // namespace

    void init()
    {
        drivers::DriverPWMParams pwmParams;
        pwmParams.channel   = pwmChannel;
        pwmParams.frequency = PWM_FREQUENCY_HZ;

        pwm = drivers::DriverPWM::Create(
            static_cast<drivers::PWMInstances>(BoardDefinitions::EINK_FRONTLIGHT_PWM_INSTANCE),
            static_cast<drivers::PWMModules>(BoardDefinitions::EINK_FRONTLIGHT_PWM_MODULE),
            pwmParams);
        pwm->SetDutyCycleToReloadValue(&dutyCycleToReloadValue);
    }

    void deinit()
    {
        turnOff();
    }

    void setBrightness(BrightnessPercentage brightness)
    {
        if (pwm) {
            pwm->SetDutyCycle(gammaCorrection(brightness), pwmChannel);
        }
    }

    void turnOn()
    {
        pwm->Start(pwmChannel);
    }

    void turnOff()
    {
        pwm->Stop(pwmChannel);
    }

    void setGammaFactor(float gamma)
    {
        gammaFactor = gamma;
    }

    void updateClockFrequency(CpuFrequencyMHz newFrequency)
    {
        const auto convertedFrequency = static_cast<std::uint32_t>(newFrequency) * bsp::HzPerMHz;
        std::uint32_t pwmFreq_Hz = convertedFrequency * minDutyCycle / maxDutyCycle;
        pwm->UpdateClockFrequency(newFrequency, pwmFreq_Hz);
    }

} // namespace bsp::eink_frontlight
