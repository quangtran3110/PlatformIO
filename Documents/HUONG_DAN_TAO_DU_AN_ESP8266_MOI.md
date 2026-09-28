# Hướng dẫn tạo, build và phát hành một dự án ESP8266 mới

Tài liệu này dành cho người bắt đầu từ con số 0 và áp dụng cho kho mã nguồn Private:

`D:\AI_PROJECTS\PIO`

Mục tiêu là tạo một dự án mới theo cách dễ hiểu, build được, lưu bằng Git, nạp USB lần đầu và sẵn sàng OTA Private cho các phiên bản sau.

> Nguyên tắc quan trọng nhất: **mã nguồn và firmware là hai thứ khác nhau**. Mã nguồn là các tệp `.cpp`, `.h`, `platformio.ini`; firmware là tệp nhị phân `firmware.bin` được tạo ra sau khi build.

---

## 1. Hiểu mô hình đang sử dụng

Toàn bộ dự án ESP được lưu trong một repository GitHub Private:

- Thư mục làm việc: `D:\AI_PROJECTS\PIO`
- GitHub: repository `PlatformIO` ở chế độ Private
- Thư viện dùng chung: `D:\AI_PROJECTS\PIO\lib`
- Mỗi trạm hoặc thiết bị là một thư mục PlatformIO riêng
- Tệp build `.pio` chỉ tồn tại trên máy, không đưa vào Git
- Firmware phát hành OTA được lưu riêng trên Cloudflare Workers KV

Ví dụ một dự án mới đặt ngay ở thư mục gốc:

```text
D:\AI_PROJECTS\PIO\TRAM_MOI
├── include
│   ├── secrets.example.h
│   └── secrets.h              ← chỉ có trên máy, Git bỏ qua
├── src
│   └── main.cpp
├── ota                        ← chỉ cần khi dự án dùng OTA Private
├── platformio.ini
└── README.md
```

Không tạo dự án mới trong OneDrive. Không copy cả thư mục `.pio` từ dự án khác.

---

## 2. Các khái niệm cần nhớ

### 2.1 PlatformIO

PlatformIO là công cụ biên dịch và nạp chương trình cho ESP. Trong máy hiện tại, lệnh chính nằm tại:

```powershell
C:\Users\quang\.platformio\penv\Scripts\platformio.exe
```

### 2.2 Git và GitHub

- Git lưu lịch sử thay đổi trên máy.
- GitHub Private lưu bản sao an toàn trên Internet.
- `commit` là một mốc lịch sử.
- `push` là gửi các commit từ máy lên GitHub.
- Không coi thư mục `.pio` là mã nguồn; nó có thể build lại bất cứ lúc nào.

### 2.3 Phiên bản firmware

Quy ước đang dùng:

```text
YYMMDD.SỐ_THỨ_TỰ
```

Ví dụ:

- `260927.1`: bản thứ nhất ngày 27/09/2026
- `260927.2`: bản thứ hai cùng ngày

Mỗi lần phát hành phải tăng phiên bản. Không phát hành hai binary khác nhau với cùng một số phiên bản.

### 2.4 Build, upload USB và OTA

- **Build:** chuyển mã nguồn thành `firmware.bin`; chưa ảnh hưởng thiết bị.
- **Upload USB:** nạp firmware qua cáp USB; thiết bị sẽ khởi động lại.
- **OTA:** thiết bị tự tải firmware qua mạng; thiết bị cũng sẽ khởi động lại.

Build thành công chưa có nghĩa là thiết bị ngoài hiện trường đã chạy đúng.

---

## 3. Chuẩn bị một lần trên máy tính

### Bước 1 — Mở đúng thư mục

Trong VS Code, chọn:

```text
File → Open Folder → D:\AI_PROJECTS\PIO
```

Luôn kiểm tra thanh tiêu đề VS Code. Nếu còn đường dẫn `OneDrive`, dừng lại và mở lại đúng thư mục trên ổ `D:`.

### Bước 2 — Kiểm tra PlatformIO

Mở Terminal trong VS Code và chạy:

```powershell
& ''C:\Users\quang\.platformio\penv\Scripts\platformio.exe'' --version
```

Nếu hiện số phiên bản thì PlatformIO đã hoạt động.

### Bước 3 — Kiểm tra Git

