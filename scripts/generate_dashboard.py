import sys
import csv
import os
import json
from collections import defaultdict

LOG_FILE_PATH = "tests/img/exr/zig_render/zig_render_log.txt"
csv_path, readme_path = sys.argv[1], sys.argv[2]

# --- 1. Read History ---
history_map = defaultdict(lambda: defaultdict(lambda: None))
unique_versions, all_modes_historical = [], set()
latest_date = "N/A"

if os.path.exists(csv_path):
    with open(csv_path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            v, m, r = row["version"], row["mode"], float(row["rmse"])
            short_v = f"b.{v.split('.')[-1]}" if "build" in v else v[-6:]
            if short_v not in unique_versions:
                unique_versions.append(short_v)
            all_modes_historical.add(m)
            history_map[short_v][m] = r
            latest_date = row["date"]

# --- 2. Historical Graph ---
window_versions = unique_versions[-20:]
trend_lines, all_h_vals = "", []
last_val = {m: None for m in all_modes_historical}

for m in sorted(all_modes_historical):
    series = []
    for v in window_versions:
        val = history_map[v][m] if history_map[v][m] is not None else last_val[m]
        series.append(val)
        if val is not None:
            all_h_vals.append(val)
            last_val[m] = val
    if any(s is not None for s in series):
        trend_lines += f"    line {json.dumps(series)}\n"

x_axis_trend = "[" + ", ".join([f'"{v}"' for v in window_versions]) + "]"
y_max_t = max(all_h_vals or [1.0]) * 1.2
trend_chart = f'```mermaid\nxychart-beta\n    title "Trend"\n    x-axis {x_axis_trend}\n    y-axis "RMSE" 0 --> {y_max_t:.4f}\n{trend_lines}```'

# --- 3. Convergence & Table ---
convergence_raw = defaultdict(list)
total_max_time = 0.0

if os.path.exists(LOG_FILE_PATH):
    with open(LOG_FILE_PATH, "r") as f:
        curr = None
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("VERSION:"):
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
num_buckets = 12  # Increased buckets slightly for better resolution
t_steps = [(total_max_time / (num_buckets - 1)) * i for i in range(num_buckets)]

if convergence_raw:
    for mode in sorted(convergence_raw.keys()):
        pts = convergence_raw[mode]
        final_time = pts[-1][1]

        for ts in t_steps:
            # If the current bucket time is past the mode's final render time,
            # we use None (which becomes null in JSON) to "stop" the line.
            if ts > final_time + (total_max_time * 0.05):  # 5% grace margin
                resampled[mode].append(None)
            else:
                current_best_r = pts[0][0]
                for r, t in pts:
                    if t <= ts:
                        current_best_r = r
                    else:
                        break
                resampled[mode].append(round(current_best_r, 4))

        # Build Summary Table
        final_p = pts[-1]
        summary_table += f"| **{mode.upper()}** | {final_p[0]:.4f} | {final_p[1]:.2f}s | {len(pts)} |\n"

        # Build Gallery/Mermaid lines
        gallery_header += f" {mode.upper()} |"
        gallery_sep += " :---: |"
        gallery_imgs += f" ![ {mode} ](renderings/latest-{mode}.png) |"

        # json.dumps converts None to null, which Mermaid understands
        conv_lines += f"    line {json.dumps(resampled[mode])}\n"

x_axis_conv = "[" + ", ".join([f'"{round(t, 1)}s"' for t in t_steps]) + "]"

# We use a max function that ignores the None values for the Y-axis calculation
y_limit = (
    max(
        [max([v for v in series if v is not None]) for series in resampled.values()]
        or [1.0]
    )
    * 1.1
)

conv_chart = f"""```mermaid
xychart-beta
    title "RMSE Convergence Over Time"
    x-axis {x_axis_conv}
    y-axis "RMSE" 0 --> {y_limit:.4f}
{conv_lines}
```"""
# --- 4. Assemble ---
with open(readme_path, "w") as f:
    f.write(
        f"# Benchmark\n## Summary\n{summary_table}\n## Trend\n{trend_chart}\n## Gallery\n{gallery_header}\n{gallery_sep}\n{gallery_imgs}\n## Convergence\n{conv_chart}"
    )
