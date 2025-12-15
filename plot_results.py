# Use: python plot_results.py

import pandas as pd
import matplotlib.pyplot as plt

# read csv
df = pd.read_csv("results.csv")


print("Columns in results.csv:", df.columns.tolist())
print(df.head())

# Example 1: Page faults vs frames for a given workload (Experiment 2: FramesSweep)

workload_name = "Mixed_multiprogrammed"

frames_df = df[
    (df["Experiment"] == "FramesSweep") &
    (df["Workload"] == workload_name)
]

plt.figure()
for alg in frames_df["Alg"].unique():
    sub = frames_df[frames_df["Alg"] == alg]
    sub = sub.sort_values("Frames")
    plt.plot(sub["Frames"], sub["Faults"], marker="o", label=alg)

plt.xlabel("Number of Frames")
plt.ylabel("Page Faults")
plt.title(f"Page Faults vs Frames ({workload_name})")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("faults_vs_frames_" + workload_name + ".png", dpi=300)

# Example 2: Thrashing events vs frames (same workload)

plt.figure()
for alg in frames_df["Alg"].unique():
    sub = frames_df[frames_df["Alg"] == alg]
    sub = sub.sort_values("Frames")
    plt.plot(sub["Frames"], sub["ThrashEvt"], marker="o", label=alg)

plt.xlabel("Number of Frames")
plt.ylabel("Thrashing Events")
plt.title(f"Thrashing Events vs Frames ({workload_name})")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("thrash_vs_frames_" + workload_name + ".png", dpi=300)

# Example 3: Window sweep for WS+LRU

win_df = df[
    (df["Experiment"] == "WindowSweep") &
    (df["Workload"] == "Locality_loop") &
    (df["Alg"] == "WS+LRU")
]

win_df = win_df.sort_values("Win")

plt.figure()
plt.plot(win_df["Win"], win_df["Faults"], marker="o")
plt.xlabel("Working-Set Window Size (W)")
plt.ylabel("Page Faults")
plt.title("WS+LRU: Page Faults vs Window Size (Locality_loop)")
plt.grid(True)
plt.tight_layout()
plt.savefig("faults_vs_window_locality.png", dpi=300)

plt.figure()
plt.plot(win_df["Win"], win_df["ThrashEvt"], marker="o")
plt.xlabel("Working-Set Window Size (W)")
plt.ylabel("Thrashing Events")
plt.title("WS+LRU: Thrashing vs Window Size (Locality_loop)")
plt.grid(True)
plt.tight_layout()
plt.savefig("thrash_vs_window_locality.png", dpi=300)

print("Plots saved as PNG files in current directory.")
