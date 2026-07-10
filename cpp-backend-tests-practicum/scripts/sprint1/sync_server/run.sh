#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint1/problems/sync_server/solution

bash ${SCRIPT_FOLDER}/build.sh || exit 1

source ${BASE_DIR}/.venv/bin/activate
export COMMAND_RUN=${SOLUTION_FOLDER}/build/bin/hello

python3 -m pytest --rootdir=${BASE_DIR} --verbose --junitxml=${BASE_DIR}/sync_server.xml ${BASE_DIR}/cpp-backend-tests-practicum/tests/test_l02_hello_beast.py
