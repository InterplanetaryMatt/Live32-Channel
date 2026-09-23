#!/usr/bin/env bash
set -euo pipefail
c++ -std=c++17 -O2 Source/DSP/Live32ChannelDSP.cpp tests/dsp_smoke.cpp -o live32_dsp_test
./live32_dsp_test
