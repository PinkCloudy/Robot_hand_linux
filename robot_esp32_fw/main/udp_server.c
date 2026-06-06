#include "udp_server.h"
#include "servo_ctrl.h"
#include "esp_log.h"
#include "lwip/sockets.h"

#define PORT 8888
static const char *TAG = "UDP_SERVER";

void udp_server_task(void *pvParameters) {
    char rx_buffer[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

    int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Loi khong the tao Socket");
        vTaskDelete(NULL);
    }

    bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    ESP_LOGI(TAG, "Dang mo cong UDP %d cho tin hieu tu Python...", PORT);

    while (1) {
        struct sockaddr_storage source_addr;
        socklen_t socklen = sizeof(source_addr);
        
        // Ham block: ESP32 se nam im cho den khi nhan duoc goi tin
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
        
        if (len > 0) {
            rx_buffer[len] = '\0'; // Chot chuoi
            
            int16_t x = 0, y = 0, rx = 0, ry = 0;
            int gripper_cmd = 0;
            
            // Boc tach chuoi <X,Y,RX,RY,BTN>
            if (sscanf(rx_buffer, "<%hd,%hd,%hd,%hd,%d>", &x, &y, &rx, &ry, &gripper_cmd) == 5) {
                // Truyen du lieu sang module dong co de xuat xung
                servo_update_pwm(x, y, ry, gripper_cmd);
            }
        }
    }
}