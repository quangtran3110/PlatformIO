import assert from "node:assert/strict";
import test from "node:test";
import worker from "./src/index.js";

const firmwareBytes = new Uint8Array([1, 2, 3, 4]);
const manifest = {
  version: "260926.2",
  objectKey: "tram-cc/releases/260926.2/firmware.bin",
  size: firmwareBytes.byteLength,
  md5: "08d6c05a21512a79a1dfeb9d2a8f262f",
  sha256: "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a",
};

function environment() {
  return {
    TRAM_CC_OTA_KEY: "device-key",
    FIRMWARE: {
      async get(key, type) {
        if (key === "tram-cc/latest.json") {
          assert.equal(type, "json");
          return manifest;
        }
        if (key === manifest.objectKey) {
          assert.equal(type, "stream");
          return firmwareBytes;
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
  assert.equal(response.headers.get("x-md5"), manifest.md5);
  assert.equal(response.headers.get("x-firmware-sha256"), manifest.sha256);
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