```powershell
git -C D:\AI_PROJECTS\PIO status
```

Trước khi bắt đầu một việc mới, kết quả nên là cây làm việc sạch, không có tệp lạ chưa commit.

---

## 4. Tạo dự án mới

Ví dụ dưới đây dùng tên `TRAM_MOI`. Khi làm thật, thay bằng tên ngắn, không dấu và không chứa ký tự đặc biệt.

Tên tốt:

```text
TRAM_TAN_AN
AP_LUC_KHU_1
VOLUME_GIENG_4
```

Tên nên tránh:

```text
Trạm mới của tôi
Test cuối cùng bản mới nhất
New folder (2)
```

### Cách tạo bằng giao diện PlatformIO

1. Mở biểu tượng PlatformIO ở thanh bên trái VS Code.
2. Chọn **PIO Home**.
3. Chọn **New Project**.
4. Project Name: nhập `TRAM_MOI`.
5. Board: chọn **NodeMCU 1.0 (ESP-12E Module)** nếu phần cứng là ESP8266 NodeMCU.
6. Framework: chọn **Arduino**.
7. Location: bỏ chọn vị trí mặc định và chọn `D:\AI_PROJECTS\PIO`.
8. Chờ PlatformIO tạo dự án.

Sau đó xác nhận có các tệp:

```text
TRAM_MOI\platformio.ini
TRAM_MOI\src\main.cpp
```

---

## 5. Cấu hình `platformio.ini`

Với dự án đặt trực tiếp dưới `D:\AI_PROJECTS\PIO`, dùng mẫu:

```ini
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
monitor_speed = 115200
lib_extra_dirs = ../lib
```

Nếu dự án nằm sâu hơn một cấp, ví dụ:

```text
D:\AI_PROJECTS\PIO\TRAM_CC\TRAM_MOI
```

thì đường dẫn thư viện phải là:

```ini
lib_extra_dirs = ../../lib
```

Quy tắc: mỗi `..` là đi ngược lên một thư mục. Không ghi lại đường dẫn OneDrive tuyệt đối.

### Kiểm tra cấu hình

Tại thư mục dự án, chạy:

```powershell
& ''C:\Users\quang\.platformio\penv\Scripts\platformio.exe'' project config --json-output
```

Nếu lệnh trả về JSON và không báo lỗi thì `platformio.ini` có cú pháp hợp lệ.

---

## 6. Tách Wi‑Fi và token cho dự án mới

Các dự án cũ được giữ nguyên. Riêng dự án mới nên tách thông tin nhạy cảm ngay từ đầu.

### Bước 1 — Tạo `include\secrets.example.h`

Tệp này được đưa vào Git nhưng chỉ chứa chỗ trống:

```cpp
#pragma once

#define WIFI_SSID_VALUE "DIEN_TEN_WIFI"
#define WIFI_PASSWORD_VALUE "DIEN_MAT_KHAU_WIFI"
#define BLYNK_AUTH_TOKEN_VALUE "DIEN_BLYNK_AUTH_TOKEN"
```

### Bước 2 — Tạo `include\secrets.h`

Copy nội dung từ `secrets.example.h`, rồi điền giá trị thật. Tệp này chỉ nằm trên máy.

Repository gốc đã được cấu hình bỏ qua:

```text
**/include/secrets.h
```

### Bước 3 — Kiểm tra trước khi commit

```powershell
git -C D:\AI_PROJECTS\PIO status --short
```

Phải thấy `secrets.example.h`, nhưng không được thấy `secrets.h`.

Nếu `secrets.h` xuất hiện trong danh sách Git, không commit; kiểm tra lại `.gitignore` trước.

---

## 7. Chương trình tối thiểu để kiểm tra ESP và Blynk

Nội dung cơ bản cho `src\main.cpp`:

```cpp
#define BLYNK_TEMPLATE_ID "DIEN_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "TRAM MOI"
#define BLYNK_FIRMWARE_VERSION "260927.1"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "secrets.h"

BlynkTimer timer;

void sendHeartbeat() {
  if (Blynk.connected()) {
    Blynk.virtualWrite(V1, millis() / 1000UL);
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID_VALUE, WIFI_PASSWORD_VALUE);

  Blynk.config(BLYNK_AUTH_TOKEN_VALUE);
  timer.setInterval(5000L, sendHeartbeat);
}

void loop() {
  Blynk.run();
  timer.run();
}
```

