import json
import sys
import os
from datetime import datetime

# Verwendung: python create_log.py <raw_rmse_value> <output_path>

if len(sys.argv) < 3:
    print("Usage: python create_log.py <rmse_score> <output_path>")
    sys.exit(1)

raw_rmse = sys.argv[1].strip()
output_path = sys.argv[2]

# Metadaten von der Pipeline (GitHub Actions) oder Fallbacks
version = os.environ.get("bench_version", "0.0.0-unknown")
commit_sha = os.environ.get("GITHUB_SHA", "local-dev")
timestamp = datetime.now().isoformat()

# Validierung: Sicherstellen, dass der RMSE ein valider Wert ist
try:
    rmse_float = float(raw_rmse)
except ValueError:
    print(f"Error: Provided RMSE '{raw_rmse}' is not a valid number.")
    sys.exit(1)

# Datenstruktur aufbauen
log_data = {
    "version": version,
    "rmse_value": rmse_float,
    "timestamp": timestamp,
    "commit": commit_sha[:8],
}

# In Datei schreiben mit den Markern für das nächste Skript
try:
    with open(output_path, "w") as f:
        f.write("---JSON_START---\n")
        json.dump(log_data, f, indent=2)
        f.write("\n---JSON_END---\n")
    print(f"Successfully created log: {output_path}")
except Exception as e:
    print(f"Error writing log file: {e}")
    sys.exit(1)
