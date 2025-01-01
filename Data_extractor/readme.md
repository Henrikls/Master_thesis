# YouTube Video Engagement Data Extractor

This project provides a pipeline for extracting engagement data from YouTube videos using heatmap visualizations provided by YouTube. The pipeline involves downloading heatmaps in SVG format, modifying them, converting them to PNGs, and finally extracting pixel data to analyze viewer engagement.

## Features

- **Heatmap Extraction**: Automatically downloads heatmap SVGs from YouTube videos using Selenium.
- **SVG Modification and Conversion**: Modifies SVG properties (such as color) and converts them to PNG format for analysis.
- **Engagement Data Extraction**: Analyzes the pixel data from PNG images to determine viewer engagement at different points in the video.
- **Video Segmentation**: Processes the video in predefined segments and maps engagement data to those segments.
- **Segment Popularity Analysis**: Calculates average engagement for each segment and saves the results to a JSON file.
- **Engagement Visualization**: Creates a detailed line plot of engagement over time and saves it as a PNG file.

> **Note**: The segmentation data currently used in this project corresponds to a specific video. A future feature will automatically extract segment data from any video, making the tool fully adaptable. (the example video is: https://www.youtube.com/watch?v=khRJMiquAjA&t)

## Installation

### Prerequisites

Ensure you have the following installed:

- **Python 3.x**
- **pip** (Python package manager)
- **Firefox** and [geckodriver](https://github.com/mozilla/geckodriver/releases) for Selenium browser automation.

Install the necessary Python packages:

```bash
pip install selenium cairosvg opencv-python-headless numpy pandas matplotlib
```

### Setup
Clone the repository:
```bash
git clone https://github.com/your-username/engagement-data-extractor.git
cd engagement-data-extractor
```

## Usage
You can run the script using:
```bash
python main.py
```
You will be prompted to enter the YouTube video URL. The pipeline consists of the following steps: 

**Step 1**: Downloads SVG heatmaps from the YouTube video using Selenium. 

**Step 2**: Modifies the downloaded SVGs and converts them to PNG format for analysis. 

**Step 3**: Extracts engagement data from the PNGs and stores it in a CSV file.

**Step 4**: Calculates segment popularity and saves it to a JSON file.

**Step 5**: Plots user engagement data as a line graph and saves it as a PNG file.

After running then your folder structure will look something like this:

```bash
.
├── svgs/               # Contains original SVG heatmaps downloaded from YouTube.
├── modified_svgs/      # Contains modified SVGs with adjusted colors.
├── pngs/               # Contains PNGs generated from the modified SVGs.
├── csv/                # Contains the extracted engagement data in CSV format.
```

The engagement data is saved in `csv/engagement_data.csv`, where you’ll find pixel-wise data such as:  

**Image**: The PNG file associated with the segment. 

**Pixel**: The pixel number on the x-axis of the image. 

**Time (s)**: The mapped time in the video corresponding to that pixel. 

**Gray Value**: The gray pixel count at that position. 

**Normalized Engagement (%)**: The normalized engagement percentage based on pixel intensity.

The segment popularity data is saved in `csv/segment_popularity.json`, containing details such as segment number, start time, end time, and average engagement percentage.

The engagement chart is saved as `csv/user_engagement_chart_wide.png`.

### Files 
`main.py`: Orchestrates the entire pipeline. 

`yt_heatmap_getter.py`: Downloads SVG heatmaps from YouTube using Selenium. 

`svg_to_png.py`: Modifies the SVGs and converts them to PNG format. 

`data_extractor.py`: Extracts gray pixel data from PNGs to compute engagement metrics. 

`segments.py`: Defines the segments for a specific video. This data is currently hardcoded but will be extracted dynamically in future versions.

`avg_popularity_segment.py`: Calculates segment popularity based on engagement data and saves the results to a JSON file.

`grapher.py`: Generates a line plot of user engagement over time and saves it as a PNG file.
