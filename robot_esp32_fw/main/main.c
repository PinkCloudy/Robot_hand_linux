#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_sta.h"
#include "servo_ctrl.h"
#include "udp_server.h"

void app_main(void) {
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    servo_init();

    wifi_init_sta();

    // Chặn tại đây cho đến khi ESP32 được DHCP cấp IP
    wifi_wait_for_ip();

    // Chỉ tạo UDP server sau khi đã có IP
    xTaskCreate(
        udp_server_task,
        "udp_server",
        4096,
        NULL,
        5,
        NULL
    );
}