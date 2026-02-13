import json
import sys
import os
from datetime import datetime

# Usage: python create_log.py <raw_rmse_value> <output_path>

if len(sys.argv) < 3:
    print("Usage: python create_log.py <rmse_score> <output_path>")
    sys.exit(1)

raw_rmse = sys.argv[1].strip()
output_path = sys.argv[2]

# Github Actions Pipeline Metadata
version = os.environ.get("bench_version", "0.0.0-unknown")
commit_sha = os.environ.get("GITHUB_SHA", "local-dev")
timestamp = datetime.now().isoformat()

# Validation: Ensure RMSE is valid
try:
    rmse_float = float(raw_rmse)
except ValueError:
    print(f"Error: Provided RMSE '{raw_rmse}' is not a valid number.")
    sys.exit(1)

# Build Datastructure
log_data = {
    "version": version,
    "rmse_value": rmse_float,
    "timestamp": timestamp,
    "commit": commit_sha[:8],
}

# Write to File
try:
    with open(output_path, "w") as f:
        json.dump(log_data, f, indent=2)
    print(f"Successfully created log: {output_path}")
except Exception as e:
    print(f"Error writing log file: {e}")
    sys.exit(1)
