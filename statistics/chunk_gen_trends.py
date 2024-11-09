import os
import re
from datetime import datetime
import matplotlib.pyplot as plt
import numpy as np

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

# def create_comparison_plots(directory):
#     # Dictionary to store data from each log file
#     all_times = {}
#     log_data = {}
    
#     # Walk through the directory
#     for root, dirs, files in os.walk(directory):
#         for file in files:
#             if file.endswith('.stats'):
#                 file_path = os.path.join(root, file)
                
#                 # Extract times for box plot
#                 times = extract_times_from_log(file_path)
#                 if times:  # Only include if we got data
#                     name = file.replace('.stats', '')
#                     all_times[name] = times
                
#                 # Read the file and get the timestamp and statistics
#                 with open(file_path, 'r') as f:
#                     content = f.read()
                    
#                     # Get timestamp
#                     timestamp_match = re.search(r'Generated on: (.*)', content)
#                     timestamp = datetime.strptime(timestamp_match.group(1), '%Y-%m-%d %H:%M:%S') if timestamp_match else datetime.fromtimestamp(os.path.getmtime(file_path))
                    
#                     # Extract chunk constructor statistics
#                     summary_match = re.search(r'chunk_constructor_mean:([\d.]+),chunk_constructor_median:([\d.]+),chunk_constructor_95th:([\d.]+)', content)
#                     if summary_match:
#                         mean = float(summary_match.group(1))
#                         median = float(summary_match.group(2))
#                         percentile_95 = float(summary_match.group(3))
                        
#                         log_data[timestamp] = {
#                             'file': file,
#                             'mean': mean,
#                             'median': median,
#                             'percentile_95': percentile_95
#                         }
    
#     # Create the plots
#     fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(15, 12), height_ratios=[1, 1])
    
#     # 1. Trend plot
#     sorted_times = sorted(log_data.keys())
#     files = [log_data[t]['file'].replace('.stats', '') for t in sorted_times]
#     means = [log_data[t]['mean'] for t in sorted_times]
#     medians = [log_data[t]['median'] for t in sorted_times]
#     percentiles_95 = [log_data[t]['percentile_95'] for t in sorted_times]
    
#     ax1.plot(range(len(files)), means, 'o-', label='Mean', color='blue', alpha=0.7)
#     ax1.plot(range(len(files)), medians, 's-', label='Median', color='green', alpha=0.7)
#     ax1.plot(range(len(files)), percentiles_95, '^-', label='95th Percentile', color='red', alpha=0.7)
    
#     ax1.set_title('Chunk Constructor Performance Trend', pad=20)
#     ax1.set_xlabel('Test Runs')
#     ax1.set_ylabel('Time (ms)')
#     ax1.grid(True, alpha=0.3)
#     ax1.legend()
    
#     # Rotate x labels
#     ax1.set_xticks(range(len(files)))
#     ax1.set_xticklabels(files, rotation=45, ha='right')
    
#     # 2. Box plot comparison
#     data = [times for times in all_times.values()]
#     labels = list(all_times.keys())
    
#     bp = ax2.boxplot(data, labels=labels, patch_artist=True)
    
#     # Customize box plot colors
#     for box in bp['boxes']:
#         box.set(facecolor='lightblue', alpha=0.7)
    
#     ax2.set_title('Distribution Comparison Across Test Runs')
#     ax2.set_xlabel('Test Runs')
#     ax2.set_ylabel('Time (ms)')
#     ax2.grid(True, alpha=0.3)
#     plt.setp(ax2.xaxis.get_majorticklabels(), rotation=45, ha='right')
    
#     plt.tight_layout()

#     output_filename = f"{directory}/trends.png"
#     plt.savefig(output_filename)

#     # plt.show()

#     # Write statistics summary to trends.log
#     stats_filename = os.path.join(directory, "trends.log")
#     with open(stats_filename, 'w') as f:
#         f.write("\nChunk Generation Time Summary\n")
#         f.write("-" * 80 + "\n")
#         f.write(f"{'File':<20} {'Mean':>10} {'Median':>10} {'95th %':>10} {'Sample Size':>12}\n")
#         f.write("-" * 80 + "\n")
#         for name, times in all_times.items():
#             f.write(f"{name:<20} {np.mean(times):>10.2f} {np.median(times):>10.2f} {np.percentile(times, 95):>10.2f} {len(times):>12}\n")

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
    sorted_all_times = dict(sorted(all_times.items(), key=lambda item: np.median(item[1]), reverse=True))
    
    fig, ax = plt.subplots(figsize=(15, 8))
    
    # Box plot comparison
    data = [times for times in sorted_all_times.values()]
    labels = list(sorted_all_times.keys())
    
    bp = ax.boxplot(data, labels=labels, patch_artist=True)
    
    # Customize box plot colors
    for box in bp['boxes']:
        box.set(facecolor='lightblue', alpha=0.7)
    
    ax.set_title('Distribution Comparison Across Test Runs')
    ax.set_xlabel('Test Runs')
    ax.set_ylabel('Time (ms)')
    ax.grid(True, alpha=0.3)
    plt.setp(ax.xaxis.get_majorticklabels(), rotation=45, ha='right')
    
    plt.tight_layout()

    output_filename = f"{directory}/trends.png"
    plt.savefig(output_filename)

    # plt.show()

    # Write statistics summary to trends.log
    stats_filename = os.path.join(directory, "trends.log")
    with open(stats_filename, 'w') as f:
        f.write("\nChunk Generation Time Summary\n")
        f.write("-" * 80 + "\n")
        f.write(f"{'File':<20} {'Mean':>10} {'Median':>10} {'95th %':>10} {'Sample Size':>12}\n")
        f.write("-" * 80 + "\n")
        for name, times in sorted_all_times.items():
            f.write(f"{name:<20} {np.mean(times):>10.2f} {np.median(times):>10.2f} {np.percentile(times, 95):>10.2f} {len(times):>12}\n")


# Usage
directory = 'data/chunk_gen'  # Replace with your directory path
create_comparison_plots(directory)