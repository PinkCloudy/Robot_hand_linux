import struct
import time
import socket # Thêm thư viện mạng

# Cấu hình thiết bị ảo
DEVICE_PATH = "/dev/robot_gamepad"
STRUCT_FORMAT = '<hhhhB'
PAYLOAD_SIZE = struct.calcsize(STRUCT_FORMAT)

# ==========================================
# CẤU HÌNH MẠNG WI-FI (TRẠM NHẬN ESP32)
# ==========================================
# Ghi chú: Bạn sẽ thay địa chỉ IP này bằng IP thực tế của ESP32 
# sau khi nạp code cho ESP32 kết nối chung mạng Wi-Fi với PC.
ESP32_IP = "192.168.1.217" 
ESP32_PORT = 8888          # Cổng giao tiếp tự chọn (phải khớp với ESP32)

def main():
    # 1. Khởi tạo "Súng bắn tỉa" UDP Socket
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    print(f"[*] Da khoi tao UDP Socket. Muc tieu: {ESP32_IP}:{ESP32_PORT}")
    print(f"[*] Dang ket noi toi {DEVICE_PATH}...")
    
    try:
        with open(DEVICE_PATH, "rb") as device_file:
            print("[+] Ket noi tay cam thanh cong! Dang phat song Wi-Fi...\n")
            
            while True:
                raw_data = device_file.read(PAYLOAD_SIZE)
                
                if raw_data:
                    # Giải mã dữ liệu
                    left_x, left_y, right_x, right_y, buttons = struct.unpack(STRUCT_FORMAT, raw_data)
                    
                    # Logic Ngàm gắp: Đơn giản hóa thành 1 (Mở) và 0 (Đóng) cho ESP32
                    gripper_cmd = 1 if (buttons & 0x10) else 0
                    gripper_state = "MO  " if gripper_cmd == 1 else "DONG"
                    
                    # =======================================================
                    # BƯỚC ĐÓNG GÓI (PAYLOAD FORMATTING) & BẮN QUA WI-FI
                    # =======================================================
                    # Định dạng chuẩn: <LeftX,LeftY,RightX,RightY,GripperCmd>
                    # Ví dụ: <-32768,15000,0,-5000,1>
                    payload = f"<{left_x},{left_y},{right_x},{right_y},{gripper_cmd}>"
                    
                    # Bắn gói tin (đã mã hóa thành byte) tới IP và Port của ESP32
                    udp_socket.sendto(payload.encode('utf-8'), (ESP32_IP, ESP32_PORT))
                    
                    # In ra màn hình để giám sát chính xác những gì đang bay trong không khí
                    print(f"ĐÁY: {left_x:6d} | VAI: {left_y:6d} | KHUỶU: {right_y:6d} | NGÀM: {gripper_state} | GÓI TIN: {payload:^35}", end="\r")
                
                # Tần số quét 50Hz (Đủ nhanh cho độ nhạy tay người)
                time.sleep(0.02)
                
    except FileNotFoundError:
        print("[-] Loi: Khong tim thay file /dev/robot_gamepad. Ban da chay insmod chua?")
    except PermissionError:
        print("[-] Loi: Chua cap quyen. Hay chay 'sudo chmod 666 /dev/robot_gamepad'")

if __name__ == "__main__":
    main()