Mẫu này chưa điều khiển relay. Mục đích đầu tiên chỉ là xác nhận:

1. ESP khởi động.
2. Wi‑Fi kết nối.
3. Blynk Online.
4. V1 cập nhật mỗi 5 giây.

Chỉ thêm relay, cảm biến và logic vận hành sau khi phần kết nối cơ bản đã ổn định.

---

## 8. Thêm relay theo nguyên tắc an toàn

Trước khi viết mã, phải biết rõ:

- Relay kích bằng HIGH hay LOW.
- Chân nào là START, STOP hoặc relay giữ mức.
- Khi ESP mất nguồn, trạng thái phần cứng thực tế ra sao.
- Chân boot ESP8266 có bị ảnh hưởng không.
- Có PCF8575, watchdog ngoài hoặc mạch tự giữ hay không.

Không copy nguyên chân relay của trạm khác.

Ví dụ relay active-LOW, trạng thái nghỉ là HIGH:

```cpp
constexpr uint8_t RELAY_PIN = D1;
constexpr uint8_t RELAY_IDLE = HIGH;

void initializeRelaySafely() {
  digitalWrite(RELAY_PIN, RELAY_IDLE);
  pinMode(RELAY_PIN, OUTPUT);
}
```

Gọi `initializeRelaySafely()` ngay đầu `setup()`, trước khi kết nối Blynk.

### Quy tắc sau reset

- Không tự phát lại lệnh START chỉ vì ESP vừa khởi động.
- Không dùng dữ liệu Blynk vừa đồng bộ để điều khiển relay ngay lập tức.
- Chỉ cho phép điều khiển khi cảm biến, cấu hình và trạng thái mạng đã hợp lệ.
- Nút Blynk dạng xung phải tự trả về 0 sau khi xử lý.
- Dùng `millis()` hoặc state machine; tránh `delay()` dài trong logic vận hành.

Ví dụ lệnh xung:

```cpp
BLYNK_WRITE(V10) {
  if (param.asInt() != 1) return;

  if (systemReadyForRemoteControl()) {
    requestStartPulse();
  }

  Blynk.virtualWrite(V10, 0);
}
```

`systemReadyForRemoteControl()` phải kiểm tra các điều kiện thật của từng trạm; không tạo một hàm luôn trả về `true` chỉ để hết lỗi build.

---

## 9. Build lần đầu

Mở PowerShell và chạy:

```powershell
$ProjectPath = ''D:\AI_PROJECTS\PIO\TRAM_MOI''
& ''C:\Users\quang\.platformio\penv\Scripts\platformio.exe'' run --project-dir $ProjectPath
```

Khi thành công sẽ thấy:

```text
SUCCESS
```

Firmware được tạo tại:

```text
D:\AI_PROJECTS\PIO\TRAM_MOI\.pio\build\nodemcuv2\firmware.bin
```

### Kiểm tra kích thước và hash

```powershell
$FirmwarePath = ''D:\AI_PROJECTS\PIO\TRAM_MOI\.pio\build\nodemcuv2\firmware.bin''
Get-Item $FirmwarePath | Select-Object FullName, Length
Get-FileHash $FirmwarePath -Algorithm SHA256
Get-FileHash $FirmwarePath -Algorithm MD5
```

- SHA‑256 dùng để đối chiếu chính xác file phát hành.
- MD5 được ESP8266HTTPUpdate dùng để phát hiện file bị lỗi khi tải.
- Hash chỉ bảo đảm file giống nhau; quyền tải firmware vẫn phải được bảo vệ bằng khóa OTA Private.

---

## 10. Nạp USB lần đầu

Một dự án mới nên được nạp USB và kiểm tra tại chỗ trước khi cho phép OTA từ xa.

### Bước 1 — Kết nối cáp

1. Dùng cáp USB có truyền dữ liệu.
2. Mở Device Manager.
3. Xem cổng COM của CH340/USB‑Serial.
4. Không đoán COM; kiểm tra bằng cách rút và cắm lại cáp.

### Bước 2 — Khai báo cổng nếu cần

Thêm vào `platformio.ini`:

```ini
upload_port = COM7
monitor_port = COM7
```

