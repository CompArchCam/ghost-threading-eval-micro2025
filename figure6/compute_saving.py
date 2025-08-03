# === Heuristically selected workloads ===
selected_workloads = {
    "bc-kron", "bc-twitter", "bc-urand", 
    "bfs-kron", "bfs-twitter", "bfs-urand",
    "cc-kron", "cc-twitter", "cc-urand", "cc-web",
    "sssp-kron", "sssp-twitter", "sssp-urand", "sssp-web",
    "tc-urand", 
    "camel", "kangaroo", "hj2", "hj8"
}

# === Input/Output filenames ===
input_file = "energy.csv"
output_file = "energy_saving.csv"

# === Read and organize data ===
with open(input_file, "r") as f:
    lines = [line.strip() for line in f if line.strip()]

header = lines[0]
data_lines = lines[1:]

# Store as: workload -> variant -> energy
data = {}
workload_order = []

for line in data_lines:
    name, energy_str = line.split(",", 1)
    workload, variant = name.rsplit("-", 1)
    energy = None if energy_str.strip() == "NA" else float(energy_str.strip())
    if workload not in data:
        data[workload] = {}
        workload_order.append(workload)
    data[workload][variant] = energy

# === Compute savings and prepare output ===
output = ["workload,selected by heuristic,swpf saving,homp saving,ghost saving"]
swpf_savings, homp_savings, ghost_savings = [], [], []

def compute_saving(baseline, variant):
    if baseline is not None and variant not in (None, 0):
        return 1 - (variant / baseline) 
    else:
        return None

for workload in workload_order:
    variants = data[workload]
    baseline = variants.get("baseline")
    swpf = compute_saving(baseline, variants.get("swpf"))
    homp = compute_saving(baseline, variants.get("homp"))
    ghost = compute_saving(baseline, variants.get("htpf"))

    selected = 1 if workload in selected_workloads else 0
    if selected == 0:
        ghost = homp  # Override ghost if not selected

    def fmt(x):
        return f"{x:.2f}" if x is not None else "NA"

    # Track values for geomean
    if swpf not in (None, 0): swpf_savings.append(swpf)
    if homp not in (None, 0): homp_savings.append(homp)
    if ghost not in (None, 0): ghost_savings.append(ghost)

    output.append(f"{workload},{selected},{fmt(swpf)},{fmt(homp)},{fmt(ghost)}")

# === Add empty line and compute geomean row ===
output.append("")

def geomean(lst):
    prod = 1
    n = 0
    for x in lst:
        if x > 0:
            prod *= x
            n += 1
    return prod**(1/n) if n else 0

g_swpf = geomean(swpf_savings)
g_homp = geomean(homp_savings)
g_ghost = geomean(ghost_savings)

output.append(f"geomean,,{g_swpf:.2f},{g_homp:.2f},{g_ghost:.2f}")

# === Write output ===
with open(output_file, "w") as f:
    for line in output:
        f.write(line + "\n")
