#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_random.h"


// 🧩 กำหนดหมายเลขขา GPIO ของแต่ละ LED
#define LED1_GPIO GPIO_NUM_2
#define LED2_GPIO GPIO_NUM_4
#define LED3_GPIO GPIO_NUM_5

// ⚙️ กำหนดค่าช่อง (Channel) ของแต่ละ LED
#define LED1_CH LEDC_CHANNEL_0
#define LED2_CH LEDC_CHANNEL_1
#define LED3_CH LEDC_CHANNEL_2

#define LED_MODE LEDC_LOW_SPEED_MODE
#define LED_TIMER LEDC_TIMER_0
#define LED_DUTY_RES LEDC_TIMER_10_BIT   // 10-bit = ค่าความสว่าง 0–1023
#define LED_FREQ_HZ 5000                 // ความถี่ PWM

// 🕒 หน่วงเวลาในแต่ละจังหวะ
#define STEP_DELAY 10                    // ความเร็วในการไล่แสง (ms)
#define BLINK_DELAY 300                  // หน่วงระหว่าง pattern

static const char *TAG = "LED_BREATH";

typedef struct {
    gpio_num_t gpio;
    ledc_channel_t channel;
} led_t;

// รายการ LED ที่มี
led_t leds[] = {
    {LED1_GPIO, LED1_CH},
    {LED2_GPIO, LED2_CH},
    {LED3_GPIO, LED3_CH},
};
const int LED_COUNT = sizeof(leds) / sizeof(leds[0]);

/**
 * @brief กำหนดค่าเริ่มต้นของ PWM (LEDC)
 */
void led_init(void) {
    // ตั้งค่าตัวตั้งเวลา (Timer)
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LED_MODE,
        .timer_num = LED_TIMER,
        .duty_resolution = LED_DUTY_RES,
        .freq_hz = LED_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // ตั้งค่า channel แต่ละ LED
    for (int i = 0; i < LED_COUNT; i++) {
        ledc_channel_config_t ledc_channel = {
            .speed_mode = LED_MODE,
            .channel = leds[i].channel,
            .timer_sel = LED_TIMER,
            .gpio_num = leds[i].gpio,
            .duty = 0,
            .hpoint = 0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }

    ESP_LOGI(TAG, "✅ LED PWM Initialized (%d LEDs)", LED_COUNT);
}

/**
 * @brief ปรับความสว่าง LED ด้วย PWM
 * @param led_index: หมายเลข LED (0–2)
 * @param duty: ค่าความสว่าง (0–1023)
 */
void led_set_brightness(int led_index, int duty) {
    ledc_set_duty(LED_MODE, leds[led_index].channel, duty);
    ledc_update_duty(LED_MODE, leds[led_index].channel);
}

/**
 * @brief ทำให้ LED “หายใจเข้าออก” (Fade In/Out)
 */
void led_breathe(int led_index) {
    // สว่างขึ้น
    for (int duty = 0; duty <= 1023; duty += 10) {
        led_set_brightness(led_index, duty);
        vTaskDelay(pdMS_TO_TICKS(STEP_DELAY));
    }
    // ดับลง
    for (int duty = 1023; duty >= 0; duty -= 10) {
        led_set_brightness(led_index, duty);
        vTaskDelay(pdMS_TO_TICKS(STEP_DELAY));
    }
}

/**
 * @brief Pattern 1: Knight Rider (แสงวิ่งไปมาแบบหายใจ)
 */
void pattern_knight_rider(void) {
    ESP_LOGI(TAG, "🚗 Pattern: Knight Rider (Breathing)");

    // วิ่งจากซ้ายไปขวา
    for (int i = 0; i < LED_COUNT; i++) {
        led_breathe(i);
    }

    // วิ่งกลับจากขวาไปซ้าย
    for (int i = LED_COUNT - 2; i > 0; i--) {
        led_breathe(i);
    }
}

/**
 * @brief Pattern 2: Binary Counter (แสดงเลขฐาน 2 แบบหายใจ)
 */
void pattern_binary_counter(void) {
    ESP_LOGI(TAG, "💡 Pattern: Binary Counter (Breathing)");

    int max_count = 1 << LED_COUNT; // เช่น 3 LED → 8 แบบ (000–111)

    for (int count = 0; count < max_count; count++) {
        for (int i = 0; i < LED_COUNT; i++) {
            int bit = (count >> i) & 1;
            if (bit)
                led_breathe(i);
            else
                led_set_brightness(i, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(BLINK_DELAY));
    }
}

/**
 * @brief Pattern 3: Random Blinking (สุ่มแบบหายใจ)
 */
void pattern_random(void) {
    ESP_LOGI(TAG, "🎲 Pattern: Random Breathing");

    for (int i = 0; i < 6; i++) {
        int led_index = rand() % LED_COUNT;
        ESP_LOGI(TAG, "LED %d breathing...", leds[led_index].gpio);
        led_breathe(led_index);
        vTaskDelay(pdMS_TO_TICKS(BLINK_DELAY));
    }
}

/**
 * @brief Task หลักสำหรับสลับ pattern ไปเรื่อย ๆ
 */
void led_pattern_task(void *pvParameters) {
    while (1) {
        pattern_knight_rider();
        pattern_binary_counter();
        pattern_random();
    }
}

/**
 * @brief Main Application
 */
void app_main(void) {
    ESP_LOGI(TAG, "🚀 ESP32 Breathing LED Pattern Demo Started");
    led_init();

    // สุ่มค่า seed เพื่อให้ random ไม่ซ้ำ
    srand(esp_random());

    // เริ่ม Task หลัก
    xTaskCreate(led_pattern_task, "led_pattern_task", 4096, NULL, 5, NULL);
}
