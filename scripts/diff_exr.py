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

# Enable OpenEXR support in OpenCV
os.environ["OPENCV_IO_ENABLE_OPENEXR"] = "1"

import cv2
import numpy as np

def generate_diff(img_a_path, img_b_path, output_path):
    img_a = cv2.imread(img_a_path, cv2.IMREAD_UNCHANGED)
    img_b = cv2.imread(img_b_path, cv2.IMREAD_UNCHANGED)

    if img_a is None:
        print(f"Error: Could not load {img_a_path}")
        sys.exit(1)
    if img_b is None:
        print(f"Error: Could not load {img_b_path}")
        sys.exit(1)

    if img_a.shape != img_b.shape:
        print(f"Error: Dimension mismatch.")
        print(f"A: {img_a.shape}, B: {img_b.shape}")
        sys.exit(1)

    diff = cv2.absdiff(img_a, img_b)

    max_diff = np.max(diff)
    avg_diff = np.mean(diff)

    print(f"Max pixel diff: {max_diff:.6f}")
    print(f"Avg pixel diff: {avg_diff:.6f}")

    if max_diff == 0:
        print("Images are identical.")
    else:
        print("Images differ.")

    if cv2.imwrite(output_path, diff):
        print(f"Difference saved to: {output_path}")
    else:
        print("Error saving output image.")
        sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate a difference image between two EXR files.")
    parser.add_argument("image1", help="Path to first EXR image")
    parser.add_argument("image2", help="Path to second EXR image")
    parser.add_argument("-o", "--output", default="diff.exr", help="Output filename (default: diff.exr)")

    args = parser.parse_args()

    generate_diff(args.image1, args.image2, args.output)
