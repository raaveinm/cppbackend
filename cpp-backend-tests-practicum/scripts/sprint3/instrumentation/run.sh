#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint3/problems/instrumentation/solution

bash ${SCRIPT_FOLDER}/build.sh || exit 1

source ${BASE_DIR}/.venv/bin/activate

export REPORT_PATH=${SOLUTION_FOLDER}/report
export BINARY_PATH=${SOLUTION_FOLDER}/event2dot
export ARG=${SOLUTION_FOLDER}/inputs

pytest --junitxml=${BASE_DIR}/instrumentation.xml ${BASE_DIR}/cpp-backend-tests-practicum/tests/test_s03_instrumentation.py
