import matplotlib.pyplot as plt

watermark_text = "arush.sharma@students.iiit.ac.in" 

data = {
    4: {"ticks": [1, 5, 10, 48, 50], "queues": [0, 1, 2, 0, 1]},
    5: {"ticks": [2, 7, 15, 48, 55], "queues": [0, 1, 2, 0, 2]},
    6: {"ticks": [3, 9, 20, 48, 60], "queues": [0, 1, 3, 0, 3]}
}

plt.figure(figsize=(10, 6))

colors = ['r', 'g', 'b', 'c', 'm', 'y']
color_idx = 0

for pid, points in data.items():
    plt.scatter(points["ticks"], points["queues"], color=colors[color_idx % len(colors)], label=f"PID {pid}", s=50)
    plt.plot(points["ticks"], points["queues"], color=colors[color_idx % len(colors)], alpha=0.5)
    color_idx += 1

# Formatting
plt.yticks([0, 1, 2, 3])
plt.gca().invert_yaxis() # Highest priority (0) at top
plt.xlabel("Time Elapsed (Ticks)")
plt.ylabel("Queue Level (0=Highest, 3=Lowest)")
plt.title("MLFQ Process Queue Transitions over Time")
plt.legend()

# Watermark requirement
plt.text(0.5, 0.5, watermark_text, fontsize=40, color='gray', alpha=0.2, 
         ha='center', va='center', transform=plt.gca().transAxes)

plt.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()
plt.savefig("mlfq_plot.png")
plt.show()