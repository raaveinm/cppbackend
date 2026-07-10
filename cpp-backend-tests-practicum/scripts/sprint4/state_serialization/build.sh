#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint4/problems/state_serialization/solution
VOLUME_DIR=${SOLUTION_FOLDER}/../volume

docker build -t state_serialization ${SOLUTION_FOLDER}

mkdir -p ${VOLUME_DIR}
chmod 777 ${VOLUME_DIR}
