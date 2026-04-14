#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BENCHMARKS_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(cd "${BENCHMARKS_DIR}/.." && pwd)"
RESULTS_DIR="${SCRIPT_DIR}/results/raw"

resolve_solver() {
    local candidate
    for candidate in \
        "${REPO_ROOT}/build/benchmarks/solve_dodo" \
        "${BENCHMARKS_DIR}/build/solve_dodo"
    do
        if [[ -x "${candidate}" ]]; then
            printf '%s\n' "${candidate}"
            return 0
        fi
    done

    echo "Could not find solve_dodo binary." >&2
    echo "Expected one of:" >&2
    echo "  ${REPO_ROOT}/build/benchmarks/solve_dodo" >&2
    echo "  ${BENCHMARKS_DIR}/build/solve_dodo" >&2
    exit 1
}

mkdir -p "${RESULTS_DIR}"

SOLVER="$(resolve_solver)"

"${SCRIPT_DIR}/bench.sh" "${SOLVER} -i t" "${BENCHMARKS_DIR}/dodo" "*.json" >> "${RESULTS_DIR}/trap.txt" 2>&1
"${SCRIPT_DIR}/bench.sh" "${SOLVER} -i s" "${BENCHMARKS_DIR}/dodo" "*.json" >> "${RESULTS_DIR}/siphon.txt" 2>&1
"${SCRIPT_DIR}/bench.sh" "${SOLVER} -i f" "${BENCHMARKS_DIR}/dodo" "*.json" >> "${RESULTS_DIR}/flow.txt" 2>&1
