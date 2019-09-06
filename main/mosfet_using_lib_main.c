#include "mjd.h"

/*
 * Logging
 */
static const char TAG[] = "myapp";

/*
 * KConfig:
 * - LED
 * - sensor GPIO's
 * - SD1306
 * - POWER C Channel MOSFET to give power to the sensor (or not, to save power consumption in battery mode)
 */
static const int MY_LED_ON_DEVBOARD_GPIO_NUM = CONFIG_MY_LED_ON_DEVBOARD_GPIO_NUM;
static const int MY_LED_ON_DEVBOARD_WIRING_TYPE = CONFIG_MY_LED_ON_DEVBOARD_WIRING_TYPE;

static const int MY_POWER_MOSFET_GATE_GPIO_NUM = CONFIG_MY_POWER_MOSFET_GATE_GPIO_NUM;

/*
 * FreeRTOS settings
 */
#define MYAPP_RTOS_TASK_STACK_SIZE_LARGE (8192)
#define MYAPP_RTOS_TASK_PRIORITY_NORMAL (RTOS_TASK_PRIORITY_NORMAL)

/*
 * TASKS
 */
void main_task(void *pvParameter) {
    ESP_LOGI(TAG, "%s()", __FUNCTION__);

    /********************************************************************************
     * Reuseable variables
     */
    esp_err_t f_retval = ESP_OK;

    /*********************************
     * LOGGING
     * Optional for Production: dump less messages
     * @doc It is possible to lower the log level for specific modules (wifi and tcpip_adapter are strong candidates)
     * @important Disable u8g2_hal DEBUG messages which are too detailed for me.
     */

    /********************************************************************************
     * STANDARD Init
     */
    mjd_log_wakeup_details();
    mjd_log_chip_info();
    mjd_log_time();
    mjd_log_memory_statistics();
    /////ESP_LOGI(TAG, "@doc Wait X seconds after power-on (start logic analyzer, let peripherals become active, ...)");
    /////vTaskDelay(RTOS_DELAY_1SEC);

    /********************************************************************************
     * On-board LED
     */
    mjd_led_config_t led_config =
                { 0 };
    led_config.gpio_num = MY_LED_ON_DEVBOARD_GPIO_NUM;
    led_config.wiring_type = MY_LED_ON_DEVBOARD_WIRING_TYPE; // 1 GND MCU Huzzah32 | 2 VCC MCU Lolin32lite
    mjd_led_config(&led_config);

    mjd_led_on(MY_LED_ON_DEVBOARD_GPIO_NUM);

    /********************************************************************************
     * Init the ESP32 GPIO driver for the POWER MOSFET
     *
     * @important I have to use a "weak" external 1M Ohm pulldown resistor with this MOSFET to pull the line down and guarantee low power consumption (battery mode).
     *            In deinit() do not use gpio_reset_pin()! "I (12790) gpio: GPIO[4]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0":
     *              for GPIO#4 the func gpio_reset_pin() pulls UP the signal with a "strong" 10-50K Ohm, overriding my "weaker" 1M Ohm pull DOWN, meaning the line is always UP and so the MOSFET is always ON!
     *
     */
    ESP_LOGI(TAG, "Init the ESP32 GPIO driver for the POWER MOSFET...");

    gpio_config_t mosfet_config;

    mosfet_config.pin_bit_mask = (1ULL << MY_POWER_MOSFET_GATE_GPIO_NUM);
    mosfet_config.mode = GPIO_MODE_OUTPUT;
    mosfet_config.pull_down_en = GPIO_PULLDOWN_DISABLE; // @important I use an external 1M Ohm pulldown resistor to pull the line down (1M Ohm = supposedly low power consumption in battery mode).
    mosfet_config.pull_up_en = GPIO_PULLUP_DISABLE;
    mosfet_config.intr_type = GPIO_PIN_INTR_DISABLE;

    f_retval = gpio_config(&mosfet_config);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "%s(). ABORT. gpio_config(mosfet_config) | err %i (%s)", __FUNCTION__, f_retval,
                esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }

    ESP_LOGI(TAG, "Wait 5 seconds before turning the Gate ON...");
    vTaskDelay(RTOS_DELAY_5SEC);

    /********************************************************************************
     * Set POWER MOSFET gate := *ON
     *
     */
    ESP_LOGI(TAG, "Set POWER MOSFET Gate := *ON (the FAN and the LED turn on)...");
    f_retval = gpio_set_level(MY_POWER_MOSFET_GATE_GPIO_NUM, 1);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "%s(). ABORT. gpio_set_level(MY_POWER_MOSFET_GATE_GPIO_NUM) | err %i (%s)", __FUNCTION__, f_retval,
                esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }

    ESP_LOGI(TAG, "Wait 15 seconds whilst the gate is ON..");
    vTaskDelay(RTOS_DELAY_15SEC);

    /********************************************************************************
     * Set POWER MOSFET gate := *OFF
     * @brief Cut power to the FAN and the external LED.
     *
     */
    ESP_LOGI(TAG, "Set POWER MOSFET Gate := *OFF (the FAN and the LED turn off)...");

    f_retval = gpio_set_level(MY_POWER_MOSFET_GATE_GPIO_NUM, 0);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "%s(). ABORT. gpio_set_level(MY_POWER_MOSFET_GATE_GPIO_NUM) | err %i (%s)", __FUNCTION__, f_retval,
                esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }

    ESP_LOGI(TAG, "Wait 5 seconds whilst the gate is OFF..");
    vTaskDelay(RTOS_DELAY_5SEC);

    /********************************************************************************
     * DEEP SLEEP & RESTART TIMER
     * @important In deep sleep mode, wireless peripherals are powered down. Before entering sleep mode, applications must disable WiFi and BT using appropriate calls (esp_wifi_stop(), esp_bluedroid_disable(), esp_bt_controller_disable()).
     * @doc https://esp-idf.readthedocs.io/en/latest/api-reference/system/sleep_modes.html
     *
     */
    ESP_LOGI(TAG, "\n\n***SECTION: DEEP SLEEP*** (the onboard LED turns off as well)");

    mjd_log_memory_statistics();

    const uint32_t MY_DEEP_SLEEP_TIME_SEC = 15; // 15 15*60 30*60
    f_retval = esp_sleep_enable_timer_wakeup(mjd_seconds_to_microseconds(MY_DEEP_SLEEP_TIME_SEC));
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "%s(). esp_sleep_enable_timer_wakeup() err %i %s", __FUNCTION__, f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }

    // @important Give ESP_LOGI() some time to log the information to UART before deep sleep kicks in!
    ESP_LOGI(TAG, "Entering deep sleep (the MCU will wake up %u seconds later)...\n\n", MY_DEEP_SLEEP_TIME_SEC);
    vTaskDelay(RTOS_DELAY_10MILLISEC);

    esp_deep_sleep_start();

    /********************************************************************************
     * LABEL
     * @important I never get to this code if no error occurs.
     *
     */
    cleanup: ;

    /*
     * LOG TIME
     */
    mjd_log_time();

    /********************************************************************************
     * Task Delete
     * @doc Passing NULL will end the current task
     */
    ESP_LOGI(TAG, "END OF %s()", __FUNCTION__);
    vTaskDelete(NULL);
}

/*
 * MAIN
 */
void app_main() {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    mjd_log_memory_statistics();

    /**********
     * CREATE TASK:
     * @important For stability (RMT + Wifi etc.): always use xTaskCreatePinnedToCore(APP_CPU_NUM) [Opposed to xTaskCreate()]
     */
    BaseType_t xReturned;
    xReturned = xTaskCreatePinnedToCore(&main_task, "main_task (name)", MYAPP_RTOS_TASK_STACK_SIZE_LARGE, NULL,
    MYAPP_RTOS_TASK_PRIORITY_NORMAL, NULL,
    APP_CPU_NUM);
    if (xReturned == pdPASS) {
        ESP_LOGI(TAG, "OK Task has been created, and is running right now");
    }

    /**********
     * END
     */
    ESP_LOGI(TAG, "END %s()", __FUNCTION__);
}
