#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint3/problems/load/solution

export DIRECTORY=${SOLUTION_FOLDER}/logs
cd ${SOLUTION_FOLDER}
yandex-tank -c ${BASE_DIR}/sprint3/problems/load/solution/load.yaml ${BASE_DIR}/sprint3/problems/load/solution/ammo.txt
pytest --junitxml=${BASE_DIR}/load.xml ${BASE_DIR}/cpp-backend-tests-practicum/tests/test_s03_load.py
