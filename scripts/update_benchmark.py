import sys
import csv
import os
import json

if len(sys.argv) < 3:
    sys.exit(1)

json_path, csv_path = sys.argv[1], sys.argv[2]

with open(json_path, "r") as f:
    results = json.load(f)

file_exists = os.path.isfile(csv_path)
os.makedirs(os.path.dirname(csv_path), exist_ok=True)

with open(csv_path, mode="a", newline="") as csvfile:
    # New schema with iterations
    fieldnames = ["date", "version", "mode", "rmse", "iterations", "commit"]
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

    if not file_exists:
        writer.writeheader()

    for entry in results:
        v_full = entry["version"]
        parts = v_full.rsplit("-", 1)
        base_v, mode_label = parts[0], parts[1] if len(parts) > 1 else "default"

        writer.writerow(
            {
                "date": entry["timestamp"],
                "version": base_v,
                "mode": mode_label,
                "rmse": entry["rmse_value"],
                "iterations": entry.get("iterations", 0),
                "commit": entry["commit"],
            }
        )
