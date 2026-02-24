import yaml
import subprocess
import os
from collections import defaultdict


def main():
    config_file = "render_config.yml"

    if not os.path.exists(config_file):
        print(f"error: {config_file} not found. did you rename it?")
        return

    with open(config_file, "r") as f:
        config = yaml.safe_load(f)

    # we group jobs by variant (mt/st) so we don't have to
    # recompile the zig binary for every single scene.
    # it saves a massive amount of time during big benchmark runs.
    groups = defaultdict(list)
    for job in config.get("jobs", []):
        groups[job["variant"]].append(job)

    for variant, jobs in groups.items():
        print(f"\n--- preparing variant: {variant} ---")

        # pass the multithreaded flag to the zig build system.
        # zig is smart enough to skip the rebuild if nothing changed.
        is_mt = "true" if variant == "mt" else "false"
        build_cmd = [
            "zig",
            "build",
            "bench-build",
            "-Doptimize=ReleaseFast",
            f"-Dmultithreaded={is_mt}",
        ]
        print(f"BUILD CMD: {build_cmd}")
        try:
            subprocess.run(build_cmd, check=True)
        except subprocess.CalledProcessError:
            print(f"failed to build variant: {variant}. skipping these jobs.")
            continue

        for job in jobs:
            scene = job["scene"]
            iters = str(job["iterations"])

            print(f"  > rendering: {scene} [{iters} iterations]")

            # call the compiled binary with CLI arguments.
            # this avoids having to parse json inside zig, which is a headache.
            run_cmd = ["./zig-out/bin/render-bench-zig", scene, iters]

            try:
                subprocess.run(run_cmd, check=True)
            except subprocess.CalledProcessError:
                print(f"  ! render failed for {scene}. moving to next job.")


if __name__ == "__main__":
    main()
