# 🌿 ESP32-S3 Lawn Mower Robot

> Robot cắt cỏ điều khiển từ xa sử dụng ESP32-S3, tay cầm PS2 không dây, driver BTS7960 và động cơ bước MD2U.

---

## 📌 Giới thiệu

Đây là đồ án tốt nghiệp thiết kế và xây dựng robot cắt cỏ điều khiển từ xa. Robot sử dụng vi điều khiển **ESP32-S3** làm bộ xử lý trung tâm, nhận lệnh từ **tay cầm PS2** và điều khiển 3 cơ cấu chấp hành:

- 🚗 **Di chuyển** — 4 motor DC, 2 driver BTS7960 43A (skid steering)
- ✂️ **Lưỡi cắt** — Relay 1 kênh bật/tắt động cơ lưỡi cắt
- ↕️ **Điều chỉnh chiều cao** — Động cơ bước qua driver MD2U-MD20 + vít me

---

## 🛠️ Phần cứng

| Linh kiện | Số lượng | Chức năng |
|-----------|----------|-----------|
| ESP32-S3-DevKitC-1U-N8R8 | 1 | Vi điều khiển trung tâm |
| BTS7960 43A | 3 | Driver motor DC bánh xe + relay lưỡi cắt |
| MD2U-MD20 | 1 | Driver động cơ bước nâng hạ lưỡi cắt |
| Opto PC817 | 16 | Cách ly quang tín hiệu logic ↔ drive |
| Tay cầm PS2 + Receiver | 1 bộ | Điều khiển không dây |
| XL4015 Buck | 1 | Hạ áp pin → 5V |
| AMS1117-3.3V | 1 | Ổn áp 5V → 3.3V |
| Công tắc hành trình | 2 | Giới hạn hành trình stepper |
| Motor DC bánh xe | 4 | Di chuyển robot |
| Động cơ bước | 1 | Nâng hạ lưỡi cắt |
| Động cơ lưỡi cắt DC 775 | 1 | Cắt cỏ (24V) |

---

## 📐 Thông số kỹ thuật

| Thông số | Giá trị |
|----------|---------|
| Vi điều khiển | ESP32-S3, Xtensa LX7 dual-core 240MHz |
| Điện áp nguồn | Pin (VBAT) → 5V → 3.3V |
| Đường kính bánh xe | 400 mm |
| Khoảng cách 2 bánh | 450 mm |
| Tần số PWM motor | 500 Hz (giới hạn bởi opto PC817) |
| Tần số xung stepper | 800 Hz |
| Chu kỳ điều khiển | 20 ms (50 Hz) |
| Giao tiếp PS2 | SPI bit-bang ~62.5 kHz |
| Debug UART | 115200 bps |

---

## 🗂️ Cấu trúc thư mục

```
esp32s3-lawn-mower-robot/
├── src/
│   ├── RobotCatCo.ino          # File chính: setup() + loop() + biến toàn cục
│   ├── 01_PS2_Direct.ino       # Giao tiếp tay PS2 (SPI bit-bang)
│   ├── 02_Joystick_Mixer.ino   # Skid steering: joystick → PWM 2 bánh
│   ├── 03_BTS7960_Drive.ino    # Điều khiển driver BTS7960
│   ├── 04_Serial_Debug.ino     # Debug qua Serial/UART
│   ├── 05_Log_UART.ino         # Nhật ký trạng thái hệ thống
│   ├── 06_MD2U_Stepper.ino     # Điều khiển driver bước MD2U
│   └── 07_Relay_BTS_LEN3.ino  # Điều khiển relay lưỡi cắt
├── schematic/
│   └── dieukhien.pdf           # Sơ đồ nguyên lý mạch điện
├── docs/
│   └── ...                     # Tài liệu báo cáo
└── README.md
```

---

## ⚙️ Cài đặt và nạp code

