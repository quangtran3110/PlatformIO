import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

import app


class OtaManagerTests(unittest.TestCase):
    def test_resolve_cpp_string_expression_with_private_macro(self):
        expression = '"https://ota.example/tram-so-4/" TRAM_SO_4_OTA_KEY "/firmware.bin"'
        resolved = app.resolve_cpp_string_expression(
            expression,
            {"TRAM_SO_4_OTA_KEY": "private-key"},
        )

        self.assertEqual(
            resolved,
            "https://ota.example/tram-so-4/private-key/firmware.bin",
        )

    def test_parse_source_ignores_commented_legacy_token(self):
        station = app.get_station("ts2")
        parsed = app.parse_source(station)
        source = app.source_path_for(station).read_text(encoding="utf-8")
        legacy_token = "ESzia3fpA-29cs8gt85pGnrPq_rICcqf"

        self.assertIn(legacy_token, source)
        self.assertNotEqual(parsed["blynkToken"], legacy_token)
        self.assertRegex(parsed["version"], r"^\d{6,}(?:\.\d+)+$")

    def test_parse_source_hides_secrets_from_station_payload(self):
        station = app.get_station("tram-cc")
        parsed = app.parse_source(station)
        payload = app.station_payload(station)
        self.assertRegex(parsed["version"], r"^\d{6,}(?:\.\d+)+$")
        self.assertTrue(parsed["otaUrl"].startswith("https://"))
        self.assertTrue(parsed["blynkToken"])
        serialized = json.dumps(payload)
        self.assertNotIn(parsed["otaUrl"], serialized)
        self.assertNotIn(parsed["blynkToken"], serialized)

    def test_station_payload_reports_build_and_release_state(self):
        station = app.get_station("tram-cc")
        payload = app.station_payload(station)
        self.assertTrue(payload["projectExists"])
        self.assertRegex(payload["sourceVersion"], r"^\d{6,}(?:\.\d+)+$")
        self.assertGreater(payload["binary"]["size"], 0)
        expected_writes = (
            payload["binary"]["size"] + app.KV_RELEASE_CHUNK_SIZE - 1
        ) // app.KV_RELEASE_CHUNK_SIZE + 2
        self.assertEqual(payload["estimatedKvWrites"], expected_writes)
        self.assertEqual(
            payload["releaseMatchesBinary"],
            payload["binaryCurrent"]
            and payload["release"]["version"] == payload["sourceVersion"]
            and payload["release"]["sha256"].lower()
            == payload["binary"]["sha256"].lower()
            and payload["release"]["size"] == payload["binary"]["size"],
        )

    def test_terminal_version_reply_requires_matching_request_id(self):
        request_id = "a1b2c3d4e5f6"
        self.assertEqual(
            app.parse_terminal_version_reply(
                '"ota_reply:a1b2c3d4e5f6|version=260928.1"',
                request_id,
            ),
            {
                "version": "260928.1",
                "lastOtaStatus": None,
                "freeHeap": None,
                "otaMinHeap": None,
            },
        )
        self.assertIsNone(
            app.parse_terminal_version_reply(
                "ota_reply:ffffffffffff|version=260928.1",
                request_id,
            )
        )

    def test_terminal_version_reply_reads_ota_health(self):
        result = app.parse_terminal_version_reply(
            "ota_reply:a1b2c3d4e5f6|version=260929.8|last=success|heap=27744|ota_min=3536",
            "a1b2c3d4e5f6",
        )
        self.assertEqual(
            result,
            {
                "version": "260929.8",
                "lastOtaStatus": "success",
                "freeHeap": 27744,
                "otaMinHeap": 3536,
            },
        )

    def test_terminal_version_reply_rejects_invalid_ota_min_heap(self):
        self.assertIsNone(
            app.parse_terminal_version_reply(
                "ota_reply:a1b2c3d4e5f6|version=260929.8|ota_min=unknown",
                "a1b2c3d4e5f6",
            )
        )

    def test_query_device_version_uses_configured_terminal_pin(self):
        station = {"terminalPin": "V12"}
        replies = iter(["ota_info:a1b2c3d4e5f6", "ota_reply:a1b2c3d4e5f6|version=260928.1"])

        def fake_request(_station, endpoint, params):
            if endpoint == "update":
                self.assertEqual(params, {"V12": "ota_info:a1b2c3d4e5f6"})
                return ""
            return next(replies)

        with patch.object(app.uuid, "uuid4", return_value=SimpleNamespace(hex="a1b2c3d4e5f60000")):
            with patch.object(app, "blynk_request", side_effect=fake_request):
                result = app.query_device_version(station, timeout_seconds=0.1, poll_interval=0.001)

        self.assertEqual(result["version"], "260928.1")
        self.assertEqual(result["terminalPin"], "V12")
        self.assertIsNone(result["lastOtaStatus"])

    def test_ota_start_reply_requires_matching_request_id(self):
        request_id = "a1b2c3d4e5f6"
        self.assertEqual(
            app.parse_ota_start_reply(
                "ota_accept:a1b2c3d4e5f6|version=260928.2",
                request_id,
            ),
            {"status": "accepted", "version": "260928.2"},
        )
        self.assertIsNone(
            app.parse_ota_start_reply(
                "ota_accept:ffffffffffff|version=260928.2",
                request_id,
            )
        )

    def test_request_ota_start_waits_for_matching_device_ack(self):
        station = {
            "terminalPin": "V12",
            "lastKnownDeviceVersion": "260928.2",
            "otaHandshakeVersion": "260928.2",
        }
        replies = iter([
            "ota_prepare:a1b2c3d4e5f6",
            "ota_accept:a1b2c3d4e5f6|version=260928.2",
        ])
        updates = []

        def fake_request(_station, endpoint, params):
            if endpoint == "update":
                updates.append(params)
                return ""
            return next(replies)

        with patch.object(app.uuid, "uuid4", return_value=SimpleNamespace(hex="a1b2c3d4e5f60000")):
            with patch.object(app, "append_job_log"):
                with patch.object(app, "blynk_request", side_effect=fake_request):
                    with patch.object(app.time, "monotonic", side_effect=[0.0, 0.0, 0.0]):
                        with patch.object(app.time, "sleep"):
                            result = app.request_ota_start(
                                station,
                                "test-job",
                                timeout_seconds=1.0,
                                poll_interval=0.001,
                            )

        self.assertEqual(updates, [{"V12": "ota_prepare:a1b2c3d4e5f6"}])
        self.assertEqual(result["mode"], "acknowledged")
        self.assertEqual(result["deviceVersion"], "260928.2")

    def test_request_ota_start_legacy_sends_update_once(self):
        station = {
            "terminalPin": "V12",
            "lastKnownDeviceVersion": None,
            "otaHandshakeVersion": "260928.2",
        }
        with patch.object(app, "append_job_log"):
            with patch.object(app, "blynk_request", return_value="") as request:
                result = app.request_ota_start(station, "test-job")

        request.assert_called_once_with(station, "update", {"V12": "update"})
        self.assertEqual(result["mode"], "legacy")

    def test_status_does_not_send_unsupported_version_query(self):
        station = {
            "terminalPin": "V12",
            "lastKnownDeviceVersion": None,
            "otaHandshakeVersion": "260928.2",
        }
        with patch.object(app, "blynk_request", return_value="true") as request:
            with patch.object(app, "query_device_version") as query_version:
                result = app.blynk_status(station)

        request.assert_called_once_with(station, "isHardwareConnected", {})
        query_version.assert_not_called()
        self.assertTrue(result["connected"])
        self.assertIsNone(result["deviceVersion"])

    def test_ota_completion_requires_terminal_version_match(self):
        station = {"id": "tram-thu", "name": "TRẠM THỬ", "terminalPin": "V12"}
        release = {"version": "260928.1", "sha256": "abc"}

        connection_states = iter(["false", "true"])

        def fake_request(_station, endpoint, params):
            if endpoint == "isHardwareConnected":
                return next(connection_states)
            self.fail(f"Unexpected endpoint: {endpoint}")

        ticks = iter([0.0, 0.0, 6.0, 6.0, 7.0, 7.0])
        version_result = {
            "version": "260928.1",
            "lastOtaStatus": "none",
            "freeHeap": 27648,
            "otaMinHeap": 3536,
            "terminalPin": "V12",
            "verifiedAt": "2026-09-28T00:00:00+00:00",
        }
        with patch.object(app, "append_job_log"):
            with patch.object(app, "remember_device_version") as remember_version:
                with patch.object(app, "request_ota_start", return_value={"mode": "legacy", "requestId": None}):
                    with patch.object(app, "blynk_request", side_effect=fake_request):
                        with patch.object(app, "query_device_version", return_value=version_result) as query_version:
                            with patch.object(app.time, "monotonic", side_effect=lambda: next(ticks)):
                                with patch.object(app.time, "sleep"):
                                    result = app.execute_ota_and_verify(
                                        station,
                                        release,
                                        "test-job",
                                        timeout_seconds=30,
                                    )

        self.assertTrue(result["versionVerified"])
        self.assertEqual(result["deviceVersion"], "260928.1")
        self.assertEqual(result["otaMinHeap"], 3536)
        self.assertTrue(result["sawOffline"])
        query_version.assert_called_once_with(station, timeout_seconds=5.0, poll_interval=0.5)
        remember_version.assert_called_once_with("tram-thu", "260928.1")

    def test_ota_completion_accepts_fast_reconnect_without_offline_edge(self):
        station = {"id": "tram-thu", "name": "TRẠM THỬ", "terminalPin": "V12"}
        release = {"version": "260930.2", "sha256": "abc"}
        version_result = {
            "version": "260930.2",
            "lastOtaStatus": "success",
            "freeHeap": 4264,
            "otaMinHeap": 11480,
            "terminalPin": "V12",
            "verifiedAt": "2026-10-01T01:57:11+00:00",
        }

        with patch.object(app, "append_job_log") as append_log:
            with patch.object(app, "remember_device_version") as remember_version:
                with patch.object(app, "request_ota_start", return_value={"mode": "acknowledged", "requestId": "abc"}):
                    with patch.object(app, "blynk_request", return_value="true"):
                        with patch.object(app, "query_device_version", return_value=version_result):
                            with patch.object(app.time, "monotonic", side_effect=[0.0, 0.0, 6.0, 6.0]):
                                result = app.execute_ota_and_verify(
                                    station,
                                    release,
                                    "test-job",
                                    timeout_seconds=30,
                                )

        self.assertTrue(result["versionVerified"])
        self.assertFalse(result["sawOffline"])
        self.assertEqual(result["deviceVersion"], "260930.2")
        self.assertTrue(any("không ghi nhận nhịp offline" in call.args[1] for call in append_log.call_args_list))
        remember_version.assert_called_once_with("tram-thu", "260930.2")

    def test_ota_fails_immediately_when_device_returns_to_old_version(self):
        station = {"id": "tram-thu", "name": "TRẠM THỬ", "terminalPin": "V12"}
        release = {"version": "260929.10", "sha256": "abc"}
        connection_states = iter(["false", "true"])

        def fake_request(_station, endpoint, params):
            if endpoint == "isHardwareConnected":
                return next(connection_states)
            self.fail(f"Unexpected endpoint: {endpoint}")

        ticks = iter([0.0, 0.0, 6.0, 6.0, 7.0, 7.0])
        version_result = {
            "version": "260929.8",
            "lastOtaStatus": "interrupted/reset",
            "terminalPin": "V12",
            "verifiedAt": "2026-09-29T14:21:10+00:00",
        }
        with patch.object(app, "append_job_log") as append_log:
            with patch.object(app, "request_ota_start", return_value={"mode": "acknowledged", "requestId": "abc"}):
                with patch.object(app, "blynk_request", side_effect=fake_request):
                    with patch.object(app, "query_device_version", return_value=version_result) as query_version:
                        with patch.object(app.time, "monotonic", side_effect=lambda: next(ticks)):
                            with patch.object(app.time, "sleep"):
                                with self.assertRaisesRegex(
                                    RuntimeError,
                                    r"firmware cũ 260929\.8.*interrupted/reset",
                                ):
                                    app.execute_ota_and_verify(
                                        station,
                                        release,
                                        "test-job",
                                        timeout_seconds=30,
                                    )

        query_version.assert_called_once()
        self.assertTrue(any("firmware cũ" in call.args[1] for call in append_log.call_args_list))

    def test_log_redaction_hides_tokens_and_private_ota_urls(self):
        text = app.redact_for_station(
            {},
            "https://example.workers.dev/station/private-key/firmware.bin?token=secret token=abc123",
        )
        self.assertNotIn("private-key", text)
        self.assertNotIn("abc123", text)
        self.assertIn("[URL_OTA_DA_CHE]", text)

    def test_bump_firmware_version_changes_only_active_define(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            project = Path(temp_dir)
            (project / "src").mkdir()
            source = project / "src" / "main.cpp"
            source.write_text(
                '// #define BLYNK_FIRMWARE_VERSION "old"\n'
                '#define BLYNK_FIRMWARE_VERSION "260929.8"\n',
                encoding="utf-8",
            )
            station = {"id": "test", "name": "Test", "projectPath": str(project)}
            with patch.object(app, "append_history"):
                result = app.bump_firmware_version(station, "260929.8")

            self.assertEqual(result["version"], "260929.9")
            revised = source.read_text(encoding="utf-8")
            self.assertIn('// #define BLYNK_FIRMWARE_VERSION "old"', revised)
            self.assertIn('#define BLYNK_FIRMWARE_VERSION "260929.9"', revised)

    def test_safe_add_station_rejects_path_outside_project_root(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            fake_settings = {
                "projectRoot": str(Path(temp_dir) / "allowed"),
            }
            payload = {
                "id": "outside-project",
                "name": "Outside",
                "projectPath": str(Path(temp_dir) / "outside"),
            }
            with patch.object(app, "load_settings", return_value=fake_settings):
                with self.assertRaisesRegex(ValueError, "Dự án phải nằm"):
                    app.safe_add_station(payload)


if __name__ == "__main__":
    unittest.main()
