// ============================================================================
// remoteid_receiver.ino — XIAO ESP32-S3 Remote ID WiFi uztvērējs
// ============================================================================
// Uztver WiFi Beacon un NaN Action rāmjus ar OpenDroneID payload, dekodē tos
// un izvada CSV formātā Serial portā, saderīgs ar DroneTag Rider shēmu.
//
// Laiku iegūst no datora (Python logger) caur Serial komandu "TIME <ms>\n".
//
// Autors: Krišjānis Baumanis, RTU bakalaura darbs
// Atsauces: opendroneid-core-c, lukeswitz/WiFi-RemoteID
// ============================================================================

#include <Arduino.h>
#include "config.h"
#include "wifi_capture.h"
#include "odid_parser.h"
#include "csv_output.h"
#include "host_time.h"

ODIDParser odid_parser;

// Raw payload saglabāšana CSV rindai (izejot no ISR konteksta)
struct PendingFrame {
  uint8_t mac[6];
  int8_t rssi;
  uint8_t payload[MAX_ODID_PAYLOAD_SIZE];
  size_t payload_len;
  bool ready;
};

volatile PendingFrame g_pending = {};
portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;

// Vai CSV header jau izdrukāts (pēc laika saņemšanas)
bool g_header_printed = false;

// ISR callback no WiFi promiscuous
void onODIDFrame(const uint8_t *mac, int8_t rssi,
                 const uint8_t *payload, size_t payload_len) {
  portENTER_CRITICAL_ISR(&g_mux);
  if (!g_pending.ready && payload_len <= MAX_ODID_PAYLOAD_SIZE) {
    memcpy((void *)g_pending.mac, mac, 6);
    g_pending.rssi = rssi;
    memcpy((void *)g_pending.payload, payload, payload_len);
    g_pending.payload_len = payload_len;
    g_pending.ready = true;
  }
  portEXIT_CRITICAL_ISR(&g_mux);
}

unsigned long last_channel_hop = 0;

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);

  // PIEZĪME: setup() laikā vēl neizvadām nevienu lietu kā CSV rindu,
  // tikai komentārus (# prefikss) un gaidām "TIME" komandu no datora.

  host_time.begin();  // Izdrukā "# READY", gaida TIME komandu
  odid_parser.begin();
  wifi_capture.begin(onODIDFrame);

  last_channel_hop = millis();
}

void loop() {
  // 1. Atjauninām host_time — saņemam iespējamas TIME komandas no datora
  host_time.update();

  // 2. Ja laiks tikko kļuva derīgs un header vēl nav izdrukāts — izdrukā tagad
  if (!g_header_printed && host_time.hasValidTime()) {
    csv_output.printHeader();
    g_header_printed = true;
  }

  // 3. Kanālu pārslēgšana
  /*
  if (millis() - last_channel_hop >= CHANNEL_HOP_INTERVAL_MS) {
    wifi_capture.hopChannel();
    last_channel_hop = millis();
  }
  */

  // 4. Apstrādājam rāmjus TIKAI pēc tam, kad laiks ir derīgs
  if (g_pending.ready && host_time.hasValidTime()) {
    // Drošā kopija no volatile bufera
    uint8_t mac[6];
    int8_t rssi;
    uint8_t payload[MAX_ODID_PAYLOAD_SIZE];
    size_t payload_len;

    portENTER_CRITICAL(&g_mux);
    memcpy(mac, (const void *)g_pending.mac, 6);
    rssi = g_pending.rssi;
    payload_len = g_pending.payload_len;
    memcpy(payload, (const void *)g_pending.payload, payload_len);
    g_pending.ready = false;
    portEXIT_CRITICAL(&g_mux);

    
    // Parsējam ODID payload
    int msg_type = odid_parser.parsePayload(payload, payload_len);
    if (msg_type < 0) {
      return;
    }

    // Izvada CSV rindu
    csv_output.printRow(mac, rssi, payload, payload_len,
                        odid_parser.getData(), (uint8_t)msg_type);
  } else if (g_pending.ready && !host_time.hasValidTime()) {
    // Laika vēl nav — atmest rāmi, lai izvairītos no bufera bloķēšanas
    portENTER_CRITICAL(&g_mux);
    g_pending.ready = false;
    portEXIT_CRITICAL(&g_mux);
  }

  delay(1);
}
