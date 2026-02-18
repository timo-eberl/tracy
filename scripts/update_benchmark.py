import sys
import csv
import os
import json

if len(sys.argv) < 3:
    print("Usage: python update_benchmark.py <json_file> <csv_file>")
    sys.exit(1)

json_path = sys.argv[1]
csv_path = sys.argv[2]

try:
    with open(json_path, "r") as f:
        runs = json.load(f)
except Exception as e:
    print(f"Error reading JSON: {e}")
    sys.exit(1)

file_exists = os.path.isfile(csv_path)
os.makedirs(os.path.dirname(csv_path), exist_ok=True)

try:
    with open(csv_path, mode="a", newline="") as csvfile:
        fieldnames = ["date", "version", "rmse", "commit"]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()

        for run in runs:
            writer.writerow(
                {
                    "date": run["timestamp"],
                    "version": run["version"],
                    "rmse": run["rmse_value"],
                    "commit": run["commit"],
                }
            )
    print(f"Successfully updated history with {len(runs)} entries.")
except Exception as e:
    print(f"Error writing CSV: {e}")
    sys.exit(1)
