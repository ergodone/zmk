/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include <bluetooth/services/bas.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/usb.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

#define LED_1_NODE DT_NODELABEL(ledu1)
#define LED_2_NODE DT_NODELABEL(ledu2)
#define LED_3_NODE DT_NODELABEL(ledu3)
#define LED_4_NODE DT_NODELABEL(ledu4)

struct led {
    const struct device *gpio_dev;
    const char *gpio_dev_name;
    const char *gpio_pin_name;
    unsigned int gpio_pin;
    unsigned int gpio_flags;
};

static void blink(const struct led *, uint32_t);
void display_battery(void);
void display_value(uint8_t value);

enum { LED_CAP, LED_NUM, LED_SCR, LED_KEY };
struct led leds[] = {[LED_CAP] =
                         {
                             .gpio_dev = NULL,
                             .gpio_dev_name = DT_GPIO_LABEL(LED_1_NODE, gpios),
                             .gpio_pin_name = DT_LABEL(LED_1_NODE),
                             .gpio_pin = DT_GPIO_PIN(LED_1_NODE, gpios),
                             .gpio_flags = GPIO_OUTPUT | DT_GPIO_FLAGS(LED_1_NODE, gpios),
                         },
                     [LED_NUM] =
                         {
                             .gpio_dev = NULL,
                             .gpio_dev_name = DT_GPIO_LABEL(LED_2_NODE, gpios),
                             .gpio_pin_name = DT_LABEL(LED_2_NODE),
                             .gpio_pin = DT_GPIO_PIN(LED_2_NODE, gpios),
                             .gpio_flags = GPIO_OUTPUT | DT_GPIO_FLAGS(LED_2_NODE, gpios),
                         },
                     [LED_SCR] =
                         {
                             .gpio_dev = NULL,
                             .gpio_dev_name = DT_GPIO_LABEL(LED_3_NODE, gpios),
                             .gpio_pin_name = DT_LABEL(LED_3_NODE),
                             .gpio_pin = DT_GPIO_PIN(LED_3_NODE, gpios),
                             .gpio_flags = GPIO_OUTPUT | DT_GPIO_FLAGS(LED_3_NODE, gpios),
                         },
                     [LED_KEY] = {
                         .gpio_dev = NULL,
                         .gpio_dev_name = DT_GPIO_LABEL(LED_4_NODE, gpios),
                         .gpio_pin_name = DT_LABEL(LED_4_NODE),
                         .gpio_pin = DT_GPIO_PIN(LED_4_NODE, gpios),
                         .gpio_flags = GPIO_OUTPUT | DT_GPIO_FLAGS(LED_4_NODE, gpios),
                     }};

static int led_init(const struct device *dev) {
    for (int i = 0; i < (sizeof(leds) / sizeof(struct led)); i++) {
        leds[i].gpio_dev = device_get_binding(leds[i].gpio_dev_name);
        if (leds[i].gpio_dev == NULL) {
            printk("Error: didn't find %s device\n", leds[i].gpio_dev_name);
            return -EIO;
        };

        int ret = gpio_pin_configure(leds[i].gpio_dev, leds[i].gpio_pin, leds[i].gpio_flags);
        if (ret != 0) {
            printk("Error %d: failed to configure pin %d '%s'\n", ret, leds[i].gpio_pin,
                   leds[i].gpio_pin_name);
            return -EIO;
        }
    }
    return 1;
}

SYS_INIT(led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

void blink(const struct led *led, uint32_t sleep_ms) {
    gpio_pin_set(led->gpio_dev, led->gpio_pin, 1);
    k_msleep(sleep_ms);
    gpio_pin_set(led->gpio_dev, led->gpio_pin, 0);
}

static inline void ledON(const struct led *led) { gpio_pin_set(led->gpio_dev, led->gpio_pin, 1); }

static inline void ledOFF(const struct led *led) { gpio_pin_set(led->gpio_dev, led->gpio_pin, 0); }

static void led_all_OFF() {
    for (int i = 0; i < (sizeof(leds) / sizeof(struct led)); i++) {
        gpio_pin_set(leds[i].gpio_dev, leds[i].gpio_pin, 0);
    }
};

#define BATTERY_LED_SLEEP_PERIOD 100

void display_battery(void) {
    uint8_t level = bt_bas_get_battery_level();
    if (level <= 10) {
        for (int i = 0; i < 5; i++) {
            blink(&leds[0], BATTERY_LED_SLEEP_PERIOD);
            k_msleep(BATTERY_LED_SLEEP_PERIOD);
        }
    } else {
        ledON(&leds[0]);
        k_msleep(BATTERY_LED_SLEEP_PERIOD);
        if (level > 20) {
            ledON(&leds[1]);
            k_msleep(BATTERY_LED_SLEEP_PERIOD);
        }
        if (level > 40) {
            ledON(&leds[2]);
            k_msleep(BATTERY_LED_SLEEP_PERIOD);
        }
        if (level > 80) {
            ledON(&leds[3]);
            k_msleep(BATTERY_LED_SLEEP_PERIOD);
        }
        k_msleep(BATTERY_LED_SLEEP_PERIOD);
    }
    led_all_OFF();
}

#define LEVEL_LED_SLEEP_PERIOD 500

void display_value(uint8_t value) {
    if (value > 3) {
        ledON(&leds[3]);
        value -= 4;
    }
    if (value > 2) {
        ledON(&leds[2]);
        value -= 3;
    }
    if (value > 1) {
        ledON(&leds[1]);
        value -= 2;
    }
    if (value > 0) {
        ledON(&leds[0]);
    }
    k_msleep(LEVEL_LED_SLEEP_PERIOD);
    led_all_OFF();
};