Thay `COM7` bằng cổng thực tế.

### Bước 3 — Nạp firmware

```powershell
$ProjectPath = ''D:\AI_PROJECTS\PIO\TRAM_MOI''
& ''C:\Users\quang\.platformio\penv\Scripts\platformio.exe'' run --project-dir $ProjectPath --target upload
```

### Bước 4 — Xem Serial

```powershell
& ''C:\Users\quang\.platformio\penv\Scripts\platformio.exe'' device monitor --port COM7 --baud 115200
```

### Nghiệm thu USB

- ESP boot một lần, không reset lặp.
- Không có relay tự tác động khi khởi động.
- Wi‑Fi kết nối.
- Blynk Online đúng thiết bị.
- Version trên Blynk đúng với mã nguồn.
- Telemetry có timestamp mới.
- Mất Wi‑Fi rồi có lại: ESP tự phục hồi.
- Tắt/bật nguồn: trạng thái boot vẫn an toàn.

Nạp firmware thường không xóa EEPROM nếu không dùng chế độ erase toàn bộ flash. Tuy nhiên trước khi nạp thiết bị đang vận hành vẫn phải xác định dữ liệu nào nằm trong EEPROM/flash và có cần sao lưu hay không.

---

## 11. Lưu dự án vào GitHub Private

Từ repository gốc:

```powershell
git -C D:\AI_PROJECTS\PIO status --short
git -C D:\AI_PROJECTS\PIO diff --check
```

Kiểm tra danh sách tệp:

- Có `platformio.ini`, `src\main.cpp`, `README.md`, `secrets.example.h`.
- Không có `.pio`.
- Không có `secrets.h`.
- Không có file chứa khóa Cloudflare hoặc token GitHub.

Sau đó:

```powershell
git -C D:\AI_PROJECTS\PIO add TRAM_MOI
git -C D:\AI_PROJECTS\PIO commit -m "Add TRAM_MOI firmware 260927.1"
git -C D:\AI_PROJECTS\PIO push origin main
```

Nếu đang sửa trạm quan trọng, nên làm trên một nhánh riêng rồi mới hợp nhất vào `main` sau khi build và review.

---

## 12. Chuẩn bị OTA Private ngay từ firmware đầu tiên

Đây là điểm rất quan trọng rút ra từ lần chuyển đổi Cái Cát:

> Firmware USB đầu tiên phải chứa sẵn URL OTA Private cuối cùng. Không dùng URL `raw.githubusercontent.com` của repository Private làm URL OTA.

Nếu firmware đang chạy chỉ biết URL GitHub Public, sau khi chuyển repository thành Private nó sẽ không tải được bản mới. Khi đó phải nạp USB lại hoặc tạm mở Public như trường hợp chuyển đổi đặc biệt.

### Kiến trúc OTA Private

```text
ESP8266
   │ HTTPS + khóa riêng của trạm
   ▼
Cloudflare Worker
   │ kiểm tra khóa và đọc manifest
   ▼
Workers KV
   ├── ten-tram/latest.json
   └── ten-tram/releases/260927.1/firmware.bin
```

Mỗi trạm cần:

- Một định danh ổn định, ví dụ `tram-tan-an`.
- Một khóa OTA riêng, đủ dài và ngẫu nhiên.
- Một URL riêng chứa khóa.
- Một manifest `latest.json`.
- Các binary được lưu theo từng phiên bản.

Không dùng chung một khóa cho toàn bộ các trạm. Không ghi khóa vào Worker source, Git, log hoặc tài liệu.

### Thêm Clean OTA vào mã nguồn

Kho hiện có thư viện dùng chung:

```text
D:\AI_PROJECTS\PIO\lib\CleanOta
```

Trong `main.cpp`:

```cpp
#include <CleanOta.h>

#define URL_fw_Bin "https://TEN-WORKER.workers.dev/ten-tram/KHOA_RIENG/firmware.bin"
```

Đầu `setup()` phải xử lý OTA trước logic thiết bị:

```cpp
void setup() {
  Serial.begin(115200);

  if (CleanOta::handleBoot(
        WIFI_SSID_VALUE,
        WIFI_PASSWORD_VALUE,
        URL_fw_Bin)) {
    return;
  }

  // Chỉ sau đây mới khởi tạo relay, EEPROM, RTC, cảm biến và Blynk.
}
```

