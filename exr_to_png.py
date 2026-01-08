import mitsuba as mi
import numpy as np
from PIL import Image

mi.set_variant('scalar_rgb')

print("--- ROBUST CONVERSION START ---")

# 1. Load EXR
bitmap = mi.Bitmap("reference.exr")
pixels_all = np.array(bitmap)
pixels_rgb = pixels_all[:, :, 0:3] # Drop Alpha

# 2. DEBUG: Pixel (320, 50)
target_raw = pixels_rgb[150, 320]
print(f"RAW Value:         {target_raw}")

# 3. Clip high intensity to 1.0
pixels_clamped = np.clip(pixels_rgb, 0.0, 1.0)
print(f"CLAMPED (0-1):     {pixels_clamped[50, 320]}")

# 4. Scale to 255 range
pixels_scaled = pixels_clamped * 255.0

# 5. SAFETY CLIP (The Fix)
# We ensure the value is strictly between 0 and 255.0 BEFORE casting.
# This handles any 255.0000001 edge cases that would wrap to 0.
pixels_safe = np.clip(pixels_scaled, 0.0, 255.0)

# 6. Cast to 8-bit
pixels_8bit = pixels_safe.astype(np.uint8)

print(f"FINAL 8-bit Value: {pixels_8bit[50, 320]}")
print("This should now definitely be [255 255 255].")

# 7. Save
img = Image.fromarray(pixels_8bit, 'RGB')
img.save("reference_final.png")
print("Saved 'reference_final.png'.")