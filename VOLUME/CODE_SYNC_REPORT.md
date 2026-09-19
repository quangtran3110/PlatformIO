# Báo Cáo Đồng Bộ Firmware Lưu Lượng (VOLUME Code Sync Report - Nghiệm Thu)

> **Ngày thực hiện**: 19/09/2026  
> **Workspace**: `C:\Users\quang\OneDrive\Work\PIO\VOLUME`  
> **Phiên bản đồng bộ**: `260919.2` (áp dụng cho toàn bộ 12 dự án)  
> **Trạng thái**: **ĐẠT kiểm tra mã nguồn và biên dịch độc lập**. Toàn bộ 12 dự án đã được bổ sung direct dependency headers prelude, chuẩn hóa `lib_extra_dirs = ../../lib`, vượt qua 380/380 kiểm tra tĩnh và 12/12 bản dựng PlatformIO ngày 19/09/2026.

---

## 1. Danh Sách File Đã Tạo và Đã Sửa

### File Tạo Mới & Lõi Chung
1. `VOLUME/shared/volume_reader_core.h`: Lõi firmware dùng chung duy nhất được chuẩn hóa:
   - Tích hợp cơ chế **OTA sạch (Clean OTA Boot)** qua ESP RTC user memory (offset 32 words / 128 bytes). Chốt trạng thái `OTA_RTC_SUCCESS` ngay trong callback `ESPhttpUpdate.onEnd(...)` trước khi restart.
   - Chuẩn hóa quy tắc **một định nghĩa `PIN_TERMINAL`** duy nhất từ `main.cpp`, loại bỏ hoàn toàn xung đột redefinition.
   - Nâng cấp **chính sách lỗi đọc EEPROM chặt chẽ**: từ chối khởi tạo/ghi đè nếu có bất kỳ short read nào tại state slots; phân biệt lỗi I2C với lỗi CRC trong hàng đợi ngày.
   - Báo cáo chẩn đoán `i2c` đầy đủ: Firmware version, reset reason, last OTA status, heap/max block, bộ đếm xung accepted/rejected/ignored-glitch, Wi-Fi RSSI.
2. `VOLUME/TRAM1/platformio.ini`: File cấu hình PlatformIO cho trạm mới `TRAM1` (`nodemcuv2`, CPU 160MHz, `lib_extra_dirs = ../../lib`).
3. `VOLUME/tools/verify_volume_sync.py`: Script kiểm tra tĩnh xác minh 12 dự án, bao gồm kiểm tra `lib_extra_dirs = ../../lib` và direct dependency headers prelude trong `main.cpp`.

### Chuẩn Hóa Cấu Hình `platformio.ini` Cả 12 Dự Án
- Toàn bộ 12 file `platformio.ini` đã được chuẩn hóa đường dẫn tương đối nhất quán: `lib_extra_dirs = ../../lib` (thay cho đường dẫn tuyệt đối hoặc bổ sung dòng bị thiếu).

