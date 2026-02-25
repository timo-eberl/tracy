"""
Install required packages:
    pip install opencv-python numpy
Usage Example:
    python diff_exr.py render_v1.exr render_v2.exr
    python diff_exr.py render_v1.exr render_v2.exr -o difference_map.exr
"""

import os
import sys
import argparse
import glob

# Enable OpenEXR support in OpenCV
os.environ["OPENCV_IO_ENABLE_OPENEXR"] = "1"

import cv2
import numpy as np


def generate_diff(render_path, ref_path, output_path):
    # Load images with IMREAD_UNCHANGED to preserve float data (HDR)
    img_render = cv2.imread(render_path, cv2.IMREAD_UNCHANGED)
    img_ref = cv2.imread(ref_path, cv2.IMREAD_UNCHANGED)

    if img_render is None:
        print(f"  [!] Error: Could not load render {render_path}")
        return False
    if img_ref is None:
        print(f"  [!] Error: Could not load reference {ref_path}")
        return False

    if img_render.shape != img_ref.shape:
        print(f"  [!] Error: Dimension mismatch for {os.path.basename(render_path)}.")
        print(f"      Render: {img_render.shape}, Ref: {img_ref.shape}")
        return False

    # Absolute difference
    diff = cv2.absdiff(img_render, img_ref)

    max_diff = np.max(diff)
    avg_diff = np.mean(diff)

    print(f"  - Max diff: {max_diff:.6f} | Avg diff: {avg_diff:.6f}")

    if cv2.imwrite(output_path, diff):
        return True
    else:
        print(f"  [!] Error saving output to {output_path}")
        return False


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Batch generate difference images between a directory and a reference EXR."
    )
    parser.add_argument("input_dir", help="Directory containing .exr renders")
    parser.add_argument("output_dir", help="Directory where diff files will be saved")
    parser.add_argument(
        "reference_dir",
        help="Path to the directory containing the reference/ground-truth EXR files",
    )

    args = parser.parse_args()

    # Create output directory if it doesn't exist
    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)

    # Find all EXR files in the input directory
    exr_files = glob.glob(os.path.join(args.input_dir, "*.exr"))

    if not exr_files:
        print(f"No EXR files found in {args.input_dir}")
        sys.exit(0)

    success_count = 0
    for render_path in exr_files:
        scene = os.path.splitext(render_path)[0].split("_")[-2]
        ref_fp = os.path.join(args.reference_dir, f"{scene}/scene.exr")

        print(f"Comparing {render_path} against reference: {ref_fp}")
        filename = os.path.basename(render_path)
        # Skip the reference image if it happens to be in the same directory

        # Build output filename (e.g., render_st.exr -> diff_render_st.exr)
        diff_filename = f"diff_{filename}"
        output_path = os.path.join(args.output_dir, diff_filename)

        print(f"Processing: {filename}")
        if generate_diff(render_path, ref_fp, output_path):
            success_count += 1

    print(f"\nDone. Generated {success_count} difference maps in '{args.output_dir}'.")
