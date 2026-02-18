import sys
import matplotlib.pyplot as plt
import os


def generate_plots(log_path, output_path):
    if not os.path.exists(log_path):
        print(f"Log not found: {log_path}")
        return

    data = {}
    current_variant = None
    all_rmse_values = []

    # Parse the raw log
    with open(log_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("VARIANT:"):
                current_variant = line.split(":")[1].strip()
                data[current_variant] = {"rmse": [], "time": []}
            elif current_variant and "," in line:
                try:
                    rmse, time = map(float, line.split(","))
                    # Cumulative time calculation
                    prev_time = (
                        data[current_variant]["time"][-1]
                        if data[current_variant]["time"]
                        else 0
                    )
                    current_cumulative = prev_time + time

                    data[current_variant]["rmse"].append(rmse)
                    data[current_variant]["time"].append(current_cumulative)

                    all_rmse_values.append(rmse)
                except ValueError:
                    continue

    # Plotting
    plt.figure(figsize=(10, 6))
    plt.style.use("ggplot")

    # Consistent color mapping
    colors = {"st": "#3498db", "mt": "#e74c3c"}

    for variant, results in data.items():
        if not results["time"]:
            continue

        # Plotting raw data points only (no T=0 injection)
        plt.plot(
            results["time"],
            results["rmse"],
            label=variant.upper(),
            color=colors.get(variant, None),
            marker="o",
            markersize=4,
            linewidth=2,
        )

    # --- Axis Limits ---
    # We still start the axis view at 0, but the lines will start at their first data point
    plt.xlim(left=0)
    plt.ylim(bottom=0)

    if all_rmse_values:
        plt.ylim(top=max(all_rmse_values) * 1.1)

    plt.title("Convergence Comparison: RMSE vs Time", fontsize=14)
    plt.xlabel("Cumulative Time (seconds)", fontsize=12)
    plt.ylabel("RMSE (Lower is Better)", fontsize=12)
    plt.grid(True, linestyle="--", alpha=0.7)
    plt.legend()

    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    print(f"Plot saved to {output_path}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python generate_plots.py <log_file> <output_png>")
        sys.exit(1)
    generate_plots(sys.argv[1], sys.argv[2])
