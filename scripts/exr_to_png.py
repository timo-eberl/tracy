import sys
import os
import glob
# import mitsuba, numpy, etc. based on your existing logic


def convert_file(in_path, out_path):
    # Place your existing Mitsuba/Pillow conversion logic here
    print(f"Converting {in_path} -> {out_path}")
    # e.g., image = mitsuba.render(...); image.write_png(out_path)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python exr_to_png.py <input_dir> <output_dir>")
        sys.exit(1)

    input_dir = sys.argv[1]
    output_dir = sys.argv[2]

    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)

    # Find all EXR files
    exr_files = glob.glob(os.path.join(input_dir, "*.exr"))

    for exr_path in exr_files:
        filename = os.path.basename(exr_path)
        # Change extension to .png
        png_filename = filename.replace(".exr", ".png")
        png_path = os.path.join(output_dir, png_filename)

        convert_file(exr_path, png_path)
