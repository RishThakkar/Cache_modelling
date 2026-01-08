import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

# ---- Config ----
CSV_PATH = "results.csv"   # run from repo root
OUT_DIR = Path("python/plots")
OUT_DIR.mkdir(parents=True, exist_ok=True)

df = pd.read_csv(CSV_PATH)

def save_line_plot(d, x, y, title, xlabel, ylabel, filename):
    if d.empty:
        print(f"[WARN] Empty dataset for: {title}")
        return

    d = d.sort_values(x)
    plt.figure()
    plt.plot(d[x], d[y], marker="o")
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.grid(True)
    path = OUT_DIR / filename
    plt.savefig(path, dpi=200, bbox_inches="tight")
    plt.close()
    print(f"Wrote {path}")

def save_bar_plot(d, x, y, title, xlabel, ylabel, filename):
    if d.empty:
        print(f"[WARN] Empty dataset for: {title}")
        return

    # Keep a stable order (use current appearance order unless you want alphabetical)
    if x == "policy":
        order = ["LRU", "FIFO", "RANDOM"]
        d["policy"] = pd.Categorical(d["policy"], categories=order, ordered=True)
        d = d.sort_values("policy")
    else:
        d = d.sort_values(x)

    plt.figure()
    plt.bar(d[x].astype(str), d[y])
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.grid(True, axis="y")
    path = OUT_DIR / filename
    plt.savefig(path, dpi=200, bbox_inches="tight")
    plt.close()
    print(f"Wrote {path}")

# ---------------------------
# Baseline (single row)
# ---------------------------
baseline = df[df["experiment"] == "baseline"]
if not baseline.empty:
    row = baseline.iloc[0]
    print("Baseline:")
    print(row.to_string())
    print()

# ---------------------------
# 1) Cache size sweep
# ---------------------------
d = df[df["experiment"] == "sweep_cache_size"]
save_line_plot(
    d, x="cache_kb", y="miss_rate",
    title="Miss rate vs Cache size (reuse working set)",
    xlabel="Cache size (KB)", ylabel="Miss rate",
    filename="sweep_cache_size_miss_rate.png"
)
save_line_plot(
    d, x="cache_kb", y="amat",
    title="AMAT vs Cache size (reuse working set)",
    xlabel="Cache size (KB)", ylabel="AMAT (cycles)",
    filename="sweep_cache_size_amat.png"
)

# ---------------------------
# 2) Associativity sweep
# ---------------------------
d = df[df["experiment"] == "sweep_associativity"]
save_line_plot(
    d, x="assoc", y="miss_rate",
    title="Miss rate vs Associativity (same-set conflict trace)",
    xlabel="Associativity (ways)", ylabel="Miss rate",
    filename="sweep_associativity_miss_rate.png"
)
save_line_plot(
    d, x="assoc", y="amat",
    title="AMAT vs Associativity (same-set conflict trace)",
    xlabel="Associativity (ways)", ylabel="AMAT (cycles)",
    filename="sweep_associativity_amat.png"
)

# ---------------------------
# 3) Line size sweep
# ---------------------------
d = df[df["experiment"] == "sweep_line_size"]
save_line_plot(
    d, x="line_size", y="miss_rate",
    title="Miss rate vs Line size (sequential stream)",
    xlabel="Line size (bytes)", ylabel="Miss rate",
    filename="sweep_line_size_miss_rate.png"
)
save_line_plot(
    d, x="line_size", y="amat",
    title="AMAT vs Line size (sequential stream)",
    xlabel="Line size (bytes)", ylabel="AMAT (cycles)",
    filename="sweep_line_size_amat.png"
)

# ---------------------------
# 4) Replacement policy sweep (categorical)
# ---------------------------
d = df[df["experiment"] == "sweep_policy"]
save_bar_plot(
    d, x="policy", y="miss_rate",
    title="Miss rate vs Replacement policy (same-set conflict trace)",
    xlabel="Policy", ylabel="Miss rate",
    filename="sweep_policy_miss_rate.png"
)
save_bar_plot(
    d, x="policy", y="amat",
    title="AMAT vs Replacement policy (same-set conflict trace)",
    xlabel="Policy", ylabel="AMAT (cycles)",
    filename="sweep_policy_amat.png"
)

print("\nDone. Open images in:", OUT_DIR)
