import sys
import csv
import os
import json
from collections import defaultdict

# --- Configuration & Args ---
# Usage: python generate_dashboard.py <csv_path> <readme_path> <optional_log_path>
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
    last_val = {m: None for m in all_modes_historical}

    for m in sorted(all_modes_historical):
        series = []
        for v in window_versions:
            # Forward Fill logic: use current or carry over last known
            val = history_map[v][m] if history_map[v][m] is not None else last_val[m]
            series.append(val)
            if val is not None:
                all_h_vals.append(val)
                last_val[m] = val

        if any(s is not None for s in series):
            trend_lines += f"    line {json.dumps(series)}\n"

    if trend_lines:
        # Mermaid Formatting Fix: Manual string list construction for X-axis
        x_axis_trend = "[" + ", ".join([f'"{v}"' for v in window_versions]) + "]"
        y_max_t = max(all_h_vals or [1.0]) * 1.2
        trend_chart = f"""```mermaid
xychart-beta
    title "Historical Performance (RMSE)"
    x-axis {x_axis_trend}
    y-axis "RMSE" 0 --> {y_max_t:.4f}
{trend_lines}
```"""

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
            # Support both VARIANT: and VERSION: just in case
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
conv_lines, resampled = "", defaultdict(list)
num_buckets = 12
t_steps = [(total_max_time / (num_buckets - 1)) * i for i in range(num_buckets)]

if convergence_raw:
    for mode in sorted(convergence_raw.keys()):
        pts = convergence_raw[mode]
        final_time = pts[-1][1]

        for ts in t_steps:
            # Line Stopping Logic: use None/null if bucket is past mode completion
            if ts > final_time + (total_max_time * 0.05):
                resampled[mode].append(None)
            else:
                current_best_r = pts[0][0]  # Respective Start
                for r, t in pts:
                    if t <= ts:
                        current_best_r = r
                    else:
                        break
                resampled[mode].append(round(current_best_r, 4))

        # Build UI Elements
        final_p = pts[-1]
        summary_table += f"| **{mode.upper()}** | {final_p[0]:.4f} | {final_p[1]:.2f}s | {len(pts)} |\n"
        gallery_header += f" {mode.upper()} |"
        gallery_sep += " :---: |"
        gallery_imgs += f" ![ {mode} ](renderings/latest-{mode}.png) |"
        conv_lines += f"    line {json.dumps(resampled[mode])}\n"

    x_axis_conv = "[" + ", ".join([f'"{round(t, 1)}s"' for t in t_steps]) + "]"
    y_limit = (
        max([max([v for v in s if v is not None]) for s in resampled.values()] or [1.0])
        * 1.1
    )

    conv_chart = f"""```mermaid
xychart-beta
    title "RMSE Convergence Over Time"
    x-axis {x_axis_conv}
    y-axis "RMSE" 0 --> {y_limit:.4f}
{conv_lines}
```"""
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
