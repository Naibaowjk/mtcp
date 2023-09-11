#!/bin/bash

# 默认版本
VERSION="21.11"
# 是否跳过安装，默认是 false，也就是执行安装
SKIP_INSTALL=false

# 函数，用于显示帮助信息
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo "Install DPDK with specific versions."
    echo
    echo "Options:"
    echo "  -v [1|2]         Version of DPDK to install. 1 for 21.11, 2 for 22.03."
    echo "  -s, --skip       Skip the installation process."
    echo "  -h, --help       Display this help message and exit."
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case "$1" in
        -v)
            shift
            if [ "$1" == "1" ]; then
                VERSION="21.11"
            elif [ "$1" == "2" ]; then
                VERSION="22.03"
            else
                echo "Invalid version specified."
                exit 1
            fi
            ;;
        -s|--skip)
            SKIP_INSTALL=true
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "Invalid option: $1"
            show_help
            exit 1
            ;;
    esac
    shift
done

if [ "$SKIP_INSTALL" = false ]; then
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
else
    echo "Skipping DPDK installation..."
fi

# 设置环境变量
if [ "$VERSION" == "21.11" ]; then
    echo "export RTE_SDK=/usr/local/lib/x86_64-linux-gnu/dpdk/pmds-22.0/" >> ~/.bashrc
    echo "Setting RTE_SDK to /usr/local/lib/x86_64-linux-gnu/dpdk/pmds-22.0/"
elif [ "$VERSION" == "22.03" ]; then
    echo "export RTE_SDK=/usr/local/lib/x86_64-linux-gnu/dpdk/pmds-22.1/" >> ~/.bashrc
    echo "Setting RTE_SDK to /usr/local/lib/x86_64-linux-gnu/dpdk/pmds-22.1/"
else
    echo "Unknown version: $VERSION"
    exit 1
fi
echo "export RTE_TARGET=x86_64-native-linuxapp-gcc" >> ~/.bashrc
echo "Setting RTE_TARGET to x86_64-native-linuxapp-gcc"


# 应用新的环境变量设置
source ~/.bashrc
