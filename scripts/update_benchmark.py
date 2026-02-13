import sys
import csv
import os
import json

# Usage: python update_benchmark.py ssim_results.json data-branch/history.csv

if len(sys.argv) < 3:
    print("Error: Missing Args.")
    print("Usage: python update_benchmark.py <json_file> <csv_file>")
    sys.exit(1)

json_path = sys.argv[1]
csv_path = sys.argv[2]


def read_benchmark_json(path):
    try:
        with open(path, "r") as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Error: File {path} not found.")
    except json.JSONDecodeError as e:
        print(f"Error: Faulty JSON-Format{path}: {e}")
    except Exception as e:
        print(f"Unexpected Error: {e}")
    return None


# read data
data = read_benchmark_json(json_path)

if not data:
    sys.exit(1)

# prepare data
row = {
    "date": data.get("timestamp", "N/A"),
    "version": data.get("version", "unknown"),
    "rmse": data.get("rmse_value", 0.0),
    "commit": data.get("commit", "unknown"),
}

# write to csv
file_exists = os.path.isfile(csv_path)
directory = os.path.dirname(csv_path)

if directory and not os.path.exists(directory):
    os.makedirs(directory)

try:
    with open(csv_path, mode="a", newline="") as csvfile:
        fieldnames = ["date", "version", "rmse", "commit"]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

        if not file_exists:
            writer.writeheader()

        writer.writerow(row)
    print(f"Success: {row['version']} (RMSE: {row['rmse']}) was added to {csv_path}.")
except Exception as e:
    print(f"Error while writing CSV: {e}")
    sys.exit(1)
