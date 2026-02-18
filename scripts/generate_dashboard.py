import sys
import csv
import os
import json
from collections import defaultdict

LOG_FILE_PATH = "tests/img/exr/zig_render/zig_render_log.txt"
csv_path, readme_path = sys.argv[1], sys.argv[2]

# --- 1. Read Historical CSV Data with Forward Fill ---
history_map = defaultdict(lambda: defaultdict(lambda: None))
unique_versions = []
all_modes = set()

if os.path.exists(csv_path):
    with open(csv_path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            ver = row["version"]
            # Formatting build number for chart legibility
            short_v = f"b.{ver.split('.')[-1]}" if "build" in ver else ver[-6:]
            mode = row["mode"]
            rmse = float(row["rmse"])

            if short_v not in unique_versions:
                unique_versions.append(short_v)

            all_modes.add(mode)
            history_map[short_v][mode] = rmse

# 2. Build aligned lists with Forward Fill
window_versions = unique_versions[-20:]
trend_lines_md = ""
all_vals = []

last_known_rmse = {mode: 0.0 for mode in all_modes}

for mode in sorted(all_modes):
    mode_series = []
    for v in window_versions:
        current_val = history_map[v][mode]

        if current_val is not None:
            last_known_rmse[mode] = current_val
            mode_series.append(current_val)
            all_vals.append(current_val)
        else:
            # Use the last known value for this mode
            mode_series.append(last_known_rmse[mode])
            if last_known_rmse[mode] > 0:
                all_vals.append(last_known_rmse[mode])

    if any(val > 0 for val in mode_series):
        trend_lines_md += f"    line {json.dumps(mode_series)}\n"  # --- 2. Read Current Render Convergence Log (Multi-Mode) ---
convergence_data = {}
if os.path.exists(LOG_FILE_PATH):
    try:
        with open(LOG_FILE_PATH, "r") as f:
            curr_mode = None
            for line in f:
                line = line.strip()
                if line.startswith("VERSION:"):
                    curr_mode = line.split(":")[1]
                    convergence_data[curr_mode] = []
                elif line and curr_mode:
                    try:
                        convergence_data[curr_mode].append(float(line))
                    except ValueError:
                        continue
    except Exception as e:
        print(f"Error reading convergence log: {e}")

# --- 3. Generate Mermaid Blocks ---
fence = "```"

# A. Historical Trend Graph (One line per mode)
trend_lines = ""
all_history_vals = []
for mode, vals in history_by_mode.items():
    # Only plot if we have data points
    if vals:
        trend_lines += f"    line {json.dumps(vals[-20:])} \n"  # Show last 20 per mode
        all_history_vals.extend(vals[-20:])

trend_chart = "No historical data."
if all_versions:
    y_max = max(all_history_vals or [1.0]) * 1.2
    trend_chart = f"""{fence}mermaid
xychart-beta
    title "RMSE Trend per Mode"
    x-axis {json.dumps(all_versions[-20:])}
    y-axis "RMSE" 0 --> {y_max:.4f}
{trend_lines}
{fence}"""

# B. Convergence Graph (One line per mode in current run)
conv_chart = "No convergence data."
if convergence_data:
    max_steps = max(len(s) for s in convergence_data.values())
    max_val = max([max(s) for s in convergence_data.values() if s] or [1.0])
    steps_x = "[" + ", ".join([f'"{i + 1}"' for i in range(max_steps)]) + "]"

    mermaid_lines = ""
    for mode, scores in convergence_data.items():
        mermaid_lines += f"    line {json.dumps(scores)}\n"

    conv_chart = f"""{fence}mermaid
xychart-beta
    title "Convergence Rate (Latest Run)"
    x-axis {steps_x}
    y-axis "RMSE" 0 --> {max_val * 1.1:.4f}
{mermaid_lines}
{fence}"""

# --- 4. Dynamic Gallery and Summary Table ---
summary_table = "| Mode | Final RMSE |\n|---|---|\n"
gallery_header = "|"
gallery_sep = "|"
gallery_imgs = "|"

for mode, scores in convergence_data.items():
    if scores:
        summary_table += f"| **{mode.upper()}** | {scores[-1]:.4f} |\n"
        gallery_header += f" {mode.upper()} |"
        gallery_sep += " :---: |"
        # Assumes images are named 'latest-{mode}.png'
        gallery_imgs += f" ![ {mode} ](renderings/latest-{mode}.png) |"

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
    print(f"Dashboard generated with {len(convergence_data)} modes.")
except Exception as e:
    print(f"Error: {e}")
    sys.exit(1)
