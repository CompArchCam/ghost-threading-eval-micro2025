# ========== Config ==========
input_file = "time.csv"
output_file = "speedup.csv"

# List of workloads selected by heuristic
selected_workloads = {
    "bc-kron", "bc-twitter", "bc-urand",
    "bfs-kron", "bfs-twitter", "bfs-urand",
    "cc-kron", "cc-twitter", "cc-urand", "cc-web",
    "sssp-kron", "sssp-twitter", "sssp-urand", "sssp-web",
    "tc-urand",
    "camel", "kangaroo", "hj2", "hj8"
}
# ============================

# Read and parse input
with open(input_file, "r") as f:
    lines = f.readlines()

rows = [line.strip().split(",") for line in lines[1:] if line.strip()]
workload_order = []
data = {}

for full_name, time_str in rows:
    if '-' not in full_name:
        continue
    workload, variant = full_name.rsplit('-', 1)
    try:
        time = float(time_str)
    except:
        time = None

    if workload not in data:
        data[workload] = {}
        workload_order.append(workload)
    data[workload][variant] = time

# Compute speedups
output = ["workload,selected by heuristic,swpf speedup,homp speedup,ghost speedup"]
geo_swpf = []
geo_homp = []
geo_ghost = []

def record_geomean(lst, val):
    if val != "NA":
        try:
            lst.append(float(val))
        except:
            pass

def format_speedup(x):
    return f"{round(x, 2)}"

for workload in workload_order:
    baseline = data[workload].get("baseline")

    def compute_speedup(vname):
        vtime = data[workload].get(vname)
        if baseline is not None and vtime not in (None, 0):
            return format_speedup(baseline / vtime)
        else:
            return "NA"

    swpf_speedup = compute_speedup("swpf")
    homp_speedup = compute_speedup("homp")
    ghost_raw = compute_speedup("htpf")
    selected = "1" if workload in selected_workloads else "0"
    ghost_speedup = ghost_raw if selected == "1" else homp_speedup

    output.append(f"{workload},{selected},{swpf_speedup},{homp_speedup},{ghost_speedup}")

    record_geomean(geo_swpf, swpf_speedup)
    record_geomean(geo_homp, homp_speedup)
    record_geomean(geo_ghost, ghost_speedup)

# Compute geometric mean
import math

def geomean(lst):
    if not lst:
        return "NA"
    product = 1.0
    for x in lst:
        product *= x
    return format_speedup(product ** (1 / len(lst)))

output.append("")  # Empty row before geomean
geo_row = f"geomean,-,{geomean(geo_swpf)},{geomean(geo_homp)},{geomean(geo_ghost)}"
output.append(geo_row)

# Write output
with open(output_file, "w") as f:
    for line in output:
        f.write(line + "\n")
