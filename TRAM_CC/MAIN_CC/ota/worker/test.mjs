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
  "tram-binh-hiep": {
    version: "261001.2",
    objectKey: "tram-binh-hiep/releases/261001.2/firmware.bin",
    size: firmwareBytes.byteLength,
    md5: "08d6c05a21512a79a1dfeb9d2a8f262f",
    sha256: "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a",
    chunkSize: 2,
    chunkCount: 2,
    chunkPrefix: "tram-binh-hiep/releases/261001.2/chunks",
  },
  "tram-so-1": {
    version: "260926.2",
    objectKey: "tram-so-1/releases/260926.2/firmware.bin",
    size: firmwareBytes.byteLength,
    md5: "08d6c05a21512a79a1dfeb9d2a8f262f",
    sha256: "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a",
    chunkSize: 2,
    chunkCount: 2,
    chunkPrefix: "tram-so-1/releases/260926.2/chunks",
  },
  "tram-so-2": {
    version: "260928.1",
    objectKey: "tram-so-2/releases/260928.1/firmware.bin",
    size: firmwareBytes.byteLength,
    md5: "08d6c05a21512a79a1dfeb9d2a8f262f",
    sha256: "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a",
    chunkSize: 2,
    chunkCount: 2,
    chunkPrefix: "tram-so-2/releases/260928.1/chunks",
  },
  "tram-so-3-vfd": {
    version: "260929.2",
    objectKey: "tram-so-3-vfd/releases/260929.2/firmware.bin",
    size: firmwareBytes.byteLength,
    md5: "08d6c05a21512a79a1dfeb9d2a8f262f",
    sha256: "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a",
  },
  "tram-so-4": {
    version: "260930.1",
    objectKey: "tram-so-4/releases/260930.1/firmware.bin",
    size: firmwareBytes.byteLength,
    md5: "08d6c05a21512a79a1dfeb9d2a8f262f",
    sha256: "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a",
    chunkSize: 2,
    chunkCount: 2,
    chunkPrefix: "tram-so-4/releases/260930.1/chunks",
  },
  "volume-tram1": {
    version: "261001.2",
    objectKey: "volume-tram1/releases/261001.2/firmware.bin",
    size: firmwareBytes.byteLength,
    md5: "08d6c05a21512a79a1dfeb9d2a8f262f",
    sha256: "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a",
    chunkSize: 2,
    chunkCount: 2,
    chunkPrefix: "volume-tram1/releases/261001.2/chunks",
  },
  "volume-tram3bpt": {
    version: "261001.2",
    objectKey: "volume-tram3bpt/releases/261001.2/firmware.bin",
    size: firmwareBytes.byteLength,
    md5: "08d6c05a21512a79a1dfeb9d2a8f262f",
    sha256: "9f64a747e1b97f131fabb6b447296c9b6f0201e79fb3c5356e6c77e89b6a806a",
    chunkSize: 2,
    chunkCount: 2,
    chunkPrefix: "volume-tram3bpt/releases/261001.2/chunks",
  },
};

