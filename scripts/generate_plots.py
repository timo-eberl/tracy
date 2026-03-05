import sys
import csv
import os
import glob
import matplotlib

matplotlib.use("Agg")  # headless mode for ci
import matplotlib.pyplot as plt
from collections import defaultdict

# human-like comments in lower-case only
# global style setup
plt.style.use("ggplot")
color_cycle = plt.rcParams["axes.prop_cycle"].by_key()["color"]


def get_color(index):
    # returns a color from the cycle based on index
    return color_cycle[index % len(color_cycle)]


def generate_convergence_plots(log_dir, output_dir):
    # we need to group logs by scene because a single graph with
    # 20 different scenes would be unreadable
    scene_logs = defaultdict(list)

    # find all the individual log files we created
    log_files = glob.glob(os.path.join(log_dir, "render_log_*.txt"))

    for f_path in log_files:
        # filename format: render_log_{scene}_{variant}.txt
        parts = (
            os.path.basename(f_path)
            .replace("render_log_", "")
            .replace(".txt", "")
            .split("_")
        )
        if len(parts) >= 2:
            scene, variant = parts[0], parts[1]
            scene_logs[scene].append((variant, f_path))

    # create a plot for each scene
    for scene, variants in scene_logs.items():
        plt.figure(figsize=(10, 5))
        all_rmse =[]

        for i, (variant, f_path) in enumerate(sorted(variants)):
            times, rmses = [],[]
            curr_time = 0.0

            with open(f_path, "r") as f:
                for line in f:
                    line = line.strip()
                    if not line or line.startswith("VARIANT:"):
                        continue
                    try:
                        r, t = map(float, line.split(","))
                        curr_time += t
                        rmses.append(r)
                        times.append(curr_time)
                        all_rmse.append(r)
                    except ValueError:
                        continue

            plt.plot(
                times,
                rmses,
                label=variant.upper(),
                color=get_color(i),
                linewidth=2,
            )

        plt.xlim(left=0)
        plt.ylim(bottom=0)
        if all_rmse:
            plt.ylim(top=max(all_rmse) * 1.1)

        plt.title(f"Convergence: {scene}", fontsize=12)
        plt.xlabel("Cumulative Time (seconds)")
        plt.ylabel("RMSE")
        plt.legend(loc="upper right", frameon=True)

        plt.tight_layout()
        # saves a specific convergence file for each scene
        plt.savefig(os.path.join(output_dir, f"convergence_{scene}.png"), dpi=150)
        plt.close()


def generate_trend_plot(csv_path, output_path, max_versions=20):
    # shows how performance changes over different builds
    if not os.path.exists(csv_path):
        return

    # we group by a combined key of scene+variant
    data = defaultdict(dict)
    versions =[]

    with open(csv_path, mode="r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            v = row["version"]
            # we create a label like "cornellbox (st)"
            label = f"{row['scene']} ({row['variant']})".lower()
            r = float(row["rmse_score"])

            # shortening the version string for the x-axis
            short_v = f"b.{v.split('.')[-1]}" if "build" in v else v[-6:]
            if short_v not in versions:
                versions.append(short_v)
            data[label][short_v] = r

    window_versions = versions[-max_versions:] if max_versions > 0 else versions
    plt.figure(figsize=(10, 6))

    for i, label in enumerate(sorted(data.keys())):
        y_values = [data[label].get(v) for v in window_versions]
        indices =[idx for idx, val in enumerate(y_values) if val is not None]
        values =[val for val in y_values if val is not None]

        if not values:
            continue
        plt.plot(
            indices,
            values,
            label=label,
            color=get_color(i),
            marker="s",
            markersize=5,
            linewidth=2,
        )

    plt.xticks(range(len(window_versions)), window_versions, rotation=45)
    plt.ylim(bottom=0)
    plt.margins(y=0.15)
    plt.title(f"Historical Trend (Last {len(window_versions)} builds)", fontsize=12)
    plt.ylabel("Final RMSE")
    plt.legend(loc="upper left", bbox_to_anchor=(1, 1), fontsize="small")

    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()


def generate_time_trend_plot(csv_path, output_path, max_versions=20):
    # tracks how long it took to converge across different builds
    if not os.path.exists(csv_path):
        return

    # store timing data indexed by scene+variant combo
    data = defaultdict(dict)
    versions =[]

    with open(csv_path, mode="r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            v = row["version"]
            # creating the same identifiable label as the rmse chart
            label = f"{row['scene']} ({row['variant']})".lower()

            # pull the convergence time instead of the error value
            try:
                t = float(row["rmse_time"])
            except (KeyError, ValueError, TypeError):
                continue

            # logic to keep version strings short and readable on the x-axis
            short_v = f"b.{v.split('.')[-1]}" if "build" in v else v[-6:]
            if short_v not in versions:
                versions.append(short_v)

            data[label][short_v] = t

    # only look at the most recent builds to avoid a cluttered graph
    window_versions = versions[-max_versions:] if max_versions > 0 else versions
    plt.figure(figsize=(10, 6))

    for i, label in enumerate(sorted(data.keys())):
        # align timing data with the version indices on the x-axis
        y_values = [data[label].get(v) for v in window_versions]
        indices =[idx for idx, val in enumerate(y_values) if val is not None]
        times = [val for val in y_values if val is not None]

        if not times:
            continue

        plt.plot(
            indices,
            times,
            label=label,
            color=get_color(i),
            marker="D",  # diamond marker to distinguish from the rmse chart
            markersize=5,
            linewidth=2,
        )

    # format x-axis to show the build version tags
    plt.xticks(range(len(window_versions)), window_versions, rotation=45)

    # y-axis starts at zero since time cannot be negative
    plt.ylim(bottom=0)

    plt.title(
        f"Runtime Trend (Last {len(window_versions)} builds)", fontsize=12
    )
    plt.ylabel("Runtime (seconds)")
    plt.xlabel("Build Version")

    # place the legend outside to the right so it doesn't overlap the lines
    plt.legend(loc="upper left", bbox_to_anchor=(1, 1), fontsize="small")

    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()


if __name__ == "__main__":
    # usage: python generate_plots.py <log_dir> <csv_path> <out_dir>
    if len(sys.argv) < 4:
        sys.exit(1)

    log_dir, csv_in, out_dir = sys.argv[1], sys.argv[2], sys.argv[3]
    num_v = int(sys.argv[4]) if len(sys.argv) > 4 else 20

    os.makedirs(out_dir, exist_ok=True)

    # generate multiple convergence plots (one per scene)
    generate_convergence_plots(log_dir, out_dir)
    # generate one big trend plot
    generate_trend_plot(
        csv_in, os.path.join(out_dir, "history_score_trend.png"), max_versions=num_v
    )
    generate_time_trend_plot(
        csv_in, os.path.join(out_dir, "history_time_trend.png"), max_versions=num_v
    )