### File Cấu Hình Từng Trạm (`src/main.cpp`) Đã Nâng Cấp Version `260919.2` & Direct Includes
Cả 12 file `src/main.cpp` đều được bổ sung direct dependency headers prelude (`<Arduino.h>`, `<BlynkSimpleEsp8266.h>`, `<ESP8266HTTPClient.h>`, `<ESP8266WiFi.h>`, `<ESP8266httpUpdate.h>`, `<I2C_eeprom.h>`, `<RTClib.h>`, `<SPI.h>`, `<TimeLib.h>`, `<WiFiClientSecure.h>`, `<UrlEncode.h>`, `<Wire.h>`) ngay trước `#include "../../shared/volume_reader_core.h"` để PlatformIO LDF nhận diện đầy đủ thư viện:
1. `VOLUME/TRAM1/src/main.cpp`: Cấu hình đầy đủ cho `TRAM1` (đã được Codex điền thông tin bí mật và Wi-Fi), RTC DS3231, EEPROM 0x57 (4096), active-LOW D6, live V24, daily V25, terminal V0, version `260919.2`.
2. `VOLUME/TRAM_CC_G1/src/main.cpp`: Version `260919.2`, bảo toàn token, pin V51/V52, EEPROM 0x57, RTC DS3231, active-HIGH D6.
3. `VOLUME/TRAM_CC_G2/src/main.cpp`: Version `260919.2`, bảo toàn token, pin V54/V55, EEPROM 0x57, RTC DS3231, active-HIGH D6.
4. `VOLUME/TRAM_CC_G3/src/main.cpp`: Version `260919.2`, bảo toàn token, pin V57/V58, EEPROM 0x57, RTC DS3231, active-HIGH D6.
5. `VOLUME/TRAM2_G1/src/main.cpp`: Version `260919.2`, bảo toàn token, pin V62/V61, EEPROM 0x50 dung lượng **32768**, RTC DS3231, active-HIGH D6 (giữ nguyên custom build flags và monitor filters).
6. `VOLUME/TRAM2_G2/src/main.cpp`: Version `260919.2`, bảo toàn token, pin V58/V59, EEPROM 0x57, RTC DS3231, active-HIGH D6.
7. `VOLUME/TRAM2_G3/src/main.cpp`: Version `260919.2`, bảo toàn token, pin V60/V63, EEPROM 0x57, RTC DS3231, active-HIGH D6.
8. `VOLUME/TRAM2BPT/src/main.cpp`: Version `260919.2`, nâng cấp từ nhánh cũ lên lõi chung, DS1307, EEPROM 0x50, active-LOW D6, live V24, daily V25.
9. `VOLUME/TRAM3/src/main.cpp`: Version `260919.2`, tích hợp tham chiếu chuẩn vào lõi chung, DS1307, EEPROM 0x50, active-LOW D6, live V23, daily V24.
10. `VOLUME/TRAM3BPT/src/main.cpp`: Version `260919.2`, nâng cấp từ nhánh cũ lên lõi chung, DS1307, EEPROM 0x50, active-LOW D6, live V29, daily V31.
11. `VOLUME/TRAM4/src/main.cpp`: Version `260919.2`, bảo toàn token, pin V37/V35, EEPROM 0x57, RTC DS3231, active-HIGH D6.
12. `VOLUME/TRAMBHD/src/main.cpp`: Version `260919.2`, nâng cấp từ nhánh cũ lên lõi chung, DS3231, EEPROM 0x57, active-LOW D6, live V32, daily V33.

---

## 2. Ma Trận Cấu Hình Toàn Bộ 12 Trạm

| STT | Trạm | Phiên Bản | RTC Chip | EEPROM Addr | Dung Lượng EEPROM | Mức Kích Xung D6 | Pin Live | Pin Daily | Pin Terminal | State Magic | OTA Thư Mục |
|:---:|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| 1 | `TRAM1` | **260919.2** | DS3231 | `0x57` | 4096 B | **LOW** | V24 | V25 | V0 | `0x54314451UL` ("T1DQ") | `TRAM1` |
| 2 | `TRAM_CC_G1` | **260919.2** | DS3231 | `0x57` | 4096 B | HIGH | V51 | V52 | V0 | `0x43314451UL` ("C1DQ") | `TRAM_CC_G1` |
| 3 | `TRAM_CC_G2` | **260919.2** | DS3231 | `0x57` | 4096 B | HIGH | V54 | V55 | V0 | `0x43324451UL` ("C2DQ") | `TRAM_CC_G2` |
| 4 | `TRAM_CC_G3` | **260919.2** | DS3231 | `0x57` | 4096 B | HIGH | V57 | V58 | V0 | `0x43334451UL` ("C3DQ") | `TRAM_CC_G3` |
| 5 | `TRAM2_G1` | **260919.2** | DS3231 | `0x50` | **32768 B** | HIGH | V62 | V61 | V0 | `0x47314451UL` ("G1DQ") | `TRAM2_G1` |
| 6 | `TRAM2_G2` | **260919.2** | DS3231 | `0x57` | 4096 B | HIGH | V58 | V59 | V0 | `0x47324451UL` ("G2DQ") | `TRAM2_G2` |
| 7 | `TRAM2_G3` | **260919.2** | DS3231 | `0x57` | 4096 B | HIGH | V60 | V63 | V0 | `0x47334451UL` ("G3DQ") | `TRAM2_G3` |
| 8 | `TRAM2BPT` | **260919.2** | DS1307 | `0x50` | 4096 B | **LOW** | V24 | V25 | V0 | `0x32424451UL` ("2BDQ") | `TRAM2BPT` |
| 9 | `TRAM3` | **260919.2** | DS1307 | `0x50` | 4096 B | **LOW** | V23 | V24 | V0 | `0x54334451UL` ("T3DQ") | `TRAM3` |
| 10 | `TRAM3BPT` | **260919.2** | DS1307 | `0x50` | 4096 B | **LOW** | V29 | V31 | V0 | `0x33424451UL` ("3BDQ") | `TRAM3BPT` |
| 11 | `TRAM4` | **260919.2** | DS3231 | `0x57` | 4096 B | HIGH | V37 | V35 | V0 | `0x54344451UL` ("T4DQ") | `TRAM4` |
| 12 | `TRAMBHD` | **260919.2** | DS3231 | `0x57` | 4096 B | **LOW** | V32 | V33 | V0 | `0x42484451UL` ("BHDQ") | `TRAMBHD` |

