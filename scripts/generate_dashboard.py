import sys
import csv
import os
import json
from collections import defaultdict

# Paths
LOG_FILE_PATH = "tests/img/exr/zig_render/zig_render_log.txt"

# Usage: python generate_dashboard.py <csv_path> <readme_path>
if len(sys.argv) < 3:
    print("Usage: python generate_dashboard.py <csv_path> <readme_path>")
    sys.exit(1)

csv_path = sys.argv[1]
readme_path = sys.argv[2]

# --- 1. Read Historical CSV Data ---
# We use a dictionary to group scores by their mode (suffix after the last dash)
history_by_mode = defaultdict(list)
all_versions = []  # To keep track of the X-axis (unique build versions)
latest_date = "N/A"

if os.path.exists(csv_path):
    try:
        with open(csv_path, "r") as f:
            rows = list(csv.DictReader(f))
            if rows:
                latest_date = rows[-1]["date"]
                for row in rows:
                    v_full = row["version"]
                    # Extract mode (e.g., "0.1.0-mt" -> mode is "mt", base is "0.1.0")
                    parts = v_full.rsplit("-", 1)
                    base_v = parts[0]
                    mode = parts[1] if len(parts) > 1 else "default"

                    # Clean up base version for X-axis labels
                    short_v = (
                        f"b.{base_v.split('.')[-1]}"
                        if "build" in base_v
                        else base_v[-6:]
                    )
                    if short_v not in all_versions:
                        all_versions.append(short_v)

                    history_by_mode[mode].append(float(row["rmse"]))

    except Exception as e:
        print(f"Error reading CSV: {e}")

# --- 2. Read Current Render Convergence Log (Multi-Mode) ---
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
