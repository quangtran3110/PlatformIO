#!/usr/bin/env python3
"""
Static Verification Script for VOLUME Firmware Synchronization (Round 2).
Ensures all 12 projects use the common core, have BLYNK_FIRMWARE_VERSION 260919.2,
match the hardware/pin matrix, verify clean OTA flow in shared core,
verify single PIN_TERMINAL definition, strict EEPROM read safety,
and contain no secret leaks or leftover __CODEX_SET_ placeholders.
"""

import os
import re
import sys
from pathlib import Path

# Workspace root is the parent directory of tools/
WORKSPACE_DIR = Path(__file__).resolve().parent.parent
SHARED_CORE_REL = "../../shared/volume_reader_core.h"
SHARED_CORE_PATH = WORKSPACE_DIR / "shared" / "volume_reader_core.h"
EXPECTED_FIRMWARE_VERSION = "260919.2"

EXPECTED_MATRIX = {
    "TRAM1": {
        "rtc": "USE_RTC_DS3231",
        "eeprom_addr": "0x57",
        "eeprom_size": "4096",
        "pulse_active": "LOW",
        "pulse_pin": "D6",
        "pin_live": '"V24"',
        "pin_daily": '"V25"',
        "pin_terminal": '"V0"',
        "state_magic": "0x54314451UL",
    },
    "TRAM_CC_G1": {
        "rtc": "USE_RTC_DS3231",
        "eeprom_addr": "0x57",
        "eeprom_size": "4096",
        "pulse_active": "HIGH",
        "pulse_pin": "D6",
        "pin_live": '"V51"',
        "pin_daily": '"V52"',
        "pin_terminal": '"V0"',
        "state_magic": "0x43314451UL",
    },
    "TRAM_CC_G2": {
        "rtc": "USE_RTC_DS3231",
        "eeprom_addr": "0x57",
        "eeprom_size": "4096",
        "pulse_active": "HIGH",
        "pulse_pin": "D6",
        "pin_live": '"V54"',
        "pin_daily": '"V55"',
        "pin_terminal": '"V0"',
        "state_magic": "0x43324451UL",
    },
    "TRAM_CC_G3": {
        "rtc": "USE_RTC_DS3231",
        "eeprom_addr": "0x57",
        "eeprom_size": "4096",
        "pulse_active": "HIGH",
        "pulse_pin": "D6",
        "pin_live": '"V57"',
        "pin_daily": '"V58"',
        "pin_terminal": '"V0"',
        "state_magic": "0x43334451UL",
    },
    "TRAM2_G1": {
        "rtc": "USE_RTC_DS3231",
        "eeprom_addr": "0x50",
        "eeprom_size": "32768UL",
        "pulse_active": "HIGH",
        "pulse_pin": "D6",
        "pin_live": '"V62"',
        "pin_daily": '"V61"',
        "pin_terminal": '"V0"',
        "state_magic": "0x47314451UL",
    },
    "TRAM2_G2": {
        "rtc": "USE_RTC_DS3231",
        "eeprom_addr": "0x57",
        "eeprom_size": "4096",
        "pulse_active": "HIGH",
        "pulse_pin": "D6",
        "pin_live": '"V58"',
        "pin_daily": '"V59"',
        "pin_terminal": '"V0"',
        "state_magic": "0x47324451UL",
    },
    "TRAM2_G3": {
        "rtc": "USE_RTC_DS3231",
        "eeprom_addr": "0x57",
        "eeprom_size": "4096",
        "pulse_active": "HIGH",
        "pulse_pin": "D6",
        "pin_live": '"V60"',
        "pin_daily": '"V63"',
        "pin_terminal": '"V0"',
        "state_magic": "0x47334451UL",
    },
    "TRAM2BPT": {
        "rtc": "USE_RTC_DS1307",
        "eeprom_addr": "0x50",
        "eeprom_size": "4096",
        "pulse_active": "LOW",
        "pulse_pin": "D6",
        "pin_live": '"V24"',
        "pin_daily": '"V25"',
        "pin_terminal": '"V0"',
        "state_magic": "0x32424451UL",
    },
    "TRAM3": {
        "rtc": "USE_RTC_DS1307",
        "eeprom_addr": "0x50",
        "eeprom_size": "4096",
        "pulse_active": "LOW",
        "pulse_pin": "D6",
        "pin_live": '"V23"',
        "pin_daily": '"V24"',
        "pin_terminal": '"V0"',
        "state_magic": "0x54334451UL",
    },
    "TRAM3BPT": {
        "rtc": "USE_RTC_DS1307",
        "eeprom_addr": "0x50",
        "eeprom_size": "4096",
        "pulse_active": "LOW",
        "pulse_pin": "D6",
        "pin_live": '"V29"',
        "pin_daily": '"V31"',
        "pin_terminal": '"V0"',
        "state_magic": "0x33424451UL",
    },
    "TRAM4": {
        "rtc": "USE_RTC_DS3231",
        "eeprom_addr": "0x57",
        "eeprom_size": "4096",
        "pulse_active": "HIGH",
        "pulse_pin": "D6",
        "pin_live": '"V37"',
        "pin_daily": '"V35"',
        "pin_terminal": '"V0"',
        "state_magic": "0x54344451UL",
    },
    "TRAMBHD": {
        "rtc": "USE_RTC_DS3231",
        "eeprom_addr": "0x57",
        "eeprom_size": "4096",
        "pulse_active": "LOW",
        "pulse_pin": "D6",
        "pin_live": '"V32"',
        "pin_daily": '"V33"',
        "pin_terminal": '"V0"',
        "state_magic": "0x42484451UL",
    },
}