> [!NOTE]
> Mọi thông tin xác thực nhạy cảm (Blynk Template ID, Auth Token, Main Token, mật khẩu Wi-Fi) của toàn bộ 12 trạm (kể cả `TRAM1` đã được Codex điền) đều được che giấu trong báo cáo này và bảo toàn nguyên vẹn trong file mã nguồn.

---

## 3. Các Điểm Kỹ Thuật Trọng Tâm Vòng 2

### 3.1. Chuẩn Hóa Khai Báo `PIN_TERMINAL`
- Trong vòng 1, `volume_reader_core.h` có đoạn mã `#ifndef PIN_TERMINAL \n constexpr const char *PIN_TERMINAL = "V0"; \n #endif`. Tuy nhiên vì trong `main.cpp`, `PIN_TERMINAL` được khai báo dưới dạng biến C++ `const char *PIN_TERMINAL = "V0";` chứ không phải tiền xử lý macro, nên điều kiện `#ifndef` vẫn trả về true, dẫn đến khai báo trùng lặp identifier.
- Đã sửa triệt để: bỏ hoàn toàn định nghĩa default trong header dùng chung; `volume_reader_core.h` chỉ sử dụng `extern const char *PIN_TERMINAL;`. Mỗi file `main.cpp` tự định nghĩa duy nhất một lần `const char *PIN_TERMINAL = "V0";` đồng bộ như `PIN_LIVE` và `PIN_DAILY`.

### 3.2. Cơ Chế OTA Sạch Toàn Diện (Clean OTA Boot Mode)
Áp dụng cơ chế OTA đã được thử nghiệm ổn định từ `TRAM2_G1` vào lõi chung cho cả 12 trạm:
1. **Lưu trạng thái trong ESP RTC User Memory**:
   - Vị trí: Offset 32 words (= 128 bytes), tránh vùng 128 byte đầu tiên do bootloader eboot sử dụng khi flash image.
   - Cấu trúc `OtaRtcState` gồm `magic` (`0x4F544131UL`), `phase`, `error`, và `check` (checksum XOR bảo vệ).
   - Các pha trạng thái: `OTA_RTC_NONE (0)`, `OTA_RTC_REQUESTED (1)`, `OTA_RTC_RUNNING (2)`, `OTA_RTC_SUCCESS (3)`, `OTA_RTC_FAILED (4)`.
2. **Quy trình khi nhận lệnh `update` (qua Blynk Terminal V0)**:
   - Lưu trạng thái lưu lượng hiện tại xuống EEPROM (`persistState()`).
   - Phản hồi thiết bị: `"Da nhan lenh OTA; ESP se khoi dong vao che do cap nhat sach"`.
   - Ngắt kết nối API client (`apiClient.stop()`).
   - Ghi trạng thái `OTA_RTC_REQUESTED` vào RTC memory rồi gọi `ESP.restart()`.
