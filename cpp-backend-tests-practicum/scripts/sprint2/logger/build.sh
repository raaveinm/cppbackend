#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../..
SOLUTION_FOLDER=${BASE_DIR}/sprint2/problems/logger/solution

source ${BASE_DIR}/.venv/bin/activate

cd ${SOLUTION_FOLDER} || exit 1

cp ${BASE_DIR}/cpp-backend-tests-practicum/tests/cpp/test_s02_logger/main.cpp ${SOLUTION_FOLDER}

mkdir -p build
cd build
conan install ..
cmake ..
cmake --build . -j $(nproc)
