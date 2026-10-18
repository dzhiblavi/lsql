#!/usr/bin/env bash
set -euxo pipefail

git clone https://github.com/Genivia/RE-flex
cd RE-flex/

autoreconf -fi
./configure
make -j $(nproc)
make install