Lệnh OTA sạch dùng chính chân Terminal đang có của dự án. Ví dụ dưới đây dùng V5; dự án khác có thể dùng V0, V10, V12 hoặc chân Terminal riêng của dự án đó:

```cpp
BLYNK_WRITE(V5) {
  String command = param.asStr();

  if (command.startsWith("ota_info:")) {
    String requestId = command.substring(9);
    requestId.trim();
    bool validRequestId = requestId.length() == 12;
    for (uint8_t i = 0; validRequestId && i < requestId.length(); i++) {
      const char c = requestId.charAt(i);
      validRequestId = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    }
    if (validRequestId) {
      String response = "ota_reply:" + requestId +
                        "|version=" + BLYNK_FIRMWARE_VERSION;
      Blynk.virtualWrite(V5, response);
    }
  } else if (command == "update") {
    Blynk.virtualWrite(V5, "Đã nhận yêu cầu OTA; ESP sẽ khởi động lại.");
    if (!CleanOta::requestAndRestart()) {
      Blynk.virtualWrite(V5, "OTA lỗi: không ghi được yêu cầu.");
    }
  }
}
```

Không dùng `terminal.println()` hoặc `WidgetTerminal.println()`. Gửi thông báo bằng `Blynk.virtualWrite()`; lỗi kỹ thuật chi tiết ghi ra Serial.

OTA Manager lưu chân Terminal riêng cho từng trạm. Mỗi lần kiểm tra, phần mềm gửi `ota_info:<mã-ngẫu-nhiên>` và chỉ nhận `ota_reply` có đúng mã đó. Vì vậy không cần tạo thêm datastream chỉ để báo phiên bản và không thể nhầm với nội dung Terminal cũ.

### Vì sao OTA sạch khởi động lại trước khi tải?

Ở lần boot OTA:

- Chưa khởi tạo relay điều khiển.
- Chưa chạy lịch tự động.
- Chưa đọc/ghi EEPROM vận hành.
- Chưa xử lý lệnh Blynk thông thường.
- Chỉ kết nối Wi‑Fi, tải firmware, kiểm tra và khởi động lại.

Cách này giảm nguy cơ OTA chặn chương trình trong khi relay hoặc watchdog vẫn đang cần được phục vụ.

---

## 13. Quy trình phát hành một phiên bản firmware mới

Luôn làm đúng thứ tự dưới đây.

### Giai đoạn A — Chuẩn bị mã nguồn

1. Xác nhận đúng thư mục dự án trên ổ `D:`.
2. Kiểm tra Git sạch.
3. Chỉ sửa những nội dung nằm trong phạm vi yêu cầu.
4. Tăng `BLYNK_FIRMWARE_VERSION`.
5. Kiểm tra diff để chắc Wi‑Fi, token, chân I/O và logic khác không bị đổi ngoài ý muốn.

### Giai đoạn B — Kiểm tra tĩnh và build

1. Kiểm tra cú pháp `platformio.ini`.
2. Build firmware.
3. Ghi lại RAM, flash, kích thước binary.
4. Tính SHA‑256 và MD5.
5. Chạy test của Worker nếu có thay đổi phần OTA.

Nếu build lỗi, không phát hành.

### Giai đoạn C — Lưu mã nguồn

1. `git diff --check`.
2. Xác nhận không có secret hoặc `.pio` trong danh sách commit.
3. Commit mã nguồn.
4. Push lên GitHub Private.

### Giai đoạn D — Phát hành binary

1. Upload binary vào khóa có phiên bản:

   ```text
   ten-tram/releases/260927.2/firmware.bin
   ```

2. Chỉ khi upload binary thành công mới cập nhật `latest.json`.
3. Tải ngược firmware qua chính URL mà ESP sẽ sử dụng.
4. So sánh kích thước, SHA‑256 và MD5 với file local.
5. Nếu khác dù chỉ một byte, dừng; không gửi lệnh OTA.

Không ghi đè một phiên bản cũ bằng binary mới. Muốn sửa lại phải tăng phiên bản.

### Giai đoạn E — Kiểm tra trước OTA

Ghi nhận:

