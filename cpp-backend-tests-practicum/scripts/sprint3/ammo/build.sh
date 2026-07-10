#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")
BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint3/problems/ammo/solution

cp ${BASE_DIR}/cpp-backend-tests-practicum/tests/test_s03_ammo.py ${SCRIPT_FOLDER}/test_s03_ammo.py
cp -r ${SOLUTION_FOLDER} ${SCRIPT_FOLDER}/solution

docker build -t ammo ${SCRIPT_FOLDER}