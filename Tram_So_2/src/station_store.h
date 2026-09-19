#pragma once
#include <Arduino.h>
#include <I2C_eeprom.h>
#include <stddef.h>
#include <string.h>

// Same 128-byte slots as the old store. The sequence is committed last.
// Two formerly-unused words hold a format marker and CRC, including sequence.
template<class T, size_t CheckOffset> class StationStore {
  static_assert(CheckOffset + 8 <= sizeof(T), "EEPROM checksum outside data layout");
  static constexpr uint32_t MARKER = 0x32534F53UL;
  static constexpr uint32_t EMPTY = 0xFFFFFFFFUL;
  I2C_eeprom *ee = nullptr;
  uint16_t slotSize = 0, slots = 0, currentSlot = 0;
  uint32_t sequence = 0;
  bool loaded = false, legacy = false;
  bool (*valid)(const T &) = nullptr;
  void (*service)() = nullptr;

  uint32_t checksum(uint32_t seq, const T &value) const {
    uint32_t crc = 0xFFFFFFFFUL;
    auto add = [&crc](uint8_t byte) {
      crc ^= byte;
      for (uint8_t k = 0; k < 8; ++k)
        crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1)));
    };
    for (uint8_t k = 0; k < 4; ++k) add(uint8_t(seq >> (k * 8)));
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&value);
    for (size_t k = 0; k < sizeof(T); ++k)
      if (k < CheckOffset + 4 || k >= CheckOffset + 8) add(bytes[k]);
    return ~crc;
  }
  bool checked(uint32_t seq, const T &value) const {
    uint32_t words[2];
    memcpy(words, reinterpret_cast<const uint8_t *>(&value) + CheckOffset, 8);
    return words[0] == MARKER && words[1] == checksum(seq, value);
  }
  bool put(uint16_t address, const uint8_t *bytes, uint16_t count) {
    if (ee->writeBlock(address, bytes, count) != 0) return false;
    // The installed library's verifier accepts a short read. Check length here.
    uint8_t check[16];
    for (uint16_t offset = 0; offset < count;) {
      uint8_t part = count - offset > 16 ? 16 : count - offset;
      if (service) service();
      if (ee->readBlock(address + offset, check, part) != part ||
          memcmp(check, bytes + offset, part) != 0) return false;
      offset += part;
    }
    return true;
  }
public:
  bool begin(I2C_eeprom &device, uint8_t pageSize, uint16_t totalPages,
             T &value, bool (*validator)(const T &), void (*hook)()) {
    ee = &device; valid = validator; service = hook;
    slotSize = ((sizeof(T) + 4 + pageSize - 1) / pageSize) * pageSize;
    slots = uint32_t(pageSize) * totalPages / slotSize;
    loaded = false;
    T candidate;
    for (uint16_t slot = 0; slot < slots; ++slot) {
      if (service) service();
      uint32_t seq;
      if (ee->readBlock(slot * slotSize, reinterpret_cast<uint8_t *>(&seq), 4) != 4)
        return false;
      if (seq == EMPTY) continue;
      if (ee->readBlock(slot * slotSize + 4, reinterpret_cast<uint8_t *>(&candidate),
                        sizeof(T)) != sizeof(T)) return false;
      if (!valid(candidate)) continue;
      bool old = candidate.version == 3;
      if (!old && !checked(seq, candidate)) continue;
      if (!loaded || (legacy && !old) ||
          (legacy == old && seq > sequence)) {
        memcpy(&value, &candidate, sizeof(T));
        sequence = seq; currentSlot = slot; legacy = old; loaded = true;
      }
      yield();
    }
    return loaded; // Never format, choose defaults, or write during scanning.
  }
  bool needsMigration() const { return legacy; }
  bool write(T &value) {
    if (!loaded || sequence >= EMPTY - 1 || !valid(value) || value.version != 4)
      return false;
    const uint16_t nextSlot = (currentSlot + 1) % slots;
    const uint16_t address = nextSlot * slotSize;
    const uint32_t nextSequence = sequence + 1, invalid = EMPTY;
    uint32_t words[] = {MARKER, 0};
    memcpy(reinterpret_cast<uint8_t *>(&value) + CheckOffset, words, 8);
    words[1] = checksum(nextSequence, value);
    memcpy(reinterpret_cast<uint8_t *>(&value) + CheckOffset, words, 8);
    // A reused legacy slot has no CRC: invalidate its version before its sequence.
    const uint8_t invalidVersion = 0;
    if (!put(address + 4 + offsetof(T, version), &invalidVersion, 1))
      return false;
    if (service) service();
    // Invalidate the reused slot before touching its payload.
    if (!put(address, reinterpret_cast<const uint8_t *>(&invalid), 4))
      return false;
    if (service) service();
    if (!put(address + 4,
          reinterpret_cast<const uint8_t *>(&value), sizeof(T))) return false;
    if (service) service();
    if (!put(address,
          reinterpret_cast<const uint8_t *>(&nextSequence), 4)) return false;
    currentSlot = nextSlot; sequence = nextSequence; legacy = false;
    return true;
  }
};
