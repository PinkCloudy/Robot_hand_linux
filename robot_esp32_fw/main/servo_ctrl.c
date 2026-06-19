#include "servo_ctrl.h"
#include "driver/ledc.h"
#include "esp_log.h"

#include <stdint.h>
#include <stdbool.h>

static const char *TAG = "SERVO";

#define SERVO_FREQ_HZ 50

#define PIN_BASE      18
#define PIN_SHOULDER  19
#define PIN_ELBOW     21
#define PIN_GRIPPER   22

/*
 * Phải hiệu chỉnh từng servo khi đã tháo liên kết cơ khí.
 * Không mặc định 819 và 1638 là hành trình an toàn.
 */

/* Khớp đế */
#define BASE_HOME  1228
#define BASE_MIN   1050
#define BASE_MAX   1400

/* Khớp vai: nên giới hạn hẹp khi thử lần đầu */
#define SHOULDER_HOME  1228
#define SHOULDER_MIN   1150
#define SHOULDER_MAX   1320

/* Khớp khuỷu */
#define ELBOW_HOME  1228
#define ELBOW_MIN   1120
#define ELBOW_MAX   1350

/* Ngàm: không dùng toàn dải 819–1638 ngay từ đầu */
#define GRIPPER_OPEN_DUTY   1150
#define GRIPPER_CLOSE_DUTY  1300

/* Joystick nằm trong vùng này được coi là đứng yên */
#define JOYSTICK_DEADZONE 3500

/*
 * Số càng lớn thì servo càng chậm.
 * Dùng 7000 trong giai đoạn thử an toàn.
 */
#define SPEED_DIVISOR 7000

/*
 * Đổi thành -1 nếu khớp quay ngược chiều mong muốn.
 * Chỉ đổi từng khớp một.
 */
#define BASE_DIRECTION      1
#define SHOULDER_DIRECTION  1
#define ELBOW_DIRECTION     1

static int32_t current_duty_base = BASE_HOME;
static int32_t current_duty_shoulder = SHOULDER_HOME;
static int32_t current_duty_elbow = ELBOW_HOME;

static int32_t clamp_value(
    int32_t value,
    int32_t minimum,
    int32_t maximum
) {
    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

static int32_t apply_deadzone(int16_t value) {
    if (value > -JOYSTICK_DEADZONE &&
        value < JOYSTICK_DEADZONE) {
        return 0;
    }

    return value;
}

static void write_servo(
    ledc_channel_t channel,
    int32_t duty
) {
    ledc_set_duty(
        LEDC_LOW_SPEED_MODE,
        channel,
        (uint32_t)duty
    );

    ledc_update_duty(
        LEDC_LOW_SPEED_MODE,
        channel
    );
}

void servo_init(void) {
    ESP_LOGI(TAG, "Khoi tao PWM tai vi tri HOME an toan");

    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_14_BIT,
        .freq_hz = SERVO_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(
        ledc_timer_config(&timer_config)
    );

    ledc_channel_config_t channels[4] = {
        {
            .channel = LEDC_CHANNEL_0,
            .gpio_num = PIN_BASE,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .timer_sel = LEDC_TIMER_0,
            .intr_type = LEDC_INTR_DISABLE,
            .duty = BASE_HOME,
            .hpoint = 0
        },
        {
            .channel = LEDC_CHANNEL_1,
            .gpio_num = PIN_SHOULDER,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .timer_sel = LEDC_TIMER_0,
            .intr_type = LEDC_INTR_DISABLE,
            .duty = SHOULDER_HOME,
            .hpoint = 0
        },
        {
            .channel = LEDC_CHANNEL_2,
            .gpio_num = PIN_ELBOW,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .timer_sel = LEDC_TIMER_0,
            .intr_type = LEDC_INTR_DISABLE,
            .duty = ELBOW_HOME,
            .hpoint = 0
        },
        {
            .channel = LEDC_CHANNEL_3,
            .gpio_num = PIN_GRIPPER,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .timer_sel = LEDC_TIMER_0,
            .intr_type = LEDC_INTR_DISABLE,
            .duty = GRIPPER_OPEN_DUTY,
            .hpoint = 0
        }
    };

    for (int i = 0; i < 4; i++) {
        ESP_ERROR_CHECK(
            ledc_channel_config(&channels[i])
        );
    }
}

void servo_update_pwm(
    int16_t left_x,
    int16_t left_y,
    int16_t right_y,
    int gripper_cmd
) {
    int32_t base_input = apply_deadzone(left_x);
    int32_t shoulder_input = apply_deadzone(left_y);
    int32_t elbow_input = apply_deadzone(right_y);

    int32_t delta_base =
        BASE_DIRECTION * base_input / SPEED_DIVISOR;

    int32_t delta_shoulder =
        SHOULDER_DIRECTION * shoulder_input / SPEED_DIVISOR;

    int32_t delta_elbow =
        ELBOW_DIRECTION * elbow_input / SPEED_DIVISOR;

    current_duty_base += delta_base;
    current_duty_shoulder += delta_shoulder;
    current_duty_elbow += delta_elbow;

    current_duty_base = clamp_value(
        current_duty_base,
        BASE_MIN,
        BASE_MAX
    );

    current_duty_shoulder = clamp_value(
        current_duty_shoulder,
        SHOULDER_MIN,
        SHOULDER_MAX
    );

    current_duty_elbow = clamp_value(
        current_duty_elbow,
        ELBOW_MIN,
        ELBOW_MAX
    );

    int32_t gripper_duty =
        gripper_cmd == 1
            ? GRIPPER_CLOSE_DUTY
            : GRIPPER_OPEN_DUTY;

    write_servo(
        LEDC_CHANNEL_0,
        current_duty_base
    );

    write_servo(
        LEDC_CHANNEL_1,
        current_duty_shoulder
    );

    write_servo(
        LEDC_CHANNEL_2,
        current_duty_elbow
    );

    write_servo(
        LEDC_CHANNEL_3,
        gripper_duty
    );
}