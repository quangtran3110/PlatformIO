const state = {
  stations: [],
  selectedId: null,
  cloudflare: null,
  device: null,
  activeJobId: null,
  pendingAction: null,
};

const $ = (selector) => document.querySelector(selector);
const API_HEADERS = { "Content-Type": "application/json", "X-OTA-Manager": "local-ui" };

function escapeHtml(value) {
  return String(value ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

function formatBytes(bytes) {
  if (!Number.isFinite(bytes)) return "Chưa có binary";
  if (bytes < 1024) return `${bytes} B`;
  return `${(bytes / 1024).toFixed(1)} KB`;
}

function formatTime(value) {
  if (!value) return "—";
  return new Intl.DateTimeFormat("vi-VN", {
    hour: "2-digit",
    minute: "2-digit",
    day: "2-digit",
    month: "2-digit",
  }).format(new Date(value));
}

function stationInitials(name) {
  return name
    .split(/\s+/)
    .filter(Boolean)
    .slice(-2)
    .map((part) => part[0])
    .join("")
    .toUpperCase();
}

function showToast(message) {
  const toast = $("#toast");
  toast.textContent = message;
  toast.classList.add("is-visible");
  window.clearTimeout(showToast.timer);
  showToast.timer = window.setTimeout(() => toast.classList.remove("is-visible"), 3400);
}

async function apiGet(path) {
  const response = await fetch(path, { cache: "no-store" });
  const data = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(data.error || `HTTP ${response.status}`);
  return data;
}

async function apiPost(path, payload = {}) {
  const response = await fetch(path, {
    method: "POST",
    headers: API_HEADERS,
    body: JSON.stringify(payload),
  });
  const data = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(data.error || `HTTP ${response.status}`);
  return data;
}

function selectedStation() {
  return state.stations.find((station) => station.id === state.selectedId) || null;
}

function renderStationList(filter = "") {
  const list = $("#station-list");
  const query = filter.trim().toLocaleLowerCase("vi");
  const stations = state.stations.filter((station) =>
    station.name.toLocaleLowerCase("vi").includes(query),
  );

  if (!stations.length) {
    list.innerHTML = '<div class="activity-empty"><div><strong>Không tìm thấy trạm</strong><p>Thử một từ khóa khác.</p></div></div>';
    return;
  }

  list.innerHTML = stations
    .map(
      (station) => `
        <button class="station-item ${station.id === state.selectedId ? "is-active" : ""}" type="button" data-station-id="${escapeHtml(station.id)}">
          <span class="station-avatar">${escapeHtml(stationInitials(station.shortName || station.name))}</span>
          <span class="station-copy">
            <strong>${escapeHtml(station.shortName || station.name)}</strong>
            <small>${escapeHtml(station.notes || "Dự án ESP")}</small>
          </span>
          <span class="station-version">${escapeHtml(station.sourceVersion || "—")}</span>
        </button>`,
    )
    .join("");

  list.querySelectorAll("[data-station-id]").forEach((button) => {
    button.addEventListener("click", () => selectStation(button.dataset.stationId));
  });
}

function setCheck(index, ok) {
  const item = $("#check-list").children[index];
  item.classList.toggle("is-ok", Boolean(ok));
}

function updateActionButtons(station) {
  const buildButton = $('[data-action="build"]');
  const publishButton = $('[data-action="publish"]');
  const otaButton = $('[data-action="ota"]');
  const busy = Boolean(station?.busy || state.activeJobId);
  const cloudflareReady = Boolean(state.cloudflare?.connected);
  const deviceReady = Boolean(state.device?.connected);
  const alreadyInstalled = Boolean(
    station?.sourceVersion &&
      state.device?.versionVerified &&
      state.device?.deviceVersion &&
      station.sourceVersion === state.device.deviceVersion,
  );

  buildButton.disabled = !station?.projectExists || !station?.platformioExists || busy;
  publishButton.disabled =
    !station?.binary || !station?.otaConfigured || !cloudflareReady || station?.releaseMatchesBinary || busy;
  otaButton.disabled = !station?.releaseMatchesBinary || !deviceReady || alreadyInstalled || busy;

  buildButton.classList.toggle("is-complete", Boolean(station?.binary));
  publishButton.classList.toggle("is-complete", Boolean(station?.releaseMatchesBinary));
  otaButton.classList.toggle("is-complete", Boolean(alreadyInstalled && station?.releaseMatchesBinary));

  if (station?.releaseMatchesBinary) {
    publishButton.querySelector("small").textContent = "Binary đã khớp Cloudflare";
  } else {
    publishButton.querySelector("small").textContent = "Tải lên và đối chiếu Cloudflare";
  }
  if (alreadyInstalled) {
    otaButton.querySelector("small").textContent = "Thiết bị đã ở phiên bản này";
  } else if (!deviceReady) {
    otaButton.querySelector("small").textContent = "Thiết bị cần Online trước OTA";
  } else {
    otaButton.querySelector("small").textContent = "Khởi động lại đúng một thiết bị";
  }
}

function renderSelectedStation() {
  const station = selectedStation();
  if (!station) return;

  $("#station-name").textContent = station.name;
  $("#station-path").textContent = station.projectPath;
  $("#source-version").textContent = station.sourceVersion || "—";
  $("#binary-version").textContent = station.binary ? station.sourceVersion || "Có file" : "Chưa build";
  $("#binary-meta").textContent = station.binary
    ? `${formatBytes(station.binary.size)} · ${station.binary.sha256.slice(0, 10).toUpperCase()}…`
    : "Chưa có firmware.bin";
  $("#release-version").textContent = station.release?.version || "Chưa phát hành";
  $("#device-version").textContent = state.device?.deviceVersion || "—";

  const online = Boolean(state.device?.connected);
  const networkQualityLabels = {
    Tot: "Tốt",
    TrungBinh: "Trung bình",
    Kem: "Kém",
  };
  const rawNetworkQuality = state.device?.networkQuality;
  const networkQuality = networkQualityLabels[rawNetworkQuality] || rawNetworkQuality;
  $("#station-state").textContent = online
    ? `Online${networkQuality ? ` · mạng ${networkQuality}` : ""}`
    : station.projectExists
      ? "Dự án sẵn sàng · thiết bị chưa kiểm tra"
      : "Không tìm thấy dự án";
  $("#device-dot").classList.toggle("is-offline", state.device !== null && !online);
  if (state.device?.versionVerified) {
    $("#device-meta").textContent = `Xác minh qua ${state.device.terminalPin} · ${formatTime(state.device.versionCheckedAt)}`;
  } else if (state.device?.deviceVersion) {
    $("#device-meta").textContent = `Dữ liệu chuyển tiếp · ${formatTime(state.device.checkedAt)}`;
  } else if (state.device?.connected) {
    $("#device-meta").textContent = "Terminal chưa phản hồi phiên bản";
  } else {
    $("#device-meta").textContent = state.device?.checkedAt
      ? `Kiểm tra ${formatTime(state.device.checkedAt)}`
      : "Đang đọc trạng thái Blynk";
  }

  const checks = [
    station.projectExists && station.platformioExists,
    Boolean(station.sourceVersion),
    Boolean(station.binary),
    Boolean(state.cloudflare?.connected),
  ];
  checks.forEach((ok, index) => setCheck(index, ok));
  $("#readiness-score").textContent = `${checks.filter(Boolean).length}/4`;
  updateActionButtons(station);
}

async function loadStations() {
  const data = await apiGet("/api/stations");
  state.stations = data.stations || [];
  state.selectedId = state.stations.some((item) => item.id === state.selectedId)
    ? state.selectedId
    : state.stations[0]?.id || null;
  renderStationList($("#station-search").value);
  renderSelectedStation();
}

async function loadCloudflare() {
  const pill = $("#cloudflare-pill");
  try {
    state.cloudflare = await apiGet("/api/cloudflare/status");
    pill.classList.toggle("is-connected", state.cloudflare.connected);
    pill.classList.toggle("state-muted", !state.cloudflare.connected);
    pill.querySelector("span").textContent = state.cloudflare.connected
      ? "Cloudflare đã đăng nhập"
      : "Kết nối Cloudflare";
  } catch (error) {
    state.cloudflare = { connected: false, error: error.message };
    pill.querySelector("span").textContent = "Cloudflare chưa sẵn sàng";
  }
  renderSelectedStation();
}

async function loadDevice() {
  const station = selectedStation();
  if (!station?.blynkConfigured) {
    state.device = null;
    renderSelectedStation();
    return;
  }
  try {
    state.device = await apiGet(`/api/stations/${encodeURIComponent(station.id)}/device`);
  } catch (error) {
    state.device = { connected: false, error: error.message, checkedAt: new Date().toISOString() };
  }
  renderSelectedStation();
}

function historyLabel(type) {
  return {
    build: "Build firmware",
    publish: "Phát hành Cloudflare",
    verify: "Xác minh firmware",
    ota: "Gửi lệnh OTA",
    "station-added": "Thêm dự án",
    "cloudflare-connect": "Kết nối Cloudflare",
  }[type] || type;
}

async function loadHistory() {
  try {
    const data = await apiGet("/api/history");
    const history = data.history || [];
    const content = $("#activity-content");
    if (!history.length) return;
    content.className = "activity-list";
    content.innerHTML = history
      .slice(0, 5)
      .map(
        (event) => `
          <div class="activity-row ${event.status === "failed" ? "is-failed" : ""}">
            <span class="activity-icon">${event.status === "failed" ? "!" : "✓"}</span>
            <span><strong>${escapeHtml(historyLabel(event.type))}</strong><small>${escapeHtml(event.stationId || "Hệ thống")}</small></span>
            <span class="activity-time">${escapeHtml(formatTime(event.at))}</span>
          </div>`,
      )
      .join("");
  } catch {
    // History is secondary; keep the empty state if unavailable.
  }
}

async function refreshAll() {
  try {
    await loadStations();
    await Promise.all([loadCloudflare(), loadDevice(), loadHistory()]);
  } catch (error) {
    showToast(error.message || "Không thể làm mới dữ liệu.");
  }
}

function jobStatusLabel(status) {
  return { queued: "Đang chờ", running: "Đang chạy", success: "Hoàn tất", failed: "Thất bại" }[status] || status;
}

async function monitorJob(jobId) {
  state.activeJobId = jobId;
  const consolePanel = $("#job-console");
  consolePanel.hidden = false;
  let finished = false;
  while (!finished) {
    const job = await apiGet(`/api/jobs/${jobId}`);
    $("#job-title").textContent = `${historyLabel(job.type)} · ${job.stationName}`;
    const status = $("#job-state");
    status.textContent = jobStatusLabel(job.status);
    status.className = `job-state ${job.status === "success" ? "is-success" : job.status === "failed" ? "is-failed" : ""}`;
    const log = $("#job-log");
    log.textContent = (job.logs || []).join("\n") || "Đang chuẩn bị…";
    log.scrollTop = log.scrollHeight;
    finished = ["success", "failed"].includes(job.status);
    if (!finished) await new Promise((resolve) => window.setTimeout(resolve, 900));
    if (finished) {
      state.activeJobId = null;
      showToast(job.status === "success" ? "Tác vụ đã hoàn tất." : job.error || "Tác vụ thất bại.");
      await refreshAll();
    }
  }
}

function configureConfirmDialog(action, station) {
  state.pendingAction = action;
  const isOta = action === "ota";
  $("#confirm-eyebrow").textContent = isOta ? "Thao tác thiết bị" : "Phát hành firmware";
  $("#confirm-title").textContent = isOta ? `OTA ${station.name}` : `Phát hành ${station.sourceVersion}`;
  $("#confirm-message").textContent = isOta
    ? `Firmware đã được xác minh. OTA sẽ khởi động lại ${station.name}, chờ thiết bị kết nối lại và tự đọc đúng phiên bản qua Terminal.`
    : `Ứng dụng sẽ build lại, tải firmware ${station.sourceVersion} lên Cloudflare và tải ngược để so hash.`;
  $("#restart-confirm-row").hidden = !isOta;
  $("#name-confirm-row").hidden = !isOta;
  $("#restart-confirm").checked = false;
  $("#name-confirm").value = "";
  $("#name-confirm").placeholder = station.name;
  $("#confirm-submit").textContent = isOta ? "Xác nhận OTA" : "Phát hành";
  $("#confirm-submit").disabled = isOta;
}

function validateConfirmDialog() {
  const station = selectedStation();
  const valid =
    state.pendingAction !== "ota" ||
    ($("#restart-confirm").checked && $("#name-confirm").value.trim() === station?.name);
  $("#confirm-submit").disabled = !valid;
}

async function runAction(action) {
  const station = selectedStation();
  if (!station) return;
  if (["publish", "ota"].includes(action)) {
    configureConfirmDialog(action, station);
    $("#confirm-dialog").showModal();
    return;
  }
  try {
    const job = await apiPost(`/api/stations/${station.id}/${action}`);
    monitorJob(job.id);
  } catch (error) {
    showToast(error.message);
  }
}

$("#confirm-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  const station = selectedStation();
  if (!station) return;
  const action = state.pendingAction;
  if (event.submitter?.value === "cancel") {
    $("#confirm-dialog").close();
    return;
  }
  validateConfirmDialog();
  if ($("#confirm-submit").disabled) return;
  $("#confirm-dialog").close();
  try {
    if (action === "publish") {
      const job = await apiPost(`/api/stations/${station.id}/publish`);
      monitorJob(job.id);
    } else if (action === "ota") {
      const job = await apiPost(`/api/stations/${station.id}/ota`, {
        confirmation: $("#name-confirm").value.trim(),
        confirmedRestart: $("#restart-confirm").checked,
      });
      showToast(`Đã bắt đầu OTA ${station.name}; ứng dụng sẽ tự xác minh phiên bản.`);
      monitorJob(job.id);
    }
  } catch (error) {
    showToast(error.message);
  }
});

$("#restart-confirm").addEventListener("change", validateConfirmDialog);
$("#name-confirm").addEventListener("input", validateConfirmDialog);

$("#station-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  if (event.submitter?.value === "cancel") {
    $("#station-dialog").close();
    return;
  }
  const form = new FormData(event.currentTarget);
  const payload = Object.fromEntries(form.entries());
  try {
    const station = await apiPost("/api/stations", payload);
    $("#station-form-error").textContent = "";
    $("#station-dialog").close();
    event.currentTarget.reset();
    state.selectedId = station.id;
    await refreshAll();
    showToast(`Đã thêm ${station.name}.`);
  } catch (error) {
    $("#station-form-error").textContent = error.message;
  }
});

