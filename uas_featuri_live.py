"""
UAS Real-Time Feature Engineering Pipeline — LIVE versija
=========================================================
Bazes versija: uas_featuri.py. Atskiribas:
  1. GALVENES-AGNOSTISKS: ja ievades CSV ir bez galvenes (ka XIAO izvads),
     automatiski pielieto 27-lauku uztvereja shemu. Ja galvene ir -> lieto to.
  2. PASSTHROUGH papildus laukiem (rssi, mac, msg_type, uas_baroalt), lai
     detektora R1 (msg_type) un adaptivais noteikums (uas_baroalt) tiem piekluts.
  3. parse_ts pienem gan ISO 8601, gan Unix epoch (sekundes vai milisekundes).

Lietosana:
    python3 uas_featuri_live.py --input raw_live.csv --output feat_live.csv
"""

import csv
import math
import time
import os
import sys
from collections import deque
from datetime import datetime, timezone

# -- Fizikalie limiti --
MAX_SPEED_MPS       = 50.0
MAX_ACCEL_MPS2      = 30.0
MAX_VERT_SPEED_MPS  = 15.0
MAX_STEP_DISTANCE_M = 200.0
MAX_DT_S            = 3.0
WINDOW_SIZE         = 5

INPUT_CSV       = "raw_live.csv"
OUTPUT_CSV      = "feat_live.csv"
POLL_INTERVAL_S = 0.1

# -- 27-lauku uztvereja shema (no flights-raw galvenes) --
SCHEMA_27 = [
    "sensor_id", "mac", "rssi", "server_timestamp", "recv_id", "recv_type",
    "recv_lat", "recv_lon", "msg_type", "uas_time", "uas_timestamp",
    "uas_timestamp_estimated", "uas_lat", "uas_lon", "uas_geoalt",
    "uas_baroalt", "uas_height", "op_lat", "op_lon", "op_alt",
    "op_timestamp", "op_timestamp_estimated", "op_speed", "op_td",
    "operator_distance_movement", "odid_raw", "timestamp",
]
# Lauki, pec kuriem nosakam, vai pirma rinda ir galvene
HEADER_MARKERS = {"uas_lat", "sensor_id", "timestamp", "msg_type"}

EARTH_R = 6_371_000.0

def haversine(lat1, lon1, lat2, lon2):
    phi1, phi2 = math.radians(lat1), math.radians(lat2)
    dphi = math.radians(lat2 - lat1)
    dlam = math.radians(lon2 - lon1)
    a = math.sin(dphi/2)**2 + math.cos(phi1)*math.cos(phi2)*math.sin(dlam/2)**2
    return 2 * EARTH_R * math.asin(math.sqrt(a))

def bearing(lat1, lon1, lat2, lon2):
    phi1, phi2 = math.radians(lat1), math.radians(lat2)
    dlam = math.radians(lon2 - lon1)
    x = math.sin(dlam) * math.cos(phi2)
    y = math.cos(phi1)*math.sin(phi2) - math.sin(phi1)*math.cos(phi2)*math.cos(dlam)
    return (math.degrees(math.atan2(x, y)) + 360) % 360

def angular_diff(a1, a2):
    return (a2 - a1 + 180) % 360 - 180

def elevation_angle(h_dist_m, alt_uas_m, alt_rcv_m=0.0):
    if h_dist_m < 0.001:
        return 90.0
    return math.degrees(math.atan2(alt_uas_m - alt_rcv_m, h_dist_m))

def slant_distance(h_dist_m, dalt_m):
    return math.sqrt(h_dist_m**2 + dalt_m**2)

def variance(values):
    if len(values) < 2:
        return 0.0
    m = sum(values) / len(values)
    return sum((v - m)**2 for v in values) / len(values)

def stddev(values):
    return math.sqrt(variance(values))

def safe_float(v, default=None):
    try:
        return float(v)
    except (TypeError, ValueError):
        return default

def parse_ts(s):
    """Pienem ISO 8601 VAI Unix epoch (s vai ms)."""
    s = str(s).strip()
    # Mēģinam ka skaitlis (Unix epoch)
    try:
        val = float(s)
        if val > 1e11:        # > ~2001. gads ms -> milisekundes
            val /= 1000.0
        return datetime.fromtimestamp(val, tz=timezone.utc)
    except ValueError:
        pass
    for fmt in ("%Y-%m-%d %H:%M:%S.%f%z", "%Y-%m-%d %H:%M:%S%z"):
        try:
            return datetime.strptime(s, fmt)
        except ValueError:
            pass
    return datetime.fromisoformat(s)

# -- Kolonnu definicijas --
# Passthrough: bazes 8 + papildus konteksts detektoram
INPUT_COLS = [
    "timestamp", "uas_lat", "uas_lon", "uas_geoalt",
    "recv_lat", "recv_lon",
    "op_lat", "op_lon",
    "rssi", "mac", "msg_type", "uas_baroalt",
]

