#!/usr/bin/env bash

./build/injector/prov start --path "/dev/shm/ts1/"
./build/injector/prov exec "./test_scripts/first_exec" --path "/dev/shm/ts1/"
./build/injector/prov exec "./test_scripts/second_exec" --path "/dev/shm/ts1/"
./build/injector/prov end
