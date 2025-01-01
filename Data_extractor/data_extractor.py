import cv2
import os
import numpy as np
import csv
from segments import segments

def map_pixels_to_time(start_time, duration, image_width):
    seconds_per_pixel = duration / image_width
    return [start_time + (pixel - 1) * seconds_per_pixel for pixel in range(1, image_width + 1)]

def count_gray_pixels(image, gray_value=128, lower_threshold=10, upper_threshold=10):
    # Calculate lower and upper bounds for gray value
    lower_bound = gray_value - lower_threshold
    upper_bound = gray_value + upper_threshold
    
    # Split the image into its RGB components
    blue, green, red = cv2.split(image)
    
    # Create a mask that checks if each channel is within the specified gray range
    gray_mask = (
        (red >= lower_bound) & (red <= upper_bound) &
        (green >= lower_bound) & (green <= upper_bound) &
        (blue >= lower_bound) & (blue <= upper_bound)
    )
    
    # Count the number of "gray" pixels in each column
    return [np.sum(gray_mask[:, col]) for col in range(gray_mask.shape[1])]

def extract_engagement_data(image_directory, csv_file_path):
    # Step 1: First pass through all images to find the global maximum gray pixel count
    global_max_gray_value = 0

    for i, (start_time, end_time) in enumerate(segments, 1):
        image_path = os.path.join(image_directory, f'heatmap_{i}.png')

        if os.path.exists(image_path):
            #print(f"Processing image: {image_path}")  # Debugging: Check if image path is correct
            
            image = cv2.imread(image_path)
            if image is None:
                print(f"Failed to load {image_path}")  # Debugging: Check if image is loading correctly
                continue

            gray_pixel_counts = count_gray_pixels(image)

            # Update global maximum if a higher gray pixel count is found
            image_max_gray_value = max(gray_pixel_counts)
            global_max_gray_value = max(global_max_gray_value, image_max_gray_value)

    #print(f"Global maximum gray pixel count across all images: {global_max_gray_value}") #debugging

    # Step 2: Second pass to write the normalized data to CSV
    with open(csv_file_path, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(["Image", "Pixel", "Time (s)", "Gray Value", "Normalized Engagement (%)"])

        for i, (start_time, end_time) in enumerate(segments, 1):
            image_path = os.path.join(image_directory, f'heatmap_{i}.png')

            if os.path.exists(image_path):
                #print(f"Processing image: {image_path}")  # Debugging: Check if image path is correct
                
                image = cv2.imread(image_path)
                if image is None:
                    print(f"Failed to load {image_path}")  # Debugging: Check if image is loading correctly
                    continue

                height, width = image.shape[:2]
                #print(f"Image dimensions (HxW): {height}x{width}")  # Debugging: Check image dimensions

                gray_pixel_counts = count_gray_pixels(image)
                #print(f"First 10 gray pixel counts: {gray_pixel_counts[:10]}")  # Debugging: Check pixel counts

                # Normalize gray pixel counts based on the global maximum
                normalized_gray_values = [(count / global_max_gray_value) * 100 for count in gray_pixel_counts]

                duration = (end_time - start_time).total_seconds()
                time_mappings = map_pixels_to_time(start_time.total_seconds(), duration, width)

                for pixel_num in range(width):
                    writer.writerow([
                        f"Image_{i}",
                        pixel_num + 1,
                        round(time_mappings[pixel_num], 3),
                        gray_pixel_counts[pixel_num],
                        round(normalized_gray_values[pixel_num], 3)
                    ])
            else:
                print(f"Image {image_path} not found.")  # Debugging: Check if the image file exists

    print(f"Engagement data saved to {csv_file_path}")
