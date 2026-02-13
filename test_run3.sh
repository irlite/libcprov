#!/usr/bin/env bash

./build/injector/prov start --path "."
./build/injector/prov exec "python3 ./test_scripts/ML/vllm_test.py" --path "."
./build/injector/prov end
