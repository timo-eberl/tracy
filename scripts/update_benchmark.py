import sys
import csv
import os
import json
import re

# Verwendung: python update_benchmark.py ssim_results.txt data-branch/history.csv

if len(sys.argv) < 3:
    print("Fehler: Fehlende Argumente.")
    sys.exit(1)

log_path = sys.argv[1]
csv_path = sys.argv[2]


def extract_json(path):
    try:
        with open(path, "r") as f:
            content = f.read()
            # Findet alles zwischen den JSON-Tags
            match = re.search(
                r"---JSON_START---(.*?)---JSON_END---", content, re.DOTALL
            )
            if match:
                return json.loads(match.group(1).strip())
    except Exception as e:
        print(f"Parsing Fehler: {e}")
    return None


# Daten extrahieren
data = extract_json(log_path)

if not data:
    print("Keine validen JSON-Daten gefunden!")
    sys.exit(1)

# CSV schreiben
file_exists = os.path.isfile(csv_path)

with open(csv_path, mode="a", newline="") as csvfile:
    # Wir definieren die Spalten, die wir in der CSV haben wollen
    fieldnames = ["date", "version", "rmse", "commit"]
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

    if not file_exists:
        writer.writeheader()

    writer.writerow(
        {
            "date": data.get("timestamp"),
            "version": data.get("version"),
            "rmse": data.get("rmse_value"),
            "commit": data.get("commit"),
        }
    )

print(f"Erfolg: Daten für {data.get('version')} importiert.")
