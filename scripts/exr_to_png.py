import sys
import os
import glob
import mitsuba as mi

mi.set_variant("scalar_rgb")


def convert_file(in_path, out_path):
    print(f"Converting: {os.path.basename(in_path)} -> {os.path.basename(out_path)}")
    # Load HDR and convert to 8-bit sRGB for PNG
    try:
        bitmap = mi.Bitmap(in_path)
        bitmap_rgb = bitmap.convert(
            mi.Bitmap.PixelFormat.RGB, mi.Struct.Type.UInt8, True
        )
        bitmap_rgb.write(out_path)
    except Exception as e:
        print(f"Error converting {in_path}: {e}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(1)
    name = ""
    # Use absolute paths so the runner doesn't get lost
    input_dir = os.path.abspath(sys.argv[1])
    output_dir = os.path.abspath(sys.argv[2])
    if len(sys.argv) > 3:
        name = sys.argv[3].strip()
    os.makedirs(output_dir, exist_ok=True)

    # Grab all EXRs in the target folder
    exr_files = glob.glob(os.path.join(input_dir, "*.exr"))
    print(f"EXR-FILES found: {exr_files}")
    if not exr_files:
        print(f"No files found in {input_dir}")
        sys.exit(1)

    for exr_path in exr_files:
        # Swap .exr for .png properly
        if not name:
            name = os.path.splitext(os.path.basename(exr_path))[0]
        png_path = os.path.join(output_dir, f"{name}.png")
        print(f"generating png: {png_path}")
        convert_file(exr_path, png_path)
