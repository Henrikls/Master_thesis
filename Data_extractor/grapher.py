import pandas as pd
import matplotlib.pyplot as plt

def plot_engagement(input_csv, output_png):
    """Plot and save user engagement chart."""
    # Load data from CSV file
    engagement_data = pd.read_csv(input_csv)

    # Extract relevant columns for plotting
    time_seconds = engagement_data['Time (s)']
    engagement_percentage = engagement_data['Normalized Engagement (%)']

    # Create a line plot with a wider aspect ratio
    plt.figure(figsize=(15, 6))  # Increase the width by adjusting figsize

    # Plot Normalized Engagement
    plt.plot(time_seconds, engagement_percentage, color='blue', label='User Engagement')

    # Add labels and title
    plt.xlabel('Time (s)')
    plt.ylabel('User Engagement (%)')
    plt.title('User Engagement Over Time')

    # Customize x-axis ticks to show intervals of 100 seconds
    plt.xticks(range(0, int(time_seconds.max()) + 1, 100))

    # Add grid, legend, and save the plot
    plt.grid(True)
    plt.legend()
    plt.savefig(output_png)
    plt.close()

    print(f"Engagement chart saved to {output_png}")
