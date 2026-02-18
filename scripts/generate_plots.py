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
    max_time = 0

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
                    # Convert relative step time to cumulative time
                    prev_time = (
                        data[current_variant]["time"][-1]
                        if data[current_variant]["time"]
                        else 0
                    )
                    current_cumulative = prev_time + time

                    data[current_variant]["rmse"].append(rmse)
                    data[current_variant]["time"].append(current_cumulative)

                    all_rmse_values.append(rmse)
                    if current_cumulative > max_time:
                        max_time = current_cumulative
                except ValueError:
                    continue

    # Plotting
    plt.figure(figsize=(10, 6))
    plt.style.use("ggplot")

    colors = {"st": "#3498db", "mt": "#e74c3c"}

    for variant, results in data.items():
        if not results["time"]:
            continue

        # Adding a starting point at T=0 with the first RMSE value
        # to ensure the line starts exactly at the Y-axis
        times = [0] + results["time"]
        rmses = [results["rmse"][0]] + results["rmse"]

        plt.plot(
            times,
            rmses,
            label=variant.upper(),
            color=colors.get(variant, None),
            marker="o",
            markersize=4,
            linewidth=2,
        )

    # --- Force Axes to Start at 0 ---
    plt.xlim(left=0)  # X starts at 0
    plt.ylim(bottom=0)  # Y starts at 0

    # Optional: Add 10% headroom so the line isn't touching the top border
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
