import sys
import csv
import os
import json

# Paths
LOG_FILE_PATH = "tests/img/exr/zig_render/zig_render_log.txt"

# Usage: python generate_dashboard.py <csv_path> <readme_path>
if len(sys.argv) < 3:
    print("Usage: python generate_dashboard.py <csv_path> <readme_path>")
    sys.exit(1)

csv_path = sys.argv[1]
readme_path = sys.argv[2]

# --- 1. Read Historical CSV Data ---
versions, rmse_values, dates = [], [], []
latest_ver, latest_date, latest_rmse = "N/A", "N/A", "N/A"

if os.path.exists(csv_path):
    try:
        with open(csv_path, "r") as f:
            reader = csv.DictReader(f)
            rows = list(reader)
            if rows:
                latest_ver = rows[-1]["version"]
                latest_date = rows[-1]["date"]
                latest_rmse = rows[-1]["rmse"]

                for row in rows[-20:]:
                    v = row["version"]
                    short_ver = f"b.{v.split('.')[-1]}" if "build" in v else v[-6:]
                    versions.append(short_ver)
                    rmse_values.append(float(row["rmse"]))
                    dates.append(row["date"])
    except Exception as e:
        print(f"Error reading CSV: {e}")

# --- 2. Read Current Render Convergence Log (Dynamic Steps) ---
convergence_rmse = []
if os.path.exists(LOG_FILE_PATH):
    try:
        with open(LOG_FILE_PATH, "r") as f:
            # Filters out empty lines and converts all numeric lines to floats
            for line in f:
                clean_line = line.strip()
                if clean_line:
                    try:
                        convergence_rmse.append(float(clean_line))
                    except ValueError:
                        continue
    except Exception as e:
        print(f"Error reading convergence log: {e}")

# --- 3. Generate Mermaid Blocks ---
fence = "```"

# Historical Trend Graph
if not versions:
    trend_graph = "No benchmark data available yet."
else:
    y_max_trend = max(rmse_values) * 1.2
    trend_graph = f"""{fence}mermaid
xychart-beta
    title "RMSE Trend"
    x-axis {json.dumps(versions)}
    y-axis "RMSE" 0 --> {y_max_trend:.4f}
    line {rmse_values}
{fence}"""

# Convergence Graph (Dynamic X-Axis)
if not convergence_rmse:
    convergence_graph = "*No convergence data found for the latest render.*"
else:
    num_steps = len(convergence_rmse)
    # WICHTIG: Mermaid benötigt doppelte Anführungszeichen für Strings auf der X-Achse
    # Wir konvertieren die Liste manuell in einen String mit doppelten Anführungszeichen
    steps_x_labels = "[" + ", ".join([f'"{i + 1}"' for i in range(num_steps)]) + "]"

    y_max_conv = max(convergence_rmse) * 1.1

    convergence_graph = f"""{fence}mermaid
---
config:
    theme: base
    themeVariables:
        xyChart:
            plotColorPalette: "#e67e22"
---
xychart-beta
    title "Convergence Rate"
    x-axis {steps_x_labels}
    y-axis "RMSE" 0 --> {y_max_conv:.4f}
    line {convergence_rmse}
{fence}"""

# --- 4. Assemble README ---
markdown_content = f"""
# Benchmark Dashboard

This dashboard tracks the image quality performance (RMSE) of the renderer.

| Metric | Latest Value |
|--------|--------------|
| **Version** | `{latest_ver}` |
| **Date** | {latest_date} |
| **Final RMSE** | **{latest_rmse}** |

## Performance Trend
{trend_graph}

## Latest Render
![Latest Render](renderings/latest.png)

### Convergence Progress
{convergence_graph}

> This graph shows how the error decreased across {len(convergence_rmse)} rendering steps.

---
*Last updated by GitHub Actions on {latest_date}.*
"""

# Write to File
try:
    with open(readme_path, "w") as f:
        f.write(markdown_content)
    print(
        f"Successfully generated dashboard at {readme_path} with {len(convergence_rmse)} steps."
    )
except Exception as e:
    print(f"Error writing README: {e}")
    sys.exit(1)
