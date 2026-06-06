#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/fs.h>         // Cấp phát Major/Minor number
#include <linux/uaccess.h>    // copy_to_user
#include <linux/device.h>     // class_create, device_create
#include <linux/version.h>    // Kiểm tra phiên bản Kernel

#define VENDOR_ID 0x045e
#define PRODUCT_ID 0x028e
#define MAX_PKT_SIZE 32

/* Cấu trúc dữ liệu Payload (Gói gọn 9 byte để truyền lên Python) */
struct robot_state {
    short left_x;   // Xoay đáy
    short left_y;   // Nâng vai
    short right_x;  // (Dự phòng)
    short right_y;  // Vươn khuỷu tay
    unsigned char buttons; // Đóng/mở ngàm
} __attribute__((packed)); // Ép dung lượng chuẩn 9 byte, không tự sinh byte rác

static struct robot_state current_state = {0}; // Biến lưu trạng thái hiện tại

static struct urb *gamepad_urb;
static unsigned char *gamepad_buf;

/* Các biến quản lý File Thiết bị Ảo */
static int major_number;
static struct class *robot_class = NULL;

static struct usb_device_id gamepad_table[] = {
    { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
    {}
};
MODULE_DEVICE_TABLE(usb, gamepad_table);

// =========================================================
// HÀM CHO USER-SPACE: Được gọi khi Python dùng lệnh file.read()
// =========================================================
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    if (len < sizeof(struct robot_state)) return -EINVAL;
    
    /* Bắn thẳng cấu trúc dữ liệu từ RAM của Kernel lên RAM của Python */
    if (copy_to_user(buffer, &current_state, sizeof(struct robot_state)) != 0) {
        return -EFAULT;
    }
    return sizeof(struct robot_state);
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_read,
};

// =========================================================
// HÀM NGẮT: Cập nhật biến current_state liên tục
// =========================================================
static void gamepad_irq(struct urb *urb) {
    if (urb->status == 0) {
        // Cập nhật nút bấm
        current_state.buttons = gamepad_buf[3];

        // Cập nhật tọa độ 16-bit
        current_state.left_x = (short)((gamepad_buf[7] << 8) | gamepad_buf[6]);
        current_state.left_y = (short)((gamepad_buf[9] << 8) | gamepad_buf[8]);
        current_state.right_x = (short)((gamepad_buf[11] << 8) | gamepad_buf[10]);
        current_state.right_y = (short)((gamepad_buf[13] << 8) | gamepad_buf[12]);
    }
    usb_submit_urb(urb, GFP_ATOMIC);
}

// =========================================================
// HÀM PROBE & DISCONNECT
// =========================================================
static int gamepad_probe(struct usb_interface *intf, const struct usb_device_id *id) {
    struct usb_host_interface *iface_desc = intf->cur_altsetting;
    struct usb_endpoint_descriptor *endpoint;
    struct usb_device *udev = interface_to_usbdev(intf);
    int i, pipe;

    if (iface_desc->desc.bInterfaceNumber != 0) return -ENODEV; 

    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        endpoint = &iface_desc->endpoint[i].desc;
        if (usb_endpoint_is_int_in(endpoint)) {
            gamepad_buf = kmalloc(MAX_PKT_SIZE, GFP_KERNEL);
            gamepad_urb = usb_alloc_urb(0, GFP_KERNEL);
            pipe = usb_rcvintpipe(udev, endpoint->bEndpointAddress);
            usb_fill_int_urb(gamepad_urb, udev, pipe, gamepad_buf, MAX_PKT_SIZE, gamepad_irq, NULL, endpoint->bInterval);
            usb_submit_urb(gamepad_urb, GFP_KERNEL);
            return 0;
        }
    }
    return -ENODEV;
}

static void gamepad_disconnect(struct usb_interface *intf) {
    if (gamepad_urb) {
        usb_kill_urb(gamepad_urb);
        usb_free_urb(gamepad_urb);
    }
    if (gamepad_buf) kfree(gamepad_buf);
}

static struct usb_driver gamepad_driver = {
    .name = "robot_gamepad_driver",
    .probe = gamepad_probe,
    .disconnect = gamepad_disconnect,
    .id_table = gamepad_table,
};

// =========================================================
// KHỞI TẠO MODULE & TẠO FILE /dev/robot_gamepad
// =========================================================
static int __init robot_init(void) {
    // Đăng ký Major Number
    major_number = register_chrdev(0, "robot_gamepad", &fops);
    
    // Tạo Class và Device Node
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
    robot_class = class_create("robot_class");
#else
    robot_class = class_create(THIS_MODULE, "robot_class");
#endif
    device_create(robot_class, NULL, MKDEV(major_number, 0), NULL, "robot_gamepad");
    
    return usb_register(&gamepad_driver);
}

static void __exit robot_exit(void) {
    usb_deregister(&gamepad_driver);
    device_destroy(robot_class, MKDEV(major_number, 0));
    class_destroy(robot_class);
    unregister_chrdev(major_number, "robot_gamepad");
}

module_init(robot_init);
module_exit(robot_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phan");
MODULE_DESCRIPTION("Driver phat song Robot 4 DOF");