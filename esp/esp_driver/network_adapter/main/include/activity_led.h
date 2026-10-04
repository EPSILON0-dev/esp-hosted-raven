// SPDX-License-Identifier: Apache-2.0
// Wi-Fi activity LED: turns on on TX/RX, off after a hold time via esp_timer.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Configure GPIO + timer. Safe to call when disabled (no-op). */
void activity_led_init(void);

/* Notify of Wi-Fi TX or RX data activity. Non-blocking, task context. */
void activity_led_notify(void);

#ifdef __cplusplus
}
#endif
