#!/bin/bash

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RAW_DIR="${SCRIPT_DIR}/results/raw"

mkdir -p "${RAW_DIR}"

"${SCRIPT_DIR}/bench_for_dodo_comparison.sh"                 2>&1 | tee -a "${RAW_DIR}/bench_base_explicit.log"
"${SCRIPT_DIR}/bench_for_dodo_comparison_lazy.sh"            2>&1 | tee -a "${RAW_DIR}/bench_base_lazy.log"
"${SCRIPT_DIR}/bench_for_dodo_comparison_antichains.sh"      2>&1 | tee -a "${RAW_DIR}/bench_base_antichains.log"
"${SCRIPT_DIR}/bench_for_dodo_comparison_antichains-incl.sh" 2>&1 | tee -a "${RAW_DIR}/bench_base_antichains_incl.log"
"${SCRIPT_DIR}/bench_for_dodo_comparison_mata_lazy.sh"       2>&1 | tee -a "${RAW_DIR}/bench_mata_lazy.log"
