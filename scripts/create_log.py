import json
import sys
import os
from datetime import datetime

LOG_FILE_PATH = "tests/img/exr/zig_render/zig_render_log.txt"

if len(sys.argv) < 2:
    print("Usage: python create_log.py <output_path>")
    sys.exit(1)

output_path = sys.argv[1]

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
            if not line:
                continue

            if line.startswith("VERSION:"):
                # Save previous run before switching
                if current_mode and current_scores:
                    runs_data.append(
                        {
                            "version": f"{base_version}-{current_mode}",
                            "rmse_value": current_scores[-1],
                            "iterations": len(current_scores),  # Count based on entries
                            "timestamp": timestamp,
                            "commit": commit_sha,
                        }
                    )
                current_mode = line.split(":", 1)[1].strip()
                current_scores = []
            else:
                try:
                    # Zig log format: score,time
                    parts = line.split(",")
                    current_scores.append(float(parts[0]))
                except (ValueError, IndexError):
                    continue

        # Catch the last run
        if current_mode and current_scores:
            runs_data.append(
                {
                    "version": f"{base_version}-{current_mode}",
                    "rmse_value": current_scores[-1],
                    "iterations": len(current_scores),
                    "timestamp": timestamp,
                    "commit": commit_sha,
                }
            )

    with open(output_path, "w") as f:
        json.dump(runs_data, f, indent=2)
except Exception as e:
    print(f"Error: {e}")
    sys.exit(1)
