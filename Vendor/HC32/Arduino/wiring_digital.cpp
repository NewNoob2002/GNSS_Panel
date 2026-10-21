#include "wiring_digital.h"
#include "gpio.h"
//#include "adc.h"
//#include "timera_pwm.h"
#include <string.h>
#include "core_debug.h"

void
pinMode(gpio_pin_t dwPin, PinMode_TypeDef dwMode, uint8_t State) {
    ASSERT_GPIO_PIN_VALID(dwPin, "pinMode");
    // if pin has ADC channel, configure ADC according to pin mode
    //    pin_adc_info_t adc_info = PIN_MAP[dwPin].adc_info;
    //    adc_device_t *adc_device = adc_info.get_device();
    //    uint8_t adc_channel = adc_info.channel;
    //    if (adc_device != NULL && adc_channel != ADC_PIN_INVALID)
    //    {
    //        // is a valid ADC pin
    //        if (dwMode == INPUT_ANALOG)
    //        {
    //            // initialize adc device (if already initialized, this will do nothing)
    //            adc_device_init(adc_device);

    //            // enable ADC channel
    //            adc_enable_channel(adc_device, adc_channel);
    //        }
    //        else
    //        {
    //            // disable ADC channel
    //            adc_disable_channel(adc_device, adc_channel);
    //        }
    //    }

    // build pin configuration
    stc_gpio_init_t pinConf;
    (void)GPIO_StructInit(&pinConf);
    switch (dwMode) {
        case INPUT: pinConf.u16PinDir = PIN_DIR_IN; break;
        case INPUT_PULLUP:
            pinConf.u16PinDir = PIN_DIR_IN;
            pinConf.u16PullUp = PIN_PU_ON;
            break;
        case INPUT_ANALOG:
            pinConf.u16PinDir = PIN_DIR_IN;
            pinConf.u16PinAttr = PIN_ATTR_ANALOG;
            break;
        case PWM:
            // get timer assignment for pin
            //        timera_config_t *unit;
            //        en_timera_channel_t channel;
            //        en_port_func_t port_function;
            //        if (!timera_get_assignment(dwPin, unit, channel, port_function))
            //        {
            //            CORE_ASSERT_FAIL("analogWrite: pin is not a PWM pin");
            //            return;
            //        }

            //        // initialize timer unit, allow incompatible config
            //        switch (timera_pwm_start(unit, 1000 /* Hz */, 16, true))
            //        {
            //        case Ok:
            //            // all good
            //            break;
            //        case ErrorOperationInProgress:
            //            // already initialized with incompatible config
            //            CORE_ASSERT_FAIL("timera_pwm_start failed: ErrorOperationInProgress");
            //            return;
            //        case ErrorInvalidParameter:
            //        default:
            //            // invalid parameter or other error
            //            CORE_ASSERT_FAIL("timera_pwm_start failed");
            //            return;
            //        }

            //        // initialize channel, start now
            //        timera_pwm_channel_start(unit, channel, true);

            //        // set pin function to TimerA output, no GPIO
            //        GPIO_SetFunc(dwPin, port_function, Disable);

            //        // return immediately, as pwm needs different function
            //        return;
        case OUTPUT: {
            pinConf.u16PinDrv = PIN_HIGH_DRV;
            pinConf.u16PinDir = PIN_DIR_OUT;
            pinConf.u16PullUp = PIN_PU_ON;
            break;
        }
        case OUTPUT_OPEN_DRAIN: {
            pinConf.u16PinDir = PIN_DIR_OUT;
            pinConf.u16PinOutputType = PIN_OUT_TYPE_NMOS;
            break;
        }
        case OUTPUT_AF_PP:
        case OUTPUT_AF_OD: {
            pinConf.u16PinDrv = PIN_HIGH_DRV;
            break;
        }
        default:
            CORE_ASSERT_FAIL("pinMode: invalid pin mode. Must be INPUT, INPUT_PULLUP, INPUT_ANALOG or OUTPUT");
            return;
    }
    pinConf.u16PinState = State;
    // set pin function and config
    _GPIO_Init(dwPin, &pinConf);
}

uint32_t
getPinMode(gpio_pin_t dwPin) {
    ASSERT_GPIO_PIN_VALID(dwPin, "getPinMode");
    // read pin configuration
    stc_gpio_init_t pinConf;
    CORE_ASSERT(GPIO_GetConfig(dwPin, &pinConf) == LL_OK, "getPinMode: GPIO_GetConfig failed");

    // read pin function
    uint16_t pinFunction;
    en_functional_state_t pinSubFunction;
    CORE_ASSERT(GPIO_GetFunc(dwPin, &pinFunction, &pinSubFunction) == LL_OK, "getPinMode: GPIO_GetFunc failed");

    // determine mode from function and configuration
    switch (pinFunction) {
        case Func_GPIO:
            // GPIO function, determine mode from configuration
            switch (pinConf.u16PinDir) {
                case PIN_DIR_OUT: return OUTPUT;
                case PIN_DIR_IN:
                    if (pinConf.u16PinAttr == PIN_ATTR_ANALOG) {
                        return INPUT_ANALOG;
                    }
                    if (pinConf.u16PullUp == PIN_PU_ON) {
                        return INPUT_PULLUP;
                    } else {
                        return INPUT;
                    }
                default: CORE_ASSERT_FAIL("getPinMode: invalid configuration for Func_Gpio"); return INPUT;
            }

        case Func_Tima0:
        case Func_Tima1:
        case Func_Tima2:
            // TimerA output function, must be PWM
            // in that case, subFunction is always disabled
            //        CORE_ASSERT(pinSubFunction == Disable, "getPinMode: pinSubFunctin is Enabled for Func_TimaX");
            //        return OUTPUT_PWM;

        default: CORE_ASSERT_FAIL("getPinMode: invalid pin function"); return INPUT;
    }
}

void
digitalWrite(gpio_pin_t dwPin, uint32_t dwVal) {
    ASSERT_GPIO_PIN_VALID(dwPin, "digitalWrite");

    if (dwVal == HIGH) {
        GPIO_SetBits(dwPin);
    } else {
        GPIO_ResetBits(dwPin);
    }
}

int
digitalRead(gpio_pin_t ulPin) {
    ASSERT_GPIO_PIN_VALID(ulPin, "digitalRead");
    return GPIO_GetBit(ulPin) ? HIGH : LOW;
}

void
digitalToggle(gpio_pin_t ulPin) {
    ASSERT_GPIO_PIN_VALID(ulPin, "digitalToggle");
    GPIO_Toggle(ulPin);
}
