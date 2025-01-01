import os
import cairosvg
from xml.etree import ElementTree as ET
from segments import segments, total_video_length  # Import shared segment data

# Scaling factor for PNGs
upscale_factor = 4

def calculate_segment_width(segment_duration, total_video_duration, base_width=1000):
    total_seconds = total_video_duration.total_seconds()
    segment_seconds = segment_duration.total_seconds()
    return (segment_seconds / total_seconds) * base_width

def modify_and_convert_svgs(input_svg_folder, modified_svg_folder, output_png_folder):
    """Modify SVGs (color and size) and convert them to PNGs."""

    if not os.path.exists(modified_svg_folder):
        os.makedirs(modified_svg_folder)
    if not os.path.exists(output_png_folder):
        os.makedirs(output_png_folder)

    def change_svg_color_to_gray(svg_file):
        tree = ET.parse(svg_file)
        root = tree.getroot()
        for elem in root.iter():
            if 'fill' in elem.attrib:
                elem.set('fill', 'rgb(128,128,128)')
        return tree, root

    for i, (start, end) in enumerate(segments):
        segment_duration = end - start
        segment_width = calculate_segment_width(segment_duration, total_video_length)

        svg_file = os.path.join(input_svg_folder, f'heatmap_{i+1}.svg')
        png_file = os.path.join(output_png_folder, f'heatmap_{i+1}.png')

        # Modify SVG color and dimensions
        tree, root = change_svg_color_to_gray(svg_file)
        svg_tag = root.tag.split('}')[1] if '}' in root.tag else root.tag
        if svg_tag == 'svg':
            root.set('width', str(segment_width))
            root.set('height', '100')

            modified_svg_file = os.path.join(modified_svg_folder, f'modified_heatmap_{i+1}.svg')
            tree.write(modified_svg_file)

            # Convert SVG to PNG with upscaling
            cairosvg.svg2png(url=modified_svg_file, write_to=png_file, scale=upscale_factor)
    print("SVG modification and PNG conversion complete.")
