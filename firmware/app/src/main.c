#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <app_version.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

#define PWM_PERIOD       PWM_MSEC(1U)
#define PWM_FADE_SPEED   1

static const struct gpio_dt_spec led_col0 = GPIO_DT_SPEC_GET(DT_ALIAS(led_col0), gpios);
// static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET_BY_NAME(DT_NODELABEL(pwm_led), red);
static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led_red));

int main(void)
{
    /// Initializing stuff
    // Initializing the column pin (Standard GPIO)
    int ret;
    if (!gpio_is_ready_dt(&led_col0)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led_col0, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

    /// Initializing the row pin (PWM)
    if (!device_is_ready(pwm_led0.dev)) {
        return 0;
    }

    // Start PWM at max duty cycle and period
    ret = pwm_set_dt(&pwm_led0, PWM_PERIOD, PWM_PERIOD);
    if (ret < 0) {
        return 0;
    }

    // Forever loop.
    uint32_t pwm_pulse_period = PWM_PERIOD;
    int multiplier = 1;
    while(1)
    {
        ret = pwm_set_pulse_dt(&pwm_led0, pwm_pulse_period);
        pwm_pulse_period -= multiplier * PWM_FADE_SPEED;
        if(pwm_pulse_period == 0 || pwm_pulse_period >= PWM_PERIOD)
        {
            multiplier *= -1;
        }

        k_msleep(SLEEP_TIME_MS);
    }
}