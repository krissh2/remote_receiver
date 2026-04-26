// ============================================================================
// host_time.cpp
// ============================================================================

#include "host_time.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "esp_timer.h"   // esp_timer_get_time() — 64-bit mikrosekundes kopš boot

HostTime host_time;

void HostTime::begin() {
  _time_valid = false;
  _cmd_len = 0;
  // Paziņo datora pusei, ka esam gatavi saņemt laiku
  Serial.println("# READY");
}

void HostTime::update() {
  // Lasām komandas no datora pa rakstzīmei
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '\n' || c == '\r') {
      if (_cmd_len > 0) {
        _cmd_buf[_cmd_len] = '\0';
        processCommand(_cmd_buf);
        _cmd_len = 0;
      }
    } else if (_cmd_len < sizeof(_cmd_buf) - 1) {
      _cmd_buf[_cmd_len++] = c;
    } else {
      // Pārāk gara komanda — resetējam buferi
      _cmd_len = 0;
    }
  }
}

void HostTime::processCommand(const char *cmd) {
  // Atbalsta tikai "TIME <unix_millis>"
  if (strncmp(cmd, "TIME ", 5) == 0) {
    const char *num = cmd + 5;
    uint64_t unix_ms = strtoull(num, nullptr, 10);
    if (unix_ms > 1700000000000ULL) {  // Sanity check: vismaz 2023. gads
      // Fiksējam pašreizējo 64-bit mikrosekunžu skaitītāju kā anchor
      _anchor_micros = (uint64_t)esp_timer_get_time();
      _anchor_unix_ms = unix_ms;
      _time_valid = true;

      Serial.print("# TIME_OK ");
      Serial.println((unsigned long long)unix_ms);
    } else {
      Serial.println("# TIME_ERR invalid value");
    }
  }
}

uint64_t HostTime::getUnixMillis() const {
  if (!_time_valid) return 0;
  uint64_t now_us = (uint64_t)esp_timer_get_time();
  uint64_t elapsed_us = now_us - _anchor_micros;
  return _anchor_unix_ms + (elapsed_us / 1000ULL);
}

void HostTime::formatISO8601(char *buf, size_t buf_size) const {
  if (!_time_valid) {
    buf[0] = '\0';
    return;
  }

  // Aprēķinām kopējo Unix laiku mikrosekundēs
  uint64_t now_us = (uint64_t)esp_timer_get_time();
  uint64_t elapsed_us = now_us - _anchor_micros;
  uint64_t total_unix_us = (uint64_t)_anchor_unix_ms * 1000ULL + elapsed_us;

  time_t t_sec = (time_t)(total_unix_us / 1000000ULL);
  uint32_t us_frac = (uint32_t)(total_unix_us % 1000000ULL);

  struct tm *tm_info = gmtime(&t_sec);
  char tmp[32];
  strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", tm_info);
  snprintf(buf, buf_size, "%s.%06lu+00:00", tmp, (unsigned long)us_frac);
}
