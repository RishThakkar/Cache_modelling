import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

df = pd.read_csv("results.csv")

outdir = Path("plots")
outdir.mkdir(exist_ok=True)

def plot(exp, x, y, title, xlabel, filename):
    d = df[df["experiment"] == exp].sort_values(x)
    if d.empty:
        print(f"[WARN] No rows for experiment={exp}")
        return

    plt.figure()
    plt.plot(d[x], d[y], marker="o")
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(y)
    plt.grid(True)
    path = outdir / filename
    plt.savefig(path, dpi=200, bbox_inches="tight")
    plt.close()
    print(f"Wrote {path}")

plot("miss_vs_size", "cache_kb", "miss_rate",
     "Miss rate vs Cache size", "Cache size (KB)", "miss_vs_size.png")

plot("miss_vs_assoc", "assoc", "miss_rate",
     "Miss rate vs Associativity", "Associativity (ways)", "miss_vs_assoc.png")

plot("amat_vs_line", "line_size", "amat",
     "AMAT vs Line size", "Line size (bytes)", "amat_vs_line.png")

plot("conflict_stride", "stride_bytes", "miss_rate",
     "Conflict misses vs Stride", "Stride (bytes)", "conflict_stride.png")

plot("capacity_workingset", "working_set_kb", "miss_rate",
     "Capacity misses vs Working set", "Working set (KB)", "capacity_workingset.png")

print("Done. Open the PNGs in ./plots/")