function environment(reads = []) {
  return {
    TRAM_CC_OTA_KEY: "device-key",
    TRAM_BINH_HIEP_OTA_KEY: "binh-hiep-key",
    TRAM_SO_1_OTA_KEY: "station-1-key",
    TRAM_SO_2_OTA_KEY: "station-2-key",
    TRAM_SO_3_VFD_OTA_KEY: "station-3-vfd-key",
    TRAM_SO_4_OTA_KEY: "station-4-key",
    VOLUME_TRAM1_OTA_KEY: "volume-tram1-key",
    VOLUME_TRAM1_BOOTSTRAP_KEY: "volume-tram1-bootstrap-key",
    VOLUME_TRAM3BPT_OTA_KEY: "volume-tram3bpt-key",
    VOLUME_TRAM3BPT_BOOTSTRAP_KEY: "volume-tram3bpt-bootstrap-key",
    VOLUME_TRAM2_G2_OTA_KEY: "volume-tram2-g2-key",
    FIRMWARE: {
      async get(key, type) {
        reads.push(key);
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
        if (manifest.chunkPrefix && key.startsWith(`${manifest.chunkPrefix}/`)) {
          assert.equal(type.type, "arrayBuffer");
          assert.equal(type.cacheTtl, 300);
          const index = Number(key.slice(manifest.chunkPrefix.length + 1));
          return firmwareBytes.slice(index * manifest.chunkSize, (index + 1) * manifest.chunkSize).buffer;
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

test("serves full firmware as a fixed-length body with integrity headers", async () => {
  const originalLog = console.log;
  const logs = [];
  console.log = (value) => logs.push(value);
  try {
    const response = await worker.fetch(
      new Request("https://ota.example/tram-cc/device-key/firmware.bin"),
      environment(),
    );
    assert.equal(response.status, 200);
    assert.equal(response.headers.get("content-length"), String(firmwareBytes.byteLength));
    assert.equal(response.headers.get("x-md5"), manifests["tram-cc"].md5);
    assert.equal(response.headers.get("x-firmware-sha256"), manifests["tram-cc"].sha256);
    assert.deepEqual(new Uint8Array(await response.arrayBuffer()), firmwareBytes);

    assert.equal(logs.length, 1);
    const event = JSON.parse(logs[0]);
    assert.equal(event.event, "ota_full");
    assert.equal(event.stationId, "tram-cc");
    assert.equal(event.bytes, firmwareBytes.byteLength);
    assert.equal(logs[0].includes("device-key"), false);
  } finally {
    console.log = originalLog;
  }
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
  const originalLog = console.log;
  const logs = [];
  console.log = (value) => logs.push(value);
  try {
    const reads = [];
    const response = await worker.fetch(
      new Request("https://ota.example/tram-so-2/station-2-key/firmware.bin", {
        headers: { Range: "bytes=1-2" },
      }),
      environment(reads),
    );
    assert.equal(response.status, 206);
    assert.equal(response.headers.get("accept-ranges"), "bytes");
    assert.equal(response.headers.get("content-range"), "bytes 1-2/4");
    assert.equal(response.headers.get("content-length"), "2");
    assert.deepEqual(new Uint8Array(await response.arrayBuffer()), new Uint8Array([2, 3]));
    assert.deepEqual(reads, [
      "tram-so-2/latest.json",
      "tram-so-2/releases/260928.1/chunks/0",
      "tram-so-2/releases/260928.1/chunks/1",
    ]);
    assert.equal(reads.includes(manifests["tram-so-2"].objectKey), false);

    assert.equal(logs.length, 1);
    const event = JSON.parse(logs[0]);
    assert.deepEqual(event, {
      event: "ota_range",
      stationId: "tram-so-2",
      version: "260928.1",
      method: "GET",
      status: 206,
      rangeStart: 1,
      rangeEnd: 2,
      bytes: 2,
      size: 4,
    });
    assert.equal(logs[0].includes("station-2-key"), false);
  } finally {
    console.log = originalLog;
  }
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
  const binhHiep = await worker.fetch(
    new Request("https://ota.example/tram-binh-hiep/binh-hiep-key/firmware.bin", {
      headers: { Range: "bytes=0-3" },
    }),
    environment(),
  );
  assert.equal(binhHiep.status, 206);
  assert.equal(binhHiep.headers.get("x-firmware-version"), "261001.2");

  const binhHiepBootstrap = await worker.fetch(
    new Request("https://ota.example/tram-binh-hiep/binh-hiep-bootstrap-key/firmware.bin"),
    environment(),
  );
  assert.equal(binhHiepBootstrap.status, 404);

  const binhHiepWrongKey = await worker.fetch(
    new Request("https://ota.example/tram-binh-hiep/station-1-key/firmware.bin"),
    environment(),
  );
  assert.equal(binhHiepWrongKey.status, 404);

  const station1 = await worker.fetch(
    new Request("https://ota.example/tram-so-1/station-1-key/firmware.bin", {
      headers: { Range: "bytes=0-3" },
    }),
    environment(),
  );
  assert.equal(station1.status, 206);
  assert.equal(station1.headers.get("x-firmware-version"), "260926.2");

  const station1WrongKey = await worker.fetch(
    new Request("https://ota.example/tram-so-1/station-2-key/firmware.bin"),
    environment(),
  );
  assert.equal(station1WrongKey.status, 404);

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

  const station4 = await worker.fetch(
    new Request("https://ota.example/tram-so-4/station-4-key/firmware.bin", {
      headers: { Range: "bytes=0-3" },
    }),
    environment(),
  );
  assert.equal(station4.status, 206);
  assert.equal(station4.headers.get("x-firmware-version"), "260930.1");

  const station4WrongKey = await worker.fetch(
    new Request("https://ota.example/tram-so-4/station-2-key/firmware.bin"),
    environment(),
  );
  assert.equal(station4WrongKey.status, 404);
});

test("keeps the bootstrap key only for the pending VOLUME station", async () => {
  const permanent = await worker.fetch(
    new Request("https://ota.example/volume-tram1/volume-tram1-key/firmware.bin", {
      headers: { Range: "bytes=0-3" },
    }),
    environment(),
  );
  assert.equal(permanent.status, 206);
  assert.equal(permanent.headers.get("x-firmware-version"), "261001.2");

  const bootstrap = await worker.fetch(
    new Request("https://ota.example/volume-tram1/volume-tram1-bootstrap-key/firmware.bin", {
      headers: { Range: "bytes=0-3" },
    }),
    environment(),
  );
  assert.equal(bootstrap.status, 404);

  const pendingBootstrap = await worker.fetch(
    new Request("https://ota.example/volume-tram3bpt/volume-tram3bpt-bootstrap-key/firmware.bin", {
      headers: { Range: "bytes=0-0" },
    }),
    environment(),
  );
  assert.equal(pendingBootstrap.status, 206);
  assert.equal(pendingBootstrap.headers.get("content-range"), "bytes 0-0/4");
  assert.deepEqual(new Uint8Array(await pendingBootstrap.arrayBuffer()), new Uint8Array([1]));

  const wrongStation = await worker.fetch(
    new Request("https://ota.example/volume-tram2-g2/volume-tram1-bootstrap-key/firmware.bin"),
    environment(),
  );
  assert.equal(wrongStation.status, 404);
});
