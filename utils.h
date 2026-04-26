// ============================================================================
// utils.h — Haversine attālums un Base64 kodēšana
// ============================================================================

#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <math.h>
#include <stdint.h>
#include <stddef.h>

// Haversine attālums metros starp diviem (lat, lon) punktiem
inline double haversineMeters(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0;  // Zemes rādiuss metros
  double dLat = (lat2 - lat1) * M_PI / 180.0;
  double dLon = (lon2 - lon1) * M_PI / 180.0;
  double a = sin(dLat / 2) * sin(dLat / 2) +
             cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
             sin(dLon / 2) * sin(dLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}

// Base64 kodēšana — izmanto mbedtls (ESP32 core iebūvēts)
#include "mbedtls/base64.h"

inline int base64Encode(const uint8_t *input, size_t input_len,
                        char *output, size_t output_size) {
  size_t olen = 0;
  int ret = mbedtls_base64_encode((unsigned char *)output, output_size, &olen,
                                  input, input_len);
  if (ret == 0) {
    output[olen] = '\0';
    return (int)olen;
  }
  return -1;
}

// MAC formatēšana uz virkni "xx:xx:xx:xx:xx:xx"
inline void formatMAC(const uint8_t *mac, char *buf) {
  snprintf(buf, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

#endif // UTILS_H
