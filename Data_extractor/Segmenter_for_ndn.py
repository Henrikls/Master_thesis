import subprocess
import os
from segments import segments  # Importing segments
from datetime import timedelta

# Get the directory where the script is located
script_dir = os.path.dirname(os.path.abspath(__file__))

# Input video file (relative to script directory) 
# You should rename the name of the file if there are spaces in there just to make it easier
input_video = os.path.join(script_dir, "video/video1.mp4")
print(f"Input video: {input_video}")

# Output directory for segments (relative to script directory)
output_dir = os.path.join(script_dir, "Segmented_vid")
os.makedirs(output_dir, exist_ok=True)  # Create directory if it doesn't exist
print(f"Output directory: {output_dir}")

# Log file for errors and status (relative to script directory)
log_file = os.path.join(output_dir, "segmenter_log.txt")
print(f"Log file: {log_file}")

# FFmpeg command template
ffmpeg_cmd = "ffmpeg -i {input} -ss {start} -to {end} -c copy {output}"

# Function to convert timedelta to HH:MM:SS format
def format_timedelta(td: timedelta):
    total_seconds = int(td.total_seconds())
    hours = total_seconds // 3600
    minutes = (total_seconds % 3600) // 60
    seconds = total_seconds % 60
    return f"{hours:02}:{minutes:02}:{seconds:02}"

# Generate segments
with open(log_file, "w") as log:
    for i, (start, end) in enumerate(segments, 1):
        start_str = format_timedelta(start)
        end_str = format_timedelta(end)
        output_file = os.path.join(output_dir, f"output_segment_{i}.mp4")

        command = f'ffmpeg -i "{input_video}" -ss {start_str} -to {end_str} -c copy "{output_file}"'
        print(f"Processing segment {i}/{len(segments)}: {start_str} to {end_str}")
        log.write(f"Running command: {command}\n")

        try:
            subprocess.run(command, shell=True, check=True, stdout=log, stderr=log)
            log.write(f"Segment {i} processed successfully: {output_file}\n")
        except subprocess.CalledProcessError as e:
            error_message = f"Error occurred for segment {i}: {e}\n"
            print(error_message)
            log.write(error_message)
