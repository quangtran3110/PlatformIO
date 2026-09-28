const STATIONS = Object.freeze({
  "tram-cc": { secretName: "TRAM_CC_OTA_KEY" },
  "tram-so-2": { secretName: "TRAM_SO_2_OTA_KEY" },
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
  return Boolean(
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
    if (!station || !(await sameSecret(decodeURIComponent(match[2]), env[station.secretName]))) {
      return notFound();
    }

    let manifest;
    try {
      manifest = await env.FIRMWARE.get(`${stationId}/latest.json`, "json");
    } catch {
      return new Response("Invalid firmware manifest", { status: 503 });
    }
    if (!validManifest(manifest, stationId)) {
      return new Response("Invalid firmware manifest", { status: 503 });
    }

    const firmware = await env.FIRMWARE.get(manifest.objectKey, "stream");
    if (!firmware) {
      return new Response("Firmware unavailable", { status: 503 });
    }

    const headers = new Headers({
      "Content-Type": "application/octet-stream",
      "Content-Length": String(manifest.size),
      "Cache-Control": "private, no-store, max-age=0",
      "X-MD5": manifest.md5,
      "X-Firmware-Version": manifest.version,
      "X-Firmware-SHA256": manifest.sha256,
      ETag: `"${manifest.sha256}"`,
    });
    return new Response(request.method === "HEAD" ? null : firmware, {
      status: 200,
      headers,
    });
  },
};