### Yêu cầu
- [Arduino IDE 2.x](https://www.arduino.cc/en/software)
- Board package: **esp32 by Espressif** (cài qua Boards Manager)
- Không cần thư viện ngoài — toàn bộ giao tiếp PS2 tự viết

### Các bước
```bash
# 1. Clone repository
git clone https://github.com/<your-username>/esp32s3-lawn-mower-robot.git

# 2. Mở thư mục src/ trong Arduino IDE 2.x
# Arduino IDE tự động nhận diện tất cả file .ino trong cùng thư mục

# 3. Chọn board: ESP32S3 Dev Module
# Tools → Board → esp32 → ESP32S3 Dev Module

# 4. Cấu hình:
# Flash Size: 8MB
# PSRAM: OPI PSRAM
# Upload Speed: 921600

# 5. Nạp code
```

---

## 🎮 Điều khiển bằng tay PS2

| Nút / Trục | Chức năng |
|-----------|-----------|
| Joystick trái ↑ | Tiến |
| Joystick trái ↓ | Lùi |
| Joystick trái ← | Quay trái |
| Joystick trái → | Quay phải |
| Joystick trái (chéo) | Tiến + rẽ đồng thời |
| Nút △ (Tam giác) | Nâng lưỡi cắt (stepper CW) |
| Nút ✕ (Cross) | Hạ lưỡi cắt (stepper CCW) |
| Nút □ (Vuông) | Bật/tắt động cơ lưỡi cắt |
| Nút SELECT | **Dừng khẩn cấp** bánh xe + stepper |
| D-PAD | Điều khiển dự phòng khi mất analog |

---

## 🖥️ Debug qua Serial

Mở Serial Monitor ở **115200 baud**, gõ lệnh:

| Lệnh | Chức năng |
|------|-----------|
| `w` | Tiến |
| `s` | Lùi |
| `a` | Quay trái |
| `d` | Quay phải |
| `x` | Dừng, trả về joystick |
| `1` / `2` | Test motor trái tiến / lùi |
| `3` / `4` | Test motor phải tiến / lùi |
| `t` | Chạy self-test toàn bộ motor |
| `p` / `o` | Relay ON / OFF |
| `r` | Toggle relay |
| `h` | Hiện menu trợ giúp |

---

## 🔌 Sơ đồ kết nối GPIO

| GPIO | Tín hiệu | Kết nối |
|------|----------|---------|
| 4 | ESP_R_PWM_1 | BTS7960 P1 — RPWM (motor trái) |
| 5 | ESP_R_EN_1  | BTS7960 P1 — REN |
| 6 | ESP_L_PWM_1 | BTS7960 P1 — LPWM |
| 7 | ESP_L_EN_1  | BTS7960 P1 — LEN |
| 8 | ESP_R_PWM_2 | BTS7960 P2 — RPWM (motor phải) |
| 9 | ESP_L_PWM_2 | BTS7960 P2 — LPWM |
| 10 | ESP_L_EN_2 | BTS7960 P2 — LEN |
| 14 | ESP_L_EN_3 | Relay qua BTS7960 P3 LEN |
| 15 | ESP_STEP   | MD2U — HOLD OFF |
| 16 | ESP_EN/CCW | MD2U — CCW xung bước |
| 17 | ESP_DIR/CW | MD2U — CW xung bước |
| 18 | ESP_R_EN_2 | BTS7960 P2 — REN |
| 19 | PS2_DAT    | Tay PS2 — DATA |
| 20 | PS2_CMD    | Tay PS2 — CMD |
| 21 | PS2_CS     | Tay PS2 — CS |
| 35 | LED_STATUS | Heartbeat LED |
| 47 | PS2_CLK    | Tay PS2 — CLK |
| 1  | HT1        | Công tắc hành trình CW |
| 2  | HT2        | Công tắc hành trình CCW |

---

## ⚠️ Lưu ý quan trọng

- **Mạch opto PC817** đảo logic: ESP HIGH → BTS LOW. Cờ `BTS_OPTO_INVERTED = 1` trong code xử lý tự động.
- **PWM 500 Hz** — không tăng cao hơn do giới hạn tốc độ đáp ứng của opto PC817.
- **Relay luôn OFF khi khởi động** — GPIO 14 được kéo xuống LOW ngay dòng đầu tiên trong `setup()`.
- **Nâng bánh xe** trước khi bật `DRIVE_SELF_TEST_ON_BOOT = 1`.
- Nếu tay PS2 clone bị lỗi nút SELECT luôn nhấn: đặt `ENABLE_SELECT_EMERGENCY_STOP = 0`.

---

## 📄 Giấy phép

MIT License — tự do sử dụng, học tập và phát triển tiếp.

---

## 👤 Tác giả
Đồ án tốt nghiệp

> Nếu project này hữu ích, hãy ⭐ star để ủng hộ!
