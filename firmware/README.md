# Sumo Robot Firmware (ESP32)
---
## 1. Các thư viện cần cài đặt
Trước khi nạp code, hãy đảm bảo bạn đã cài đặt các thư viện sau thông qua **Library Manager** trên Arduino IDE (hoặc cấu hình trong `platformio.ini`):

*   `Wire.h` (Thư viện gốc có sẵn trong gói bo mạch ESP32)
*   `VL53L0X.h` (Thư viện điều khiển cảm biến khoảng cách ToF - Khuyên dùng bản của *Pololu*)

---

## 🔌 2. Sơ đồ cấu hình chân (Pin Mapping)
Dưới đây là cấu hình kết nối dự kiến giữa ESP32 và các mô-đun ngoại vi. (Có thể điều chỉnh lại trong quá trình thiết kế mạch PCB):

### Động cơ & Driver (Ví dụ dùng TB6612FNG / L298N)
*   `MOTOR_A_IN1` -> GPIO 12
*   `MOTOR_A_IN2` -> GPIO 13
*   `MOTOR_B_IN1` -> GPIO 14
*   `MOTOR_B_IN2` -> GPIO 27
*   `MOTOR_A_PWM` -> GPIO 25 (Điều khiển tốc độ bánh trái)
*   `MOTOR_B_PWM` -> GPIO 26 (Điều khiển tốc độ bánh phải)

### Cảm biến dò biên (Hồng ngoại hướng xuống sàn)
*   `LINE_SENSOR_LEFT`  -> GPIO 34 (Chân Analog/Digital đọc vạch trắng)
*   `LINE_SENSOR_RIGHT` -> GPIO 35

### Cảm biến khoảng cách ToF (VL53L0X - Giao tiếp I2C)
*   `SDA` -> GPIO 21 (Mặc định ESP32)
*   `SCL` -> GPIO 22 (Mặc định ESP32)

### Nút nhấn khởi động & Tín hiệu
*   `START_BUTTON` -> GPIO 0 (Nút nhấn kích hoạt chế độ chờ 5 giây)
*   `BUZZER`       -> GPIO 2 (Còi báo tín hiệu đếm ngược)

---

## 3. Kiến trúc thuật toán (Máy trạng thái)
Firmware được thiết kế chạy vòng lặp tuần hoàn (`loop`) chuyển đổi giữa các trạng thái cốt lõi:

1.  **Trạng thái Chờ (START_WAIT):** Nhấn nút khởi động -> Robot đếm ngược trễ đúng 5 giây (luật thi đấu bắt buộc) rồi mới chuyển sang trạng thái tiếp theo.
2.  **Trạng thái Dò tìm (SEARCH):** Xoay tròn hoặc di chuyển zic-zac để cảm biến VL53L0X quét tìm đối thủ trong phạm vi hiệu quả.
3.  **Trạng thái Tấn công (ATTACK):** Khi phát hiện đối thủ -> Tăng tốc tối đa hai động cơ húc thẳng để đẩy đối phương ra khỏi vòng tròn Dohyo.
4.  **Trạng thái Tránh biên (EVADE - Ưu tiên cao nhất):** Khi bất kỳ cảm biến dòng (Line Sensor) nào chạm phải vạch trắng -> Lập tức dừng, lùi lại và quay xe hướng ngược lại để tự bảo vệ.

---

## 4. Hướng dẫn nạp Code
1. Mở phần mềm **Arduino IDE**.
2. Vào `Tools` -> `Board` -> `ESP32 Arduino` -> Chọn đúng loại mạch bạn dùng (Ví dụ: `ESP32 Dev Module`).
3. Kết nối ESP32 với máy tính qua cáp Micro-USB / Type-C. Chọn đúng cổng `COM Port`.
4. Nhấn nút **Upload** (Mũi tên bên góc trái) để biên dịch và nạp code xuống mạch.