$("#station-search").addEventListener("input", (event) => renderStationList(event.target.value));
$("#refresh").addEventListener("click", async () => {
  await refreshAll();
  showToast("Đã làm mới trạng thái.");
});
$("#refresh-history").addEventListener("click", loadHistory);
$("#add-station").addEventListener("click", () => $("#station-dialog").showModal());

$("#open-project").addEventListener("click", async () => {
  const station = selectedStation();
  if (!station) return;
  try {
    await apiPost(`/api/stations/${station.id}/open`);
    showToast("Đã mở thư mục dự án.");
  } catch (error) {
    showToast(error.message);
  }
});

$("#cloudflare-pill").addEventListener("click", async () => {
  if (state.cloudflare?.connected) {
    showToast("Phiên đăng nhập Cloudflare đã được lưu và sẵn sàng phát hành.");
    return;
  }
  try {
    const job = await apiPost("/api/cloudflare/connect");
    monitorJob(job.id);
  } catch (error) {
    showToast(error.message);
  }
});

document.querySelectorAll("[data-action]").forEach((button) => {
  button.addEventListener("click", () => runAction(button.dataset.action));
});

async function selectStation(id) {
  state.selectedId = id;
  state.device = null;
  renderStationList($("#station-search").value);
  renderSelectedStation();
  await loadDevice();
}

refreshAll();