FEATURE_COLS = [
    "uas_dt_s", "uas_step_distance", "uas_speed_mps", "uas_vertical_speed_mps",
    "uas_3d_speed_mps", "uas_accel_mps2", "uas_vertical_accel_mps2",
    "uas_3d_accel_mps2", "uas_heading_deg", "distance_to_receiver",
    "bearing_to_receiver", "bearing_to_receiver_rate",
    "elevation_angle_to_receiver", "dist_to_operator", "bearing_to_operator",
    "operator_uas_bearing", "speed_variance_w5", "straight_line_ratio",
    "tortuosity_w5", "trajectory_smoothness", "dt_anomaly_flag",
    "timestamp_seq_flag", "speed_feasibility_flag", "accel_feasibility_flag",
    "altitude_jump_flag", "duplicate_position_flag", "position_jump_flag",
]

ALL_COLS = INPUT_COLS + FEATURE_COLS

# -- Stavoklis --
prev_ts = prev_lat = prev_lon = prev_alt = None
prev_speed = prev_vspeed = prev_3dspeed = prev_heading = None
prev_b2r = last_ts_raw = None
speed_window = deque(maxlen=WINDOW_SIZE)
pos_window   = deque(maxlen=WINDOW_SIZE)


def read_all_rows(path):
    """Galvenes-agnostiska nolasisana. Atgriez sarakstu ar dict."""
    with open(path, "r", newline="") as f:
        first = f.readline()
        if not first:
            return []
        tokens = [t.strip() for t in first.strip().split(",")]
        has_header = len(HEADER_MARKERS & set(tokens)) >= 2
        f.seek(0)
        if has_header:
            return list(csv.DictReader(f))
        # Nav galvenes -> pielietojam SCHEMA_27
        rows = []
        for parts in csv.reader(f):
            if not parts:
                continue
            if len(parts) < len(SCHEMA_27):
                # nepilna (varbut daleji ierakstita) rinda -> izlaiz
                continue
            rows.append(dict(zip(SCHEMA_27, parts)))
        return rows


