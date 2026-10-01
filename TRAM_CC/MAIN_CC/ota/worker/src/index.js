const STATIONS = Object.freeze({
  "tram-cc": { secretName: "TRAM_CC_OTA_KEY" },
  "tram-binh-hiep": { secretName: "TRAM_BINH_HIEP_OTA_KEY" },
  "tram-so-1": { secretName: "TRAM_SO_1_OTA_KEY" },
  "tram-so-2": { secretName: "TRAM_SO_2_OTA_KEY" },
  "tram-so-3-vfd": { secretName: "TRAM_SO_3_VFD_OTA_KEY" },
  "tram-so-4": { secretName: "TRAM_SO_4_OTA_KEY" },
  "volume-tram1": { secretName: "VOLUME_TRAM1_OTA_KEY" },
  "volume-tram-cc-g1": { secretName: "VOLUME_TRAM_CC_G1_OTA_KEY" },
  "volume-tram-cc-g2": { secretName: "VOLUME_TRAM_CC_G2_OTA_KEY" },
  "volume-tram-cc-g3": { secretName: "VOLUME_TRAM_CC_G3_OTA_KEY" },
  "volume-tram2-g1": { secretName: "VOLUME_TRAM2_G1_OTA_KEY" },
  "volume-tram2-g2": { secretName: "VOLUME_TRAM2_G2_OTA_KEY" },
  "volume-tram2-g3": { secretName: "VOLUME_TRAM2_G3_OTA_KEY" },
  "volume-tram2bpt": { secretName: "VOLUME_TRAM2BPT_OTA_KEY" },
  "volume-tram3": { secretName: "VOLUME_TRAM3_OTA_KEY" },
  "volume-tram3bpt": {
    secretName: "VOLUME_TRAM3BPT_OTA_KEY",
    bootstrapSecretName: "VOLUME_TRAM3BPT_BOOTSTRAP_KEY",
  },
  "volume-tram4": { secretName: "VOLUME_TRAM4_OTA_KEY" },
  "volume-trambhd": { secretName: "VOLUME_TRAMBHD_OTA_KEY" },
});

function notFound() {
  return new Response("Not found", {
    status: 404,
    headers: { "Cache-Control": "no-store" },
  });
}

function bytesOf(value) {
  return new TextEncoder().encode(value);
}

async function sameSecret(provided, expected) {
  if (!provided || !expected) return false;
  const [left, right] = await Promise.all([
    crypto.subtle.digest("SHA-256", bytesOf(provided)),
    crypto.subtle.digest("SHA-256", bytesOf(expected)),
  ]);
  const a = new Uint8Array(left);
  const b = new Uint8Array(right);
  let difference = a.length ^ b.length;
  for (let index = 0; index < a.length; index += 1) {
    difference |= a[index] ^ b[index];
  }
  return difference === 0;
}

function validManifest(value, stationId) {
  const baseValid = Boolean(
    value &&
      typeof value.version === "string" &&
      /^\d{6}\.\d+$/.test(value.version) &&
      typeof value.objectKey === "string" &&
      value.objectKey.startsWith(`${stationId}/releases/`) &&
      typeof value.size === "number" &&
      value.size > 0 &&
      typeof value.md5 === "string" &&
      /^[a-f0-9]{32}$/.test(value.md5) &&
      typeof value.sha256 === "string" &&
      /^[a-f0-9]{64}$/.test(value.sha256),
  );
  if (!baseValid) return false;

  const hasChunkField = value.chunkSize !== undefined ||
    value.chunkCount !== undefined || value.chunkPrefix !== undefined;
  if (!hasChunkField) return true;
  return Boolean(
    Number.isInteger(value.chunkSize) && value.chunkSize >= 1 && value.chunkSize <= 16384 &&
      Number.isInteger(value.chunkCount) && value.chunkCount === Math.ceil(value.size / value.chunkSize) &&
      typeof value.chunkPrefix === "string" &&
      value.chunkPrefix.startsWith(`${stationId}/releases/`) &&
      value.chunkPrefix.endsWith("/chunks"),
  );
}

async function hasValidStationSecret(provided, station, env) {
  const permanent = sameSecret(provided, env[station.secretName]);
  const bootstrap = station.bootstrapSecretName
    ? sameSecret(provided, env[station.bootstrapSecretName])
    : Promise.resolve(false);
  const [permanentValid, bootstrapValid] = await Promise.all([permanent, bootstrap]);
  return permanentValid || bootstrapValid;
}

function parseRange(value, size) {
  if (!value) return null;
  const match = /^bytes=(\d+)-(\d*)$/.exec(value.trim());
  if (!match) return false;

  const start = Number(match[1]);
  const requestedEnd = match[2] ? Number(match[2]) : size - 1;
  if (!Number.isSafeInteger(start) || !Number.isSafeInteger(requestedEnd) ||
      start < 0 || start >= size || requestedEnd < start) {
    return false;
  }
  return {
    start,
    end: Math.min(requestedEnd, size - 1),
  };
}


