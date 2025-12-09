#!/usr/bin/env python3
import subprocess
import csv
import os
import time
from datetime import datetime
import argparse

PROGRAMS = ["treap", "sorting", "matmul", "bsearch", "memthrash"]
PROGRAM_DIR = "programs"
TIME_CMD = "/usr/bin/time"  
LOADER = "./loader/target/release/fixed_loader"

def parse_time_output(stderr_text: str):
    data = {
        "user_time": 0.0, "sys_time": 0.0, "voluntary_ctx": 0,
        "involuntary_ctx": 0, "max_rss_kb": 0, "minor_faults": 0,
        "major_faults": 0
    }

    for line in stderr_text.splitlines():
        line = line.strip()
        if line.startswith("User time (seconds):"):
            try: data["user_time"] = float(line.split(":", 1)[1].strip())
            except ValueError: pass
        elif line.startswith("System time (seconds):"):
            try: data["sys_time"] = float(line.split(":", 1)[1].strip())
            except ValueError: pass
        elif line.startswith("Voluntary context switches:"):
            try: data["voluntary_ctx"] = int(line.split(":", 1)[1].strip())
            except ValueError: pass
        elif line.startswith("Involuntary context switches:"):
            try: data["involuntary_ctx"] = int(line.split(":", 1)[1].strip())
            except ValueError: pass
        elif line.startswith("Maximum resident set size (kbytes):"):
            try: data["max_rss_kb"] = int(line.split(":", 1)[1].strip())
            except ValueError: pass
        elif line.startswith("Minor (reclaiming a frame) page faults:"):
            try: data["minor_faults"] = int(line.split(":", 1)[1].strip())
            except ValueError: pass
        elif line.startswith("Major (requiring I/O) page faults:"):
            try: data["major_faults"] = int(line.split(":", 1)[1].strip())
            except ValueError: pass
            
    return data


def run_batch_concurrent(commands):
    """
    Exec variant: Spawns N processes sequentially.
    Returns: Aggregated stats (Sum of CPU/RSS, Max Wall Time).
    """
    procs = []
    stderrs = []

    start_ns = time.perf_counter_ns()
    for cmd in commands:
        p = subprocess.Popen(
            [TIME_CMD, "-v"] + cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        procs.append(p)

    for p in procs:
        _, stderr = p.communicate()
        stderrs.append(stderr)
    end_ns = time.perf_counter_ns()

    agg = {
        "user_time": 0.0,
        "sys_time": 0.0,
        "voluntary_ctx": 0,
        "involuntary_ctx": 0,
        "max_rss_kb": 0,
        "minor_faults": 0,
        "major_faults": 0,
        "exit_code": 0
    }

    for i, p in enumerate(procs):
        parsed = parse_time_output(stderrs[i])
        if p.returncode != 0:
            agg["exit_code"] = p.returncode
            print(f"[WARN] Worker {i} failed: {p.returncode}")
        for k in parsed:
            agg[k] += parsed[k]

    agg["wall_time"] = (end_ns - start_ns) / 1e9
    return agg


def run_batch_simple(commands):
    """
    Lib/Clam variant: Runs a single Loader process with many arguments.
    Returns: Stats of that single loader process.
    """
    cmd = commands[0]
    full_cmd = [TIME_CMD, "-v"] + cmd
    
    start_ns = time.perf_counter_ns()
    p = subprocess.Popen(
        full_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    _, stderr = p.communicate()
    end_ns = time.perf_counter_ns()

    parsed = parse_time_output(stderr)
    parsed["exit_code"] = p.returncode
    parsed["wall_time"] = (end_ns - start_ns) / 1e9
    
    if p.returncode != 0:
        print(f"[WARN] Loader failed: {p.returncode}")
        
    return parsed


def get_benchmark_config(prog: str, variant: str, concurrency: int):
    if variant == "exec":
        exe_path = os.path.join(PROGRAM_DIR, f"{prog}_exec")
        cmd = [exe_path]
        return [cmd for _ in range(concurrency)], [exe_path]

    elif variant == "lib":
        so_path = os.path.join(PROGRAM_DIR, f"{prog}_lib.so")
        cmd = [LOADER] + [so_path] * concurrency
        return [cmd], [LOADER, so_path]

    elif variant == "clam":
        so_path = os.path.join(PROGRAM_DIR, f"{prog}_clam.so")
        cmd = [LOADER] + [so_path] * concurrency
        return [cmd], [LOADER, so_path]

    else:
        raise ValueError(f"Unknown variant: {variant}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--trials", type=int, default=5)
    parser.add_argument("--concurrency", type=int, default=8)
    parser.add_argument("--out-prefix", type=str, default="results")
    args = parser.parse_args()

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    out_csv = f"{args.out_prefix}_{timestamp}.csv"

    fieldnames = [
        "program", "variant", "concurrency",
        "exit_code", "avg_wall_time", "avg_user_time", "avg_sys_time",
        "avg_voluntary_ctx", "avg_involuntary_ctx", "avg_max_rss_kb",
        "avg_minor_faults", "avg_major_faults"
    ]

    with open(out_csv, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()

        variants = ["exec", "lib", "clam"]

        for prog in PROGRAMS:
            for variant in variants:
                commands, to_check = get_benchmark_config(prog, variant, args.concurrency)

                if any(not os.path.exists(p) for p in to_check):
                    print(f"[WARN] Missing resources for {prog} {variant}, skipping.")
                    continue

                print(f"[+] Benchmarking {prog} ({variant}) concurrency={args.concurrency}")

                collected = []
                for trial in range(1, args.trials + 1):
                    print(f"    Trial {trial}/{args.trials}...")
                    if variant == "exec":
                        res = run_batch_concurrent(commands)
                    else:
                        res = run_batch_simple(commands)
                    collected.append(res)

                def avg(key):
                    return sum(r[key] for r in collected) / len(collected)

                avg_row = {
                    "program": prog,
                    "variant": variant,
                    "concurrency": args.concurrency,
                    "exit_code": max(r["exit_code"] for r in collected),
                    "avg_wall_time": avg("wall_time"),
                    "avg_user_time": avg("user_time"),
                    "avg_sys_time": avg("sys_time"),
                    "avg_voluntary_ctx": avg("voluntary_ctx"),
                    "avg_involuntary_ctx": avg("involuntary_ctx"),
                    "avg_max_rss_kb": avg("max_rss_kb"),
                    "avg_minor_faults": avg("minor_faults"),
                    "avg_major_faults": avg("major_faults"),
                }

                writer.writerow(avg_row)

    print(f"[+] Results written to {out_csv}")


if __name__ == "__main__":
    main()
