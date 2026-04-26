// ============================================================================
// wifi_capture.h — WiFi promiscuous režīms Remote ID rāmju uztveršanai
// ============================================================================
// Uztver divus rāmju tipus:
//  1. 802.11 Beacon rāmjus ar vendor-specific IE (OUI: FA:0B:BC vai CID: Cursor
//     ON Dron standarts) — izmanto DJI un citi "Direct Remote ID" ražotāji
//  2. 802.11 NaN Action rāmjus ar OpenDroneID service — izmanto vairāki
//     add-on moduļi
//
// Rāmji tiek nodoti odid_parser tālākai dekodēšanai.
// ============================================================================

#ifndef WIFI_CAPTURE_H
#define WIFI_CAPTURE_H

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <esp_wifi_types.h>   // wifi_promiscuous_pkt_type_t definīcijai

// Callback tipa definīcija: izsauc, kad atrasts ODID payload rāmī
// Parametri: MAC (6 baiti), RSSI, payload buferis, payload garums
typedef void (*ODIDFrameCallback)(const uint8_t *mac, int8_t rssi,
                                  const uint8_t *payload, size_t payload_len);

class WiFiCapture {
public:
  void begin(ODIDFrameCallback cb);
  void hopChannel();              // Pāriet uz nākamo kanālu
  uint8_t currentChannel() const { return _channel; }

private:
  static void IRAM_ATTR promiscuousCallback(void *buf, wifi_promiscuous_pkt_type_t type);
  static WiFiCapture *_instance;
  static ODIDFrameCallback _user_callback;

  uint8_t _channel = 1;
};

extern WiFiCapture wifi_capture;

#endif // WIFI_CAPTURE_H