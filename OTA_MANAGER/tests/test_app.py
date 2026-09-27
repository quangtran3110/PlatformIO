import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import app


class OtaManagerTests(unittest.TestCase):
    def test_parse_source_hides_secrets_from_station_payload(self):
        station = app.get_station("tram-cc")
        parsed = app.parse_source(station)
        payload = app.station_payload(station)
        self.assertEqual(parsed["version"], "260926.4")
        self.assertTrue(parsed["otaUrl"].startswith("https://"))
        self.assertTrue(parsed["blynkToken"])
        serialized = json.dumps(payload)
        self.assertNotIn(parsed["otaUrl"], serialized)
        self.assertNotIn(parsed["blynkToken"], serialized)

    def test_station_payload_matches_current_release(self):
        station = app.get_station("tram-cc")
        payload = app.station_payload(station)
        self.assertTrue(payload["projectExists"])
        self.assertEqual(payload["sourceVersion"], "260926.4")
        self.assertEqual(payload["binary"]["size"], 451808)
        self.assertTrue(payload["releaseMatchesBinary"])

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
