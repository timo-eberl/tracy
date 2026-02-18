import sys
import csv
import os
import json
from collections import defaultdict

# --- Configuration ---
LOG_FILE_PATH = "tests/img/exr/zig_render/zig_render_log.txt"

# Usage: python generate_dashboard.py <csv_path> <readme_path>
if len(sys.argv) < 3:
    print("Usage: python generate_dashboard.py <csv_path> <readme_path>")
    sys.exit(1)

csv_path = sys.argv[1]
readme_path = sys.argv[2]

# --- 1. Read Historical CSV Data ---
# history_map[short_version][mode] = rmse
history_map = defaultdict(lambda: defaultdict(lambda: None))
all_versions_raw = []
all_modes = set()
latest_date = "N/A"

if os.path.exists(csv_path):
    try:
        with open(csv_path, "r") as f:
            reader = csv.DictReader(f)
            for row in reader:
                ver = row["version"]
                # Format build number for chart legibility (e.g., 0.0.1-build.5 -> b.5)
                short_v = f"b.{ver.split('.')[-1]}" if "build" in ver else ver[-6:]
                mode = row["mode"]
                rmse = float(row["rmse"])
                latest_date = row["date"]

                if short_v not in all_versions_raw:
                    all_versions_raw.append(short_v)

                all_modes.add(mode)
                history_map[short_v][mode] = rmse
    except Exception as e:
        print(f"Error reading CSV: {e}")

# --- 2. Build aligned lists with Forward Fill (Requirement #2) ---
window_versions = all_versions_raw[-20:]  # Show last 20 unique build versions
trend_lines_md = ""
all_history_vals = []

# Tracker for Forward Fill: keeps the last seen value for each mode
last_known_rmse = {mode: None for mode in all_modes}

for mode in sorted(all_modes):
    mode_series = []
    for v in window_versions:
        current_val = history_map[v][mode]

        if current_val is not None:
            last_known_rmse[mode] = current_val
            mode_series.append(current_val)
            all_history_vals.append(current_val)
        else:
            # Carry over the previous value (Forward Fill)
            fill_val = last_known_rmse[mode]
            mode_series.append(fill_val)
            if fill_val is not None:
                all_history_vals.append(fill_val)

    # Only plot if we have at least one numeric value in the series
    if any(val is not None for val in mode_series):
        # json.dumps converts None to null, which Mermaid handles as gaps
        trend_lines_md += f"    line {json.dumps(mode_series)}\n"

trend_chart = "No historical data available."
if window_versions:
    y_max_trend = max(all_history_vals or [1.0]) * 1.2
    trend_chart = f"""```mermaid
xychart-beta
    title "RMSE Trend per Mode (Forward Filled)"
    x-axis {json.dumps(window_versions)}
    y-axis "RMSE" 0 --> {y_max_trend:.4f}
{trend_lines_md}```"""

# --- 3. Parse Convergence with Time ---
convergence_data = defaultdict(list)  # Stores (rmse, cumulative_time)
if os.path.exists(LOG_FILE_PATH):
    try:
        with open(LOG_FILE_PATH, "r") as f:
            curr_mode = None
            for line in f:
                line = line.strip()
                if not line:
                    continue
                if line.startswith("VERSION:"):
                    curr_mode = line.split(":", 1)[1].strip()
                    convergence_data[curr_mode] = []
                    cumulative_time = 0.0
                elif curr_mode:
                    try:
                        rmse_val, step_time = map(float, line.split(","))
                        cumulative_time += step_time
                        convergence_data[curr_mode].append((rmse_val, cumulative_time))
                    except ValueError:
                        continue
    except Exception as e:
        print(f"Error reading log: {e}")

# Build the Graph
conv_chart = "No data."
if convergence_data:
    # To compare them on the same axis, we need a unified set of X-axis labels
    # We'll use a representative set of time markers or just show the actual timestamps
    mermaid_lines = ""
    all_times = []

    for mode in sorted(convergence_data.keys()):
        points = convergence_data[mode]
        rmses = [p[0] for p in points]
        times = [p[1] for p in points]
        all_times.extend(times)
        mermaid_lines += f"    line {json.dumps(rmses)}\n"

    # We use the time of the longest run to set the X-axis scale
    max_time = max(all_times) if all_times else 1.0
    # Create 10 time-based buckets for the X-axis labels
    x_labels = [f'"{round((max_time / 9) * i, 2)}s"' for i in range(10)]

    conv_chart = f"""```mermaid
xychart-beta
    title "RMSE vs Cumulative Time (Seconds)"
    x-axis [{", ".join(x_labels)}]
    y-axis "RMSE" 0 --> {max(max(p[0] for p in s) for s in convergence_data.values()) * 1.1:.4f}
{mermaid_lines}
```"""
# --- 5. Assemble README ---

markdown_content = f"""
# Path Tracer Benchmark Dashboard

## Latest Run Summary
{summary_table}

## Historical Performance (Per Mode)
{trend_chart}

## Latest Render Gallery
{gallery_header}
{gallery_sep}
{gallery_imgs}

## Convergence Comparison
{conv_chart}

---
*Last updated: {latest_date}*
"""

# Write to File
try:
    with open(readme_path, "w") as f:
        f.write(markdown_content)
    print("Dashboard generated successfully.")
except Exception as e:
    print(f"Error writing README: {e}")
    sys.exit(1)
