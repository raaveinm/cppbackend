#!/usr/bin/env bash

set -euo pipefail

echo "system upgrade"
sudo apt-get update
sudo apt-get install -y \
    python3 \
    python3-pip \
    python3-venv \
    python3-dev \
    build-essential \
    libffi-dev \
    gfortran \
    libssl-dev \
    git \
    cmake \
    libboost-all-dev \
    libev-dev \
    libssl-dev

echo "phantom installation"
BUILD_DIR=$(mktemp -d)
cd "$BUILD_DIR"

git clone https://github.com/yandex-load/phantom.git
cd phantom
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install

cd "$HOME"
rm -rf "$BUILD_DIR"

echo "venv initialization"
VENV_PATH="$HOME/yandex-tank-venv"
python3 -m venv "$VENV_PATH"

source "$VENV_PATH/bin/activate"

pip install --upgrade pip setuptools wheel
pip install yandex-tank

echo ""
echo "=========================================="
echo " installation completed"
echo " source $VENV_PATH/bin/activate"
echo "=========================================="