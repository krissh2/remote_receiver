// ============================================================================
// csv_output.h — DroneTag Rider saderīgas CSV rindas formatēšana
// ============================================================================
// Kolonnu secība (27 kolonnas, atbilstoši DroneTag Rider):
//   sensor_id, mac, rssi, server_timestamp, recv_id, recv_type, recv_lat,
//   recv_lon, msg_type, uas_time, uas_timestamp, uas_timestamp_estimated,
//   uas_lat, uas_lon, uas_geoalt, uas_baroalt, uas_height, op_lat, op_lon,
//   op_alt, op_timestamp, op_timestamp_estimated, op_speed, op_td,
//   operator_distance_movement, odid_raw, timestamp
// ============================================================================

#ifndef CSV_OUTPUT_H
#define CSV_OUTPUT_H

#include <Arduino.h>
#include <stdint.h>
#include "odid_parser.h"

class CSVOutput {
public:
  // Izdrukā CSV header rindu Serial portā (izsauc vienu reizi setup()-ā)
  void printHeader();

  // Izdrukā datu rindu, apvienojot uztvērēja metadatus un ODID saturu
  void printRow(const uint8_t *mac, int8_t rssi,
                const uint8_t *raw_payload, size_t raw_len,
                const ODID_UAS_Data &uas,
                uint8_t msg_type);

private:
  // Pirmā redzētā operatora pozīcija (operator_distance_movement aprēķinam)
  double _op_first_lat = 0;
  double _op_first_lon = 0;
  bool _op_first_valid = false;
};

extern CSVOutput csv_output;

#endif // CSV_OUTPUT_H
