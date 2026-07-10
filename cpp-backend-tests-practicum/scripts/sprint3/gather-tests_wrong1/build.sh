#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint3/problems/gather-tests/solution
CPP_FOLDER=${BASE_DIR}/cpp-backend-tests-practicum/tests/cpp/test_s03_gather-tests

cp -r ${CPP_FOLDER}/* ${SOLUTION_FOLDER}/src/
cp ${CPP_FOLDER}/wrong/collision_detector.cpp ${SOLUTION_FOLDER}/src/
cp ${CPP_FOLDER}/wrong/CMakeLists.txt ${SOLUTION_FOLDER}/
cp ${CPP_FOLDER}/wrong/Dockerfile1 ${SOLUTION_FOLDER}/Dockerfile

docker build -t gather-tests_wrong1 ${SOLUTION_FOLDER}