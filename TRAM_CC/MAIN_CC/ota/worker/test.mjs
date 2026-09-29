import assert from "node:assert/strict";
import test from "node:test";
import worker from "./src/index.js";

const firmwareBytes = new Uint8Array([1, 2, 3, 4]);
const manifests = {
  "tram-cc": {
    version: "260926.2",
    objectKey: "tram-cc/releases/260926.2/firmware.bin",
    size: firmwareBytes.byteLength,
    md5: "08d6c05a21512a79a1dfeb9d2a8f262f",
    sha256: "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a",
  },
  "tram-so-2": {
    version: "260928.1",
    objectKey: "tram-so-2/releases/260928.1/firmware.bin",
    size: firmwareBytes.byteLength,
    md5: "08d6c05a21512a79a1dfeb9d2a8f262f",
    sha256: "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a",
  },
  "tram-so-3-vfd": {
    version: "260929.2",
    objectKey: "tram-so-3-vfd/releases/260929.2/firmware.bin",
    size: firmwareBytes.byteLength,
    md5: "08d6c05a21512a79a1dfeb9d2a8f262f",
    sha256: "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a",
  },
};

function environment() {
  return {
    TRAM_CC_OTA_KEY: "device-key",
    TRAM_SO_2_OTA_KEY: "station-2-key",
    TRAM_SO_3_VFD_OTA_KEY: "station-3-vfd-key",
    FIRMWARE: {
      async get(key, type) {
        const stationId = key.split("/")[0];
        const manifest = manifests[stationId];
        if (key === `${stationId}/latest.json` && manifest) {
          assert.equal(type, "json");
          return manifest;
        }
        if (key === manifest.objectKey) {
          assert.ok(type === "stream" || type === "arrayBuffer");
          return type === "arrayBuffer" ? firmwareBytes.buffer.slice(0) : firmwareBytes;
        }
        return null;
      },
    },
  };
}

test("rejects a missing or incorrect station key", async () => {
  const missing = await worker.fetch(
    new Request("https://ota.example/tram-cc/wrong/firmware.bin"),
    environment(),
  );
  assert.equal(missing.status, 404);
});

test("streams firmware with integrity headers", async () => {
  const response = await worker.fetch(
    new Request("https://ota.example/tram-cc/device-key/firmware.bin"),
    environment(),
  );
  assert.equal(response.status, 200);
  assert.equal(response.headers.get("x-md5"), manifests["tram-cc"].md5);
  assert.equal(response.headers.get("x-firmware-sha256"), manifests["tram-cc"].sha256);
  assert.deepEqual(new Uint8Array(await response.arrayBuffer()), firmwareBytes);
});

test("HEAD returns headers without a body", async () => {
  const response = await worker.fetch(
    new Request("https://ota.example/tram-cc/device-key/firmware.bin", { method: "HEAD" }),
    environment(),
  );
  assert.equal(response.status, 200);
  assert.equal(response.headers.get("content-length"), String(firmwareBytes.byteLength));
  assert.equal((await response.arrayBuffer()).byteLength, 0);
});

test("serves an exact byte range for resumable OTA", async () => {
  const response = await worker.fetch(
    new Request("https://ota.example/tram-so-2/station-2-key/firmware.bin", {
      headers: { Range: "bytes=1-2" },
    }),
    environment(),
  );
  assert.equal(response.status, 206);
  assert.equal(response.headers.get("accept-ranges"), "bytes");
  assert.equal(response.headers.get("content-range"), "bytes 1-2/4");
  assert.equal(response.headers.get("content-length"), "2");
  assert.deepEqual(new Uint8Array(await response.arrayBuffer()), new Uint8Array([2, 3]));
});

test("rejects an invalid or unsatisfiable range", async () => {
  const response = await worker.fetch(
    new Request("https://ota.example/tram-so-2/station-2-key/firmware.bin", {
      headers: { Range: "bytes=9-12" },
    }),
    environment(),
  );
  assert.equal(response.status, 416);
  assert.equal(response.headers.get("content-range"), "bytes */4");
});

test("keeps firmware and keys isolated per station", async () => {
  const accepted = await worker.fetch(
    new Request("https://ota.example/tram-so-2/station-2-key/firmware.bin"),
    environment(),
  );
  assert.equal(accepted.status, 200);
  assert.equal(accepted.headers.get("x-firmware-version"), "260928.1");

  const wrongStationKey = await worker.fetch(
    new Request("https://ota.example/tram-so-2/device-key/firmware.bin"),
    environment(),
  );
  assert.equal(wrongStationKey.status, 404);

  const station3 = await worker.fetch(
    new Request("https://ota.example/tram-so-3-vfd/station-3-vfd-key/firmware.bin"),
    environment(),
  );
  assert.equal(station3.status, 200);
  assert.equal(station3.headers.get("x-firmware-version"), "260929.2");
});
