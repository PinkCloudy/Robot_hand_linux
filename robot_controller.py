import struct
import time

# Đường dẫn tới file thiết bị ảo do Driver C tạo ra
DEVICE_PATH = "/dev/robot_gamepad"

# Struct định dạng: 4 số short (h) = 8 bytes, 1 unsigned char (B) = 1 byte
# Ký hiệu '<' nghĩa là đọc theo chuẩn Little-Endian của Linux
STRUCT_FORMAT = '<hhhhB'
PAYLOAD_SIZE = struct.calcsize(STRUCT_FORMAT)

def main():
    print(f"[*] Dang ket noi toi {DEVICE_PATH}...")
    try:
        with open(DEVICE_PATH, "rb") as device_file:
            print("[+] Ket noi thanh cong! Dang nhan du lieu...\n")
            
            while True:
                # Đọc chính xác 9 byte từ Kernel
                raw_data = device_file.read(PAYLOAD_SIZE)
                
                if raw_data:
                    # Giải mã dữ liệu nhị phân thành các số thực tế
                    left_x, left_y, right_x, right_y, buttons = struct.unpack(STRUCT_FORMAT, raw_data)
                    
                    # Logic đóng/mở ngàm gắp (Trích xuất bit)
                    gripper_state = "MO  " if (buttons & 0x10) else "DONG"
                    
                    # In ra màn hình console để giám sát
                    print(f"ĐÁY: {left_x:6d} | VAI: {left_y:6d} | KHUỶU: {right_y:6d} | NGÀM: {gripper_state}", end="\r")
                
                # Tần số quét 50Hz (20ms/lần) - Đủ nhanh và mượt cho Robot
                time.sleep(0.02)
                
    except FileNotFoundError:
        print("[-] Loi: Khong tim thay tay cam. Ban da chay insmod chua?")
    except PermissionError:
        print("[-] Loi: Chua cap quyen truy cap file. Hay chay 'sudo chmod 666 /dev/robot_gamepad'")

if __name__ == "__main__":
    main()
