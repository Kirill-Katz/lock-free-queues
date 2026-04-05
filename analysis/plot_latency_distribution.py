import csv
import glob
import os
import sys
import matplotlib.pyplot as plt
from collections import defaultdict

def weighted_percentile(latencies, counts, percentile):
    total = sum(counts)
    threshold = total * percentile
    acc = 0
    for latency, count in zip(latencies, counts):
        acc += count
        if acc >= threshold:
            return latency
    return latencies[-1]

def load_and_merge_consumer_csvs(indir):
    merged = defaultdict(int)

    pattern = os.path.join(indir, "consumer_latency_*.csv")
    files = sorted(glob.glob(pattern))

    if not files:
        raise RuntimeError(f"No consumer_latency_*.csv files found in {indir}")

    for path in files:
        with open(path, "r", newline="") as f:
            reader = csv.DictReader(f)
            for row in reader:
                latency = int(row["latency_ns"])
                count = int(row["count"])
                merged[latency] += count

    return merged

def plot_combined_latency_distribution(indir, outfile):
    merged = load_and_merge_consumer_csvs(indir)

    data = sorted(merged.items())  # (latency, count)
    latencies, counts = zip(*data)

    p50  = weighted_percentile(latencies, counts, 0.50)
    p95  = weighted_percentile(latencies, counts, 0.95)
    p99  = weighted_percentile(latencies, counts, 0.99)
    p999 = weighted_percentile(latencies, counts, 0.999)

    total_count = sum(counts)
    avg_latency = sum(l * c for l, c in zip(latencies, counts)) / total_count

    # clip for plotting
    clipped = [(l, c) for l, c in data if l <= 300]
    if not clipped:
        raise RuntimeError("No data ≤ 300 ns")

    bx, by = zip(*clipped)

    plt.figure(figsize=(10, 6))
    plt.bar(bx, by, width=1.0)

    plt.xlabel("Latency (ns)")
    plt.ylabel("Count")
    plt.title("Combined Consumer Latency Distribution")

    text = (
        f"samples = {total_count}\n"
        f"avg  = {avg_latency:.2f} ns\n"
        f"p50  = {p50} ns\n"
        f"p95  = {p95} ns\n"
        f"p99  = {p99} ns\n"
        f"p999 = {p999} ns"
    )

    plt.text(
        0.98, 0.95,
        text,
        transform=plt.gca().transAxes,
        fontsize=10,
        verticalalignment="top",
        horizontalalignment="right",
        bbox=dict(boxstyle="round", facecolor="white", alpha=0.85)
    )

    plt.axvline(p50, linestyle="--", linewidth=2, color="red", alpha=0.8, label="p50")
    plt.axvline(p99, linestyle="--", linewidth=2, color="orange", alpha=0.8, label="p99")

    plt.legend()
    plt.tight_layout()
    plt.savefig(outfile, dpi=300)
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("usage: python plot_consumers.py <input_dir> <output_png>")
        sys.exit(1)

    indir = sys.argv[1]
    outfile = sys.argv[2]

    plot_combined_latency_distribution(indir, outfile)

