#!/bin/bash
IP_ADDRESS=${1:-"10.0.0.4"}

# 首先运行第一个epwget命令
./epwget $IP_ADDRESS/1.txt 10000000 -n 2 -c 1000 -f epwget-multiprocess.conf
./epwget $IP_ADDRESS/1.txt 10000000 -n 3 -c 1000 -f epwget-multiprocess.conf

# 使用循环运行其余的epwget命令
# for i in {1..7}
# do
#     ./epwget 10.0.0.43/example.txt 1000000 -n $i -c 1000 -f epwget-multiprocess.conf
# done

# 脚本结束
