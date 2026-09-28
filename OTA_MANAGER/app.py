from __future__ import annotations

import hashlib
import json
import mimetypes
import os
import re
import subprocess
import tempfile
import threading
import time
import uuid
from datetime import datetime, timezone
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any, Callable
from urllib.error import HTTPError, URLError
from urllib.parse import quote, unquote, urlencode, urlparse
from urllib.request import Request, urlopen


APP_ROOT = Path(__file__).resolve().parent
WEB_ROOT = APP_ROOT / "web"
CONFIG_PATH = APP_ROOT / "config" / "stations.json"
SETTINGS_PATH = APP_ROOT / "config" / "settings.json"
DATA_ROOT = APP_ROOT / "data"
HISTORY_PATH = DATA_ROOT / "history.json"
HOST = "127.0.0.1"
PORT = 8765
API_HEADER = "X-OTA-Manager"
API_HEADER_VALUE = "local-ui"
MAX_JOB_LOG_LINES = 1200

JOBS: dict[str, dict[str, Any]] = {}
JOBS_LOCK = threading.Lock()


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()


def read_json(path: Path, default: Any) -> Any:
    if not path.exists():
        return default
    return json.loads(path.read_text(encoding="utf-8"))


def write_json_atomic(path: Path, payload: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temp_path = path.with_suffix(path.suffix + ".tmp")
    temp_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    temp_path.replace(path)


def load_settings() -> dict[str, Any]:
    return read_json(SETTINGS_PATH, {})


def load_station_config() -> dict[str, Any]:
    return read_json(CONFIG_PATH, {"stations": []})


def save_station_config(payload: dict[str, Any]) -> None:
    write_json_atomic(CONFIG_PATH, payload)


def remember_device_version(station_id: str, version: str) -> None:
    config = load_station_config()
    for station in config.get("stations", []):
        if station.get("id") == station_id:
            if station.get("lastKnownDeviceVersion") != version:
                station["lastKnownDeviceVersion"] = version
                save_station_config(config)
            return


def get_station(station_id: str) -> dict[str, Any]:
    for station in load_station_config().get("stations", []):
        if station.get("id") == station_id:
            return station
    raise KeyError(f"Không tìm thấy trạm: {station_id}")


def project_path_for(station: dict[str, Any]) -> Path:
    return Path(station["projectPath"]).resolve()


def source_path_for(station: dict[str, Any]) -> Path:
    return project_path_for(station) / "src" / "main.cpp"


def binary_path_for(station: dict[str, Any]) -> Path:
    environment = station.get("environment", "nodemcuv2")
    return project_path_for(station) / ".pio" / "build" / environment / "firmware.bin"


def parse_source(station: dict[str, Any]) -> dict[str, Any]:
    source_path = source_path_for(station)
    if not source_path.exists():
        return {"version": None, "otaUrl": None, "blynkToken": None, "error": "Không tìm thấy src/main.cpp"}
    source = source_path.read_text(encoding="utf-8", errors="replace")
    version_match = re.search(r'#define\s+BLYNK_FIRMWARE_VERSION\s+"([^"]+)"', source)
    ota_match = re.search(r'#define\s+URL_fw_Bin\s+"([^"]+)"', source)
    token_match = re.search(r'#define\s+BLYNK_AUTH_TOKEN\s+"([^"]+)"', source)
    return {
        "version": version_match.group(1) if version_match else None,
        "otaUrl": ota_match.group(1) if ota_match else None,
        "blynkToken": token_match.group(1) if token_match else None,
        "error": None if version_match else "Không tìm thấy BLYNK_FIRMWARE_VERSION",
    }


def file_hashes(path: Path) -> dict[str, Any]:
    sha256 = hashlib.sha256()
    md5 = hashlib.md5(usedforsecurity=False)
    with path.open("rb") as handle:
        while chunk := handle.read(1024 * 1024):
            sha256.update(chunk)
            md5.update(chunk)
    stat = path.stat()
    return {
        "path": str(path),
        "size": stat.st_size,
        "sha256": sha256.hexdigest(),
        "md5": md5.hexdigest(),
        "updatedAt": datetime.fromtimestamp(stat.st_mtime, tz=timezone.utc).isoformat(),
    }


def manifest_for(station: dict[str, Any]) -> dict[str, Any] | None:
    manifest_path = project_path_for(station) / "ota" / "release-output" / "latest.json"
    if not manifest_path.exists():
        return None
    try:
        manifest = read_json(manifest_path, None)
        if not isinstance(manifest, dict):
            return None
        return {**manifest, "path": str(manifest_path)}
    except (OSError, json.JSONDecodeError):
        return None


def station_payload(station: dict[str, Any]) -> dict[str, Any]:
    project_path = project_path_for(station)
    source = parse_source(station)
    binary_path = binary_path_for(station)
    binary = file_hashes(binary_path) if binary_path.exists() else None
    manifest = manifest_for(station)
    release_matches = bool(
        binary
        and manifest
        and manifest.get("version") == source.get("version")
        and str(manifest.get("sha256", "")).lower() == binary["sha256"].lower()
        and int(manifest.get("size", -1)) == binary["size"]
    )
    return {
        **station,
        "sourceVersion": source["version"],
        "sourceError": source["error"],
        "projectExists": project_path.exists(),
        "platformioExists": (project_path / "platformio.ini").exists(),
        "binary": binary,
        "release": manifest,
        "releaseMatchesBinary": release_matches,
        "otaConfigured": bool(source["otaUrl"]),
        "blynkConfigured": bool(source["blynkToken"]),
        "busy": active_job_for_station(station["id"]),
    }


def load_stations() -> list[dict[str, Any]]:
    return [station_payload(station) for station in load_station_config().get("stations", [])]


def append_history(event: dict[str, Any]) -> None:
    with JOBS_LOCK:
        history = read_json(HISTORY_PATH, [])
        history.insert(0, {"at": utc_now(), **event})
        write_json_atomic(HISTORY_PATH, history[:250])


def active_job_for_station(station_id: str) -> dict[str, Any] | None:
    with JOBS_LOCK:
        for job in JOBS.values():
            if job["stationId"] == station_id and job["status"] in {"queued", "running"}:
                return {key: job[key] for key in ("id", "type", "status", "startedAt")}
    return None


def wrap_command(command: list[str]) -> list[str]:
    if command and command[0].lower().endswith((".cmd", ".bat")):
        return [os.environ.get("COMSPEC", "cmd.exe"), "/d", "/c", *command]
    return command


def run_job(
    job_id: str,
    command: list[str],
    cwd: Path,
    environment: dict[str, str] | None,
    after_success: Callable[[], dict[str, Any]] | None,
) -> None:
    with JOBS_LOCK:
        job = JOBS[job_id]
        job["status"] = "running"
        job["startedAt"] = utc_now()
    try:
        env = os.environ.copy()
        if environment:
            env.update(environment)
        process = subprocess.Popen(
            wrap_command(command),
            cwd=str(cwd),
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            bufsize=1,
        )
        assert process.stdout is not None
        for line in process.stdout:
            clean = line.rstrip("\r\n")
            with JOBS_LOCK:
                JOBS[job_id]["logs"].append(clean)
                JOBS[job_id]["logs"] = JOBS[job_id]["logs"][-MAX_JOB_LOG_LINES:]
        return_code = process.wait()
        if return_code != 0:
            raise RuntimeError(f"Tiến trình kết thúc với mã lỗi {return_code}")
        result = after_success() if after_success else {}
        with JOBS_LOCK:
            JOBS[job_id].update(status="success", completedAt=utc_now(), result=result)
        append_history({"stationId": job["stationId"], "type": job["type"], "status": "success", "result": result})
    except Exception as exc:  # pragma: no cover - subprocess boundary
        with JOBS_LOCK:
            JOBS[job_id].update(status="failed", completedAt=utc_now(), error=str(exc))
        append_history({"stationId": job["stationId"], "type": job["type"], "status": "failed", "error": str(exc)})


def create_job(
    job_type: str,
    station: dict[str, Any],
    command: list[str],
    cwd: Path,
    environment: dict[str, str] | None = None,
    after_success: Callable[[], dict[str, Any]] | None = None,
) -> dict[str, Any]:
    if active_job_for_station(station["id"]):
        raise RuntimeError("Trạm đang có một tác vụ khác; hãy chờ tác vụ đó hoàn tất.")
    job_id = uuid.uuid4().hex
    job = {
        "id": job_id,
        "type": job_type,
        "stationId": station["id"],
        "stationName": station["name"],
        "status": "queued",
        "createdAt": utc_now(),
        "startedAt": None,
        "completedAt": None,
        "logs": [],
        "result": None,
        "error": None,
    }
    with JOBS_LOCK:
        JOBS[job_id] = job
    thread = threading.Thread(
        target=run_job,
        args=(job_id, command, cwd, environment, after_success),
        daemon=True,
    )
    thread.start()
    return {key: job[key] for key in ("id", "type", "stationId", "status", "createdAt")}


def append_job_log(job_id: str, message: str) -> None:
    with JOBS_LOCK:
        JOBS[job_id]["logs"].append(message)
        JOBS[job_id]["logs"] = JOBS[job_id]["logs"][-MAX_JOB_LOG_LINES:]


def run_task_job(job_id: str, task: Callable[[str], dict[str, Any]]) -> None:
    with JOBS_LOCK:
        job = JOBS[job_id]
        job["status"] = "running"
        job["startedAt"] = utc_now()
    try:
        result = task(job_id)
        with JOBS_LOCK:
            JOBS[job_id].update(status="success", completedAt=utc_now(), result=result)
        append_history({"stationId": job["stationId"], "type": job["type"], "status": "success", "result": result})
    except Exception as exc:  # pragma: no cover - network/device boundary
        with JOBS_LOCK:
            JOBS[job_id].update(status="failed", completedAt=utc_now(), error=str(exc))
        append_history({"stationId": job["stationId"], "type": job["type"], "status": "failed", "error": str(exc)})


def create_task_job(
    job_type: str,
    station: dict[str, Any],
    task: Callable[[str], dict[str, Any]],
) -> dict[str, Any]:
    if active_job_for_station(station["id"]):
        raise RuntimeError("Trạm đang có một tác vụ khác; hãy chờ tác vụ đó hoàn tất.")
    job_id = uuid.uuid4().hex
    job = {
        "id": job_id,
        "type": job_type,
        "stationId": station["id"],
        "stationName": station["name"],
        "status": "queued",
        "createdAt": utc_now(),
        "startedAt": None,
        "completedAt": None,
        "logs": [],
        "result": None,
        "error": None,
    }
    with JOBS_LOCK:
        JOBS[job_id] = job
    thread = threading.Thread(target=run_task_job, args=(job_id, task), daemon=True)
    thread.start()
    return {key: job[key] for key in ("id", "type", "stationId", "status", "createdAt")}


def settings_environment() -> dict[str, str]:
    settings = load_settings()
    config_home = settings.get("wranglerConfigHome")
    return {"XDG_CONFIG_HOME": config_home} if config_home else {}


def verify_release(station: dict[str, Any]) -> dict[str, Any]:
    source = parse_source(station)
    ota_url = source.get("otaUrl")
    binary_path = binary_path_for(station)
    if not ota_url:
        raise RuntimeError("Dự án chưa có URL OTA Private.")
    if not binary_path.exists():
        raise RuntimeError("Chưa có firmware.bin trên máy.")
    local = file_hashes(binary_path)
    request = Request(ota_url, headers={"User-Agent": "OTA-Manager/1.0"})
    try:
        with urlopen(request, timeout=45) as response:
            remote_bytes = response.read()
            headers = {key.lower(): value for key, value in response.headers.items()}
            status = response.status
    except (HTTPError, URLError, TimeoutError) as exc:
        raise RuntimeError(f"Không tải được firmware từ cổng OTA: {exc}") from exc
    remote_sha256 = hashlib.sha256(remote_bytes).hexdigest()
    remote_md5 = hashlib.md5(remote_bytes, usedforsecurity=False).hexdigest()
    exact_match = len(remote_bytes) == local["size"] and remote_sha256 == local["sha256"]
    if not exact_match:
        raise RuntimeError("Firmware tải ngược không khớp binary trên máy; đã dừng trước OTA.")
    return {
        "status": status,
        "version": headers.get("x-firmware-version") or source.get("version"),
        "size": len(remote_bytes),
        "sha256": remote_sha256,
        "md5": remote_md5,
        "exactMatch": True,
        "verifiedAt": utc_now(),
    }


def blynk_request(station: dict[str, Any], endpoint: str, params: dict[str, str]) -> str:
    source = parse_source(station)
    token = source.get("blynkToken")
    if not token:
        raise RuntimeError("Không đọc được Blynk AuthToken của dự án.")
    server = station.get("blynkServer", "https://sgp1.blynk.cloud").rstrip("/")
    query = urlencode({"token": token, **params})
    request = Request(f"{server}/external/api/{endpoint}?{query}", headers={"User-Agent": "OTA-Manager/1.0"})
    try:
        with urlopen(request, timeout=15) as response:
            return response.read().decode("utf-8", errors="replace").strip()
    except (HTTPError, URLError, TimeoutError) as exc:
        raise RuntimeError(f"Không kết nối được Blynk: {exc}") from exc


def terminal_pin_for(station: dict[str, Any]) -> str:
    return str(station.get("terminalPin") or station.get("otaPin") or "V5").upper()


def normalize_blynk_value(value: str) -> str:
    clean = value.strip()
    try:
        parsed = json.loads(clean)
        if isinstance(parsed, str):
            return parsed.strip()
        if isinstance(parsed, list) and len(parsed) == 1 and isinstance(parsed[0], str):
            return parsed[0].strip()
    except json.JSONDecodeError:
        pass
    return clean


def parse_terminal_version_reply(value: str, request_id: str) -> str | None:
    clean = normalize_blynk_value(value)
    match = re.fullmatch(
        rf"ota_reply:{re.escape(request_id)}\|version=([A-Za-z0-9._-]{{1,32}})",
        clean,
    )
    return match.group(1) if match else None


def query_device_version(
    station: dict[str, Any],
    timeout_seconds: float = 3.0,
    poll_interval: float = 0.35,
) -> dict[str, Any]:
    pin = terminal_pin_for(station)
    request_id = uuid.uuid4().hex[:12]
    command = f"ota_info:{request_id}"
    blynk_request(station, "update", {pin: command})
    deadline = time.monotonic() + timeout_seconds
    last_error: RuntimeError | None = None
    while time.monotonic() < deadline:
        try:
            value = blynk_request(station, "get", {pin: ""})
            version = parse_terminal_version_reply(value, request_id)
            if version:
                return {
                    "version": version,
                    "terminalPin": pin,
                    "requestId": request_id,
                    "verifiedAt": utc_now(),
                }
        except RuntimeError as exc:
            last_error = exc
        time.sleep(poll_interval)
    if last_error:
        raise RuntimeError(f"Terminal chưa trả lời phiên bản: {last_error}") from last_error
    raise RuntimeError("Terminal chưa hỗ trợ lệnh tự xác minh phiên bản.")


def blynk_status(station: dict[str, Any]) -> dict[str, Any]:
    connected_text = blynk_request(station, "isHardwareConnected", {})
    connected = connected_text.lower() == "true"
    quality = None
    if connected and station.get("networkQualityPin"):
        try:
            quality = blynk_request(station, "get", {station["networkQualityPin"]: ""})
        except RuntimeError:
            quality = None
    device_version = None
    version_verified = False
    version_error = None
    version_checked_at = None
    if connected:
        try:
            version_result = query_device_version(station)
            device_version = version_result["version"]
            version_verified = True
            version_checked_at = version_result["verifiedAt"]
        except RuntimeError as exc:
            version_error = str(exc)
    if not device_version:
        device_version = station.get("lastKnownDeviceVersion")
    return {
        "connected": connected,
        "networkQuality": quality,
        "deviceVersion": device_version,
        "versionVerified": version_verified,
        "versionError": version_error,
        "versionCheckedAt": version_checked_at,
        "terminalPin": terminal_pin_for(station),
        "checkedAt": utc_now(),
    }


def execute_ota_and_verify(
    station: dict[str, Any],
    release: dict[str, Any],
    job_id: str,
    timeout_seconds: float = 180.0,
) -> dict[str, Any]:
    pin = terminal_pin_for(station)
    expected_version = str(release.get("version") or parse_source(station).get("version") or "")
    if not expected_version:
        raise RuntimeError("Không xác định được phiên bản cần xác minh sau OTA.")

    append_job_log(job_id, f"Firmware {expected_version} đã khớp Cloudflare.")
    blynk_request(station, "update", {pin: "update"})
    sent_at = utc_now()
    append_job_log(job_id, f"Đã gửi lệnh update qua Terminal {pin}.")
    append_job_log(job_id, "Đang chờ thiết bị khởi động lại và kết nối Blynk…")

    deadline = time.monotonic() + timeout_seconds
    first_probe_at = time.monotonic() + 5.0
    saw_offline = False
    announced_reconnect = False
    last_version = None
    last_error = None
    while time.monotonic() < deadline:
        if time.monotonic() < first_probe_at:
            time.sleep(1.0)
            continue
        try:
            connected_text = blynk_request(station, "isHardwareConnected", {})
            connected = connected_text.lower() == "true"
        except RuntimeError as exc:
            connected = False
            last_error = str(exc)

        if not connected:
            if not saw_offline:
                append_job_log(job_id, "Thiết bị đã ngắt kết nối để cập nhật.")
            saw_offline = True
            time.sleep(2.0)
            continue

        if saw_offline and not announced_reconnect:
            append_job_log(job_id, "Thiết bị đã kết nối lại; đang đọc phiên bản qua Terminal…")
            announced_reconnect = True
        try:
            version_result = query_device_version(station, timeout_seconds=5.0, poll_interval=0.5)
            last_version = version_result["version"]
            if last_version == expected_version:
                remember_device_version(station["id"], last_version)
                append_job_log(job_id, f"Đã xác minh thiết bị đang chạy phiên bản {last_version}.")
                return {
                    "accepted": True,
                    "target": station["name"],
                    "terminalPin": pin,
                    "expectedVersion": expected_version,
                    "deviceVersion": last_version,
                    "versionVerified": True,
                    "sawOffline": saw_offline,
                    "sentAt": sent_at,
                    "verifiedAt": version_result["verifiedAt"],
                    "release": release,
                }
            last_error = f"Thiết bị phản hồi phiên bản {last_version}, chưa phải {expected_version}."
        except RuntimeError as exc:
            last_error = str(exc)
        time.sleep(4.0)

    detail = last_error or "không nhận được phản hồi phiên bản"
    if last_version:
        detail = f"phiên bản cuối cùng nhận được là {last_version}"
    raise RuntimeError(f"OTA đã được gửi nhưng chưa xác minh đạt: {detail}.")


def cloudflare_status() -> dict[str, Any]:
    settings = load_settings()
    wrangler = settings.get("wranglerPath")
    if not wrangler or not Path(wrangler).exists():
        return {"connected": False, "error": "Không tìm thấy Wrangler", "checkedAt": utc_now()}
    config_home = settings.get("wranglerConfigHome")
    credential_path = Path(config_home) / ".wrangler" / "config" / "default.toml" if config_home else None
    credentials_saved = bool(credential_path and credential_path.is_file() and credential_path.stat().st_size > 0)
    return {
        "connected": credentials_saved,
        "authSource": "local-oauth" if credentials_saved else None,
        "checkedAt": utc_now(),
        "error": None if credentials_saved else "Cloudflare chưa đăng nhập",
    }


def safe_add_station(payload: dict[str, Any]) -> dict[str, Any]:
    settings = load_settings()
    project_root = Path(settings["projectRoot"]).resolve()
    station_id = str(payload.get("id", "")).strip().lower()
    name = str(payload.get("name", "")).strip()
    project_path = Path(str(payload.get("projectPath", ""))).resolve()
    if not re.fullmatch(r"[a-z0-9][a-z0-9-]{1,48}", station_id):
        raise ValueError("Mã trạm chỉ dùng chữ thường, số và dấu gạch ngang.")
    if len(name) < 2:
        raise ValueError("Tên trạm chưa hợp lệ.")
    try:
        project_path.relative_to(project_root)
    except ValueError as exc:
        raise ValueError("Dự án phải nằm trong D:\\AI_PROJECTS\\PIO.") from exc
    if not (project_path / "platformio.ini").exists():
        raise ValueError("Không tìm thấy platformio.ini trong dự án.")
    config = load_station_config()
    if any(item.get("id") == station_id for item in config.get("stations", [])):
        raise ValueError("Mã trạm đã tồn tại.")
    station = {
        "id": station_id,
        "name": name,
        "shortName": str(payload.get("shortName") or name).strip(),
        "projectPath": str(project_path),
        "environment": str(payload.get("environment") or "nodemcuv2").strip(),
        "releaseScript": str(payload.get("releaseScript") or "").strip(),
        "terminalPin": str(payload.get("terminalPin") or payload.get("otaPin") or "V5").upper(),
        "lastKnownDeviceVersion": None,
        "cloudflareWorker": str(payload.get("cloudflareWorker") or "").strip(),
        "networkQualityPin": str(payload.get("networkQualityPin") or "V76").upper(),
        "notes": str(payload.get("notes") or "Dự án ESP").strip(),
    }
    config.setdefault("stations", []).append(station)
    save_station_config(config)
    append_history({"stationId": station_id, "type": "station-added", "status": "success"})
    return station_payload(station)


class OtaManagerHandler(BaseHTTPRequestHandler):
    server_version = "OTA-Manager/1.0"

    def log_message(self, fmt: str, *args: object) -> None:
        print(f"[{self.log_date_time_string()}] {fmt % args}")

    def end_headers(self) -> None:
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("X-Frame-Options", "DENY")
        self.send_header("Referrer-Policy", "no-referrer")
        self.send_header("Content-Security-Policy", "default-src 'self'; style-src 'self'; script-src 'self'; img-src 'self' data:; connect-src 'self'")
        super().end_headers()

    def send_json(self, payload: object, status: int = HTTPStatus.OK) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def read_payload(self) -> dict[str, Any]:
        length = int(self.headers.get("Content-Length", "0"))
        if length > 64 * 1024:
            raise ValueError("Dữ liệu gửi lên quá lớn.")
        if length == 0:
            return {}
        return json.loads(self.rfile.read(length).decode("utf-8"))

    def require_local_action(self) -> bool:
        host = self.headers.get("Host", "")
        origin = self.headers.get("Origin")
        allowed_hosts = {f"127.0.0.1:{PORT}", f"localhost:{PORT}"}
        allowed_origins = {f"http://127.0.0.1:{PORT}", f"http://localhost:{PORT}"}
        valid = host in allowed_hosts and self.headers.get(API_HEADER) == API_HEADER_VALUE
        if origin:
            valid = valid and origin in allowed_origins
        if not valid:
            self.send_json({"error": "Yêu cầu không hợp lệ."}, HTTPStatus.FORBIDDEN)
        return valid

    def serve_file(self, relative_path: str) -> None:
        relative_path = relative_path.lstrip("/") or "index.html"
        candidate = (WEB_ROOT / unquote(relative_path)).resolve()
        try:
            candidate.relative_to(WEB_ROOT.resolve())
        except ValueError:
            self.send_error(HTTPStatus.FORBIDDEN)
            return
        if not candidate.is_file():
            candidate = WEB_ROOT / "index.html"
        content = candidate.read_bytes()
        content_type = mimetypes.guess_type(candidate.name)[0] or "application/octet-stream"
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(content)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(content)

    def do_GET(self) -> None:
        path = urlparse(self.path).path
        try:
            if path == "/api/health":
                self.send_json({"ok": True, "service": "OTA Manager", "version": "1.1.0"})
                return
            if path == "/api/stations":
                self.send_json({"stations": load_stations()})
                return
            if path == "/api/history":
                self.send_json({"history": read_json(HISTORY_PATH, [])[:50]})
                return
            if path == "/api/cloudflare/status":
                self.send_json(cloudflare_status())
                return
            station_match = re.fullmatch(r"/api/stations/([a-z0-9-]+)/device", path)
            if station_match:
                self.send_json(blynk_status(get_station(station_match.group(1))))
                return
            job_match = re.fullmatch(r"/api/jobs/([a-f0-9]+)", path)
            if job_match:
                with JOBS_LOCK:
                    job = JOBS.get(job_match.group(1))
                    if not job:
                        self.send_json({"error": "Không tìm thấy tác vụ."}, HTTPStatus.NOT_FOUND)
                    else:
                        self.send_json(job)
                return
            if path.startswith("/api/"):
                self.send_json({"error": "Không tìm thấy API."}, HTTPStatus.NOT_FOUND)
                return
            self.serve_file(path)
        except KeyError as exc:
            self.send_json({"error": str(exc)}, HTTPStatus.NOT_FOUND)
        except Exception as exc:  # pragma: no cover - HTTP boundary
            self.send_json({"error": str(exc)}, HTTPStatus.INTERNAL_SERVER_ERROR)

    def do_POST(self) -> None:
        if not self.require_local_action():
            return
        path = urlparse(self.path).path
        try:
            payload = self.read_payload()
            if path == "/api/stations":
                self.send_json(safe_add_station(payload), HTTPStatus.CREATED)
                return
            if path == "/api/cloudflare/connect":
                settings = load_settings()
                pseudo_station = {"id": "cloudflare", "name": "Cloudflare"}
                job = create_job(
                    "cloudflare-connect",
                    pseudo_station,
                    [settings["wranglerPath"], "login", "--no-use-keyring"],
                    APP_ROOT,
                    settings_environment(),
                    cloudflare_status,
                )
                self.send_json(job, HTTPStatus.ACCEPTED)
                return
            match = re.fullmatch(r"/api/stations/([a-z0-9-]+)/(build|publish|verify|ota|open)", path)
            if not match:
                self.send_json({"error": "Không tìm thấy thao tác."}, HTTPStatus.NOT_FOUND)
                return
            station = get_station(match.group(1))
            action = match.group(2)
            settings = load_settings()
            project_path = project_path_for(station)

            if action == "build":
                platformio = Path(settings["platformioPath"])
                if not platformio.exists():
                    raise RuntimeError("Không tìm thấy PlatformIO trong cấu hình.")
                job = create_job(
                    "build",
                    station,
                    [str(platformio), "run", "--project-dir", str(project_path)],
                    project_path,
                    after_success=lambda: file_hashes(binary_path_for(station)),
                )
                self.send_json(job, HTTPStatus.ACCEPTED)
                return

            if action == "publish":
                current = station_payload(station)
                if not current["binary"]:
                    raise RuntimeError("Hãy build firmware trước khi phát hành.")
                if current["release"] and current["release"]["version"] == current["sourceVersion"] and not current["releaseMatchesBinary"]:
                    raise RuntimeError("Version hiện tại đã từng phát hành với binary khác. Hãy tăng version trước khi phát hành.")
                if current["releaseMatchesBinary"]:
                    raise RuntimeError("Binary hiện tại đã được phát hành và khớp hoàn toàn; không cần phát hành lại.")
                release_script = project_path / station.get("releaseScript", "")
                if not release_script.exists():
                    raise RuntimeError("Trạm chưa có script phát hành Cloudflare.")
                powershell = settings.get("powershellPath", "powershell.exe")
                job = create_job(
                    "publish",
                    station,
                    [powershell, "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(release_script)],
                    project_path,
                    settings_environment(),
                    after_success=lambda: verify_release(station),
                )
                self.send_json(job, HTTPStatus.ACCEPTED)
                return

            if action == "verify":
                result = verify_release(station)
                append_history({"stationId": station["id"], "type": "verify", "status": "success", "result": result})
                self.send_json(result)
                return

            if action == "open":
                os.startfile(str(project_path))
                self.send_json({"opened": True})
                return

            if action == "ota":
                if payload.get("confirmation") != station["name"] or payload.get("confirmedRestart") is not True:
                    raise ValueError("Xác nhận OTA chưa đúng.")
                current = station_payload(station)
                if not current["releaseMatchesBinary"]:
                    raise RuntimeError("Firmware phát hành chưa khớp binary trên máy.")
                connected_text = blynk_request(station, "isHardwareConnected", {})
                if connected_text.lower() != "true":
                    raise RuntimeError("Thiết bị đang Offline; không gửi lệnh OTA.")
                verified = verify_release(station)
                job = create_task_job(
                    "ota",
                    station,
                    lambda job_id: execute_ota_and_verify(station, verified, job_id),
                )
                self.send_json(job, HTTPStatus.ACCEPTED)
                return
        except (ValueError, json.JSONDecodeError) as exc:
            self.send_json({"error": str(exc)}, HTTPStatus.BAD_REQUEST)
        except KeyError as exc:
            self.send_json({"error": str(exc)}, HTTPStatus.NOT_FOUND)
        except Exception as exc:
            self.send_json({"error": str(exc)}, HTTPStatus.CONFLICT)


def main() -> None:
    DATA_ROOT.mkdir(parents=True, exist_ok=True)
    server = ThreadingHTTPServer((HOST, PORT), OtaManagerHandler)
    print(f"OTA Manager running at http://{HOST}:{PORT}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
