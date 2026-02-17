import mitsuba as mi
import numpy as np
from PIL import Image
import os

# Set mitsuba variant
mi.set_variant("scalar_rgb")


# Define the path used by the Zig renderer
INPUT_PATH = "tests/img/exr/zig_render/render_zig.exr"
OUTPUT_PATH = "preview.png"

if not os.path.exists(INPUT_PATH):
    print(
        f"Error: {INPUT_PATH} not found. Ensure the Zig renderer has finished at least one step."
    )
    exit(1)

# Load the HDR EXR
try:
    bitmap = mi.Bitmap(INPUT_PATH)
    pixels_all = np.array(bitmap)
    pixels_rgb = pixels_all[:, :, 0:3]  # Drop Alpha channel if present
except Exception as e:
    print(f"Failed to load EXR: {e}")
    exit(1)

# DEBUG: Log raw values for a specific pixel
# Note: Using [150, 320] consistent with your previous snippet
target_raw = pixels_rgb[150, 320]
print(f"RAW HDR Value at (320, 150): {target_raw}")

# Tone Mapping: Clip high intensity (HDR) to 1.0 (LDR)
pixels_clamped = np.clip(pixels_rgb, 0.0, 1.0)

# 4. Scale to 8-bit range (0.0 - 255.0)
pixels_scaled = pixels_clamped * 255.0

# We ensure the value is strictly between 0.0 and 255.0 BEFORE casting.
# This prevents floating point overflow (e.g., 255.00001) wrapping to 0.
pixels_safe = np.clip(pixels_scaled, 0.0, 255.0)

# cast to 8-bit unsigned integers
pixels_8bit = pixels_safe.astype(np.uint8)

print(f"FINAL 8-bit Value:           {pixels_8bit[150, 320]}")

# save png
try:
    img = Image.fromarray(pixels_8bit, "RGB")
    img.save(OUTPUT_PATH)
    print(f"Successfully converted {INPUT_PATH} to {OUTPUT_PATH}")
except Exception as e:
    print(f"Error saving image: {e}")
    exit(1)
