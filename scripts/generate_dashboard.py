import sys
import csv
import os

# Usage: python generate_dashboard.py <csv_path> <readme_path>
if len(sys.argv) < 3:
    print("Usage: python generate_dashboard.py <csv_path> <readme_path>")
    sys.exit(1)

csv_path = sys.argv[1]
readme_path = sys.argv[2]

# Read Data
versions = []
rmse_values = []
dates = []
latest_ver = "N/A"
latest_date = "N/A"
latest_rmse = "N/A"

if os.path.exists(csv_path):
    try:
        with open(csv_path, "r") as f:
            reader = csv.DictReader(f)
            rows = list(reader)

            # Update latest stats
            if rows:
                latest_ver = rows[-1]["version"]
                latest_date = rows[-1]["date"]
                latest_rmse = rows[-1]["rmse"]

            # Keep last 20 entries for the graph
            recent_rows = rows[-20:]

            for row in recent_rows:
                # Clean up Version ID for the X-Axis
                # Example: "0.1.0-build.145" -> "b.145"
                v = row["version"]
                if "build" in v:
                    short_ver = "b." + v.split(".")[-1]
                else:
                    short_ver = v[-6:]  # Fallback: last 6 chars

                versions.append(short_ver)
                rmse_values.append(float(row["rmse"]))
                dates.append(row["date"])
    except Exception as e:
        print(f"Error reading CSV: {e}")

# Generate Mermaid Graph
# We use a variable for the backticks to avoid breaking the script when copying
fence = "```"

if not versions:
    graph_block = "No benchmark data available yet."
else:
    # Prepare data strings for Mermaid
    # We format the list of versions as ["v1", "v2", ...]
    x_axis = str(versions).replace("'", '"')

    # We format the list of values as [0.1, 0.2, ...]
    data_series = str(rmse_values)

    # Dynamic Y-axis max
    y_max = max(rmse_values) * 1.2 if rmse_values else 1.0

    graph_block = f"""{fence}mermaid
---
config:
    theme: base
    themeVariables:
        xyChart:
            plotColorPalette: "#2980b9"
---
xychart-beta
    title "RMSE Convergence Error (Lower is Better)"
    x-axis {x_axis}
    y-axis "RMSE" 0 --> {y_max:.4f}
    line {data_series}
{fence}"""

# Assemble the README Content
markdown_content = f"""
# Benchmark Dashboard

This dashboard tracks the image quality performance (RMSE) of the renderer over time.

| Metric | Latest Value |
|--------|--------------|
| **Version** | `{latest_ver}` |
| **Date** | {latest_date} |
| **RMSE** | **{latest_rmse}** |

## Performance Trend
{graph_block}

---
*Last updated by GitHub Actions on {latest_date}.*
"""

# Write to File
try:
    with open(readme_path, "w") as f:
        f.write(markdown_content)
    print(f"Successfully generated dashboard at {readme_path}")
except Exception as e:
    print(f"Error writing README: {e}")
    sys.exit(1)
