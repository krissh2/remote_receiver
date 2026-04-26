// ============================================================================
// config.h — Uztvērēja konfigurācija
// ============================================================================
// SVARĪGI: Pirms katra eksperimenta pārbaudi recv_lat, recv_lon un SENSOR_ID!
// Ja uztvērējs pārvietots, šīs vērtības JĀATJAUNO.
// ============================================================================

#ifndef CONFIG_H
#define CONFIG_H

// ---------- Uztvērēja identitāte ----------
#define SENSOR_ID       "ESP32S3_RX_01"     // Unikāls ID šim uztvērējam
#define RECV_TYPE       "ESP32-S3"           // Uztvērēja tips (var atstāt tukšu)
#define RECV_ID         0                    // Ja vairāki uztvērēji — unikāls skaits

// ---------- Uztvērēja atrašanās vieta (HARDKODĒTA!) ----------
// Atjauno pirms katra eksperimenta, ja uztvērējs pārvietots!
#define RECV_LAT        56.9496              // Rīga (piemērs — NOMAINI!)
#define RECV_LON        24.1052              // Rīga (piemērs — NOMAINI!)

// ---------- WiFi kanālu skenēšana ----------
#define CHANNEL_HOP_INTERVAL_MS   250        // Kanāla maiņas intervāls (ms)
#define WIFI_CHANNEL_MIN          1          // ES: kanāli 1-13
#define WIFI_CHANNEL_MAX          13

// ---------- Serial izvade ----------
#define SERIAL_BAUD     921600  // Ātrs baud rate liela CSV apjoma pārsūtīšanai

// ---------- Pufera izmēri ----------
#define MAX_ODID_PAYLOAD_SIZE   256    // Max ODID payload no WiFi rāmja
#define CSV_LINE_BUFFER_SIZE    2048   // Viena CSV rinda ar Base64 var būt gara

// ---------- Debug ----------
#define DEBUG_PRINT     0    // 0 = tikai CSV izvade, 1 = arī debug info

#endif // CONFIG_H
