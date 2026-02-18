import sys
import matplotlib.pyplot as plt
import os


def generate_plots(log_path, output_path):
    if not os.path.exists(log_path):
        print(f"Log not found: {log_path}")
        return

    data = {}
    current_variant = None

    # Parse the raw log
    with open(log_path, "r") as f:
        for line in f:
            line = line.strip()
            if line.startswith("VARIANT:"):
                current_variant = line.split(":")[1].strip()
                data[current_variant] = {"rmse": [], "time": []}
            elif current_variant and "," in line:
                rmse, time = map(float, line.split(","))
                # Convert relative step time to cumulative time
                prev_time = (
                    data[current_variant]["time"][-1]
                    if data[current_variant]["time"]
                    else 0
                )
                data[current_variant]["rmse"].append(rmse)
                data[current_variant]["time"].append(prev_time + time)

    # Plotting
    plt.figure(figsize=(10, 6))
    plt.style.use("ggplot")  # Clean, professional look

    colors = {"st": "#3498db", "mt": "#e74c3c"}  # Blue for ST, Red for MT

    for variant, results in data.items():
        plt.plot(
            results["time"],
            results["rmse"],
            label=variant.upper(),
            color=colors.get(variant, None),
            marker="o",
            markersize=4,
            linewidth=2,
        )

    plt.title("Convergence Comparison: RMSE vs Time", fontsize=14)
    plt.xlabel("Cumulative Time (seconds)", fontsize=12)
    plt.ylabel("RMSE (Lower is Better)", fontsize=12)
    plt.grid(True, linestyle="--", alpha=0.7)
    plt.legend()

    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    print(f"Plot saved to {output_path}")


if __name__ == "__main__":
    # Usage: python generate_plots.py <log_file> <output_png>
    generate_plots(sys.argv[1], sys.argv[2])
