#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../..
SOLUTION_FOLDER=${BASE_DIR}/sprint2/problems/join_game/solution

docker build -t join_game ${SOLUTION_FOLDER}