import mitsuba as mi
import numpy as np
from PIL import Image
import os
import sys

# Set mitsuba variant
mi.set_variant("scalar_rgb")

# --- Argument Parsing ---
if len(sys.argv) < 3:
    print("Usage: python exr_to_png.py <input_exr_path> <output_png_path>")
    sys.exit(1)

input_path = sys.argv[1]
output_path = sys.argv[2]

if not os.path.exists(input_path):
    print(f"Error: {input_path} not found.")
    sys.exit(1)

# --- Processing ---
try:
    # Load the HDR EXR
    bitmap = mi.Bitmap(input_path)
    pixels_rgb = np.array(bitmap)[:, :, 0:3]  # Drop Alpha channel if present

    # DEBUG: Log raw values for a specific pixel
    target_raw = pixels_rgb[150, 320]
    print(f"RAW HDR Value at (320, 150): {target_raw}")

    # Tone Mapping: Clamp 0-1, Scale to 255, and cast to 8-bit
    # We clip twice to ensure floating point math doesn't cause overflow wraps
    pixels_8bit = (np.clip(pixels_rgb, 0.0, 1.0) * 255.0).clip(0, 255).astype(np.uint8)

    print(f"FINAL 8-bit Value:           {pixels_8bit[150, 320]}")

    # Save PNG
    img = Image.fromarray(pixels_8bit, "RGB")
    img.save(output_path)
    print(f"Successfully converted {input_path} to {output_path}")

except Exception as e:
    print(f"Operation failed: {e}")
    sys.exit(1)
