import re

def parse_large_file(input_file, output_file):
    I_total = 0.0
    J_total = 0.0
    time_ttl_exact = None
    time_ttl_c = None

    with open(input_file, 'r') as file:
        lines = file.readlines()

    # Iterate through lines
    for i in range(len(lines)):
        line = lines[i]

        # Look for FLAG 1
        if "[FLAG 2] Fd1A = I * A:" in line:
            # Look 2 lines below for the time in seconds
            if i + 2 < len(lines):
                match = re.search(r"([\d\.e\-]+) sec", lines[i+2])
                if match:
                    I_total += float(match.group(1))

        # Look for FLAG 2
        elif "[FLAG 1] Fd1A = Fd1A * J:" in line:
            # Look 2 lines below for the time in seconds
            if i + 2 < len(lines):
                match = re.search(r"([\d\.e\-]+) sec", lines[i+2])
                if match:
                    J_total += float(match.group(1))

        # Look for LAGr_EdgeBetweennessCentrality time
        elif "Time for LAGr_EdgeBetweennessCentrality" in line:
            match = re.search(r"([\d\.e\-]+) sec", line)
            if match:
                time_ttl_exact = float(match.group(1))

        # Look for LG_check_edgeBetweennessCentrality time
        elif "Time for LG_check_edgeBetweennessCentrality" in line:
            match = re.search(r"([\d\.e\-]+) sec", line)
            if match:
                time_ttl_c = float(match.group(1))

    # Output to console with maximum precision
    print(f"I_total: {I_total:.16f}")
    print(f"J_total: {J_total:.16f}")
    if time_ttl_exact is not None:
        print(f"time_ttl_exact: {time_ttl_exact:.16f}")
    if time_ttl_c is not None:
        print(f"time_ttl_c: {time_ttl_c:.16f}")

    # Save output to a file
    with open(output_file, 'w') as out_file:
        out_file.write(f"I_total: {I_total:.16f}\n")
        out_file.write(f"J_total: {J_total:.16f}\n")
        if time_ttl_exact is not None:
            out_file.write(f"time_ttl_exact: {time_ttl_exact:.16f}\n")
        if time_ttl_c is not None:
            out_file.write(f"time_ttl_c: {time_ttl_c:.16f}\n")

if __name__ == "__main__":
    input_file = "output.txt"  # Replace with your file path
    output_file = "output_times.txt"  # File to save the results
    parse_large_file(input_file, output_file)