3. **Quá trình khởi động vào chế độ OTA sạch (`runCleanOtaMode`)**:
   - Được kiểm tra ngay đầu hàm `setup()` thông qua `handleOtaBootState()`. Nếu phát hiện `OTA_RTC_REQUESTED`, ESP chuyển sang pha `OTA_RTC_RUNNING` và chạy môi trường OTA tinh gọn tuyệt đối:
     - `WiFi.persistent(false)`
     - `WiFi.mode(WIFI_STA)`
     - `WiFi.setSleepMode(WIFI_NONE_SLEEP)`
     - Chờ Wi-Fi tối đa 45 giây có feed watchdog liên tục (`ESP.wdtFeed()`). Nếu timeout, ghi nhận lỗi `OTA_ERROR_WIFI_TIMEOUT (-1001)` và restart về firmware cũ.
     - Tải và nạp firmware qua HTTPS `ESPhttpUpdate.update(updateClient, URL_fw_Bin)`.
     - Không khởi động Blynk, không bật timer, không chạy ngắt xung và không bật I2C EEPROM, giải phóng tối đa bộ nhớ RAM (Free Heap) cho quá trình nạp SSL/TLS.
     - Tiến độ tải firmware được log ra Serial theo **bucket 10%** (0%, 10%, 20%, ..., 100%) kèm `ESP.wdtFeed()`, không spam log từng byte.
     - **Chốt trạng thái `OTA_RTC_SUCCESS` ngay trong callback `ESPhttpUpdate.onEnd(...)`**: Do ESP8266 có thể tự động khởi động lại ngay khi kết thúc quá trình nạp flash trước khi `ESPhttpUpdate.update()` trả về, `writeOtaRtcState(OTA_RTC_SUCCESS, 0)` được gọi trực tiếp trong callback `onEnd`. Điều này đảm bảo trạng thái thành công được lưu chắc chắn vào RTC memory trước khi chip restart (tránh bị kẹt ở `RUNNING` và boot sau báo sai `interrupted/reset`). Nhánh `HTTP_UPDATE_OK` sau lệnh `update()` vẫn ghi lại để đảm bảo tính idempotent; nếu thất bại ghi nhận `OTA_RTC_FAILED(error)`.
4. **Hậu kiểm sau khi khởi động lại**:
   - ESP khởi động lại vào firmware mới, đọc RTC memory: ghi nhận trạng thái vào biến `lastOtaStatus` (`"success"`, `"failed (code)"` hoặc `"interrupted/reset"`), in ra Serial và xóa vùng nhớ RTC state.
   - Khi kỹ thuật viên gửi lệnh `i2c`, phản hồi trả về bao gồm:
     - Phiên bản firmware (`BLYNK_FIRMWARE_VERSION`)
     - Nguyên nhân reset (`ESP.getResetReason()`)
     - Trạng thái OTA gần nhất (`lastOtaStatus`)
     - Thông số RAM: Free Heap và Max Free Block Size
     - Kết quả quét I2C bus
     - Thống kê xung: Accepted, Rejected, Glitches Ignored
     - Cường độ sóng Wi-Fi (dBm).

### 3.3. Chính Sách An Toàn Lỗi Đọc EEPROM Chặt Chẽ
1. **Trong hàm `loadState()`**:
   - Duyệt qua toàn bộ 64 slot trạng thái trong EEPROM.
   - Nếu **bất kỳ slot nào** trả về số byte đọc không đủ (`bytesRead != sizeof(candidate)` - lỗi short read / lỗi I2C bus), hệ thống đánh dấu `readErrorDetected = true`.
   - Khi có lỗi đọc, hàm lập tức từ chối khởi tạo, đặt `storageReady = false` và trả về `false`, **tuyệt đối không ghi đè hay zero hóa EEPROM**, kể cả khi các slot khác có dữ liệu hợp lệ (bảo vệ nguy cơ slot bị lỗi I2C tạm thời lại chính là slot chứa generation mới nhất).
