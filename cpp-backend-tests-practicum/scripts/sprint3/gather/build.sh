#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint3/problems/gather/solution
CPP_FOLDER=${BASE_DIR}/cpp-backend-tests-practicum/tests/cpp/test_s03_gather

cp -r ${CPP_FOLDER}/* ${SOLUTION_FOLDER}/src/
docker build -t gather ${SOLUTION_FOLDER}
