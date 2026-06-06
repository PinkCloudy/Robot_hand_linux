#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_sta.h"
#include "servo_ctrl.h"
#include "udp_server.h"

void app_main(void) {
    // 1. Khoi tao NVS (Bat buoc cho Wi-Fi cua ESP32)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Khoi tao phan cung PWM
    servo_init();

    // 3. Ket noi Wi-Fi
    wifi_init_sta();

    // 4. Mo luong (Task) chay ngam tren he dieu hanh de thu thap goi tin UDP
    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
}