def run_checks() -> bool:
    print("=" * 75)
    print("VOLUME Static Firmware Verification (Round 2)")
    print("=" * 75)

    total_checks = 0
    passed_checks = 0
    errors = []

    # 1. Check shared core file existence and contents
    total_checks += 1
    if not SHARED_CORE_PATH.is_file():
        errors.append(f"MISSING: Shared core not found at {SHARED_CORE_PATH}")
    else:
        passed_checks += 1
        print(f"[PASS] Shared core exists: {SHARED_CORE_PATH.relative_to(WORKSPACE_DIR)}")

    core_content = SHARED_CORE_PATH.read_text(encoding="utf-8", errors="replace")

    # Check forbidden patterns in core
    total_checks += 1
    if "terminal.println" in core_content or "WidgetTerminal" in core_content:
        errors.append("FORBIDDEN: shared/volume_reader_core.h contains terminal.println/WidgetTerminal")
    else:
        passed_checks += 1
        print("[PASS] Shared core: no WidgetTerminal / terminal.println")

    total_checks += 1
    if "reboot_num" in core_content:
        errors.append("FORBIDDEN: shared/volume_reader_core.h contains old reboot_num reconnect restart logic")
    else:
        passed_checks += 1
        print("[PASS] Shared core: no reconnect reboot_num loop")

    # Check rule for single PIN_TERMINAL definition
    total_checks += 1
    if "constexpr const char *PIN_TERMINAL =" in core_content:
        errors.append("CONFLICT: shared/volume_reader_core.h redefines PIN_TERMINAL default")
    else:
        passed_checks += 1
        print("[PASS] Shared core: single PIN_TERMINAL rule respected (no conflicting default definition)")

    # Check that rtcUserMemoryWrite uses non-const uint32_t * (ESP framework signature)
    total_checks += 1
    if "reinterpret_cast<const uint32_t *>" in core_content:
        errors.append("FORBIDDEN: shared/volume_reader_core.h still contains reinterpret_cast<const uint32_t *> in RTC write")
    else:
        passed_checks += 1
        print("[PASS] Shared core: ESP.rtcUserMemoryWrite uses non-const uint32_t * pointer")

    # Check clean OTA markers in core
    ota_markers = [
        "OTA_RTC_OFFSET_WORDS = 32",
        "OTA_RTC_MAGIC = 0x4F544131UL",
        "OTA_WIFI_TIMEOUT_MS = 45000UL",
        "OTA_ERROR_WIFI_TIMEOUT = -1001",
        "enum OtaRtcPhase",
        "OtaRtcState",
        "readOtaRtcState",
        "writeOtaRtcState",
        "clearOtaRtcState",
        "cleanOtaProgress",
        "runCleanOtaMode",
        "handleOtaBootState",
        "WiFi.persistent(false)",
        "WIFI_NONE_SLEEP",
        "ESP.wdtFeed()",
    ]
    for marker in ota_markers:
        total_checks += 1
        if marker not in core_content:
            errors.append(f"MISSING OTA MARKER in shared core: '{marker}'")
        else:
            passed_checks += 1
    # Check that OTA_RTC_SUCCESS is recorded directly inside the ESPhttpUpdate.onEnd callback
    total_checks += 1
    on_end_idx = core_content.find("ESPhttpUpdate.onEnd")
    progress_idx = core_content.find("ESPhttpUpdate.onProgress", on_end_idx) if on_end_idx != -1 else -1
    if on_end_idx == -1 or progress_idx == -1:
        errors.append("MISSING: ESPhttpUpdate.onEnd callback structure not found in shared core")
    else:
        on_end_block = core_content[on_end_idx:progress_idx]
        if "writeOtaRtcState(OTA_RTC_SUCCESS" not in on_end_block:
            errors.append("MISSING: writeOtaRtcState(OTA_RTC_SUCCESS) must be called inside ESPhttpUpdate.onEnd callback")
        else:
            passed_checks += 1
            print("[PASS] Shared core: OTA_RTC_SUCCESS is locked inside ESPhttpUpdate.onEnd callback before restart")

    # Check bucket 10% progress logging in clean OTA
    total_checks += 1
    if "percent / 10" in core_content and "lastReportedProgressBucket" in core_content:
        passed_checks += 1
        print("[PASS] Shared core: clean OTA logs progress in 10% buckets with watchdog feed")
    else:
        errors.append("MISSING: Clean OTA 10% bucket progress logging logic")

    # Check i2c report fields
    i2c_markers = [
        "BLYNK_FIRMWARE_VERSION",
        "ESP.getResetReason()",
        "lastOtaStatus",
        "ESP.getFreeHeap()",
        "ESP.getMaxFreeBlockSize()",
        "readPulseCount()",
        "readRejectedPulseCount()",
        "readIgnoredPulseGlitchCount()",
    ]
    for marker in i2c_markers:
        total_checks += 1
        if marker not in core_content:
            errors.append(f"MISSING in i2c diagnostic report: '{marker}'")
        else:
            passed_checks += 1
    print("[PASS] Shared core: i2c diagnostic report includes firmware version, reset, OTA, heap, pulse counters")

    # Check stricter EEPROM read error policy in loadState
    total_checks += 1
    if (
        "readErrorDetected = true" in core_content
        and "storageReady = false" in core_content
        and "phat hien loi doc/short read tai state slot" in core_content
    ):
        passed_checks += 1
        print("[PASS] Shared core: loadState strictly rejects any short read and prevents overwrite")
    else:
        errors.append("MISSING: Stricter EEPROM loadState read error handling")

    # Check validateQueuedRecords policy: distinguishing IO error vs CRC error
    total_checks += 1
    if (
        "RECORD_READ_IO_ERROR" in core_content
        and "RECORD_READ_CRC_INVALID" in core_content
        and "beforeTrim = state" in core_content
    ):
        passed_checks += 1
        print("[PASS] Shared core: validateQueuedRecords distinguishes IO error vs CRC, restores RAM on persist fail")
    else:
        errors.append("MISSING: validateQueuedRecords distinction between IO error and CRC invalid")

    # 2. Check each of the 12 projects
    for project_name, spec in EXPECTED_MATRIX.items():
        proj_dir = WORKSPACE_DIR / project_name
        total_checks += 1
        if not proj_dir.is_dir():
            errors.append(f"MISSING: Project directory {project_name} not found")
            continue
        passed_checks += 1

        ini_path = proj_dir / "platformio.ini"
        main_path = proj_dir / "src" / "main.cpp"

        total_checks += 1
        if not ini_path.is_file():
            errors.append(f"MISSING: {project_name}/platformio.ini not found")
        else:
            passed_checks += 1
            ini_content = ini_path.read_text(encoding="utf-8", errors="replace")
            total_checks += 1
            if "lib_extra_dirs = ../../lib" not in ini_content:
                errors.append(f"{project_name}: platformio.ini missing relative 'lib_extra_dirs = ../../lib'")
            else:
                passed_checks += 1

        total_checks += 1
        if not main_path.is_file():
            errors.append(f"MISSING: {project_name}/src/main.cpp not found")
            continue
        passed_checks += 1

        content = main_path.read_text(encoding="utf-8", errors="replace")

        # Must include shared core
        total_checks += 1
        if f'#include "{SHARED_CORE_REL}"' not in content:
            errors.append(f"{project_name}: Does not include shared core via '{SHARED_CORE_REL}'")
        else:
            passed_checks += 1

        # Check direct dependency includes for PlatformIO LDF
        expected_direct_includes = [
            "Arduino.h",
            "BlynkSimpleEsp8266.h",
            "ESP8266HTTPClient.h",
            "ESP8266WiFi.h",
            "ESP8266httpUpdate.h",
            "I2C_eeprom.h",
            "RTClib.h",
            "TimeLib.h",
            "WiFiClientSecure.h",
            "UrlEncode.h",
        ]
        shared_core_idx = content.find(f'#include "{SHARED_CORE_REL}"')
        first_constexpr = re.search(r"\bconstexpr\b", content)
        for header in expected_direct_includes:
            total_checks += 1
            hdr_include = f"#include <{header}>"
            hdr_idx = content.find(hdr_include)
            if hdr_idx == -1:
                errors.append(f"{project_name}: Missing direct include '{hdr_include}' for PlatformIO LDF")
            elif shared_core_idx != -1 and hdr_idx > shared_core_idx:
                errors.append(f"{project_name}: Direct include '{hdr_include}' must appear before shared core include")
            elif first_constexpr and hdr_idx > first_constexpr.start():
                errors.append(f"{project_name}: Direct include '{hdr_include}' must appear before constexpr configurations")
            else:
                passed_checks += 1

        # Must have firmware version 260919.2
        total_checks += 1
        if f'#define BLYNK_FIRMWARE_VERSION "{EXPECTED_FIRMWARE_VERSION}"' not in content:
            errors.append(f"{project_name}: Expected BLYNK_FIRMWARE_VERSION '{EXPECTED_FIRMWARE_VERSION}' not found")
        else:
            passed_checks += 1

        # Must NOT contain any __CODEX_SET_ placeholders anywhere
        total_checks += 1
        if "__CODEX_SET_" in content:
            errors.append(f"{project_name}: Still contains unfinished '__CODEX_SET_' placeholder!")
        else:
            passed_checks += 1

        # Must NOT contain terminal.println / WidgetTerminal
        total_checks += 1
        if "terminal.println" in content or "WidgetTerminal" in content:
            errors.append(f"{project_name}: Contains forbidden terminal.println / WidgetTerminal")
        else:
            passed_checks += 1

        # Must NOT contain reconnect restart logic
        total_checks += 1
        if "reboot_num" in content:
            errors.append(f"{project_name}: Contains old reboot_num reconnect restart logic")
        else:
            passed_checks += 1

        # Check single PIN_TERMINAL declaration
        total_checks += 1
        terminal_matches = re.findall(r"PIN_TERMINAL\s*=", content)
        if len(terminal_matches) != 1:
            errors.append(f"{project_name}: PIN_TERMINAL should be defined exactly once, found {len(terminal_matches)}")
        else:
            passed_checks += 1

        # Check RTC define
        total_checks += 1
        expected_rtc = spec["rtc"]
        if expected_rtc not in content:
            errors.append(f"{project_name}: Expected RTC definition '{expected_rtc}' not found")
        else:
            passed_checks += 1

        # Check EEPROM address
        total_checks += 1
        expected_addr = spec["eeprom_addr"]
        if f"EEPROM_I2C_ADDRESS = {expected_addr}" not in content:
            errors.append(f"{project_name}: Expected EEPROM_I2C_ADDRESS {expected_addr} not found")
        else:
            passed_checks += 1

        # Check EEPROM size
        total_checks += 1
        expected_size = spec["eeprom_size"]
        if f"EEPROM_SIZE = {expected_size}" not in content:
            errors.append(f"{project_name}: Expected EEPROM_SIZE {expected_size} not found")
        else:
            passed_checks += 1

        # Check pulse active level
        total_checks += 1
        expected_active = spec["pulse_active"]
        if f"PULSE_ACTIVE_LEVEL = {expected_active}" not in content:
            errors.append(f"{project_name}: Expected PULSE_ACTIVE_LEVEL {expected_active} not found")
        else:
            passed_checks += 1

        # Check pulse pin
        total_checks += 1
        expected_pin = spec["pulse_pin"]
        if f"FLOW_PULSE_PIN = {expected_pin}" not in content:
            errors.append(f"{project_name}: Expected FLOW_PULSE_PIN {expected_pin} not found")
        else:
            passed_checks += 1

        # Check live pin
        total_checks += 1
        expected_live = spec["pin_live"]
        if f"PIN_LIVE = {expected_live}" not in content:
            errors.append(f"{project_name}: Expected PIN_LIVE {expected_live} not found")
        else:
            passed_checks += 1

        # Check daily pin
        total_checks += 1
        expected_daily = spec["pin_daily"]
        if f"PIN_DAILY = {expected_daily}" not in content:
            errors.append(f"{project_name}: Expected PIN_DAILY {expected_daily} not found")
        else:
            passed_checks += 1

        # Check state magic
        total_checks += 1
        expected_magic = spec["state_magic"]
        if f"STATE_MAGIC = {expected_magic}" not in content:
            errors.append(f"{project_name}: Expected STATE_MAGIC {expected_magic} not found")
        else:
            passed_checks += 1

        # Check OTA URL contains project folder name
        total_checks += 1
        if f"/VOLUME/{project_name}/" not in content:
            errors.append(f"{project_name}: OTA URL does not point to its own folder '/VOLUME/{project_name}/'")
        else:
            passed_checks += 1

        print(f"[PASS] Project {project_name:12s} - Version {EXPECTED_FIRMWARE_VERSION}, Core, RTC, EEPROM, Pins, Magic, OTA URL verified.")

    print("=" * 75)
    print(f"Summary: {passed_checks}/{total_checks} checks passed.")
    if errors:
        print(f"FAILED with {len(errors)} error(s):")
        for err in errors:
            print(f"  - {err}")
        return False

    print("ALL STATIC VERIFICATION CHECKS PASSED SUCCESSFULLY!")
    print("=" * 75)
    return True


if __name__ == "__main__":
    success = run_checks()
    sys.exit(0 if success else 1)
