import sys
import csv
import os
import json
from collections import defaultdict

# --- Configuration ---
LOG_FILE_PATH = "tests/img/exr/zig_render/zig_render_log.txt"

if len(sys.argv) < 3:
    print("Usage: python generate_dashboard.py <csv_path> <readme_path>")
    sys.exit(1)

csv_path = sys.argv[1]
readme_path = sys.argv[2]

# --- 1. Read Historical CSV Data ---
history_map = defaultdict(lambda: defaultdict(lambda: None))
unique_versions = []
all_modes_historical = set()
latest_date = "N/A"

if os.path.exists(csv_path):
    try:
        with open(csv_path, "r") as f:
            reader = csv.DictReader(f)
            for row in reader:
                ver = row["version"]
                short_v = f"b.{ver.split('.')[-1]}" if "build" in ver else ver[-6:]
                mode = row["mode"]
                rmse = float(row["rmse"])
                latest_date = row["date"]

                if short_v not in unique_versions:
                    unique_versions.append(short_v)

                all_modes_historical.add(mode)
                history_map[short_v][mode] = rmse
    except Exception as e:
        print(f"Error reading CSV: {e}")

# --- 2. Build Historical Trend with Forward Fill ---
window_versions = unique_versions[-20:]
trend_lines_md = ""
all_history_vals = []
last_known_rmse = {mode: None for mode in all_modes_historical}

for mode in sorted(all_modes_historical):
    mode_series = []
    for v in window_versions:
        current_val = history_map[v][mode]
        if current_val is not None:
            last_known_rmse[mode] = current_val
            mode_series.append(current_val)
            all_history_vals.append(current_val)
        else:
            fill_val = last_known_rmse[mode]
            mode_series.append(fill_val)
            if fill_val is not None:
                all_history_vals.append(fill_val)

    if any(val is not None for val in mode_series):
        trend_lines_md += f"    line {json.dumps(mode_series)}\n"

trend_chart = "No historical data available."
if window_versions:
    y_max_trend = max(all_history_vals or [1.0]) * 1.2
    trend_chart = f'```mermaid\nxychart-beta\n    title "Historical Trend (Forward Filled)"\n    x-axis {json.dumps(window_versions)}\n    y-axis "RMSE" 0 --> {y_max_trend:.4f}\n{trend_lines_md}```'

# --- 3. Parse Convergence Log (Current Run) ---
convergence_raw = defaultdict(list)
total_max_time = 0.0

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
                    cumulative = 0.0
                elif curr_mode:
                    try:
                        rmse_val, step_time = map(float, line.split(","))
                        cumulative += step_time
                        convergence_raw[curr_mode].append((rmse_val, cumulative))
                        total_max_time = max(total_max_time, cumulative)
                    except ValueError:
                        continue
    except Exception as e:
        print(f"Error reading log: {e}")

# --- 4. Resample Convergence for Mermaid Alignment ---
conv_chart = "No convergence data."
summary_table = "| Mode | Final RMSE | Total Time |\n|---|---|---|\n"
gallery_header = "|"
gallery_sep = "|"
gallery_imgs = "|"

if convergence_raw:
    num_buckets = 10
    time_steps = [(total_max_time / (num_buckets - 1)) * i for i in range(num_buckets)]
    resampled_data = defaultdict(list)

    for mode in sorted(convergence_raw.keys()):
        points = convergence_raw[mode]
        # Resampling logic
        for t in time_steps:
            last_rmse = points[0][0]
            for p_rmse, p_time in points:
                if p_time <= t:
                    last_rmse = p_rmse
                else:
                    break
            resampled_data[mode].append(round(last_rmse, 4))

        # Populate Table and Gallery
        final_p = points[-1]
        summary_table += (
            f"| **{mode.upper()}** | {final_p[0]:.4f} | {final_p[1]:.2f}s |\n"
        )
        gallery_header += f" {mode.upper()} |"
        gallery_sep += " :---: |"
        gallery_imgs += f" ![ {mode} ](renderings/latest-{mode}.png) |"

    # Build Mermaid Lines
    mermaid_conv_lines = ""
    for mode in sorted(resampled_data.keys()):
        mermaid_conv_lines += f"    line {json.dumps(resampled_data[mode])}\n"

    x_labels = [f'"{round(t, 2)}s"' for t in time_steps]
    max_val_conv = max([max(v) for v in resampled_data.values()] or [1.0])

    conv_chart = f"""```mermaid
xychart-beta
    title "RMSE vs Time (Resampled)"
    x-axis {json.dumps(x_labels if "x_axis_labels" in locals() else x_labels)}
    y-axis "RMSE" 0 --> {max_val_conv * 1.1:.4f}
{mermaid_conv_lines}
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

## Convergence Comparison (Time-based)
{conv_chart}

---
*Last updated: {latest_date}*
"""

with open(readme_path, "w") as f:
    f.write(markdown_content)
