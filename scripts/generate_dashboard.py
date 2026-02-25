import sys
import csv
import os
from collections import defaultdict

# human-like comments in lower-case only
# args: <csv_path> <readme_path>
if len(sys.argv) < 3:
    print("usage: python generate_dashboard.py <csv_path> <readme_path>")
    sys.exit(1)

csv_path = sys.argv[1]
readme_path = sys.argv[2]

# 1. parse the history csv
# we pull the latest data to know which scenes actually exist
latest_entries = {}
latest_date = "n/a"

if os.path.exists(csv_path):
    with open(csv_path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            latest_date = row.get("date", "n/a")
            s, v = row["scene"], row["variant"]
            latest_entries[(s, v)] = row

# group by scene
scenes = defaultdict(list)
for (s, v), data in latest_entries.items():
    scenes[s].append(data)

# 2. build the summary table
summary_table = "| Scene | Variant | RelMSE Score | Render Time | Iterations | Date |\n|---|---|---|---|---|---|\n"
for s in sorted(scenes.keys()):
    for entry in scenes[s]:
        summary_table += f"| {s} | **{entry['variant']}** | {float(entry['rmse_score']):.5f} | {float(entry['rmse_time']):.2f}s | {entry['iterations']} | {entry['date']} |\n"

# 3. build the dynamic gallery and convergence sections
# we look for the specific convergence plots created by generate_plots.py
gallery_sections = ""
for s in sorted(scenes.keys()):
    gallery_sections += f"### Scene: {s}\n\n"

    # render/diff table
    header = "| Type |"
    sep = "| :---: |"
    row_render = "| **Render** |"
    row_diff = "| **Diff** |"

    for entry in sorted(scenes[s], key=lambda x: x["variant"]):
        v = entry["variant"]
        img_name = f"latest-render_{s}_{v}.png"
        diff_name = f"latest-diff_render_{s}_{v}.png"

        header += f" {v} |"
        sep += " :---: |"
        row_render += f" ![ {v} ](renderings/{img_name}) |"
        row_diff += f" ![ diff {v} ](renderings/{diff_name}) |"

    # add reference image
    header += " Reference |"
    sep += " :---: |"
    row_render += f" ![ Reference](renderings/reference_{s}.png) |"
    row_diff += " |"
    gallery_sections += f"{header}\n{sep}\n{row_render}\n{row_diff}\n\n"

    # link the specific convergence plot for this scene
    conv_plot = f"plots/convergence_{s}.png"
    if (
        os.path.exists(f"data-branch/{conv_plot}") or True
    ):  # assume it exists if generating
        gallery_sections += (
            f"### Convergence: {s}\n\n![ {s} convergence ]({conv_plot})\n\n---\n"
        )

# 4. finalize the readme
with open(readme_path, "w") as f:
    f.write(f"""# Path Tracer Benchmark Dashboard

## Summary Results
{summary_table}

## RelMSE Trend
![RelMSE Trend](plots/history_score_trend.png)

## Runtime Trend
![Runtime Trend](plots/history_time_trend.png)

## Render Gallery & Convergence
{gallery_sections if scenes else "*no renderings found.*"}

---
*last updated: {latest_date} (commit: {os.environ.get("GITHUB_SHA", "local")[:8]})*
""")
