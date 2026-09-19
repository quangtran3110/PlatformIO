#pragma once
#include <ESP8266WiFi.h>
extern "C" {
#include <lwip/dns.h>
#include <lwip/tcp.h>
}
#include <BlynkApiArduino.h>
#include <Blynk/BlynkProtocol.h>
#include "network_health.h"

// lwIP callbacks only copy bounded bytes/update socket state. Application work
// stays in loop(). No synchronous DNS, connect(), readBytes(), flush() or GET().
template <size_t RX, size_t TX> class StationTcp {
public:
  enum Phase : uint8_t { IDLE, DNS, CONNECTING, OPEN };
  Phase phase = IDLE;
  uint8_t rx[RX], tx[TX];
  size_t rxSize = 0, txSize = 0;
  uint32_t started = 0, oldestByte = 0, queuedAt = 0;
  bool failed = false;

  bool active() const { return phase != IDLE; }
  bool ready() const { return phase == OPEN; }

  bool start(const char *host, uint16_t port) {
    if (active() || dnsPending || !host || strlen(host) >= sizeof(queryHost))
      return false;
    stop();
    failed = false;
    started = millis();
    targetPort = port;
    strcpy(queryHost, host);
    phase = DNS;
    queryGeneration = generation;
    ip_addr_t address;
    err_t result = dns_gethostbyname(queryHost, &address, resolved, this);
    if (result == ERR_OK) openSocket(&address);
    else if (result == ERR_INPROGRESS) dnsPending = true;
    else fail();
    return active();
  }

  void stop() {
    ++generation; // A DNS callback from a cancelled attempt cannot open a socket.
    closeSocket();
    rxSize = txSize = 0;
  }

  bool queue(const void *bytes, size_t count) {
    if (!active() || count > TX - txSize) return false;
    if (!txSize) queuedAt = millis();
    memcpy(tx + txSize, bytes, count);
    txSize += count;
    return true;
  }

  void consume(void *bytes, size_t count) {
    memcpy(bytes, rx, count);
    rxSize -= count;
    memmove(rx, rx + count, rxSize);
  }

  void service() {
    uint32_t now = millis();
    if (active() && (WiFi.status() != WL_CONNECTED ||
        (!ready() && uint32_t(now - started) >= 3000) ||
        (txSize && uint32_t(now - queuedAt) >= 1500))) {
      fail();
      return;
    }
    if (!ready() || !txSize) return;
    size_t count = txSize < tcp_sndbuf(pcb) ? txSize : tcp_sndbuf(pcb);
    if (!count) return;
    err_t result = tcp_write(pcb, tx, count, TCP_WRITE_FLAG_COPY);
    if (result == ERR_MEM) return; // Try on a later loop, never wait here.
    if (result != ERR_OK) { fail(); return; }
    txSize -= count;
    memmove(tx, tx + count, txSize);
    result = tcp_output(pcb);
    if (result != ERR_OK && result != ERR_MEM) fail();
  }

  void fail() { failed = true; stop(); }

private:
  tcp_pcb *pcb = nullptr;
  char queryHost[64] = {};
  uint16_t targetPort = 0;
  uint32_t generation = 0, queryGeneration = 0;
  bool dnsPending = false;

  void closeSocket() {
    phase = IDLE;
    if (!pcb) return;
    tcp_arg(pcb, nullptr);
    tcp_recv(pcb, nullptr);
    tcp_err(pcb, nullptr);
    tcp_abort(pcb);
    pcb = nullptr;
  }

  void openSocket(const ip_addr_t *address) {
    if (!address || !(pcb = tcp_new())) { fail(); return; }
    phase = CONNECTING;
    tcp_arg(pcb, this);
    tcp_recv(pcb, received);
    tcp_err(pcb, errored);
    tcp_nagle_disable(pcb);
    if (tcp_connect(pcb, address, targetPort, connected) != ERR_OK) fail();
  }

  static void resolved(const char *, const ip_addr_t *address, void *arg) {
    auto *self = static_cast<StationTcp *>(arg);
    self->dnsPending = false;
    if (self->phase == DNS && self->queryGeneration == self->generation)
      self->openSocket(address);
  }

  static err_t connected(void *arg, tcp_pcb *, err_t error) {
    auto *self = static_cast<StationTcp *>(arg);
    if (error != ERR_OK) { self->fail(); return ERR_ABRT; }
    self->phase = OPEN;
    return ERR_OK;
  }

  static err_t received(void *arg, tcp_pcb *socket, pbuf *packet, err_t error) {
    auto *self = static_cast<StationTcp *>(arg);
    if (!packet) {
      // Preserve already-received HTTP response bytes across a peer FIN.
      self->closeSocket();
      return ERR_ABRT;
    }
    if (error != ERR_OK || packet->tot_len > RX - self->rxSize) {
      pbuf_free(packet);
      self->fail();
      return ERR_ABRT;
    }
    if (!self->rxSize) self->oldestByte = millis();
    pbuf_copy_partial(packet, self->rx + self->rxSize, packet->tot_len, 0);
    self->rxSize += packet->tot_len;
    tcp_recved(socket, packet->tot_len);
    pbuf_free(packet);
    return ERR_OK;
  }

  static void errored(void *arg, err_t) {
    auto *self = static_cast<StationTcp *>(arg);
    self->pcb = nullptr; // lwIP has already freed it.
    self->phase = IDLE;
    self->failed = true;
    self->rxSize = self->txSize = 0;
  }
};

