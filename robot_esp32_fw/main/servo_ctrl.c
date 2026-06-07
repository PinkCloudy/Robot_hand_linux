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
static uint32_t current_duty_base     = (DUTY_MIN + DUTY_MAX) / 2;
static uint32_t current_duty_shoulder = (DUTY_MIN + DUTY_MAX) / 2;
static uint32_t current_duty_elbow    = (DUTY_MIN + DUTY_MAX) / 2;
// Hàm toán học nội suy tuyến tính (Tương đương map() của Arduino)

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
    uint32_t center_duty = (DUTY_MIN + DUTY_MAX) / 2;
    // 2. Cấu hình 4 Kênh xuất xung
    ledc_channel_config_t channels[4] = {
        {.channel = LEDC_CHANNEL_0, .gpio_num = PIN_BASE,     .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = LEDC_TIMER_0, .intr_type = LEDC_INTR_DISABLE, .duty = center_duty, .hpoint = 0},
        {.channel = LEDC_CHANNEL_1, .gpio_num = PIN_SHOULDER, .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = LEDC_TIMER_0, .intr_type = LEDC_INTR_DISABLE, .duty = center_duty, .hpoint = 0},
        {.channel = LEDC_CHANNEL_2, .gpio_num = PIN_ELBOW,    .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = LEDC_TIMER_0, .intr_type = LEDC_INTR_DISABLE, .duty = center_duty, .hpoint = 0},
        {.channel = LEDC_CHANNEL_3, .gpio_num = PIN_GRIPPER,  .speed_mode = LEDC_LOW_SPEED_MODE, .timer_sel = LEDC_TIMER_0, .intr_type = LEDC_INTR_DISABLE, .duty = DUTY_MIN,    .hpoint = 0} // Ngàm gắp mặc định mở
    };

    for (int i = 0; i < 4; i++) {
        ledc_channel_config(&channels[i]);
    }
}

// Hàm nhận tọa độ từ tay cầm và xuất xung
// HÀM UPDATE MỚI: SỬ DỤNG BƯỚC NHẢY (DELTA)
void servo_update_pwm(int16_t left_x, int16_t left_y, int16_t right_y, int gripper_cmd) {
    
    // 1. Chia tỷ lệ để lấy Bước nhảy (Delta). 
    // Hệ số 6500 giúp biến dải 32767 thành bước nhảy tối đa khoảng ±5 đơn vị duty mỗi lần UDP tới.
    // (Tương đương mất khoảng 3 giây để tay robot quay hết 180 độ - Rất êm và thật)
    int32_t delta_base     = left_x / 6500;
    int32_t delta_shoulder = left_y / 6500;
    int32_t delta_elbow    = right_y / 6500;

    // 2. Cộng dồn bước nhảy vào vị trí hiện tại
    current_duty_base     += delta_base;
    current_duty_shoulder += delta_shoulder;
    current_duty_elbow    += delta_elbow;

    // 3. Chốt chặn an toàn (Clamp) để Servo không bị gãy khi quay quá 180 độ hoặc dưới 0 độ
    if (current_duty_base < DUTY_MIN) current_duty_base = DUTY_MIN;
    if (current_duty_base > DUTY_MAX) current_duty_base = DUTY_MAX;

    if (current_duty_shoulder < DUTY_MIN) current_duty_shoulder = DUTY_MIN;
    if (current_duty_shoulder > DUTY_MAX) current_duty_shoulder = DUTY_MAX;

    if (current_duty_elbow < DUTY_MIN) current_duty_elbow = DUTY_MIN;
    if (current_duty_elbow > DUTY_MAX) current_duty_elbow = DUTY_MAX;
    
    // 4. Ngàm gắp vẫn dùng logic cũ (Chỉ có 2 trạng thái)
    uint32_t duty_gripper = (gripper_cmd == 1) ? DUTY_MAX : DUTY_MIN;

    // 5. Cập nhật tín hiệu xung ra các chân
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, current_duty_base);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, current_duty_shoulder);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, current_duty_elbow);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, duty_gripper);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3);
}