- Tên thiết bị Blynk.
- Device URI.
- Phiên bản đang chạy được thiết bị trả lời qua Terminal.
- Trạng thái Online.
- Thời gian telemetry mới nhất.
- Chất lượng mạng.
- Trạng thái vận hành cần theo dõi.

Chỉ gửi lệnh khi chắc chắn đúng thiết bị và firmware đích đã sẵn sàng.

### Giai đoạn F — Gửi OTA

Gửi đúng một lệnh `update` đến chân Terminal đã cấu hình của đúng thiết bị.

Không gửi liên tiếp nhiều lần. Thiết bị cần thời gian:

1. Ghi yêu cầu vào RTC memory.
2. Khởi động lại.
3. Kết nối Wi‑Fi.
4. Tải firmware.
5. Ghi flash.
6. Khởi động firmware mới.
7. Kết nối lại Blynk.

### Giai đoạn G — Nghiệm thu sau OTA

Chỉ kết luận thành công khi đủ các điều kiện:

- Thiết bị reconnect.
- Thiết bị trả lời đúng phiên bản mới qua Terminal bằng mã kiểm tra của lần nghiệm thu.
- Build date thay đổi đúng.
- Telemetry mới tiếp tục xuất hiện.
- Không reset lặp.
- Cảm biến trả dữ liệu hợp lý.
- Trạng thái relay/máy không thay đổi ngoài dự kiến.
- Theo dõi ổn định thêm ít nhất vài chu kỳ; với thay đổi quan trọng nên theo dõi 24–48 giờ.

Việc V5 trở về 0 hoặc thiết bị Online lại chưa đủ để chứng minh OTA thành công.

---

## 14. Quản lý firmware

### Tệp nào được giữ ở đâu?

| Nội dung | Nơi lưu | Đưa vào Git? |
|---|---|---|
| `main.cpp`, `.h`, `platformio.ini` | GitHub Private | Có |
| `secrets.example.h` | GitHub Private | Có |
| `secrets.h` | Máy lập trình/an toàn riêng | Không |
| `.pio\...\firmware.bin` | Máy build tạm thời | Không |
| Firmware đã phát hành | Cloudflare KV | Không nằm trong Git |
| `latest.json` sinh ra | Thư mục release-output | Không |
| SHA‑256, version, thời gian phát hành | Nhật ký phát hành | Có thể lưu |

### Nên giữ bao nhiêu phiên bản?

Mỗi trạm nên giữ 2–3 phiên bản gần nhất:

- Bản hiện tại.
- Bản ngay trước đó để rollback.
- Một bản ổn định dài hạn nếu cần.

### Rollback

Rollback không phải đổi tên file cũ thành tên mới. Cách an toàn:

1. Lấy mã nguồn của bản ổn định trước.
2. Tăng số phiên bản mới, ví dụ từ lỗi `260927.3` thành rollback `260927.4`.
3. Build lại.
4. Phát hành như một phiên bản mới.
5. Kiểm tra hash rồi OTA.

---

## 15. Khi nào phải dùng USB thay vì OTA?

Dùng USB khi:

- Firmware hiện tại chưa có OTA.
- URL OTA trong firmware hiện tại không còn truy cập được.
- Wi‑Fi hoặc chứng chỉ TLS không kết nối được.
- OTA bị watchdog ngoài reset giữa chừng.
- Thiết bị reset lặp trước khi kết nối mạng.
- Thay đổi bootloader, flash layout hoặc cần kiểm tra Serial tại chỗ.
- Chưa xác định chắc phần cứng relay/watchdog.

Không mở repository Public như quy trình bình thường. Việc mở Public tạm thời cho Cái Cát ngày 27/09/2026 chỉ là cầu nối cho firmware cũ đã chứa URL GitHub Public; dự án mới phải tránh tình huống này bằng cách nạp URL OTA Private ngay trong lần USB đầu tiên.

---

## 16. Các lỗi thường gặp

### Build báo thiếu thư viện

Kiểm tra `lib_extra_dirs`. Dự án ở cấp gốc dùng `../lib`; dự án sâu hai cấp dùng `../../lib`.

### Build được nhưng không có `firmware.bin`

Kiểm tra đúng environment `nodemcuv2` và đọc dòng cuối log. Nếu build thất bại ở bước link thì binary không được tạo.

### ESP không vào được Blynk

Kiểm tra lần lượt:

