import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

import app


class OtaManagerTests(unittest.TestCase):
    def test_parse_source_ignores_commented_legacy_token(self):
        station = app.get_station("ts2")
        parsed = app.parse_source(station)
        source = app.source_path_for(station).read_text(encoding="utf-8")
        legacy_token = "ESzia3fpA-29cs8gt85pGnrPq_rICcqf"

        self.assertIn(legacy_token, source)
        self.assertNotEqual(parsed["blynkToken"], legacy_token)
        self.assertEqual(parsed["version"], "260928.1")

    def test_parse_source_hides_secrets_from_station_payload(self):
        station = app.get_station("tram-cc")
        parsed = app.parse_source(station)
        payload = app.station_payload(station)
        self.assertEqual(parsed["version"], "260928.1")
        self.assertTrue(parsed["otaUrl"].startswith("https://"))
        self.assertTrue(parsed["blynkToken"])
        serialized = json.dumps(payload)
        self.assertNotIn(parsed["otaUrl"], serialized)
        self.assertNotIn(parsed["blynkToken"], serialized)

    def test_station_payload_matches_published_firmware(self):
        station = app.get_station("tram-cc")
        payload = app.station_payload(station)
        self.assertTrue(payload["projectExists"])
        self.assertEqual(payload["sourceVersion"], "260928.1")
        self.assertGreater(payload["binary"]["size"], 0)
        self.assertTrue(payload["releaseMatchesBinary"])

    def test_terminal_version_reply_requires_matching_request_id(self):
        request_id = "a1b2c3d4e5f6"
        self.assertEqual(
            app.parse_terminal_version_reply(
                '"ota_reply:a1b2c3d4e5f6|version=260928.1"',
                request_id,
            ),
            "260928.1",
        )
        self.assertIsNone(
            app.parse_terminal_version_reply(
                "ota_reply:ffffffffffff|version=260928.1",
                request_id,
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

    def test_ota_completion_requires_terminal_version_match(self):
        station = {"id": "tram-thu", "name": "TRẠM THỬ", "terminalPin": "V12"}
        release = {"version": "260928.1", "sha256": "abc"}

        def fake_request(_station, endpoint, params):
            if endpoint == "update":
                self.assertEqual(params, {"V12": "update"})
                return ""
            if endpoint == "isHardwareConnected":
                return "true"
            self.fail(f"Unexpected endpoint: {endpoint}")

        ticks = iter([0.0, 0.0, 6.0, 6.0])
        version_result = {
            "version": "260928.1",
            "terminalPin": "V12",
            "verifiedAt": "2026-09-28T00:00:00+00:00",
        }
        with patch.object(app, "append_job_log"):
            with patch.object(app, "remember_device_version") as remember_version:
                with patch.object(app, "blynk_request", side_effect=fake_request):
                    with patch.object(app, "query_device_version", return_value=version_result):
                        with patch.object(app.time, "monotonic", side_effect=lambda: next(ticks)):
                            result = app.execute_ota_and_verify(
                                station,
                                release,
                                "test-job",
                                timeout_seconds=30,
                            )

        self.assertTrue(result["versionVerified"])
        self.assertEqual(result["deviceVersion"], "260928.1")
        remember_version.assert_called_once_with("tram-thu", "260928.1")

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
