// ============================================================================
// wifi_capture.cpp — TĪRA RAŽOŠANAS VERSIJA
// ============================================================================
// WiFi promiscuous režīms Remote ID rāmju uztveršanai.
// Atbalsta gan ASTM F3411 OUI (FA:0B:BC) — drone-swarmer un DJI,
// gan Wi-Fi Alliance OUI (50:6F:9A) — daži add-on moduļi.
//
// Pareizi izgriež OUI(3) + vendor_type(1) + beacon_counter(1) = 5 baiti
// pirms nodot ODID payload parsētājam.
//
// Pašlaik kanāls fiksēts uz 6 (drone-swarmer raida tikai uz 6).
// ============================================================================

#include "wifi_capture.h"
#include "config.h"
#include <WiFi.h>
#include <esp_wifi.h>

WiFiCapture wifi_capture;
WiFiCapture *WiFiCapture::_instance = nullptr;
ODIDFrameCallback WiFiCapture::_user_callback = nullptr;

// ASTM F3411 OUI (drone-swarmer, DJI un citi reāli droni)
static const uint8_t ODID_ASTM_OUI[3]    = { 0xFA, 0x0B, 0xBC };
// Wi-Fi Alliance OUI (rezerves variants)
static const uint8_t WIFI_ALLIANCE_OUI[3] = { 0x50, 0x6F, 0x9A };
// NaN service ID hash for "org.opendroneid.remoteid"
static const uint8_t ODID_NAN_SERVICE_ID[6] = { 0x88, 0x69, 0x19, 0x9D, 0x92, 0x09 };

// IEEE 802.11 frame control type/subtype
#define FC_TYPE_MGMT        0x00
#define FC_SUBTYPE_BEACON   0x08
#define FC_SUBTYPE_ACTION   0x0D

void WiFiCapture::begin(ODIDFrameCallback cb) {
  _instance = this;
  _user_callback = cb;

  // Pilns Station mode — pareizi inicializē WiFi stack
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(200);

  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&WiFiCapture::promiscuousCallback);

  // Filtrs: tikai management rāmji (Beacon, Action, u.c.)
  wifi_promiscuous_filter_t filter = {};
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
  esp_wifi_set_promiscuous_filter(&filter);

  // Kanāls fiksēts uz 6 (drone-swarmer raida tikai uz 6)
  _channel = 6;
  esp_wifi_set_channel(_channel, WIFI_SECOND_CHAN_NONE);
}

void WiFiCapture::hopChannel() {
  _channel++;
  if (_channel > WIFI_CHANNEL_MAX) _channel = WIFI_CHANNEL_MIN;
  esp_wifi_set_channel(_channel, WIFI_SECOND_CHAN_NONE);
}

// ---------- ODID payload meklēšana Beacon IE sarakstā ----------
// Beacon rāmja formāts:
//   [MAC header 24B][timestamp 8B][interval 2B][caps 2B][IEs...]
// IE struktūra:
//   [tag 1B][len 1B][OUI 3B][vendor_type 1B][beacon_counter 1B][ODID payload...]
//                   |       |               |
//                   pos+2   pos+5           pos+6  payload sākas pos+7
static int findODIDBeaconPayload(const uint8_t *frame, size_t frame_len,
                                 const uint8_t **payload_out, size_t *payload_len_out) {
  const size_t IE_OFFSET = 24 + 12;
  if (frame_len < IE_OFFSET + 2) return -1;

  size_t pos = IE_OFFSET;
  while (pos + 2 <= frame_len) {
    uint8_t tag = frame[pos];
    uint8_t len = frame[pos + 1];
    if (pos + 2 + len > frame_len) return -1;

    if (tag == 0xDD && len >= 5) {
      const uint8_t *oui = &frame[pos + 2];
      uint8_t vendor_type = frame[pos + 5];

      if ((memcmp(oui, ODID_ASTM_OUI, 3) == 0 ||
           memcmp(oui, WIFI_ALLIANCE_OUI, 3) == 0) &&
          vendor_type == 0x0D) {
        // Pārlec OUI(3) + vendor_type(1) + beacon_counter(1) = 5 baiti
        *payload_out = &frame[pos + 7];
        *payload_len_out = len - 5;
        return 0;
      }
    }
    pos += 2 + len;
  }
  return -1;
}

// NaN Action rāmja ODID atrašana
static int findODIDNaNPayload(const uint8_t *frame, size_t frame_len,
                              const uint8_t **payload_out, size_t *payload_len_out) {
  const size_t NAN_HDR = 24 + 6;
  if (frame_len < NAN_HDR + 4) return -1;
  if (frame[24] != 0x04 || frame[25] != 0x09) return -1;
  if (memcmp(&frame[26], WIFI_ALLIANCE_OUI, 3) != 0) return -1;
  if (frame[29] != 0x13) return -1;

  size_t pos = NAN_HDR;
  while (pos + 3 <= frame_len) {
    uint8_t attr_id = frame[pos];
    uint16_t attr_len = frame[pos + 1] | (frame[pos + 2] << 8);
    if (pos + 3 + attr_len > frame_len) return -1;

    if (attr_id == 0x03 && attr_len >= 7) {
      if (memcmp(&frame[pos + 3], ODID_NAN_SERVICE_ID, 6) == 0) {
        size_t off = pos + 3 + 7;
        if (off < frame_len) {
          *payload_out = &frame[off];
          *payload_len_out = (pos + 3 + attr_len) - off;
          return 0;
        }
      }
    }
    pos += 3 + attr_len;
  }
  return -1;
}

void IRAM_ATTR WiFiCapture::promiscuousCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;

  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  const uint8_t *frame = pkt->payload;
  size_t frame_len = pkt->rx_ctrl.sig_len;
  int8_t rssi = pkt->rx_ctrl.rssi;

  if (frame_len < 24) return;

  uint8_t fc0 = frame[0];
  uint8_t frame_type    = (fc0 >> 2) & 0x03;
  uint8_t frame_subtype = (fc0 >> 4) & 0x0F;
  if (frame_type != FC_TYPE_MGMT) return;

  const uint8_t *sender_mac = &frame[10];
  const uint8_t *payload = nullptr;
  size_t payload_len = 0;

  if (frame_subtype == FC_SUBTYPE_BEACON) {
    if (findODIDBeaconPayload(frame, frame_len, &payload, &payload_len) != 0) return;
  } else if (frame_subtype == FC_SUBTYPE_ACTION) {
    if (findODIDNaNPayload(frame, frame_len, &payload, &payload_len) != 0) return;
  } else {
    return;
  }

  if (_user_callback && payload && payload_len > 0) {
    _user_callback(sender_mac, rssi, payload, payload_len);
  }
}
