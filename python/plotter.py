import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

CSV_PATH = "results.csv"          # run from repo root
OUT_DIR = Path("python/plots")
OUT_DIR.mkdir(parents=True, exist_ok=True)

df = pd.read_csv(CSV_PATH)

# Candidate sweep axes in priority order
CANDIDATE_X = [
    "cache_kb",
    "working_set_kb",
    "assoc",
    "line_size",
    "stride_bytes",
    "miss_penalty",
    "hit_latency",
    "policy",  # categorical
]

METRICS = ["miss_rate", "amat"]

def pick_x_column(d: pd.DataFrame) -> str | None:
    """
    Pick an x-axis column automatically:
    - Prefer numeric columns that vary (nunique > 1)
    - If only 'policy' varies, use policy
    - If multiple vary, pick the first by priority in CANDIDATE_X
    """
    nunique = {c: d[c].nunique(dropna=False) for c in CANDIDATE_X if c in d.columns}

    # Pick first varying column by priority
    for c in CANDIDATE_X:
        if c in nunique and nunique[c] > 1:
            return c
    return None

def is_categorical_x(xcol: str) -> bool:
    return xcol == "policy"

def stable_sort(d: pd.DataFrame, xcol: str) -> pd.DataFrame:
    d = d.copy()
    if xcol == "policy":
        order = ["LRU", "FIFO", "RANDOM"]
        if "policy" in d.columns:
            d.loc[:, "policy"] = pd.Categorical(d["policy"], categories=order, ordered=True)
        return d.sort_values("policy")
    else:
        return d.sort_values(xcol)

def plot_line(d: pd.DataFrame, xcol: str, ycol: str, title: str, xlabel: str, outpath: Path):
    d = stable_sort(d, xcol)
    plt.figure()
    plt.plot(d[xcol], d[ycol], marker="o")
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ycol)
    plt.grid(True)
    plt.savefig(outpath, dpi=200, bbox_inches="tight")
    plt.close()
    print(f"Wrote {outpath}")

def plot_bar(d: pd.DataFrame, xcol: str, ycol: str, title: str, xlabel: str, outpath: Path):
    d = stable_sort(d, xcol)
    plt.figure()
    plt.bar(d[xcol].astype(str), d[ycol])
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ycol)
    plt.grid(True, axis="y")
    plt.savefig(outpath, dpi=200, bbox_inches="tight")
    plt.close()
    print(f"Wrote {outpath}")

def safe_name(s: str) -> str:
    return "".join(ch if ch.isalnum() or ch in ("-", "_") else "_" for ch in s)

# Optional: show baseline rows in terminal if present
if "experiment" in df.columns:
    base = df[df["experiment"].str.contains("baseline", na=False)]
    if not base.empty:
        print("Baseline rows:")
        print(base.to_string(index=False))
        print()

# Generate plots per experiment
experiments = sorted(df["experiment"].unique())

for exp in experiments:
    d = df[df["experiment"] == exp].copy()
    if d.empty:
        continue

    xcol = pick_x_column(d)
    if xcol is None:
        print(f"[SKIP] {exp}: No varying parameter found (all candidate x columns constant).")
        continue

    # Create a short description string for titles
    # We'll show non-varying key params to give context in the title.
    context_cols = ["cache_kb", "line_size", "assoc", "hit_latency", "miss_penalty", "policy", "trace"]
    context_parts = []
    for c in context_cols:
        if c in d.columns and d[c].nunique(dropna=False) == 1 and c != xcol:
            context_parts.append(f"{c}={d[c].iloc[0]}")
    context = ", ".join(context_parts[:4])  # keep title short

    xlabel = xcol
    if xcol == "cache_kb":
        xlabel = "Cache size (KB)"
    elif xcol == "working_set_kb":
        xlabel = "Working set (KB)"
    elif xcol == "assoc":
        xlabel = "Associativity (ways)"
    elif xcol == "line_size":
        xlabel = "Line size (bytes)"
    elif xcol == "stride_bytes":
        xlabel = "Stride (bytes)"
    elif xcol == "miss_penalty":
        xlabel = "Miss penalty (cycles)"
    elif xcol == "hit_latency":
        xlabel = "Hit latency (cycles)"
    elif xcol == "policy":
        xlabel = "Replacement policy"

    for metric in METRICS:
        if metric not in d.columns:
            continue

        title = f"{exp}: {metric} vs {xcol}"
        if context:
            title += f" ({context})"

        outpath = OUT_DIR / f"{safe_name(exp)}_{metric}.png"

        if is_categorical_x(xcol):
            plot_bar(d, xcol, metric, title, xlabel, outpath)
        else:
            plot_line(d, xcol, metric, title, xlabel, outpath)

print("\nDone. Open images in:", OUT_DIR)
