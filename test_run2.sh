#!/usr/bin/env bash

./injector/build/prov start --path "."
#./injector/build/prov exec "./test_scripts/test_hooks_synthetic" --path "/dev/shm/prov_test"
#./injector/build/prov exec "./test_scripts/test_hooks_synthetic_clang" --path "/dev/shm/prov_test"
#./injector/build/prov exec "python3 ./test_scripts/test_hooks_synthetic.py" --path "/dev/shm/prov_test"
#./injector/build/prov exec "Rscript ./test_scripts/test_hooks_synthetic.r" --path "/dev/shm/prov_test"
./injector/build/prov exec "julia ./test_scripts/test_hooks_synthetic.jl" --path "/dev/shm/prov_test"
#./injector/build/prov exec "./test_scripts/test_hooks_synthetic_go" --path "/dev/shm/prov_test"
#./injector/build/prov exec "./test_scripts/test_fork_execv" --path "/dev/shm/execv_fail_test"
#./injector/build/prov exec "./test_scripts/test_multithreading" --path "/dev/shm/prov_test_mt_mp"
./injector/build/prov end
