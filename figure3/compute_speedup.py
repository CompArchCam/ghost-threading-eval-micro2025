# Set file names
input_file = "time.csv"
output_file = "speedup.csv"

# Read and parse input
with open(input_file, "r") as f:
    lines = f.readlines()

# Skip header and parse rows
rows = [line.strip().split(",") for line in lines[1:] if line.strip()]
workload_order = []
data = {}

for full_name, time_str in rows:
    if '-' not in full_name:
        continue
    # Split full name into workload and variant from the right
    workload, variant = full_name.rsplit('-', 1)
    try:
        time = float(time_str)
    except:
        time = None

    if workload not in data:
        data[workload] = {}
        workload_order.append(workload)
    data[workload][variant] = time

# Prepare output
output = ["workload,swpf speedup,homp speedup,ghost speedup"]

def compute_speedup(baseline, variant_time):
    if baseline is not None and variant_time not in (None, 0):
        return f"{round(baseline / variant_time, 2)}"
    else:
        return "NA"

for workload in workload_order:
    variants = data[workload]
    baseline = variants.get("baseline")
    swpf_speedup = compute_speedup(baseline, variants.get("swpf"))
    homp_speedup = compute_speedup(baseline, variants.get("homp"))
    ghost_speedup = compute_speedup(baseline, variants.get("htpf"))

    output.append(f"{workload},{swpf_speedup},{homp_speedup},{ghost_speedup}")

# Write to output
with open(output_file, "w") as f:
    for line in output:
        f.write(line + "\n")
