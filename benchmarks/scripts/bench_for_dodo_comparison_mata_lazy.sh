#!/bin/env bash

set -uo pipefail

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
benchmarkdir="${BENCHMARKS_DIR}/dodo"

INPUTS=(
  [0]="t s f tf sf ts tsf:$benchmarkdir/Berkeley.json exclusiveexclusive"
  [1]="t s f sf:$benchmarkdir/Berkeley.json exclusiveunowned"
  [2]="t s f sf:$benchmarkdir/Berkeley.json exclusivenonexclusive"
  [3]="t s f:$benchmarkdir/Berkeley.json deadlock"
  [4]="t s f sf:$benchmarkdir/Burns.json nomutex"
  [5]="t s f:$benchmarkdir/Burns.json deadlock"
  [6]="t s f:$benchmarkdir/Dijkstra-Scholten.json twob"
  [7]="t s f:$benchmarkdir/Dijkstra-Scholten.json twod"
  [8]="t s f:$benchmarkdir/Dijkstra-Scholten.json deadlock"
  [9]="t s f ts:$benchmarkdir/Dijkstra-ring.json nomutex"
  [10]="t s f:$benchmarkdir/Dijkstra-ring.json deadlock"
  [11]="t s f sf:$benchmarkdir/DijkstraMutEx.json nomutex"
  [12]="t s f:$benchmarkdir/DijkstraMutEx.json deadlock"
  [13]="t s f sf:$benchmarkdir/FutureBus.json pendingrightsecond"
  [14]="t s f tf sf ts tsf:$benchmarkdir/FutureBus.json sharedexclusive"
  [15]="t s f sf:$benchmarkdir/FutureBus.json secondpending"
  [16]="t s f tf sf ts tsf:$benchmarkdir/FutureBus.json exclusiveexclusive"
  [17]="t s f:$benchmarkdir/FutureBus.json deadlock"
  [18]="t s f sf:$benchmarkdir/Herman.json notoken"
  [19]="t s f tf sf ts tsf:$benchmarkdir/Illinois.json dirtydirty"
  [20]="t s f tf sf ts tsf:$benchmarkdir/Illinois.json dirtyshared"
  [21]="t s f:$benchmarkdir/Illinois.json deadlock"
  [22]="t s f sf:$benchmarkdir/Israeli-Jafon.json notoken"
  [23]="t s f sf:$benchmarkdir/MESI.json modifiedmodified"
  [24]="t s f sf:$benchmarkdir/MESI.json sharedmodified"
  [25]="t s f:$benchmarkdir/MESI.json deadlock"
  [26]="t s f sf:$benchmarkdir/MOESI.json modifiedmodified"
  [27]="t s f sf:$benchmarkdir/MOESI.json exclusiveexclusive"
  [28]="t s f sf:$benchmarkdir/MOESI.json sharedexclusive"
  [29]="t s f sf:$benchmarkdir/MOESI.json ownedexclusive"
  [30]="t s f sf:$benchmarkdir/MOESI.json exclusivemodified"
  [31]="t s f sf:$benchmarkdir/MOESI.json ownedmodified"
  [32]="t s f sf:$benchmarkdir/MOESI.json sharedmodified"
  [33]="t s f:$benchmarkdir/MOESI.json deadlock"
  [34]="t s f tf sf ts tsf:$benchmarkdir/Szymanski.json nomutex"
  [35]="t s f:$benchmarkdir/Szymanski.json deadlock"
  [36]="t s f sf:$benchmarkdir/atomic-dining-philosophers.json deadlock"
  [37]="t s f sf:$benchmarkdir/bakery.json nomutex"
  [38]="t s f:$benchmarkdir/dining-cryptographers.json internal"
  [39]="t s f:$benchmarkdir/dining-cryptographers.json external"
  [40]="t s f sf:$benchmarkdir/dragon.json dirtydirty"
  [41]="t s f sf:$benchmarkdir/dragon.json exclusiveexclusive"
  [42]="t s f sf:$benchmarkdir/dragon.json dirtysharedexclusive"
  [43]="t s f sf:$benchmarkdir/dragon.json exclusiveshared"
  [44]="t s f sf:$benchmarkdir/dragon.json exclusivedirty"
  [45]="t s f sf:$benchmarkdir/dragon.json shareddirty"
  [46]="t s f sf tsf:$benchmarkdir/dragon.json dirtyshareddirty"
  [47]="t s f:$benchmarkdir/dragon.json deadlock"
  [48]="t s f tf sf ts tsf:$benchmarkdir/firefly.json dirtydirty"
  [49]="t s f tf sf ts tsf:$benchmarkdir/firefly.json exclusiveexclusive"
  [50]="t s f tf sf ts tsf:$benchmarkdir/firefly.json dirtyshared"
  [51]="t s f tf sf ts tsf:$benchmarkdir/firefly.json dirtyexclusive"
  [52]="t s f:$benchmarkdir/firefly.json deadlock"
  [53]="t s f tf sf ts tsf:$benchmarkdir/left-dining-philosophers.json deadlock"
  [54]="t s f sf:$benchmarkdir/lehmann-rabin.json deadlock"
  [55]="t s f sf:$benchmarkdir/synapse.json dirtydirty"
  [56]="t s f sf:$benchmarkdir/synapse.json dirtyvalid"
  [57]="t s f:$benchmarkdir/synapse.json deadlock"
  [58]="t s f:$benchmarkdir/token-passing-no-invariant.json notoken"
  [59]="t s f ts:$benchmarkdir/token-passing-no-invariant.json manytoken"
  [60]="t s f:$benchmarkdir/token-passing.json notoken"
  [61]="t s f:$benchmarkdir/token-passing.json manytoken"
)

TIMEOUT=1200

for INDEX in ${!INPUTS[@]}
do
  echo "${INDEX} of 61"
  MODES=$(echo ${INPUTS[${INDEX}]} | awk -F: "{print \$1}" - )
  FILEPROPERTY=$(echo ${INPUTS[${INDEX}]} | awk -F: "{print \$2}" - )
  FILEPROPERTYARRAY=($FILEPROPERTY)
  FILE=${FILEPROPERTYARRAY[0]}
  PROPERTY=${FILEPROPERTYARRAY[1]}
  for MODE in ${MODES}
    do
      # for now, only consider modes t, s, and f (e.g. tsf would be a convolution, need support in mata still...)
      if [[ "$MODE" =~ ^(s|t|f)$ ]]; then
        echo $FILE
        echo $MODE
        echo $PROPERTY
        time systemd-run --user --scope \
              -p MemoryMax=32G \
              -p MemorySwapMax=7G \
              timeout $TIMEOUT "${SOLVER}" -i $MODE --universality-alg explicit -p $PROPERTY $FILE
		echo "********************"
      fi
    done
done
