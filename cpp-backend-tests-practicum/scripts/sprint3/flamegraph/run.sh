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

bash ${SCRIPT_FOLDER}/build.sh || exit 1

source ${BASE_DIR}/.venv/bin/activate

cd ${SOLUTION_FOLDER} || exit 1

export DIRECTORY=${SOLUTION_FOLDER}

sudo python3 shoot.py "${MAP_JSON_PROGRAM} ${MAP_JSON_CONFIG}" || exit 1

sudo chown -R $USER:$USER ${SOLUTION_FOLDER}

pytest --junitxml=${BASE_DIR}/flamegraph.xml ${BASE_DIR}/cpp-backend-tests-practicum/tests/test_s03_flamegraph.py
