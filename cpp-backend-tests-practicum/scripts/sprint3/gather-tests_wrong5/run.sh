#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint3/problems/gather-tests/solution

bash ${SCRIPT_FOLDER}/build.sh || exit 1

docker run --rm --entrypoint /app/build/collision_detection_tests gather-tests_wrong5  || [ $? -ne 0 ]