class StationBlynkTransport {
public:
  StationTcp<1024, 2048> socket;
  NetworkHealth health;
  uint16_t probeId = 0, acknowledgedId = 0;
  uint32_t probeStarted = 0;
  uint8_t framesRead = 0;

  void begin(const char *host, uint16_t port) {
    disconnect();
    strncpy(domain, host, sizeof(domain) - 1);
    domain[sizeof(domain) - 1] = 0;
    targetPort = port;
  }

  bool connect() {
    if (int32_t(millis() - retryAt) < 0) return false;
    if (!socket.start(domain, targetPort)) return false;
    // Blynk may queue its login while DNS/TCP are pending. service() sends it
    // only after TCP opens; connection/login still have their own deadlines.
    return true;
  }

  void disconnect() {
    if (socket.active()) deferRetry();
    socket.stop();
    probeId = 0;
    health.service(millis(), false);
  }

  bool connected() { return socket.active(); }

  size_t write(const void *bytes, size_t count) {
    return socket.queue(bytes, count) ? count : 0;
  }

  int available() {
    if (framesRead >= 4 || !socket.ready() || socket.rxSize < 5) return 0;
    size_t length = (size_t(socket.rx[3]) << 8) | socket.rx[4];
    size_t frame = socket.rx[0] == BLYNK_CMD_RESPONSE ? 5 : 5 + length;
    if (frame > BLYNK_MAX_READBYTES + 5) { socket.fail(); return 0; }
    return socket.rxSize >= frame ? int(frame) : 0;
  }

  size_t read(void *bytes, size_t count) {
    if (count > socket.rxSize) return 0;
    if (count == 5) ++framesRead;
    if (count == 5 && socket.rx[0] == BLYNK_CMD_RESPONSE) {
      uint16_t id = (uint16_t(socket.rx[1]) << 8) | socket.rx[2];
      uint16_t status = (uint16_t(socket.rx[3]) << 8) | socket.rx[4];
      if (probeId && id == probeId) {
        if (status == BLYNK_SUCCESS) {
          health.reply(millis(), uint32_t(millis() - probeStarted));
          acknowledgedId = id;
          retryDelay = 5000;
        } else health.fail();
        probeId = 0;
      }
    }
    socket.consume(bytes, count);
    return count;
  }

  void service() {
    framesRead = 0; // Bound incoming application work per loop.
    socket.service();
    if ((probeId && uint32_t(millis() - probeStarted) > 1500) ||
        (socket.rxSize && uint32_t(millis() - socket.oldestByte) > 1500))
      socket.fail();
    if (socket.failed) {
      socket.failed = false;
      probeId = 0;
      health.fail();
      deferRetry();
    }
  }

private:
  char domain[64] = {};
  uint16_t targetPort = 80;
  uint32_t retryAt = 0, retryDelay = 5000;

  void deferRetry() {
    retryAt = millis() + retryDelay;
    retryDelay = retryDelay < 30000 ? retryDelay * 2 : 60000;
  }
};

class StationBlynk : public BlynkProtocol<StationBlynkTransport> {
public:
  explicit StationBlynk(StationBlynkTransport &transport)
    : BlynkProtocol<StationBlynkTransport>(transport) {}

  void config(const char *auth) {
    BlynkProtocol<StationBlynkTransport>::begin(auth);
    conn.begin("sgp1.blynk.cloud", 80);
  }
};

static StationBlynkTransport stationNetwork;
StationBlynk Blynk(stationNetwork);
#include <BlynkWidgets.h>

class StationHttp {
public:
  StationTcp<512, 768> socket;
  bool busy = false;
  int result = 0;
  uint32_t started = 0;

  bool send(const String &path) {
    if (busy || result || path.length() > 620 || WiFi.status() != WL_CONNECTED)
      return false;
    result = 0;
    started = millis();
    if (!socket.start("sgp1.blynk.cloud", 80)) return false;
    String request; request.reserve(path.length() + 100);
    request = F("GET /external/api/");
    request += path;
    request += F(" HTTP/1.1\r\nHost: sgp1.blynk.cloud\r\nConnection: close\r\n\r\n");
    if (!socket.queue(request.c_str(), request.length())) {
      socket.stop();
      return false;
    }
    busy = true;
    return true;
  }

  void service() {
    if (!busy) return;
    socket.service();
    // Parse only a complete HTTP header; no blocking body download.
    for (size_t k = 3; k < socket.rxSize; ++k) {
      if (socket.rx[k - 3] == '\r' && socket.rx[k - 2] == '\n' &&
          socket.rx[k - 1] == '\r' && socket.rx[k] == '\n') {
        if (socket.rxSize >= 12 &&
            (!memcmp(socket.rx, "HTTP/1.1 ", 9) ||
             !memcmp(socket.rx, "HTTP/1.0 ", 9)) &&
            socket.rx[9] >= '1' && socket.rx[9] <= '5' &&
            socket.rx[10] >= '0' && socket.rx[10] <= '9' &&
            socket.rx[11] >= '0' && socket.rx[11] <= '9')
          result = (socket.rx[9] - '0') * 100 +
                   (socket.rx[10] - '0') * 10 + socket.rx[11] - '0';
        if (!result) result = -1;
        finish();
        return;
      }
    }
    if (socket.failed || !socket.active() ||
        uint32_t(millis() - started) >= 1500) {
      result = -1; // Delivery is uncertain: never retry this command automatically.
      finish();
    }
  }

  void cancel() { result = -1; finish(); }

private:
  void finish() { socket.stop(); busy = false; }
};
