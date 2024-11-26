import os
import re
from datetime import datetime
import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import make_interp_spline


def extract_times_from_log(file_path):
    """Extract all individual times from the original log file"""
    times = []
    with open(file_path, 'r') as f:
        content = f.read()
        # Find the parent file path that generated this log
        parent_file = file_path.replace('.stats', '.txt')
        if os.path.exists(parent_file):
            # Read and parse the original data file to get individual times
            with open(parent_file, 'r') as data_file:
                for line in data_file:
                    if 'Chunk constructor' in line:
                        try:
                            parts = line.split(' - ')
                            if len(parts) == 2:
                                time_ms = float(parts[1].replace('ms', '').strip())
                                times.append(time_ms)
                        except ValueError:
                            continue
    return times

def create_comparison_plots(directory):
    # Dictionary to store data from each log file
    all_times = {}
    log_data = {}
    
    # Walk through the directory
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.stats'):
                file_path = os.path.join(root, file)
                
                # Extract times for box plot
                times = extract_times_from_log(file_path)
                if times:  # Only include if we got data
                    name = file.replace('.stats', '')
                    all_times[name] = times
                
                # Read the file and get the timestamp and statistics
                with open(file_path, 'r') as f:
                    content = f.read()
                    
                    # Get timestamp
                    timestamp_match = re.search(r'Generated on: (.*)', content)
                    timestamp = datetime.strptime(timestamp_match.group(1), '%Y-%m-%d %H:%M:%S') if timestamp_match else datetime.fromtimestamp(os.path.getmtime(file_path))
                    
                    # Extract chunk constructor statistics
                    summary_match = re.search(r'chunk_constructor_mean:([\d.]+),chunk_constructor_median:([\d.]+),chunk_constructor_95th:([\d.]+)', content)
                    if summary_match:
                        mean = float(summary_match.group(1))
                        median = float(summary_match.group(2))
                        percentile_95 = float(summary_match.group(3))
                        
                        log_data[timestamp] = {
                            'file': file,
                            'mean': mean,
                            'median': median,
                            'percentile_95': percentile_95
                        }
  
    # Sort all_times by median values in descending order
    sorted_all_times = dict(sorted(all_times.items(), key=lambda item: np.max(item[1]), reverse=True))
    
    # Plot 1: Box plot comparison
    fig, ax = plt.subplots(figsize=(20, 11))
    data = [times for times in sorted_all_times.values()]
    labels = list(sorted_all_times.keys())
    
    bp = ax.boxplot(data, labels=labels, patch_artist=True)
    
    for box in bp['boxes']:
        box.set(facecolor='lightblue', alpha=0.7)
    
    ax.set_title('Distribution Comparison Across Test Runs')
    ax.set_xlabel('Test Runs')
    ax.set_ylabel('Time (ms)')
    ax.grid(True, alpha=0.3)
    plt.setp(ax.xaxis.get_majorticklabels(), rotation=45, ha='right')
    
    plt.tight_layout()
    output_filename = f"{directory}/trends_boxplot.png"
    plt.savefig(output_filename)
    plt.close(fig)
    
    # Plot 2: Scatter plot of times across all test runs
    fig, ax = plt.subplots(figsize=(15, 10))
    for name, times in sorted_all_times.items():
        ax.scatter([name] * len(times), times, alpha=0.5, label=name)
    
    ax.set_title('Scatter Plot of Chunk Constructor Times')
    ax.set_xlabel('Test Runs')
    ax.set_ylabel('Time (ms)')
    ax.grid(True, alpha=0.3)
    plt.setp(ax.xaxis.get_majorticklabels(), rotation=45, ha='right')
    
    plt.tight_layout()
    output_filename = f"{directory}/trends_scatter_plot.png"
    plt.savefig(output_filename)
    plt.close(fig)

    # Plot 3: Violin plot
    fig, ax = plt.subplots(figsize=(15, 10))
    ax.violinplot(data, showmeans=True, showmedians=True)
    ax.set_title('Violin Plot of Chunk Generation Times')
    ax.set_xlabel('Test Runs')
    ax.set_ylabel('Time (ms)')
    # Set the x-tick positions and labels
    ax.set_xticks(np.arange(1, len(data) + 1))  # Positions for each violin plot
    ax.set_xticklabels(list(sorted_all_times.keys()), rotation=45, ha='right')
    plt.tight_layout()
    output_filename = f"{directory}/trends_violin_plot.png"
    plt.savefig(output_filename)
    plt.close(fig)

    # Write statistics summary to trends.txt
    stats_filename = os.path.join(directory, "trends.txt")
    with open(stats_filename, 'w') as f:
        f.write("\nChunk Generation Time Summary\n")
        f.write("-" * 80 + "\n")
        f.write(f"{'File':<20} {'Mean':>10} {'Median':>10} {'95th %':>10} {'Sample Size':>12}\n")
        f.write("-" * 80 + "\n")
        for name, times in sorted_all_times.items():
            f.write(f"{name:<20} {np.mean(times):>10.2f} {np.median(times):>10.2f} {np.percentile(times, 95):>10.2f} {len(times):>12}\n")


directory = 'data/chunk_feature_addition'
create_comparison_plots(directory)