1. SSID và mật khẩu Wi‑Fi.
2. ESP có nhận IP không.
3. AuthToken có thuộc đúng thiết bị không.
4. Template ID có đúng không.
5. Blynk server và mạng Internet.

### OTA nhận lệnh nhưng vẫn ở phiên bản cũ

Kiểm tra:

1. URL nhúng trong firmware đang chạy, không chỉ URL trong mã nguồn mới.
2. URL trả HTTP 200 từ mạng ngoài.
3. Binary tải xuống có đúng kích thước/hash.
4. RTC request có được ghi thành công không.
5. Wi‑Fi có kết nối trong thời gian OTA không.
6. Watchdog ngoài có reset ESP giữa quá trình không.
7. Metadata Blynk có cập nhật sau reconnect chưa.

### Thiết bị reconnect nhưng metadata vẫn cũ

Chờ thêm một chu kỳ refresh. Nếu telemetry đã mới nhưng version vẫn cũ sau reconnect đầy đủ, coi OTA chưa đạt; không gửi lặp liên tục, phải chẩn đoán nguyên nhân.

### Git báo `.pio` hoặc `firmware.bin`

Không commit. Kiểm tra `.gitignore`. Nếu trước đây file đã được theo dõi, cần bỏ theo dõi bằng Git nhưng vẫn giữ file local.

### Git báo `secrets.h`

Dừng commit ngay. Không đẩy file này lên GitHub. Kiểm tra mẫu ignore `**/include/secrets.h`.

---

## 17. Checklist ngắn cho mỗi lần phát hành

```text
[ ] Đúng dự án trên D:\AI_PROJECTS\PIO
[ ] Git sạch trước khi sửa
[ ] Đã tăng version
[ ] Không đổi Wi‑Fi/token/chân I/O ngoài ý muốn
[ ] Build SUCCESS
[ ] Ghi RAM/flash/kích thước
[ ] Ghi SHA‑256 và MD5
[ ] Test Worker đạt nếu phần OTA thay đổi
[ ] Commit và push mã nguồn lên GitHub Private
[ ] Upload binary theo đúng version
[ ] latest.json chỉ cập nhật sau binary
[ ] Tải ngược và hash khớp tuyệt đối
[ ] Đúng tên và Device URI trên Blynk
[ ] Thiết bị Online, telemetry mới, mạng đủ tốt
[ ] Chỉ gửi một lệnh update
[ ] Reconnect đúng version mới
[ ] Telemetry tiếp tục cập nhật
[ ] Không reset lặp
[ ] Theo dõi vận hành sau OTA
```

---

## 18. Ví dụ nghiệm thu thực tế: Trạm Cái Cát

Ngày 27/09/2026:

- Phiên bản trước: `260926.2`
- Phiên bản mới: `260926.4`
- Binary: 451.808 byte
- SHA‑256: `9776653ECDD82E2F6B54253C5EB9468BECA225165F525789331A446463D715C0`
- Worker test: 3/3 đạt
- Binary tải ngược từ Cloudflare khớp tuyệt đối
- Blynk xác nhận reconnect với `260926.4`
- Telemetry tiếp tục cập nhật
- Chất lượng mạng sau OTA: `2`
- Không có reset lặp trong thời gian nghiệm thu ban đầu
- Repository đã được đóng lại Private ngay sau bootstrap

Trường hợp này cần mở Public tạm thời vì firmware cũ đã nhúng URL GitHub Public. Đây là thao tác chuyển đổi một lần, không phải quy trình phát hành chuẩn cho dự án mới.

---

## 19. Quy trình chuẩn nên nhớ

```text
Sửa mã nguồn
   ↓
Tăng version
   ↓
Review diff
   ↓
Build + test
   ↓
Tính hash
   ↓
Commit/push GitHub Private
   ↓
Upload Cloudflare KV
   ↓
Tải ngược và so hash
   ↓
Kiểm tra thiết bị trước OTA
   ↓
Gửi một lệnh update
   ↓
Xác nhận reconnect + đúng version + telemetry
   ↓
Theo dõi ổn định và ghi nhật ký phát hành
```

Nếu bất kỳ bước nào chưa đạt, dừng tại bước đó. Không dùng việc thiết bị “vẫn Online” hoặc “build thành công” để thay cho nghiệm thu đầy đủ.