def compute_features(row):
    global prev_ts, prev_lat, prev_lon, prev_alt
    global prev_speed, prev_vspeed, prev_3dspeed, prev_heading
    global prev_b2r, last_ts_raw

    row = {k.strip(): v for k, v in row.items()}

    lat   = safe_float(row.get("uas_lat"))
    lon   = safe_float(row.get("uas_lon"))
    alt   = safe_float(row.get("uas_geoalt"), 0.0)
    r_lat = safe_float(row.get("recv_lat"))
    r_lon = safe_float(row.get("recv_lon"))
    o_lat = safe_float(row.get("op_lat"))
    o_lon = safe_float(row.get("op_lon"))

    if lat is None or lon is None:
        raise ValueError(f"lat/lon truksts vai nelasams: uas_lat={row.get('uas_lat')!r} uas_lon={row.get('uas_lon')!r}")

    ts   = parse_ts(row["timestamp"])
    feat = {c: "" for c in FEATURE_COLS}

    feat["timestamp_seq_flag"] = 1 if (last_ts_raw is not None and ts <= last_ts_raw) else 0
    last_ts_raw = ts

    if prev_ts is not None:
        dt = (ts - prev_ts).total_seconds()
        feat["uas_dt_s"]        = round(dt, 6)
        feat["dt_anomaly_flag"] = 1 if (dt <= 0 or dt > MAX_DT_S) else 0

        h_dist = haversine(prev_lat, prev_lon, lat, lon)
        feat["uas_step_distance"]       = round(h_dist, 4)
        feat["duplicate_position_flag"] = 1 if (lat == prev_lat and lon == prev_lon and alt == prev_alt) else 0
        feat["position_jump_flag"]      = 1 if h_dist > MAX_STEP_DISTANCE_M else 0

        if dt > 0:
            spd   = h_dist / dt
            vspd  = (alt - prev_alt) / dt
            spd3d = slant_distance(h_dist, alt - prev_alt) / dt
        else:
            spd = vspd = spd3d = 0.0

        feat["uas_speed_mps"]          = round(spd,   4)
        feat["uas_vertical_speed_mps"] = round(vspd,  4)
        feat["uas_3d_speed_mps"]       = round(spd3d, 4)

        if prev_speed is not None and dt > 0:
            feat["uas_accel_mps2"]          = round((spd   - prev_speed)  / dt, 4)
            feat["uas_vertical_accel_mps2"] = round((vspd  - prev_vspeed) / dt, 4)
            feat["uas_3d_accel_mps2"]       = round((spd3d - prev_3dspeed)/ dt, 4)

        if h_dist > 0.01:
            hdg = bearing(prev_lat, prev_lon, lat, lon)
            feat["uas_heading_deg"] = round(hdg, 2)
            prev_heading = hdg
        else:
            feat["uas_heading_deg"] = round(prev_heading, 2) if prev_heading is not None else ""

        feat["speed_feasibility_flag"] = 1 if spd > MAX_SPEED_MPS else 0
        a_val = feat["uas_accel_mps2"]
        feat["accel_feasibility_flag"] = 1 if (a_val != "" and abs(float(a_val)) > MAX_ACCEL_MPS2) else 0
        feat["altitude_jump_flag"]     = 1 if abs(vspd) > MAX_VERT_SPEED_MPS else 0

        speed_window.append(spd)
        prev_speed, prev_vspeed, prev_3dspeed = spd, vspd, spd3d

    if r_lat is not None and r_lon is not None:
        h   = haversine(lat, lon, r_lat, r_lon)
        b2r = bearing(lat, lon, r_lat, r_lon)
        feat["distance_to_receiver"]        = round(slant_distance(h, alt), 2)
        feat["bearing_to_receiver"]         = round(b2r, 2)
        feat["elevation_angle_to_receiver"] = round(elevation_angle(h, alt), 2)
        if prev_b2r is not None and prev_ts is not None:
            dt_b = (ts - prev_ts).total_seconds()
            if dt_b > 0:
                feat["bearing_to_receiver_rate"] = round(angular_diff(prev_b2r, b2r) / dt_b, 4)
        prev_b2r = b2r

    if o_lat is not None and o_lon is not None:
        h = haversine(lat, lon, o_lat, o_lon)
        feat["dist_to_operator"]     = round(slant_distance(h, alt), 2)
        feat["bearing_to_operator"]  = round(bearing(lat, lon, o_lat, o_lon), 2)
        feat["operator_uas_bearing"] = round(bearing(o_lat, o_lon, lat, lon), 2)

    pos_window.append((lat, lon))
    if len(speed_window) >= 2:
        sl = list(speed_window)
        feat["speed_variance_w5"]     = round(variance(sl), 6)
        feat["trajectory_smoothness"] = round(stddev(sl), 4)

    if len(pos_window) >= 2:
        actual = sum(
            haversine(pos_window[i][0], pos_window[i][1],
                      pos_window[i+1][0], pos_window[i+1][1])
            for i in range(len(pos_window) - 1)
        )
        straight = haversine(pos_window[0][0], pos_window[0][1],
                             pos_window[-1][0], pos_window[-1][1])
        if actual > 0.001:
            ratio = round(straight / actual, 4)
            feat["straight_line_ratio"] = ratio
            feat["tortuosity_w5"]       = round(1.0 / ratio, 4) if ratio > 0 else ""

    prev_ts, prev_lat, prev_lon, prev_alt = ts, lat, lon, alt
    return feat


def run(input_path=INPUT_CSV, output_path=OUTPUT_CSV):
    print(f"[UAS Live] Skatas : {input_path}")
    print(f"[UAS Live] Raksta : {output_path}")
    print("[UAS Live] Ctrl-C lai apturetu.\n")

    processed_rows = 0
    out_file = out_writer = None

    while True:
        try:
            if not os.path.exists(input_path):
                time.sleep(POLL_INTERVAL_S)
                continue

            all_rows = read_all_rows(input_path)
            new_rows = all_rows[processed_rows:]
            if not new_rows:
                time.sleep(POLL_INTERVAL_S)
                continue

            if out_file is None:
                out_file   = open(output_path, "w", newline="")
                out_writer = csv.DictWriter(out_file, fieldnames=ALL_COLS)
                out_writer.writeheader()
                out_file.flush()

            for row in new_rows:
                try:
                    feat    = compute_features(row)
                    out_row = {k: row.get(k, "") for k in INPUT_COLS}
                    out_row.update(feat)
                    out_writer.writerow(out_row)
                    out_file.flush()
                    processed_rows += 1
                    print(f"  [{processed_rows:4d}] {str(row.get('timestamp',''))[:26]}"
                          f"  spd={feat.get('uas_speed_mps','')} m/s"
                          f"  msg={row.get('msg_type','')}")
                except Exception as row_err:
                    processed_rows += 1
                    print(f"[IZLAIZ rinda {processed_rows}] {row_err}", file=sys.stderr)

        except KeyboardInterrupt:
            print("\n[UAS Live] Apturets.")
            if out_file:
                out_file.close()
            break
        except Exception as e:
            print(f"[KLUDA] {e}", file=sys.stderr)
            time.sleep(1.0)


if __name__ == "__main__":
    import argparse
    p = argparse.ArgumentParser(description="UAS live feature pipeline")
    p.add_argument("--input",  default=INPUT_CSV,  help="Ievades CSV cels")
    p.add_argument("--output", default=OUTPUT_CSV, help="Izvades CSV cels")
    args = p.parse_args()
    run(args.input, args.output)