#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../..
SOLUTION_FOLDER=${BASE_DIR}/sprint3/problems/find_return/solution

bash ${SCRIPT_FOLDER}/build.sh || exit 1

source ${BASE_DIR}/.venv/bin/activate

export CONFIG_PATH=${SOLUTION_FOLDER}/data/config.json

export IMAGE_NAME=find_return
export ENTRYPOINT=/app/game_server
export CONTAINER_ARGS='--config-file /app/data/config.json --www-root /app/static/'

pytest --workers auto --junitxml=${BASE_DIR}/find_return.xml ${BASE_DIR}/cpp-backend-tests-practicum/tests/test_s03_find_return.py
