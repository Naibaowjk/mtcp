#!/bin/bash
./epserver -p ./file -f epserver-multiprocess.conf -N 1 -c 0
./epserver -p ./file -f epserver-mutliprocess.conf -N 1 -c 1
