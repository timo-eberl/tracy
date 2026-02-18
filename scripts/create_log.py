import json
import sys
import os
from datetime import datetime

LOG_FILE_PATH = "tests/img/exr/zig_render/zig_render_log.txt"

if len(sys.argv) < 2:
    print("Usage: python create_log.py <output_path>")
    sys.exit(1)

output_path = sys.argv[1]

# Pipeline Metadata
base_version = os.environ.get("bench_version", "0.0.0-unknown")
commit_sha = os.environ.get("GITHUB_SHA", "local-dev")[:8]
timestamp = datetime.now().isoformat()

runs_data = []

try:
    with open(LOG_FILE_PATH, "r") as f:
        current_mode = None
        current_scores = []

        for line in f:
            line = line.strip()

            # Start of a new version block
            if line.startswith("VERSION:"):
                # If we were already tracking a run, save its FINAL score before switching
                if current_mode and current_scores:
                    runs_data.append(
                        {
                            "version": f"{base_version}-{current_mode}",
                            "rmse_value": current_scores[-1],  # The 10th/Final score
                            "timestamp": timestamp,
                            "commit": commit_sha,
                        }
                    )
                current_mode = line.split(":", 1)[1]
                current_scores = []
            elif line:
                try:
                    current_scores.append(float(line))
                except ValueError:
                    continue

        # Catch the very last run in the file
        if current_mode and current_scores:
            runs_data.append(
                {
                    "version": f"{base_version}-{current_mode}",
                    "rmse_value": current_scores[-1],
                    "timestamp": timestamp,
                    "commit": commit_sha,
                }
            )

    with open(output_path, "w") as f:
        json.dump(runs_data, f, indent=2)
    print(f"Logged final RMSE for {len(runs_data)} runs.")

except FileNotFoundError:
    print(f"Error: Log file not found at {LOG_FILE_PATH}")
    sys.exit(1)
