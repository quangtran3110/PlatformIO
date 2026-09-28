# OTA Control Center

Ứng dụng web chạy cục bộ để quản lý build, phát hành Cloudflare và OTA cho các dự án ESP trong `D:\AI_PROJECTS\PIO`.

## Cách mở

Nhấp đúp:

```text
START_OTA_MANAGER.cmd
```

Ứng dụng tự chạy ẩn và mở trình duyệt tại:

```text
http://127.0.0.1:8765
```

Ứng dụng chỉ lắng nghe trên máy tính này, không mở ra mạng LAN hoặc Internet.

## Luồng sử dụng hằng ngày

1. Chọn trạm ở cột bên trái.
2. Kiểm tra version mã nguồn và trạng thái thiết bị.
3. Bấm **Build firmware**.
4. Khi build thành công, bấm **Phát hành**.
5. Ứng dụng build lại, upload Cloudflare và tải ngược để so hash.
6. Chỉ khi file khớp và thiết bị Online, nút **Cập nhật OTA** mới được mở.
7. OTA yêu cầu đánh dấu xác nhận và nhập đúng tên trạm.
8. Sau khi gửi OTA, ứng dụng chờ thiết bị kết nối lại, hỏi phiên bản qua chân Terminal và chỉ báo hoàn tất khi phiên bản khớp.

## Ý nghĩa trạng thái

- **Mã nguồn:** version đọc từ `BLYNK_FIRMWARE_VERSION`.
- **Firmware trên máy:** file `.pio\build\<environment>\firmware.bin`.
- **Đã phát hành:** manifest gần nhất do script phát hành tạo.
- **Thiết bị:** phiên bản firmware vừa được thiết bị trả lời qua Terminal.
- **Cloudflare đã đăng nhập:** máy đang có phiên OAuth do Wrangler lưu. Khi phát hành, Cloudflare sẽ xác thực lại phiên này.

## Thêm dự án

Bấm dấu **+** cạnh “Danh sách trạm”, sau đó nhập:

- Tên trạm.
- Mã trạm dạng `tram-tan-an`.
- Đường dẫn dự án trong `D:\AI_PROJECTS\PIO`.
- PlatformIO environment, thông thường là `nodemcuv2`.
- Chân Terminal đang nhận lệnh của dự án, ví dụ `V5`, `V10` hoặc `V12`.
- Đường dẫn tương đối đến script phát hành nếu trạm đã có OTA Private.

Dự án mới có thể dùng chức năng Build ngay. Nút Phát hành chỉ được mở khi dự án có URL OTA và script phát hành hợp lệ.

## Bảo mật

- Giao diện không hiển thị Blynk token hoặc khóa OTA.
- Key trong URL firmware chỉ được backend đọc khi xác minh file.
- Tệp cấu hình giao diện không lưu khóa OTA.
- Thông tin đăng nhập Cloudflare do Wrangler quản lý trong thư mục cục bộ đã cấu hình và không đưa lên Git.
- Các API thay đổi trạng thái chỉ chấp nhận yêu cầu từ giao diện cục bộ.
- OTA yêu cầu xác nhận hai lớp và kiểm tra lại hash ngay trước khi gửi lệnh.
- Mỗi lần hỏi phiên bản dùng một mã ngẫu nhiên; phản hồi cũ trên Terminal không được chấp nhận.

## Các tệp chính

```text
OTA_MANAGER/
├── app.py                       Backend cục bộ
├── START_OTA_MANAGER.cmd        Mở ứng dụng bằng nhấp đúp
├── start-ota-manager.ps1        Trình khởi động ẩn
├── config/
│   ├── settings.json            Đường dẫn công cụ trên máy
│   └── stations.json            Danh sách trạm, không chứa secret
├── data/                         Lịch sử và log cục bộ, Git bỏ qua
└── web/                          Giao diện người dùng
```

## Nguyên tắc an toàn

- Build không tác động thiết bị.
- Phát hành chỉ cập nhật kho firmware; chưa OTA.
- OTA là bước riêng và luôn cần xác nhận.
- Nếu binary tải ngược khác file trên máy dù một byte, ứng dụng dừng trước OTA.
- Không phát hành lại một version cũ bằng binary khác; phải tăng version.
- Không gửi OTA khi thiết bị Offline.
- Không báo OTA thành công nếu thiết bị chưa kết nối lại và trả về đúng phiên bản mục tiêu.

## Giao thức Terminal dùng chung

Không cần tạo thêm datastream chỉ để báo phiên bản. OTA Manager gửi lệnh chỉ đọc vào chân Terminal đã có:

```text
ota_info:a1b2c3d4e5f6
```

Firmware trả lời trên cùng chân bằng `Blynk.virtualWrite()`:

```text
ota_reply:a1b2c3d4e5f6|version=260928.1
```

Mã 12 ký tự thay đổi trong mỗi lần kiểm tra. Lệnh này không ghi EEPROM, không điều khiển relay và không khởi động lại thiết bị. Trong giai đoạn chuyển tiếp, thiết bị đang chạy firmware cũ có thể chỉ hiện phiên bản nghiệm thu gần nhất cho đến lần OTA đầu tiên có giao thức mới.
