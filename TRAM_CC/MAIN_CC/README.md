# Thu nghiem OTA rieng tu - Tram Cai Cat

Muc tieu cua ban thu nghiem:

- Ma nguon duoc luu trong GitHub Private.
- Nguoi lap trinh van chi sua `src/main.cpp`.
- Firmware nhi phan nam trong Cloudflare Workers KV private.
- ESP chi tai firmware qua mot Worker co khoa rieng cua tram.
- Firmware duoc kiem tra MD5 boi `ESP8266HTTPUpdate` truoc khi ghi flash.

Quy trinh phat hanh sau khi thiet lap mot lan:

1. Sua `src/main.cpp` va tang `BLYNK_FIRMWARE_VERSION`.
2. Chay `ota/release-tram-cc.ps1`.
3. Script bien dich, tinh SHA-256/MD5, tai binary len KV theo phien ban va cap nhat `latest.json` sau cung.
4. Gui lenh `update` tren V5 khi muon tram cai ban moi.

Khong dua khoa OTA vao Worker source, `wrangler.jsonc`, log hay KV. Khoa chi nam trong
Cloudflare Worker Secret va firmware cua dung tram.
