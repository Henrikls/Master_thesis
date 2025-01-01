import pandas as pd
import json
from datetime import timedelta
from segments import segments  # Assuming you have the segments file

def calculate_segment_popularity(input_csv, output_json):
    """Calculate and save segment popularity."""
    # Load the engagement data CSV
    engagement_data = pd.read_csv(input_csv)

    # Calculate average engagement for each segment
    segment_popularity = []
    for i, (start, end) in enumerate(segments, start=1):
        start_seconds = int(start.total_seconds())
        end_seconds = int(end.total_seconds())

        # Filter data for the current segment
        segment_data = engagement_data[
            (engagement_data['Time (s)'] >= start_seconds) & 
            (engagement_data['Time (s)'] < end_seconds)
        ]

        # Calculate average engagement
        average_engagement = segment_data['Normalized Engagement (%)'].mean()

        # Append to the segment popularity list
        segment_popularity.append({
            "Segment": i,
            "Start Time": str(start),
            "End Time": str(end),
            "Average Engagement": average_engagement
        })

    # Sort segments by average engagement
    sorted_segments = sorted(segment_popularity, key=lambda x: x['Average Engagement'], reverse=True)

    # Save to JSON file
    with open(output_json, "w") as f:
        json.dump(sorted_segments, f, indent=4)

    print(f"Segment popularity data saved to {output_json}")
