#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint2/problems/static_content/solution

bash ${SCRIPT_FOLDER}/build.sh || exit 1

source ${BASE_DIR}/.venv/bin/activate

export CONFIG_PATH=${SOLUTION_FOLDER}/data/config.json

export DELIVERY_APP=${SOLUTION_FOLDER}/build/bin/game_server
export DATA_PATH=${SOLUTION_FOLDER}/static/

export COMMAND_RUN="${DELIVERY_APP} ${CONFIG_PATH} ${DATA_PATH}"

python3 -m pytest --rootdir=${BASE_DIR} --verbose --junitxml=${BASE_DIR}/static_content.xml ${BASE_DIR}/cpp-backend-tests-practicum/tests/test_s02_static_content.py
