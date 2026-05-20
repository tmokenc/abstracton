import pathlib
import sys
import re

benchmarks_folder = "benchmarks/dodo"
benchmarks_suffix = "json"
results_folder = pathlib.Path(__file__).parent / "results"

def line_to_min_sec(s):
    s1 = s.split("m")
    mins = int(''.join(c for c in s1[0] if c.isdigit()))
    secs = float(s1[1].replace("s", "").replace(",", "."))
    return (mins, secs)

def extract_name(line):
    # name_match = re.search(r"[^\/\s]*\.json", line) # problem: some benchmarks are named "main.json" with the folder giving the actual name...
    name_match = re.search(benchmarks_folder + r".*/([\S]*)\." + benchmarks_suffix, line)
    if name_match:
        return name_match.group(1)
    else:
        return None

def parse_list(l):
    return [i.strip() for i in l.strip("[]").split(",")]

def parse_raw_bench(f):
    results = []

    lines = [l.strip() for l in f.readlines()]

    for i in range(len(lines)):
        if benchmarks_folder in lines[i] and ("." + benchmarks_suffix) in lines[i]:
            # read result
            instance = {}
            instance["name"] = re.search(r"dodo/([\S]*)\.json", lines[i]).group(1)
            instance["interpretation"] = lines[i + 1].strip()
            instance["property"] = lines[i + 2].strip()
            j = i
            while not lines[j].startswith("******"):
                j += 1
                if lines[j].startswith("Result"):
                    if "[1]" in lines[j]:
                        instance["output"] = 1
                    elif "[0]" in lines[j]:
                        instance["output"] = 0
                    else:
                        raise Exception
                elif lines[j].startswith("real"):
                    # determine time instance needed
                    (m1, s1) = line_to_min_sec(lines[j + 1])
                    (m2, s2) = line_to_min_sec(lines[j + 2])
                    m = m1 + m2
                    s = s1 + s2
                    if s >= 60.0:
                        m = m + 1
                        s = s - 60.0
                    if "output" not in instance:
                        instance["output"] = "TIMEOUT"
                    else:
                        instance["time"] = "{:.3f}".format(60*m + s)
                elif "Killed" in lines[j]:
                    instance["output"] = "OOM"
            results.append(instance)

    return results

# take a list of dicts as input, extract keys, and build csv from it (with NA if a key is missing in a dict)
def print_as_csv(dicts, filename, header=True):
    # collect keys
    keys = set()
    for d in dicts:
        for k in d:
            keys.add(k)
    keys = list(keys)
    # write csv
    if header:
        filename.write(",".join(keys) + "\n")
    for d in dicts:
        d_keys = []
        for k in keys:
            if k in d:
                d_keys.append(str(d[k]))
            else:
                d_keys.append("NA")
        filename.write(",".join(d_keys) + "\n")

### dodo parsing ###

def parse_dodo(filename):
    results = []

    lines = [l.strip() for l in filename.readlines()]

    for i in range(len(lines)):
        if match := re.search(r"timeout 20m.*dodo\.jar oneshot (t|s|f) benchmark/([\S]*)\.json ([\S]*)", lines[i]):
            print(f"benchmark: {match.group(2)}, interpretation: {match.group(1)}, property: {match.group(3)}")

            # try to read answer
            result_with_cache = {}
            result_with_cache["name"] = match.group(2)
            result_with_cache["interpretation"] = match.group(1)
            result_with_cache["property"] = match.group(3)
            j = i + 1
            if lines[j].strip() == "{":
                while lines[j].strip() != "}":
                    if match := re.search(r"\"result\": \"([\S]*)\",", lines[j]):
                        # result_with_cache["output"] = match.group(1)
                        assert(match.group(1) == "success")
                        result_with_cache["output"] = 1
                    if match := re.search(r"\"counterexample\": ", lines[j]):
                        # result_with_cache["output"] = "failure"
                        result_with_cache["output"] = 0
                    if match := re.search(r"\"time\(ms\)\": ([0-9]*),", lines[j]):
                        result_with_cache["time"] = "{:.3f}".format(int(match.group(1)) / 1000)
                    j += 1
            else:
                result_with_cache["output"] = "TIMEOUT/OOM"
            # possible 2-factor optimization: set i to min(i, j - 1) here (or j...can't bother to modify for loop or find out how modifying loop variable in python behaves)
            results.append(result_with_cache)

    return results

# Generic pipeline: every *.log under results/raw/ is parsed into the equivalent
# *.csv under results/. dodo* logs are parsed with parse_dodo; everything else
# uses parse_raw_bench (the abstracton/mata raw-bench format).
raw_dir = results_folder / "raw"
raw_dir.mkdir(parents=True, exist_ok=True)

for log_path in sorted(raw_dir.glob("*.log")):
    csv_path = results_folder / (log_path.stem + ".csv")
    with open(log_path) as f:
        if log_path.stem.startswith("dodo"):
            rows = parse_dodo(f)
        else:
            rows = parse_raw_bench(f)
            rows = [{k: v for k, v in r.items() if k not in ["solved", "unsolved"]} for r in rows]
    with open(csv_path, "w") as out:
        print_as_csv(rows, out, True)
    print(f"{log_path.name} -> {csv_path.name} ({len(rows)} rows)")

# count OOM errors (kept as an out-of-band debug aid; unrelated to the *.log pipeline)
dodo_dbg_path = raw_dir / "dodo_oneshot_debug.txt"
if dodo_dbg_path.exists():
    with open(dodo_dbg_path) as dodo_dbg:
        oom_counter = 0
        for line in dodo_dbg.readlines():
            if re.search(r"Exception in thread", line):
                oom_counter += 1
        print(f"dodo: {oom_counter} OOMs")
