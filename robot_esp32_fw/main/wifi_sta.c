#include "wifi_sta.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

// Thay đổi thông tin mạng Hotspot của bạn tại đây
#define WIFI_SSID "ok"
#define WIFI_PASS "conlonbeo"

#define WIFI_CONNECTED_BIT BIT0

static const char *TAG = "WIFI";
static EventGroupHandle_t wifi_event_group;

static void event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
) {
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START) {

        ESP_LOGI(TAG, "Bat dau ket noi Hotspot...");
        ESP_ERROR_CHECK(esp_wifi_connect());
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_CONNECTED) {

        ESP_LOGI(TAG, "Da ket noi Access Point, dang cho DHCP cap IP...");
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {

        wifi_event_sta_disconnected_t *event =
            (wifi_event_sta_disconnected_t *)event_data;

        ESP_LOGW(
            TAG,
            "Mat ket noi Wi-Fi, reason=%d. Dang ket noi lai...",
            event->reason
        );

        xEventGroupClearBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );

        esp_wifi_connect();
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "================================");
        ESP_LOGI(
            TAG,
            "ESP32 da nhan IP dong: " IPSTR,
            IP2STR(&event->ip_info.ip)
        );
        ESP_LOGI(
            TAG,
            "Gateway: " IPSTR,
            IP2STR(&event->ip_info.gw)
        );
        ESP_LOGI(TAG, "================================");

        xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
    }
}

void wifi_init_sta(void) {
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *sta_netif =
        esp_netif_create_default_wifi_sta();

    if (sta_netif == NULL) {
        ESP_LOGE(TAG, "Khong tao duoc Wi-Fi STA netif");
        return;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            event_handler,
            NULL,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            event_handler,
            NULL,
            NULL
        )
    );

    wifi_config_t wifi_config = {0};

    strncpy(
        (char *)wifi_config.sta.ssid,
        WIFI_SSID,
        sizeof(wifi_config.sta.ssid) - 1
    );

    strncpy(
        (char *)wifi_config.sta.password,
        WIFI_PASS,
        sizeof(wifi_config.sta.password) - 1
    );

    wifi_config.sta.threshold.authmode =
        WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    // Bỏ khối code ép IP tĩnh, để mặc định ESP-IDF tự động chạy DHCP Client

    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_wait_for_ip(void) {
    ESP_LOGI(TAG, "Dang cho ket noi mang va cap phat IP...");

    xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY
    );

    ESP_LOGI(TAG, "Da co IP dong, he thong mang san sang! Tiep tuc khoi tao UDP.");
}