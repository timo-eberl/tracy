import yaml
import json
import subprocess
import os
import sys
import time
from collections import defaultdict

# The definitive list of parameters Zig expects
PARAM_SCHEMA = {
    "scene_id": int,
    "max_depth": int,
    "width": int,
    "height": int,
    "filter_type": int,
    "cam_angle_x": float,
    "cam_angle_y": float,
    "cam_dist": float,
    "focus_x": float,
    "focus_y": float,
    "focus_z": float,
}

# The baseline configuration used if values are missing in YAML
DEFAULT_PARAMS = {
    "scene_id": 0,
    "max_depth": 5,
    "width": 640,
    "height": 480,
    "filter_type": 0,
    "cam_angle_x": 0.04258603374866164,
    "cam_angle_y": 0.0,
    "cam_dist": 5.5,
    "focus_x": 0.0,
    "focus_y": 1.25,
    "focus_z": 0.0,
}


def validate_and_merge(job_data, templates):
    """
    Layers the configuration:
    1. System Defaults -> 2. Tag Template -> 3. Job Overrides
    """
    tag = job_data.get("tag")
    template = templates.get(tag, {})

    # Start with system defaults
    final_params = DEFAULT_PARAMS.copy()

    # Layer in the template provided by the tag
    final_params.update({k: v for k, v in template.items() if k in PARAM_SCHEMA})

    # Layer in the specific job overrides (including non-render fields like 'scene')
    final_params.update(
        {
            k: v
            for k, v in job_data.items()
            if k in PARAM_SCHEMA or k in ["scene", "variant", "iterations", "tag"]
        }
    )

    # Validate keys to catch typos early
    all_input_keys = list(job_data.keys()) + (list(template.keys()) if template else [])
    for k in all_input_keys:
        if k not in PARAM_SCHEMA and k not in [
            "tag",
            "name",
            "description",
            "scene",
            "variant",
            "iterations",
        ]:
            print(f"  Note: Parameter '{k}' is unrecognized and will be ignored.")

    return final_params


def main():
    config_path = "render_config.yml"

    if not os.path.exists(config_path):
        print(f"Error: Configuration file '{config_path}' not found.")
        sys.exit(1)

    print(f"Reading configuration: {config_path}")
    with open(config_path, "r") as f:
        raw_data = yaml.safe_load(f)

    # Build template lookup table
    templates = {cfg["tag"]: cfg for cfg in raw_data.get("scenes", [])}

    # Resolve all jobs into unified parameter sets
    resolved_jobs = [validate_and_merge(j, templates) for j in raw_data.get("jobs", [])]

    # Group jobs by variant to minimize recompilation
    job_groups = defaultdict(list)
    for job in resolved_jobs:
        job_groups[job["variant"]].append(job)

    print(f"Total jobs: {len(resolved_jobs)} | Variants: {len(job_groups)}\n")

    for variant, jobs in job_groups.items():
        print(f"--- Variant: {variant} ---")

        # Build the Zig binary for this specific variant
        is_mt = "true"
        is_rr = "true" if variant == "rr" else "false"
        build_cmd = [
            "zig",
            "build",
            "bench-build",
            "-Doptimize=ReleaseFast",
            f"-Dmultithreaded={is_mt}",
            f"-Drussianroulette={is_rr}",
        ]

        start_build = time.time()
        build_proc = subprocess.run(build_cmd, capture_output=True, text=True)

        if build_proc.returncode != 0:
            print(f"Build Failed: Could not compile {variant} variant.")
            print(f"Reason: {build_proc.stderr}")
            continue

        print(f"Build successful ({time.time() - start_build:.2f}s)")

        for job in jobs:
            # Strip job metadata to send only the numeric parameters to Zig
            zig_params = {k: job[k] for k in PARAM_SCHEMA.keys()}
            json_payload = json.dumps(zig_params, separators=(",", ":"))

            print(f"  Render: {job['scene']}")
            print(
                f"    Config: {job.get('tag', 'custom')} | {job['width']}x{job['height']} | {job['iterations']} iterations"
            )

            run_cmd = [
                "./zig-out/bin/render-bench-zig",
                job["scene"],
                str(job["iterations"]),
                json_payload,
            ]

            start_render = time.time()
            render_proc = subprocess.run(run_cmd)

            if render_proc.returncode == 0:
                print(f"    Success ({time.time() - start_render:.2f}s)\n")
            else:
                print(
                    f"    Error: Render process exited with code {render_proc.returncode}\n"
                )


if __name__ == "__main__":
    print("Benchmark Execution Started")
    print("-" * 40)
    main()
    print("-" * 40)
    print("Benchmark Execution Complete")
