import json
import sys
import os
from datetime import datetime

# Input file path
LOG_FILE_PATH = "tests/img/exr/zig_render/zig_render_log.txt"

# Usage check: Now only requires the output path
if len(sys.argv) < 2:
    print("Usage: python create_log.py <output_path>")
    sys.exit(1)

output_path = sys.argv[1]

# 1. Read the last line from the log file
try:
    with open(LOG_FILE_PATH, "r") as f:
        lines = f.readlines()
        if not lines:
            print(f"Error: Log file '{LOG_FILE_PATH}' is empty.")
            sys.exit(1)

        # Get the last non-empty line
        raw_rmse = lines[-1].strip()
        while not raw_rmse and len(lines) > 1:
            lines.pop()
            raw_rmse = lines[-1].strip()

except FileNotFoundError:
    print(f"Error: Log file not found at {LOG_FILE_PATH}")
    sys.exit(1)

# Github Actions Pipeline Metadata
version = os.environ.get("bench_version", "0.0.0-unknown")
commit_sha = os.environ.get("GITHUB_SHA", "local-dev")
timestamp = datetime.now().isoformat()

# 2. Validation: Ensure RMSE is valid
try:
    rmse_float = float(raw_rmse)
except ValueError:
    print(f"Error: The last line of the log ('{raw_rmse}') is not a valid number.")
    sys.exit(1)

# Build Datastructure
log_data = {
    "version": version,
    "rmse_value": rmse_float,
    "timestamp": timestamp,
    "commit": commit_sha[:8],
}

# 3. Write to File
try:
    # Ensure the directory for the output path exists (per your previous request)
    output_dir = os.path.dirname(output_path)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)

    with open(output_path, "w") as f:
        json.dump(log_data, f, indent=2)
    print(f"Successfully created log: {output_path} (RMSE: {rmse_float})")
except Exception as e:
    print(f"Error writing log file: {e}")
    sys.exit(1)
