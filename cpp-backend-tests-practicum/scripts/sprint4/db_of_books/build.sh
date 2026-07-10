#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint4/problems/db_of_books/solution

source ${BASE_DIR}/.venv/bin/activate

cd ${SOLUTION_FOLDER} || exit 1
mkdir -p build
cd build
conan install .. --build=missing -s compiler.libcxx=libstdc++11 -s build_type=Release
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j $(nproc)