function logOta(event, details) {
  console.log(JSON.stringify({ event, ...details }));
}

async function loadManifest(env, stationId) {
  try {
    const manifest = await env.FIRMWARE.get(`${stationId}/latest.json`, "json");
    return validManifest(manifest, stationId) ? manifest : null;
  } catch {
    return null;
  }
}

function firmwareHeaders(manifest) {
  return new Headers({
    "Content-Type": "application/octet-stream",
    "Cache-Control": "private, no-store, max-age=0",
    "Accept-Ranges": "bytes",
    "X-MD5": manifest.md5,
    "X-Firmware-Version": manifest.version,
    "X-Firmware-SHA256": manifest.sha256,
    ETag: `"${manifest.sha256}"`,
  });
}

async function loadFirmwareRange(env, manifest, range) {
  if (!manifest.chunkSize || !manifest.chunkPrefix) {
    const firmware = await env.FIRMWARE.get(manifest.objectKey, "arrayBuffer");
    if (!firmware || firmware.byteLength !== manifest.size) return null;
    return firmware.slice(range.start, range.end + 1);
  }

  const firstChunk = Math.floor(range.start / manifest.chunkSize);
  const lastChunk = Math.floor(range.end / manifest.chunkSize);
  const chunks = await Promise.all(
    Array.from({ length: lastChunk - firstChunk + 1 }, (_, offset) =>
      env.FIRMWARE.get(`${manifest.chunkPrefix}/${firstChunk + offset}`, {
        type: "arrayBuffer",
        cacheTtl: 300,
      }),
    ),
  );
  if (chunks.some((chunk) => !chunk)) return null;

  const body = new Uint8Array(range.end - range.start + 1);
  let outputOffset = 0;
  for (let index = firstChunk; index <= lastChunk; index += 1) {
    const chunk = new Uint8Array(chunks[index - firstChunk]);
    const expectedLength = Math.min(manifest.chunkSize, manifest.size - index * manifest.chunkSize);
    if (chunk.byteLength !== expectedLength) return null;
    const from = index === firstChunk ? range.start % manifest.chunkSize : 0;
    const to = index === lastChunk ? (range.end % manifest.chunkSize) + 1 : chunk.byteLength;
    body.set(chunk.subarray(from, to), outputOffset);
    outputOffset += to - from;
  }
  return body.buffer;
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const match = /^\/([a-z0-9-]+)\/([^/]+)\/firmware\.bin$/.exec(url.pathname);
    if (!match || (request.method !== "GET" && request.method !== "HEAD")) {
      return notFound();
    }

    const stationId = match[1];
    const station = STATIONS[stationId];
    if (!station || !(await hasValidStationSecret(decodeURIComponent(match[2]), station, env))) {
      return notFound();
    }

    const manifest = await loadManifest(env, stationId);
    if (!manifest) {
      logOta("ota_error", { stationId, stage: "manifest_validate", status: 503 });
      return new Response("Invalid firmware manifest", { status: 503 });
    }

    const headers = firmwareHeaders(manifest);

    let range = parseRange(request.headers.get("Range"), manifest.size);
    if (range === false) {
      headers.set("Content-Range", `bytes */${manifest.size}`);
      logOta("ota_range_rejected", {
        stationId,
        version: manifest.version,
        method: request.method,
        status: 416,
        size: manifest.size,
      });
      return new Response(null, { status: 416, headers });
    }

    if (range) {
      const body = await loadFirmwareRange(env, manifest, range);
      if (!body) {
        logOta("ota_error", {
          stationId,
          version: manifest.version,
          stage: "firmware_read",
          status: 503,
        });
        return new Response("Firmware unavailable", { status: 503 });
      }
      headers.set("Content-Length", String(body.byteLength));
      headers.set("Content-Range", `bytes ${range.start}-${range.end}/${manifest.size}`);
      logOta("ota_range", {
        stationId,
        version: manifest.version,
        method: request.method,
        status: 206,
        rangeStart: range.start,
        rangeEnd: range.end,
        bytes: body.byteLength,
        size: manifest.size,
      });
      return new Response(request.method === "HEAD" ? null : body, {
        status: 206,
        headers,
      });
    }

    if (request.method === "HEAD") {
      headers.set("Content-Length", String(manifest.size));
      logOta("ota_full", {
        stationId,
        version: manifest.version,
        method: request.method,
        status: 200,
        bytes: 0,
        size: manifest.size,
      });
      return new Response(null, { status: 200, headers });
    }

    const firmware = await env.FIRMWARE.get(manifest.objectKey, "stream");
    if (!firmware) {
      logOta("ota_error", {
        stationId,
        version: manifest.version,
        stage: "firmware_read",
        status: 503,
      });
      return new Response("Firmware unavailable", { status: 503 });
    }
    headers.set("Content-Length", String(manifest.size));
    logOta("ota_full", {
      stationId,
      version: manifest.version,
      method: request.method,
      status: 200,
      bytes: manifest.size,
      size: manifest.size,
    });
    return new Response(firmware, {
      status: 200,
      headers,
    });
  },
};
