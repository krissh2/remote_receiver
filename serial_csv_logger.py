#!/usr/bin/env python3
"""
serial_csv_logger.py — Logger ESP32-S3 Remote ID uztvērējam.

Uzdevumi:
  1. Atver Serial portu uz ESP32-S3.
  2. Sagaida "# READY" sveicienu no ESP32.
  3. Nosūta pašreizējo UTC laiku Unix ms formātā: "TIME <ms>\n".
  4. Periodiski (reizi 60 s) atjaunina laiku, lai kompensētu kristāla drift.
  5. Saglabā visas NE-komentāru rindas (tās, kas nesākas ar "#") CSV failā.

Lietošana:
    python serial_csv_logger.py --port COM3 --output flight_001.csv
    python serial_csv_logger.py --port /dev/ttyACM0 --output flight_001.csv
"""

import argparse
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

try:
    import serial
except ImportError:
    print("TRŪKST: pip install pyserial", file=sys.stderr)
    sys.exit(1)


TIME_RESYNC_INTERVAL_S = 60.0   # Cik bieži pārsūtīt laiku uz ESP32


def unix_ms_now() -> int:
    """Atgriež pašreizējo UTC Unix laiku milisekundēs."""
    return int(time.time() * 1000)


def send_time(ser: serial.Serial) -> None:
    """Nosūta pašreizējo laiku uz ESP32."""
    ms = unix_ms_now()
    cmd = f"TIME {ms}\n".encode("ascii")
    ser.write(cmd)
    ser.flush()
    print(f"[host] Nosūtīts TIME {ms} ({datetime.now(timezone.utc).isoformat()})",
          file=sys.stderr)


def main() -> int:
    parser = argparse.ArgumentParser(description="Remote ID CSV logger")
    parser.add_argument("--port", required=True, help="Serial ports (piem. COM3 vai /dev/ttyACM0)")
    parser.add_argument("--baud", type=int, default=921600, help="Baud rate (default 921600)")
    parser.add_argument("--output", required=True, help="Izvades CSV faila ceļš")
    parser.add_argument("--append", action="store_true", help="Pievienot esošam failam")
    args = parser.parse_args()

    output_path = Path(args.output)
    mode = "ab" if args.append else "wb"

    print(f"[host] Atveram {args.port} @ {args.baud} baud", file=sys.stderr)
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1.0)
    except serial.SerialException as e:
        print(f"KĻŪDA: nevar atvērt {args.port}: {e}", file=sys.stderr)
        return 1

    # ESP32 pēc USB pievienošanas dažreiz restartējas — dodam laiku sāknēšanai
    time.sleep(2.0)
    ser.reset_input_buffer()

    # Gaidām "# READY" sveicienu (vai nekavējoties sūtām laiku pēc timeout)
    print("[host] Gaidām '# READY' no ESP32...", file=sys.stderr)
    ready_deadline = time.time() + 10.0
    got_ready = False
    while time.time() < ready_deadline:
        line = ser.readline()
        if not line:
            continue
        decoded = line.decode("utf-8", errors="replace").strip()
        if decoded.startswith("# READY"):
            got_ready = True
            print("[host] Saņemts READY", file=sys.stderr)
            break
        elif decoded:
            print(f"[esp32] {decoded}", file=sys.stderr)

    if not got_ready:
        print("[host] Brīdinājums: nesaņēma READY, tomēr nosūtām laiku", file=sys.stderr)

    # Nosūtām pirmo laika sinhronizāciju
    send_time(ser)
    last_time_sync = time.time()

    # Atver CSV failu
    with open(output_path, mode) as out_file:
        print(f"[host] Logošana uz {output_path}", file=sys.stderr)
        try:
            while True:
                # Periodiska laika atjaunināšana
                if time.time() - last_time_sync >= TIME_RESYNC_INTERVAL_S:
                    send_time(ser)
                    last_time_sync = time.time()

                line = ser.readline()
                if not line:
                    continue

                # Dekodējam kā tekstu, bet saglabājam kā baitus (lai saglabātu
                # oriģinālos CR/LF apzīmējumus)
                try:
                    decoded = line.decode("utf-8", errors="replace").rstrip()
                except UnicodeDecodeError:
                    continue

                if not decoded:
                    continue

                if decoded.startswith("#"):
                    # Komentāri / diagnostika — drukā stderr pusē, nelogojam CSV
                    print(f"[esp32] {decoded}", file=sys.stderr)
                    continue

                # CSV rinda (header vai datu rinda) — saglabājam failā
                out_file.write(line)
                out_file.flush()

        except KeyboardInterrupt:
            print("\n[host] Pārtraukts ar Ctrl+C", file=sys.stderr)
        finally:
            ser.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
