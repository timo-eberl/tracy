import json
import sys
import os
import glob
from datetime import datetime

# The directory containing your Zig render logs
LOG_DIR = "tests/img/exr/zig_render/"

if len(sys.argv) < 2:
    print("Usage: python create_log.py <output_path>")
    sys.exit(1)

output_path = sys.argv[1]

# Metadata from environment
base_version = os.environ.get("bench_version", "0.0.0-unknown")
commit_sha = os.environ.get("GITHUB_SHA", "local-dev")[:8]
timestamp = datetime.now().isoformat()

runs_data = []

try:
    # Look for all .txt files matching the pattern
    search_pattern = os.path.join(LOG_DIR, "render_log_*.txt")
    log_files = glob.glob(search_pattern)

    for file_path in log_files:
        filename = os.path.basename(file_path)

        # Expected format: render_log_sceneTag_variantTag.txt
        # Remove prefix and extension, then split
        parts = filename.replace("render_log_", "").replace(".txt", "").split("_")

        if len(parts) < 2:
            print(f"Skipping malformed filename: {filename}")
            continue

        scene_tag = parts[0]
        variant_tag = parts[1]

        current_scores = []

        with open(file_path, "r") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("VARIANT:"):
                    continue

                try:
                    # Zig log format: score,time_seconds
                    score_str = line.split(",")[0]
                    current_scores.append(float(score_str))
                except (ValueError, IndexError):
                    continue

        if current_scores:
            runs_data.append(
                {
                    "version": f"{base_version}",
                    "variant": variant_tag,
                    "scene": scene_tag,
                    "rmse_value": current_scores[-1],  # Last score
                    "iterations": len(current_scores),
                    "timestamp": timestamp,
                    "commit": commit_sha,
                }
            )

    # Write the collected data to the JSON output
    with open(output_path, "w") as f:
        json.dump(runs_data, f, indent=2)
        print(f"Successfully processed {len(runs_data)} log files into {output_path}")

except Exception as e:
    print(f"Error: {e}")
    sys.exit(1)
