# Room Control System (YoloUno / ESP32-S3)

Hệ thống IoT quản lí phòng chạy trực tiếp trên board **YoloUno (ESP32-S3)**.
Thiết bị phát **Wi-Fi cục bộ (AP)** để người dùng kết nối, đăng nhập và điều khiển các thiết bị trong phòng.
Ngoài ra có thể chuyển sang **STA mode** để ESP32-S3 kết nối vào Wi-Fi có sẵn.

## Features

- Web dashboard (truy cập bằng trình duyệt) để:
    - Xem nhiệt độ/độ ẩm (giá trị lấy từ biến toàn cục cập nhật bởi task cảm biến)
    - Bật/tắt 2 kênh đèn (LED1/LED2)
    - Khóa/mở khóa cửa (relay)
- Authentication cơ bản qua trang đăng nhập (ID) + session cookie.
- Hỗ trợ AP/STA:
    - Mặc định: AP mode
    - Từ trang Settings: nhập SSID/PASS để chuyển sang STA
    - Nếu STA connect thất bại sau ~10s: tự quay về AP

## Hardware

- Controller: YoloUno (ESP32-S3)
- Actuators:
    - LED thường (2 kênh)
    - NeoPixel (1 kênh)
    - Relay điều khiển khóa cửa
- Cảm biến: nhiệt độ/độ ẩm (đang cập nhật vào `glob_temperature`, `glob_humidity`)

Thông số liên quan trong firmware:

- Door relay pin: `GPIO17` (xem `DOOR_RELAY_PIN`)
- Boot button: `GPIO0` (nhấn để chuyển về AP mode)

## Project structure

- `src/`:
    - `main.cpp`: tạo các FreeRTOS tasks
    - `mainserver.cpp`: Wi-Fi (AP/STA) + WebServer + handlers
    - `temp_humi_monitor.cpp`: cập nhật `glob_temperature`, `glob_humidity`
    - `led_blinky.cpp`, `neo_blinky.cpp`: điều khiển LED/NeoPixel theo biến global
- `include/`: headers
- `web/`: static web assets (hiện tại firmware đang render HTML inline trong `mainserver.cpp`; thư mục này dùng cho phiên bản serve từ SPIFFS nếu bạn mở rộng sau)

## Build & Upload (PlatformIO)

Yêu cầu:

- VS Code + PlatformIO extension
- Board profile: `yolo_uno` (đã cấu hình trong `platformio.ini`)

Lệnh thường dùng (PlatformIO Core):

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

## Usage

### 1) AP mode (default)

1. Sau khi upload, mở Serial Monitor để xem log.
2. Thiết bị phát Wi-Fi:
     - SSID (mặc định): `MY-ESP32-NETWORK`
     - Password (mặc định): `12345678`
3. Kết nối vào Wi-Fi này và mở trình duyệt tới IP AP (thường là `192.168.4.1`).
4. Đăng nhập bằng ID được cấp.

### 2) STA mode (connect to existing Wi-Fi)

1. Đăng nhập vào dashboard.
2. Vào `Wi-Fi Settings` và nhập SSID/PASS.
3. Thiết bị sẽ thử kết nối STA và in ra IP mới trong Serial Monitor khi kết nối thành công.
4. Nếu quá thời gian (~10s) không kết nối được, thiết bị tự quay về AP mode.

### Authentication (current behavior)

- Login form gửi `POST /login` với field `id`.
- ID hợp lệ hiện được hardcode trong firmware: `2210000` (`kAuthorizedId`).
- Sau khi login, server set cookie `session=1`.

Lưu ý: đây là cơ chế xác thực cơ bản phục vụ demo/lab, chưa phải mô hình bảo mật production.

## HTTP Endpoints (Firmware)

Các endpoint chính (xem `src/mainserver.cpp`):

- `GET /`:
    - Nếu chưa login: trả về trang login
    - Nếu đã login: trả về dashboard
- `GET /login`: trang login
- `POST /login`: submit login (field `id`)
- `GET /logout`: logout, clear cookie
- `GET /toggle?led=1|2`: toggle LED1/LED2, trả JSON:
    - `{"led1":"ON|OFF","led2":"ON|OFF"}`
- `GET /sensors`: trả JSON cảm biến + trạng thái cửa:
    - `{"temp":<float>,"hum":<float>,"door":"LOCKED|UNLOCKED"}`
- `GET /door?action=lock|unlock|toggle`: điều khiển cửa, trả JSON:
    - `{"state":"LOCKED|UNLOCKED"}`
- `GET /settings`: trang cấu hình Wi-Fi
- `GET /connect?ssid=<...>&pass=<...>`: bắt đầu kết nối STA

## Notes / Troubleshooting

- Nếu truy cập web bị redirect liên tục: thử `Logout` rồi login lại.
- Nếu STA connect xong không biết IP: xem Serial Monitor log dòng `STA IP address:`.
- Nếu cần đổi mức kích relay (active-low/active-high): chỉnh `DOOR_LOCK_LEVEL` / `DOOR_UNLOCK_LEVEL` trong `src/mainserver.cpp`.