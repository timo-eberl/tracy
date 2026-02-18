import sys
import csv
import os
import matplotlib

matplotlib.use("Agg")  # Headless mode for CI
import matplotlib.pyplot as plt
from collections import defaultdict

# --- Global Style ---
plt.style.use("ggplot")
# Use a standard categorical color palette (Tab10 provides 10 distinct colors)
COLOR_CYCLE = plt.rcParams["axes.prop_cycle"].by_key()["color"]


def get_color(index):
    """Returns a color from the cycle based on index."""
    return COLOR_CYCLE[index % len(COLOR_CYCLE)]


def generate_convergence_plot(log_path, output_path):
    """Generates the RMSE vs Time plot for an arbitrary number of variants."""
    if not os.path.exists(log_path):
        print(f"Log not found: {log_path}")
        return

    data = {}
    current_variant = None
    all_rmse_values = []

    with open(log_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("VARIANT:"):
                current_variant = line.split(":")[1].strip().lower()
                if current_variant not in data:
                    data[current_variant] = {"rmse": [], "time": []}
            elif current_variant and "," in line:
                try:
                    rmse, time = map(float, line.split(","))
                    prev_time = (
                        data[current_variant]["time"][-1]
                        if data[current_variant]["time"]
                        else 0
                    )
                    data[current_variant]["rmse"].append(rmse)
                    data[current_variant]["time"].append(prev_time + time)
                    all_rmse_values.append(rmse)
                except ValueError:
                    continue

    plt.figure(figsize=(10, 5))

    # Plot each detected mode with a unique color from the cycle
    for i, (variant, results) in enumerate(sorted(data.items())):
        if not results["time"]:
            continue
        plt.plot(
            results["time"],
            results["rmse"],
            label=variant.upper(),
            color=get_color(i),
            marker="o",
            markersize=4,
            linewidth=2,
        )

    plt.xlim(left=0)
    plt.ylim(bottom=0)
    if all_rmse_values:
        plt.ylim(top=max(all_rmse_values) * 1.1)

    plt.title("Convergence", fontsize=12)
    plt.xlabel("Cumulative Time (seconds)")
    plt.ylabel("RMSE")
    plt.legend(loc="lower left", frameon=True, facecolor="white", framealpha=0.8)

    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()


def generate_trend_plot(csv_path, output_path, max_versions=20):
    """Generates the Historical RMSE Trend plot for all modes found in CSV."""
    if not os.path.exists(csv_path):
        print(f"CSV not found: {csv_path}")
        return

    data = defaultdict(dict)
    versions = []

    with open(csv_path, mode="r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            v = row["version"]
            m = row["mode"].lower()
            r = float(row["rmse"])
            short_v = f"b.{v.split('.')[-1]}" if "build" in v else v[-6:]
            if short_v not in versions:
                versions.append(short_v)
            data[m][short_v] = r

    window_versions = versions[-max_versions:] if max_versions > 0 else versions

    plt.figure(figsize=(10, 5))

    # Iterate through all modes found in the CSV history
    for i, mode in enumerate(sorted(data.keys())):
        y_values = [data[mode].get(v) for v in window_versions]

        # Identify indices where we actually have data points
        plot_indices = [idx for idx, val in enumerate(y_values) if val is not None]
        plot_values = [val for val in y_values if val is not None]

        if not plot_values:
            continue

        plt.plot(
            plot_indices,
            plot_values,
            label=mode.upper(),
            color=get_color(i),
            marker="s",
            markersize=5,
            linewidth=2,
        )

    plt.xticks(range(len(window_versions)), window_versions, rotation=45)
    plt.ylim(bottom=0)
    plt.title(
        f"Historical Performance Trend (Last {len(window_versions)} builds)",
        fontsize=12,
    )
    plt.xlabel("Build Version")
    plt.ylabel("RMSE")
    plt.legend(loc="lower left", frameon=True, facecolor="white", framealpha=0.8)

    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(
            "Usage: python generate_plots.py <log_path> <csv_path> <out_dir> [num_versions]"
        )
        sys.exit(1)

    log_in, csv_in, out_dir = sys.argv[1], sys.argv[2], sys.argv[3]
    num_v = int(sys.argv[4]) if len(sys.argv) > 4 else 20

    os.makedirs(out_dir, exist_ok=True)

    generate_convergence_plot(log_in, os.path.join(out_dir, "convergence.png"))
    generate_trend_plot(
        csv_in, os.path.join(out_dir, "history_trend.png"), max_versions=num_v
    )

    print(f"Success: Plots generated in {out_dir}")
