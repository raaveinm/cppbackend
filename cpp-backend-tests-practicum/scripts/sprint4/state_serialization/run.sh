#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint4/problems/state_serialization/solution
VOLUME_DIR=${BASE_DIR}/sprint4/problems/state_serialization/volume

bash ${SCRIPT_FOLDER}/build.sh || exit 1

source ${BASE_DIR}/.venv/bin/activate

export CONFIG_PATH=${SOLUTION_FOLDER}/data/config.json

export VOLUME_PATH=${VOLUME_DIR}
export IMAGE_NAME=state_serialization

pytest --junitxml=${BASE_DIR}/state_serialization.xml ${BASE_DIR}/cpp-backend-tests-practicum/tests/test_s04_state_serialization.py
