# Remote ID Receiver

ESP32-S3 firmware for capturing OpenDroneID Remote ID broadcasts over WiFi,
with output compatible with the DroneTag Rider CSV format.

Part of bachelor's thesis at RTU (DITEF), 2026.
Author: Krišjānis Baumanis (231RDB144)

## Hardware
- Seeed XIAO ESP32-S3
- Compatible with WiFi Remote ID broadcasts (ASTM F3411 OUI: FA:0B:BC)

## Files
- `remoteid_receiver.ino` — main Arduino sketch
- `wifi_capture.cpp/.h` — WiFi promiscuous mode capture
- `odid_parser.cpp/.h` — OpenDroneID payload decoder
- `csv_output.cpp/.h` — DroneTag Rider compatible CSV format
- `host_time.cpp/.h` — host time synchronization via Serial
- `serial_csv_logger.py` — Python logger for capturing data to CSV

## Related
- [drone-swarmer fork](https://github.com/krissh2/droneswarmer) — spoofed signal generator
- [opendroneid-core-c](https://github.com/opendroneid/opendroneid-core-c)
