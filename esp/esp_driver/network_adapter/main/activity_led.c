// SPDX-License-Identifier: Apache-2.0
// Wi-Fi activity LED driver: GPIO on at packet event, off via one-shot timer.

#include "sdkconfig.h"
#include "activity_led.h"

#ifdef CONFIG_ESP_ACTIVITY_LED_ENABLE

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char TAG[] = "ACT_LED";

#define ACT_LED_GPIO        ((gpio_num_t) CONFIG_ESP_ACTIVITY_LED_GPIO)
#define ACT_LED_ON_LEVEL    (CONFIG_ESP_ACTIVITY_LED_ACTIVE_HIGH ? 1 : 0)
#define ACT_LED_OFF_LEVEL   (CONFIG_ESP_ACTIVITY_LED_ACTIVE_HIGH ? 0 : 1)
#define ACT_LED_HOLD_US     ((uint64_t) CONFIG_ESP_ACTIVITY_LED_HOLD_MS * 1000ULL)

static esp_timer_handle_t s_off_timer;
static bool s_inited;

static void activity_led_off_cb(void *arg)
{
    (void) arg;
    gpio_set_level(ACT_LED_GPIO, ACT_LED_OFF_LEVEL);
}

static bool activity_led_gpio_conflicts(void)
{
#ifdef CONFIG_ESP_SPI_HOST_INTERFACE
    if (CONFIG_ESP_ACTIVITY_LED_GPIO == CONFIG_ESP_SPI_GPIO_HANDSHAKE) {
        return true;
    }
    if (CONFIG_ESP_ACTIVITY_LED_GPIO == CONFIG_ESP_SPI_GPIO_DATA_READY) {
        return true;
    }
#endif
#ifdef CONFIG_ESP_SDIO_HOST_INTERFACE
    if (CONFIG_ESP_ACTIVITY_LED_GPIO == CONFIG_HOST_WAKEUP_GPIO) {
        return true;
    }
#ifdef CONFIG_SDIO_CARD_DETECTION_PIN_SUPPORT
    if (CONFIG_ESP_ACTIVITY_LED_GPIO == CONFIG_SDIO_CD_PIN_GPIO) {
        return true;
    }
#endif
#endif
    return false;
}

void activity_led_init(void)
{
    if (activity_led_gpio_conflicts()) {
        ESP_LOGE(TAG, "GPIO %d clashes with transport pin, activity LED disabled",
                 CONFIG_ESP_ACTIVITY_LED_GPIO);
        return;
    }

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << CONFIG_ESP_ACTIVITY_LED_GPIO),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed for GPIO %d, activity LED disabled",
                 CONFIG_ESP_ACTIVITY_LED_GPIO);
        return;
    }
    gpio_set_level(ACT_LED_GPIO, ACT_LED_OFF_LEVEL);

    const esp_timer_create_args_t args = {
        .callback = &activity_led_off_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "act_led_off",
    };
    if (esp_timer_create(&args, &s_off_timer) != ESP_OK) {
        ESP_LOGE(TAG, "esp_timer_create failed, activity LED disabled");
        return;
    }

    s_inited = true;
    ESP_LOGI(TAG, "Activity LED on GPIO %d (%s, hold %d ms)",
             CONFIG_ESP_ACTIVITY_LED_GPIO,
             CONFIG_ESP_ACTIVITY_LED_ACTIVE_HIGH ? "active-high" : "active-low",
             CONFIG_ESP_ACTIVITY_LED_HOLD_MS);
}

void activity_led_notify(void)
{
    if (!s_inited) {
        return;
    }
    gpio_set_level(ACT_LED_GPIO, ACT_LED_ON_LEVEL);
    /* Coalesce bursts: re-arm hold timer on every packet. */
    esp_timer_stop(s_off_timer);
    esp_timer_start_once(s_off_timer, ACT_LED_HOLD_US);
}

#else /* CONFIG_ESP_ACTIVITY_LED_ENABLE not set: zero-overhead stubs */

void activity_led_init(void)
{
}

void activity_led_notify(void)
{
}

#endif
