#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint3/problems/flamegraph/solution

MAP_JSON_FOLDER=${BASE_DIR}/sprint1/problems/map_json/solution
MAP_JSON_CONFIG=${MAP_JSON_FOLDER}/data/config.json
MAP_JSON_PROGRAM=${MAP_JSON_FOLDER}/build/bin/game_server

source ${BASE_DIR}/.venv/bin/activate

cd ${SCRIPT_FOLDER}/../..
./sprint1/map_json/build.sh || exit 1


cd ${SOLUTION_FOLDER} || exit 1

FOLDER=FlameGraph
if ! git clone https://github.com/brendangregg/FlameGraph; then
  (
  cd "${FOLDER}" || exit 1
  git pull
  )
fi

chmod +x ${MAP_JSON_PROGRAM}
