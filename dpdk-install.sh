#!/bin/bash

# 默认版本
VERSION="21.11"

# 解析命令行参数
while getopts "v:" OPTION; do
    case $OPTION in
        v)
            if [ "$OPTARG" == "1" ]; then
                VERSION="21.11"
            elif [ "$OPTARG" == "2" ]; then
                VERSION="22.03"
            else
                echo "Invalid version specified."
                exit 1
            fi
            ;;
        *)
            echo "Usage: $0 [-v version]"
            exit 1
            ;;
    esac
done

echo "Installing DPDK version: $VERSION"

# 下载和安装 DPDK
rm -rf dpdk-$VERSION
wget https://github.com/DPDK/dpdk/archive/refs/tags/v$VERSION.tar.gz
tar -xzvf v$VERSION.tar.gz
cd dpdk-$VERSION

meson setup build
cd build
meson configure -Dbuildtype=debug -Dmax_lcores=4
ninja
sudo ninja install
sudo ldconfig

# export RTE in bashrc
if [ "$VERSION" == "21.11" ]; then
    echo "export RTE_SDK=/usr/local/lib/x86_64-linux-gnu/dpdk/pmds-22.0/" >> ~/.bashrc
elif [ "$VERSION" == "22.03" ]; then
    echo "export RTE_SDK=/usr/local/lib/x86_64-linux-gnu/dpdk/pmds-22.1/" >> ~/.bashrc
else
    echo "Unknown version: $VERSION"
    exit 1
fi
echo "export RTE_TARGET=x86_64-native-linuxapp-gcc" >> ~/.bashrc

source ~/.bashrc
