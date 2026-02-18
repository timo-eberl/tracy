import sys
import csv
import os
from collections import defaultdict

# --- Configuration & Args ---
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
                short_v = f"b.{v.split('.')[-1]}" if "build" in v else v[-6:]
                if short_v not in unique_versions:
                    unique_versions.append(short_v)
                all_modes_historical.add(m)
                history_map[short_v][m] = r
                latest_date = row["date"]
            except (ValueError, KeyError):
                continue

# --- 2. Historical Trend Graph (Mermaid) ---
trend_chart = "## Historical Trend\n![Historical Trend](renderings/history_trend.png)"

# --- 3. Table & Gallery Parsing ---
# We still need to parse the log to fill out the Summary Table data
convergence_raw = defaultdict(list)
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
                except:
                    continue

summary_table = "| Mode | Final RMSE | Total Time | Steps |\n|---|---|---|---|\n"
gallery_header, gallery_sep, gallery_imgs = "|", "|", "|"

if convergence_raw:
    for mode in sorted(convergence_raw.keys()):
        pts = convergence_raw[mode]
        final_p = pts[-1]
        summary_table += f"| **{mode.upper()}** | {final_p[0]:.4f} | {final_p[1]:.2f}s | {len(pts)} |\n"
        gallery_header += f" {mode.upper()} |"
        gallery_sep += " :---: |"
        gallery_imgs += f" ![ {mode} ](renderings/latest-{mode}.png) |"
else:
    summary_table = "*No summary data available.*"

conv_chart_html = (
    "## Convergence Comparison\n![Convergence Plot](renderings/convergence.png)"
)

# --- 4. Assemble Final README ---
with open(readme_path, "w") as f:
    f.write(f"""# Path Tracer Benchmark Dashboard

## Summary
{summary_table}

## Historical Trend
{trend_chart}

## Latest Render Gallery
{gallery_header if convergence_raw else ""}
{gallery_sep if convergence_raw else ""}
{gallery_imgs if convergence_raw else ""}

{conv_chart_html}

---
*Last updated: {latest_date} (Commit: {os.environ.get("GITHUB_SHA", "local")[:8]})*
""")
