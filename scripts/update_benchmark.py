import os
import sys
import csv
import random
from datetime import datetime

# Usage: python mock_benchmark.py <log_file_path> <csv_file_path>
# Example: python scripts/mock_benchmark.py ssim_results.txt data-branch/history.csv

if len(sys.argv) < 3:
    print("Error: Missing arguments.")
    print("Usage: python mock_benchmark.py <log_file_path> <csv_file_path>")
    sys.exit(1)

# Argument 1: The log file (IGNORED for now, but accepted to match future API)
log_file_path = sys.argv[1]

# Argument 2: The CSV history file
csv_file_path = sys.argv[2]

print(f"DEBUG: Received log file '{log_file_path}' (Ignored in mock mode)")
print(f"DEBUG: Targeting CSV file '{csv_file_path}'")

# --- MOCK DATA GENERATION ---

# 1. Timestamp
current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

# 2. Version (Try to get from ENV, otherwise random)
# We look for the 'bench_version' env var set in your pipeline
version = os.environ.get("bench_version", f"0.0.1-mock.{random.randint(100, 999)}")

# 3. RMSE (Random float between 0.000 and 0.050)
# Simulates a "metric" that changes slightly every run
rmse_score = round(random.uniform(0.001, 0.050), 6)

# 4. Commit Hash (From Github Actions ENV)
commit_hash = os.environ.get("GITHUB_SHA", "unknown_commit")[:8]


# --- CSV APPENDING LOGIC ---

# Check if file exists to know if we need a header
file_exists = os.path.isfile(csv_file_path)

# Ensure directory exists (e.g., if 'data-branch' is fresh)
directory = os.path.dirname(csv_file_path)
if directory and not os.path.exists(directory):
    os.makedirs(directory)

with open(csv_file_path, mode="a", newline="") as csvfile:
    fieldnames = ["date", "version", "rmse", "commit"]
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

    # Write header only if file is new
    if not file_exists:
        writer.writeheader()

    writer.writerow(
        {
            "date": current_time,
            "version": version,
            "rmse": rmse_score,
            "commit": commit_hash,
        }
    )

print(
    f"SUCCESS: Added mock entry (Version={version}, RMSE={rmse_score}) to {csv_file_path}"
)