2. **Trong hàm `validateQueuedRecords()`**:
   - Phân biệt rõ giữa **lỗi I2C/short read** (`RECORD_READ_IO_ERROR`) và **record đọc đủ nhưng sai CRC/flags** (`RECORD_READ_CRC_INVALID`).
   - Nếu gặp `RECORD_READ_IO_ERROR`: **không được cắt ngắn hàng đợi, không ghi metadata persist**, vô hiệu hóa quyền ghi storage cho phiên boot đó (`storageReady = false`) và cảnh báo ra Serial.
   - Chỉ khi record được đọc đủ số byte nhưng kiểm tra CRC thất bại (`RECORD_READ_CRC_INVALID`), hệ thống mới trim hàng đợi về tiền tố hợp lệ (`validCount`).
   - Nếu quá trình lưu metadata trim thất bại (`!persistState()`), RAM lập tức khôi phục lại metadata ban đầu (`state = beforeTrim`).

### 3.4. Khắc Phục Lỗi PlatformIO Library Dependency Finder (LDF)
- **Nguyên nhân**: Khi thực hiện biên dịch thật, PlatformIO LDF chỉ quét dependency trực tiếp từ thư mục `src/` của từng dự án (`src/main.cpp`). Do các thư viện phụ thuộc (`BlynkSimpleEsp8266.h`, `I2C_eeprom.h`, `RTClib.h`, `TimeLib.h`, `UrlEncode.h`) trước đây chỉ được `#include` bên trong `../../shared/volume_reader_core.h` (nằm ngoài project source tree), LDF báo `No dependencies` và compiler báo lỗi `BlynkSimpleEsp8266.h: No such file or directory` dù `lib_extra_dirs` đã có.
- **Giải pháp xử lý**:
  1. **Direct Dependency Headers Prelude**: Thêm danh sách đầy đủ các header thư viện trực tiếp vào `src/main.cpp` để LDF quét thấy ngay lập tức từ `main.cpp` và nạp toàn bộ include paths của các thư viện trong `PIO/lib` vào lệnh compile xtensa-gcc.
  2. **Chuẩn hóa `lib_extra_dirs = ../../lib`**: Chuyển toàn bộ đường dẫn tuyệt đối hoặc bổ sung dòng thiếu trong 12 file `platformio.ini` thành đường dẫn tương đối nhất quán `../../lib`, đảm bảo tính khả chuyển khi build ở bất kỳ máy nào.

### 3.5. Sửa Thứ Tự Include và Chữ Ký RTC Write Trong Build Thật Vòng Hai
- **Sửa thứ tự `#include <Arduino.h>` trước các `constexpr`**:
  - Khi biên dịch thật, các khai báo cấu hình phần cứng dùng `constexpr uint8_t/uint32_t`, `D6`, `LOW/HIGH` yêu cầu các kiểu dữ liệu và macro từ Arduino core.
  - Đã chuẩn hóa vị trí include trong cả 12 file `src/main.cpp`: đặt `#include <Arduino.h>` và các direct dependency includes ngay sau các macro Blynk bắt buộc (`BLYNK_TEMPLATE_ID`, `BLYNK_AUTH_TOKEN`, `BLYNK_PRINT Serial`, `APP_DEBUG`) và trước mọi biến `ssid`, token và `constexpr` cấu hình. Tránh include lặp lại không cần thiết ở cuối file.
- **Sửa chữ ký `ESP.rtcUserMemoryWrite` trong shared core**:
  - Phương thức `ESP.rtcUserMemoryWrite` trong ESP8266 Arduino Core nhận tham số buffer là `uint32_t *` (non-const pointer).
  - Hai lời gọi trong `writeOtaRtcState` và `clearOtaRtcState` trước đó dùng `reinterpret_cast<const uint32_t *>` dẫn đến lỗi incompatible pointer type khi biên dịch g++.
  - Đã chuyển cả hai lời gọi sang `reinterpret_cast<uint32_t *>(&otaState)` tương tự bản `TRAM2_G1` đã build ổn định.

