import sys
import csv
import os
import json
from collections import defaultdict

# --- Configuration & Args ---
# Usage: python generate_dashboard.py <csv_path> <readme_path> [log_path]
if len(sys.argv) < 3:
    print("Usage: python generate_dashboard.py <csv_path> <readme_path> [log_path]")
    sys.exit(1)

csv_path = sys.argv[1]
readme_path = sys.argv[2]
LOG_FILE_PATH = (
    sys.argv[3] if len(sys.argv) > 3 else "tests/img/exr/zig_render/zig_render_log.txt"
)

# --- 1. Read History ---
history_map = defaultdict(lambda: defaultdict(lambda: None))
unique_versions, all_modes_historical = [], set()
latest_date = "N/A"

if os.path.exists(csv_path):
    with open(csv_path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                v, m, r = row["version"], row["mode"], float(row["rmse"])
                # Extract short version for X-axis (e.g., build.77 -> b.77)
                short_v = f"b.{v.split('.')[-1]}" if "build" in v else v[-6:]

                if short_v not in unique_versions:
                    unique_versions.append(short_v)

                all_modes_historical.add(m)
                history_map[short_v][m] = r
                latest_date = row["date"]
            except (ValueError, KeyError):
                continue

# --- 2. Historical Trend Graph ---
trend_chart = "*No historical data available yet.*"
if unique_versions:
    window_versions = unique_versions[-20:]
    trend_lines, all_h_vals = "", []
    plotted_modes = []

    for m in sorted(all_modes_historical):
        series = []

        # Find first non-null value in the window to prevent leading null errors
        first_val = None
        for v in window_versions:
            if history_map[v][m] is not None:
                first_val = history_map[v][m]
                break

        if first_val is None:
            continue

        last_known_val = first_val
        for v in window_versions:
            val = history_map[v][m]
            # Forward fill: use actual value or the last known good value
            current_entry = val if val is not None else last_known_val
            series.append(round(current_entry, 4))
            all_h_vals.append(current_entry)
            last_known_val = current_entry

        trend_lines += f"    line {json.dumps(series)}\n"
        plotted_modes.append(m.upper())

    if trend_lines:
        x_axis_trend = "[" + ", ".join([f'"{v}"' for v in window_versions]) + "]"
        y_max_t = max(all_h_vals or [1.0]) * 1.2
        legend_labels = " | ".join(
            [f"Line {i + 1}: **{name}**" for i, name in enumerate(plotted_modes)]
        )

        # Use single string to avoid Mermaid parsing Legend text as code
        trend_chart = f'```mermaid\nxychart-beta\n    title "Historical Performance (RMSE)"\n    x-axis {x_axis_trend}\n    y-axis "RMSE" 0 --> {y_max_t:.4f}\n{trend_lines}```\n> **Legend:** {legend_labels}'

# --- 3. Convergence & Table Parsing ---
convergence_raw = defaultdict(list)
total_max_time = 0.0

if os.path.exists(LOG_FILE_PATH):
    with open(LOG_FILE_PATH, "r") as f:
        curr = None
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("VARIANT:") or line.startswith("VERSION:"):
                curr, cumulative = line.split(":")[1].strip(), 0.0
            elif curr:
                try:
                    r, t = map(float, line.split(","))
                    cumulative += t
                    convergence_raw[curr].append((r, cumulative))
                    total_max_time = max(total_max_time, cumulative)
                except:
                    continue

summary_table = "| Mode | Final RMSE | Total Time | Steps |\n|---|---|---|---|\n"
gallery_header, gallery_sep, gallery_imgs = "|", "|", "|"
conv_lines = ""
num_buckets = 12
t_steps = [(total_max_time / (num_buckets - 1)) * i for i in range(num_buckets)]
plotted_conv_modes = []

if convergence_raw:
    for mode in sorted(convergence_raw.keys()):
        pts = convergence_raw[mode]
        final_time = pts[-1][1]
        final_rmse = pts[-1][0]

        mode_series = []
        for ts in t_steps:
            if ts > final_time:
                # Forward fill the line after completion to avoid Mermaid 'null' error
                mode_series.append(round(final_rmse, 4))
            else:
                current_best_r = pts[0][0]
                for r, t in pts:
                    if t <= ts:
                        current_best_r = r
                    else:
                        break
                mode_series.append(round(current_best_r, 4))

        conv_lines += f"    line {json.dumps(mode_series)}\n"
        plotted_conv_modes.append(mode.upper())

        # Build UI Elements
        final_p = pts[-1]
        summary_table += f"| **{mode.upper()}** | {final_p[0]:.4f} | {final_p[1]:.2f}s | {len(pts)} |\n"
        gallery_header += f" {mode.upper()} |"
        gallery_sep += " :---: |"
        gallery_imgs += f" ![ {mode} ](renderings/latest-{mode}.png) |"

    x_axis_conv = "[" + ", ".join([f'"{round(t, 1)}s"' for t in t_steps]) + "]"
    # Scale Y axis based on historical data for better context
    y_limit = max(all_h_vals or [1.0]) * 1.1

    conv_legend = " | ".join(
        [f"Line {i + 1}: **{m}**" for i, m in enumerate(plotted_conv_modes)]
    )
    conv_chart = f'```mermaid\nxychart-beta\n    title "RMSE Convergence Over Time"\n    x-axis {x_axis_conv}\n    y-axis "RMSE" 0 --> {y_limit:.4f}\n{conv_lines}```\n> **Legend:** {conv_legend}'
else:
    conv_chart = "*No convergence data available for this run.*"

# --- 4. Assemble Final README ---
with open(readme_path, "w") as f:
    f.write(f"""# Path Tracer Benchmark Dashboard

## Summary
{summary_table if convergence_raw else "*No summary data available.*"}

## Historical Trend
{trend_chart}

## Latest Render Gallery
{gallery_header if convergence_raw else ""}
{gallery_sep if convergence_raw else ""}
{gallery_imgs if convergence_raw else ""}

## Convergence Comparison
{conv_chart}

---
*Last updated: {latest_date} (Commit: {os.environ.get("GITHUB_SHA", "local")[:8]})*
""")

print(f"Successfully generated dashboard at {readme_path}")
