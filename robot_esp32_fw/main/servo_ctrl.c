#include "servo_ctrl.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "SERVO";

// Cấu hình tần số và dải PWM (Cho Timer 14-bit)
#define SERVO_FREQ_HZ 50
#define DUTY_MIN      819   // Tương đương xung 1ms (Góc 0 độ)
#define DUTY_MAX      1638  // Tương đương xung 2ms (Góc 180 độ)

// Định nghĩa chân cắm thực tế (Hãy sửa lại theo dây cắm của bạn)
#define PIN_BASE      18 // Khớp Đáy (Xoay)
#define PIN_SHOULDER  19 // Khớp Vai (Nâng)
#define PIN_ELBOW     21 // Khớp Khuỷu (Vươn)
#define PIN_GRIPPER   22 // Ngàm gắp

// Hàm toán học nội suy tuyến tính (Tương đương map() của Arduino)
static uint32_t map_val(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void servo_init(void) {
    ESP_LOGI(TAG, "Khoi tao phan cung PWM LEDC...");

    // 1. Cấu hình Timer chung cho cả 4 Servo
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_14_BIT,
        .freq_hz          = SERVO_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // 2. Cấu hình 4 Kênh xuất xung
    ledc_channel_config_t channels[4] = {
        {.channel = LEDC_CHANNEL_0, .gpio_num = PIN_BASE,     .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = LEDC_TIMER_0, .intr_type = LEDC_INTR_DISABLE, .duty = 0, .hpoint = 0},
        {.channel = LEDC_CHANNEL_1, .gpio_num = PIN_SHOULDER, .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = LEDC_TIMER_0, .intr_type = LEDC_INTR_DISABLE, .duty = 0, .hpoint = 0},
        {.channel = LEDC_CHANNEL_2, .gpio_num = PIN_ELBOW,    .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = LEDC_TIMER_0, .intr_type = LEDC_INTR_DISABLE, .duty = 0, .hpoint = 0},
        {.channel = LEDC_CHANNEL_3, .gpio_num = PIN_GRIPPER,  .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = LEDC_TIMER_0, .intr_type = LEDC_INTR_DISABLE, .duty = 0, .hpoint = 0}
    };

    for (int i = 0; i < 4; i++) {
        ledc_channel_config(&channels[i]);
    }
}

// Hàm nhận tọa độ từ tay cầm và xuất xung
void servo_update_pwm(int16_t left_x, int16_t left_y, int16_t right_y, int gripper_cmd) {
    // Chuyển đổi dải Analog (-32768 -> 32767) sang Duty Cycle (819 -> 1638)
    uint32_t duty_base     = map_val(left_x, -32768, 32767, DUTY_MIN, DUTY_MAX);
    uint32_t duty_shoulder = map_val(left_y, -32768, 32767, DUTY_MIN, DUTY_MAX);
    uint32_t duty_elbow    = map_val(right_y, -32768, 32767, DUTY_MIN, DUTY_MAX);
    
    // Ngàm gắp: Chỉ có 2 trạng thái Đóng/Mở
    uint32_t duty_gripper  = (gripper_cmd == 1) ? DUTY_MAX : DUTY_MIN;

    // Cập nhật tín hiệu xung ra các chân
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_base);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty_shoulder);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty_elbow);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, duty_gripper);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3);
}