---

## 4. Kết Quả Kiểm Tra Độc Lập Của Codex

### 4.1. Kiểm Tra Tĩnh
```powershell
python tools/verify_volume_sync.py
```
**Kết quả**: `380/380` tiêu chí đạt, không có lỗi.

**Nội dung kiểm tra tự động**:
- Khẳng định lõi chung `shared/volume_reader_core.h` đầy đủ các marker OTA sạch, callback `onEnd` chốt `OTA_RTC_SUCCESS`, bucket 10%, không có định nghĩa trùng `PIN_TERMINAL`, có chính sách EEPROM strict error handling.
- Kiểm tra `ESP.rtcUserMemoryWrite` trong lõi chung không còn `reinterpret_cast<const uint32_t *>`.
- Kiểm tra toàn bộ 12 file `platformio.ini` đều có cấu hình `lib_extra_dirs = ../../lib`.
- Kiểm tra toàn bộ 12 file `src/main.cpp` đều có `#include <Arduino.h>` và các dependency headers đứng trước mọi khai báo `constexpr` và trước shared core.
- Kiểm tra toàn bộ 12 trạm đều đạt phiên bản `260919.2`.
- Kiểm tra không còn bất kỳ chuỗi `__CODEX_SET_` nào tồn tại trong cả 12 file mã nguồn (xác nhận `TRAM1` đã hoàn tất cấu hình).
- Xác minh tính chính xác của ma trận phần cứng: RTC, EEPROM địa chỉ/dung lượng, active level, pin live/daily/terminal, state magic, OTA URL đúng từng thư mục.
- Kiểm tra không có `terminal.println` / `WidgetTerminal` và không có logic `reboot_num` restart do mất mạng.

### 4.2. Biên Dịch PlatformIO

**Kết quả**: `12/12` dự án biên dịch thành công cho `nodemcuv2`:

- `TRAM1`, `TRAM_CC_G1`, `TRAM_CC_G2`, `TRAM_CC_G3`
- `TRAM2_G1`, `TRAM2_G2`, `TRAM2_G3`, `TRAM2BPT`
- `TRAM3`, `TRAM3BPT`, `TRAM4`, `TRAMBHD`

Kích thước các tệp `firmware.bin` từ 440.720 đến 440.864 byte; không dự án nào vượt giới hạn RAM hoặc Flash. Trường hợp `TRAM_CC_G3` từng gặp lỗi khóa tệp tạm của OneDrive sau khi tạo nhị phân; biên dịch lại riêng dự án đã thành công hoàn toàn.

---

## 5. Lưu Ý An Toàn và Xác Nhận Phần Cứng

> [!WARNING]
> **Xác nhận mức xung Active Level tại bàn thử (Bench Test) trước khi nạp thiết bị**:
> Bốn mạch mới gồm:
> - `TRAM1`
> - `TRAM2BPT`
> - `TRAM3BPT`
> - `TRAMBHD`
> 
> Hiện tại đang được thiết lập mức tích cực **`active-LOW`** trên chân D6 (`PULSE_ACTIVE_LEVEL = LOW`).
> Kỹ thuật viên / Codex **cần kiểm tra dạng sóng tín hiệu đầu ra của optocoupler (PC817 hoặc tương đương) trên mạch thực tế tại bàn thử**:
> - Nếu khi có xung lưu lượng, tín hiệu kéo về 0V (GND) -> Giữ nguyên `active-LOW`.
> - Nếu khi có xung lưu lượng, tín hiệu kéo lên 3.3V (VCC) -> Đổi thành `PULSE_ACTIVE_LEVEL = HIGH`.

> [!IMPORTANT]
> Toàn bộ các bước biên dịch nhị phân PlatformIO và kiểm tra tính năng OTA thực địa đang chờ Codex thực hiện độc lập theo các lệnh trên. Báo cáo này không xác nhận trạng thái hoạt động thực địa.
