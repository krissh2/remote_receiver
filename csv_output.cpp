// ============================================================================
// csv_output.cpp
// ============================================================================

#include "csv_output.h"
#include "config.h"
#include "host_time.h"
#include "utils.h"
#include <string.h>

CSVOutput csv_output;

void CSVOutput::printHeader() {
  Serial.println(
    "sensor_id,mac,rssi,server_timestamp,recv_id,recv_type,recv_lat,recv_lon,"
    "msg_type,uas_time,uas_timestamp,uas_timestamp_estimated,uas_lat,uas_lon,"
    "uas_geoalt,uas_baroalt,uas_height,op_lat,op_lon,op_alt,op_timestamp,"
    "op_timestamp_estimated,op_speed,op_td,operator_distance_movement,"
    "odid_raw,timestamp"
  );
}

// Palīgfunkcija: aizpilda "" ja vērtība nav derīga, citādi drukā skaitli
static void printDoubleOrEmpty(double v, int precision, bool valid) {
  if (valid && !isnan(v)) {
    Serial.print(v, precision);
  }
  // Ja nav derīga — neko nedrukā (tukša vērtība starp komatiem)
}

void CSVOutput::printRow(const uint8_t *mac, int8_t rssi,
                         const uint8_t *raw_payload, size_t raw_len,
                         const ODID_UAS_Data &uas,
                         uint8_t msg_type) {
  char buf[64];

  // --- 1-8: uztvērēja metadati ---
  Serial.print(SENSOR_ID);
  Serial.print(',');

  formatMAC(mac, buf);
  Serial.print(buf);
  Serial.print(',');

  Serial.print((int)rssi);
  Serial.print(',');

  Serial.print((unsigned long long)host_time.getUnixMillis());
  Serial.print(',');

  Serial.print(RECV_ID);
  Serial.print(',');

  Serial.print(RECV_TYPE);
  Serial.print(',');

  Serial.print(RECV_LAT, 7);
  Serial.print(',');

  Serial.print(RECV_LON, 7);
  Serial.print(',');

  // --- 9: msg_type ---
  Serial.print(msg_type);
  Serial.print(',');

  // --- 10-17: UAS (drona) dati no Location ziņojuma ---
  if (uas.LocationValid) {
    // uas_time: ODID Location.TimeStamp ir dekasekundes kopš pilnas stundas
    // (0-6000 nozīmē 0-600.0 sekundes pēc pēdējās UTC stundas).
    // DroneTag šo pārveido par pilnu ISO 8601.
    float tstamp = uas.Location.TimeStamp;
    if (tstamp >= 0) {
      // Aprēķinām aktuālo UAS timestamp:
      // ņemam tagadējo UTC stundu un pievienojam sekundes no ODID
      uint64_t now_ms = host_time.getUnixMillis();
      time_t now_sec = now_ms / 1000;
      struct tm *tm_info = gmtime(&now_sec);
      tm_info->tm_min = 0;
      tm_info->tm_sec = 0;
      time_t hour_start = mktime(tm_info);
      time_t uas_sec = hour_start + (time_t)tstamp;
      uint32_t uas_us = (uint32_t)((tstamp - (int)tstamp) * 1000000);

      struct tm *uas_tm = gmtime(&uas_sec);
      char tb[32];
      strftime(tb, sizeof(tb), "%Y-%m-%d %H:%M:%S", uas_tm);
      Serial.print(tb);
      Serial.printf(".%06lu+00:00", (unsigned long)uas_us);
      Serial.print(',');

      // uas_timestamp (Unix float sekundēs)
      double uas_unix = (double)uas_sec + (double)uas_us / 1000000.0;
      Serial.print(uas_unix, 6);
      Serial.print(',');

      Serial.print("False,");  // uas_timestamp_estimated (no ODID = reāls)
    } else {
      Serial.print(",,True,");
    }

    Serial.print(uas.Location.Latitude, 7);
    Serial.print(',');
    Serial.print(uas.Location.Longitude, 7);
    Serial.print(',');

    // uas_geoalt (ģeometriskais augstums WGS-84 virs jūras līmeņa)
    if (uas.Location.AltitudeGeo != INV_ALT) {
      Serial.print(uas.Location.AltitudeGeo, 1);
    }
    Serial.print(',');

    // uas_baroalt — DJI parasti nepārraida, paliek tukšs
    if (uas.Location.AltitudeBaro != INV_ALT) {
      Serial.print(uas.Location.AltitudeBaro, 1);
    }
    Serial.print(',');

    // uas_height (augstums virs starta punkta / virs zemes)
    if (uas.Location.Height != INV_ALT) {
      Serial.print(uas.Location.Height, 1);
    }
    Serial.print(',');
  } else {
    // Nav derīgu Location datu — 8 tukšas kolonnas
    Serial.print(",,,,,,,,");
  }

  // --- 18-24: operatora dati no System ziņojuma ---
  double op_lat_now = 0, op_lon_now = 0;
  bool op_valid = false;

  if (uas.SystemValid) {
    op_lat_now = uas.System.OperatorLatitude;
    op_lon_now = uas.System.OperatorLongitude;
    op_valid = (op_lat_now != 0.0 || op_lon_now != 0.0);

    Serial.print(op_lat_now, 7);
    Serial.print(',');
    Serial.print(op_lon_now, 7);
    Serial.print(',');

    // op_alt (operatora augstums, ja pieejams)
    if (uas.System.OperatorAltitudeGeo != INV_ALT) {
      Serial.print(uas.System.OperatorAltitudeGeo, 1);
    }
    Serial.print(',');

    // op_timestamp (System ziņojumā ir kopš 2019-01-01, pārvēršam uz ISO)
    // DroneTag atzīmē to kā "estimated=True" parasti, jo sistēmas ziņojumā
    // laiks ir tikai sekunžu precizitātē.
    if (uas.System.Timestamp > 0) {
      // ODID System.Timestamp: sekundes kopš 2019-01-01 00:00:00 UTC
      time_t sys_sec = 1546300800 + uas.System.Timestamp;
      struct tm *sys_tm = gmtime(&sys_sec);
      char tb[32];
      strftime(tb, sizeof(tb), "%Y-%m-%d %H:%M:%S", sys_tm);
      Serial.print(tb);
      Serial.print("+00:00");
    }
    Serial.print(',');

    Serial.print("True,");  // op_timestamp_estimated (tipiski True)

    // op_speed — ODID nepārraida tieši, paliek 0
    Serial.print("0,");

    // op_td (track direction?) — paliek 0 līdz noskaidrots
    Serial.print("0,");
  } else {
    Serial.print(",,,,,,,");  // 7 tukšas kolonnas
  }

  // --- 25: operator_distance_movement (Haversine no pirmās redzētās pozīcijas) ---
  if (op_valid) {
    if (!_op_first_valid) {
      _op_first_lat = op_lat_now;
      _op_first_lon = op_lon_now;
      _op_first_valid = true;
      Serial.print("0.0");
    } else {
      double dist = haversineMeters(_op_first_lat, _op_first_lon,
                                    op_lat_now, op_lon_now);
      Serial.print(dist, 2);
    }
  }
  Serial.print(',');

  // --- 26: odid_raw (Base64 no nolasītā payload) ---
  if (raw_payload && raw_len > 0) {
    // Base64 izmērs = ceil(raw_len / 3) * 4 + 1
    size_t b64_size = ((raw_len + 2) / 3) * 4 + 8;
    char *b64 = (char *)malloc(b64_size);
    if (b64) {
      if (base64Encode(raw_payload, raw_len, b64, b64_size) > 0) {
        Serial.print('"');
        Serial.print(b64);
        Serial.print('"');
      }
      free(b64);
    }
  }
  Serial.print(',');

  // --- 27: timestamp (ISO 8601 ar mikrosekundēm) ---
  char iso[40];
  host_time.formatISO8601(iso, sizeof(iso));
  Serial.print(iso);

  Serial.println();
}
