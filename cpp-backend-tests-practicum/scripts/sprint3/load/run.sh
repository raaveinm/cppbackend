#!/bin/bash

function real_dir() {
  pushd "$1" >/dev/null
  pwd -P
  popd >/dev/null
}
SCRIPT_FOLDER=$(real_dir "$(dirname "$0")")

BASE_DIR=${SCRIPT_FOLDER}/../../../../
SOLUTION_FOLDER=${BASE_DIR}/sprint3/problems/load/solution

bash ${SCRIPT_FOLDER}/build.sh || exit 1

source ${BASE_DIR}/.venv/bin/activate

NETWORK_NAME=ammo_network
docker network rm ${NETWORK_NAME}
NETWORK_ID=$(docker network create ${NETWORK_NAME})

docker run --rm -d --network=${NETWORK_ID} --name=cppserver praktikumcpp/game_server:latest

export DIRECTORY=/solution/logs

docker run --rm --privileged --network=${NETWORK_ID} -e DIRECTORY=${DIRECTORY} load

docker container stop cppserver && docker network rm ${NETWORK_NAME}

rm -r ${SCRIPT_FOLDER}/solution ${SCRIPT_FOLDER}/test_s03_load.py