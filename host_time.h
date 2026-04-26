// ============================================================================
// host_time.h — UTC laika pārvaldnieks (laiks no datora caur Serial)
// ============================================================================
// ESP32 pie starta sagaida "TIME <unix_millis>\n" komandu no datora (Python
// logger puses). Pēc tam glabā offset starp datora UTC un iekšējo millis().
//
// Datora puse var periodiski atjaunināt laiku, lai kompensētu ESP32 kristāla
// drift (parasti ~20-50 ppm, t.i., ~1-3 sekundes dienā).
//
// Protokola formāts (no datora uz ESP32):
//   TIME <unix_millis>\n    — uzstāda pašreizējo UTC laiku
//
// Protokola formāts (no ESP32 uz datoru):
//   # READY                 — ESP32 gatavs saņemt laiku
//   # TIME_OK <unix_millis>  — apstiprinājums, ka laiks uzstādīts
// ============================================================================

#ifndef HOST_TIME_H
#define HOST_TIME_H

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

class HostTime {
public:
  void begin();
  void update();                          // Izsauc loop() iekšā bieži

  bool hasValidTime() const { return _time_valid; }

  // Atgriež pašreizējo Unix timestamp milisekundēs (UTC)
  uint64_t getUnixMillis() const;

  // Aizpilda ISO 8601 virkni formā: "2025-10-31 14:37:27.199951+00:00"
  void formatISO8601(char *buf, size_t buf_size) const;

private:
  // Cik sekundes bija Unix laika, kad mēs saņēmām TIME komandu
  uint64_t _anchor_unix_ms = 0;
  // Cik mikrosekundes bija ESP32 iekšējā micros(), kad saņēmām to komandu
  uint64_t _anchor_micros = 0;
  bool _time_valid = false;

  // Komandu lasīšanas buferis
  char _cmd_buf[64];
  uint8_t _cmd_len = 0;

  void processCommand(const char *cmd);
};

extern HostTime host_time;

#endif // HOST_TIME_H
