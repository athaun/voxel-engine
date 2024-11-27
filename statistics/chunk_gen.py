import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import os
from chunk_gen_trends import create_comparison_plots

def clean_and_parse_data(input_file):
    face_culling_times = []
    chunk_constructor_times = []

    with open(input_file, 'r') as file:
        for line in file:
            line = line.strip()
            if 'Face culling' in line or 'Chunk constructor' in line:
                try:
                    # Extract time value from the line
                    parts = line.split(' - ')
                    if len(parts) == 2:
                        time_ms = float(parts[1].replace('ms', '').strip())
                        if 'Face culling' in parts[0]:
                            face_culling_times.append(time_ms)
                        elif 'Chunk constructor' in parts[0]:
                            chunk_constructor_times.append(time_ms)
                except ValueError:
                    # Skip malformed lines
                    continue

    return np.array(face_culling_times), np.array(chunk_constructor_times)

def create_visualization_suite(input_file, face_culling_times, chunk_constructor_times):
    # Create a figure with 1x2 subplots
    fig, ax2 = plt.subplots(1, figsize=(15, 6))
    
    # 2. KDE (Kernel Density Estimation) Plot - smooth density
    sns.kdeplot(data=chunk_constructor_times, label='Chunk Constructor', ax=ax2, fill=True)
    ax2.set_title('Density Distribution of Chunk Generation Times')
    ax2.set_xlabel('Time (ms)')
    ax2.set_ylabel('Density')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()

    filename, _ = os.path.splitext(input_file)
    output_filename = f"{filename}.png"
    plt.savefig(output_filename)

def write_statistics_to_log(input_file, face_culling_times, chunk_constructor_times):
    # Create log filename by adding '_stats.log' to the base name
    base_name = input_file.rsplit('.', 1)[0]  # Remove extension
    log_file = f"{base_name}.stats"
    
    # Calculate statistics
    face_stats = {
        'mean': np.mean(face_culling_times),
        'median': np.median(face_culling_times),
        'std_dev': np.std(face_culling_times),
        'min': np.min(face_culling_times),
        'max': np.max(face_culling_times),
        'count': len(face_culling_times),
        '95th_percentile': np.percentile(face_culling_times, 95),
        '99th_percentile': np.percentile(face_culling_times, 99)
    }
    
    chunk_stats = {
        'mean': np.mean(chunk_constructor_times),
        'median': np.median(chunk_constructor_times),
        'std_dev': np.std(chunk_constructor_times),
        'min': np.min(chunk_constructor_times),
        'max': np.max(chunk_constructor_times),
        'count': len(chunk_constructor_times),
        '95th_percentile': np.percentile(chunk_constructor_times, 95),
        '99th_percentile': np.percentile(chunk_constructor_times, 99)
    }
    
    # Write to log file
    with open(log_file, 'w') as f:
        f.write(f"Statistics for {input_file}\n")
        f.write("=" * 50 + "\n\n")
        
        f.write("Face Culling Statistics (ms):\n")
        f.write("-" * 30 + "\n")
        for key, value in face_stats.items():
            f.write(f"{key}: {value:.2f}\n")
        
        f.write("\nChunk Constructor Statistics (ms):\n")
        f.write("-" * 30 + "\n")
        for key, value in chunk_stats.items():
            f.write(f"{key}: {value:.2f}\n")
        
        # Add timestamp
        from datetime import datetime
        f.write(f"\nGenerated on: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        
        # Add summary line for easy parsing
        f.write("\nSUMMARY (for parsing):\n")
        f.write(f"face_culling_mean:{face_stats['mean']:.2f},face_culling_median:{face_stats['median']:.2f},")
        f.write(f"face_culling_95th:{face_stats['95th_percentile']:.2f},")
        f.write(f"chunk_constructor_mean:{chunk_stats['mean']:.2f},chunk_constructor_median:{chunk_stats['median']:.2f},")
        f.write(f"chunk_constructor_95th:{chunk_stats['95th_percentile']:.2f}\n")

def process_all_chunk_gen_files(directory):
    # List all .txt files, excluding trends.txt
    txt_files = [
        os.path.join(directory, file) for file in os.listdir(directory)
        if file.endswith('.txt') and file != 'trends.txt'
    ]

    # Process each file
    for input_file in txt_files:
        face_culling_times, chunk_constructor_times = clean_and_parse_data(input_file)
        write_statistics_to_log(input_file, face_culling_times, chunk_constructor_times)
        create_visualization_suite(input_file, face_culling_times, chunk_constructor_times)
    
    create_comparison_plots(directory)

directory = 'data/chunk_feature_addition'
process_all_chunk_gen_files(directory)
