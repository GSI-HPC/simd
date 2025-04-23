# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright © 2025      GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
#                       Matthias Kretz <m.kretz@gsi.de>

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get update && apt-get install -y --no-install-recommends \
    software-properties-common \
    wget \
    curl \
    ca-certificates \
    gnupg \
    lsb-release \
    make \
    cmake \
    git \
    zsh \
    build-essential \
    ninja-build \
    python3 \
    python3-pip \
    gcc-13 g++-13 g++-13-multilib \
    gcc-14 g++-14 g++-14-multilib \
    file m4 libtool autoconf2.69 dwz gawk lzma xz-utils libzstd-dev zlib1g-dev systemtap-sdt-dev \
    binutils:native binutils-hppa64-linux-gnu:native gperf bison flex gettext nvptx-tools amdgcn-tools-18 \
    texinfo locales-all sharutils procps netbase libisl-dev libmpc-dev libmpfr-dev libgmp-dev lib32z1-dev libx32z1-dev \
    coreutils chrpath lsb-release time pkgconf && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 140 --slave /usr/bin/g++ g++ /usr/bin/g++-14 && \
    wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 20 && \
    ./llvm.sh 21 && \
    rm llvm.sh

# GCC 15
RUN mkdir -p /root/gcc-15/obj && \
    cd /root/gcc-15 && \
    curl -L https://github.com/gcc-mirror/gcc/archive/releases/gcc-15.tar.gz \
    | tar -xz --strip-components=1 && \
    cd /root/gcc-15/obj && ../configure --enable-languages=c,c++,lto --prefix=/opt/gcc-15 --disable-checking --disable-bootstrap && \
    make -j3 && \
    make -j3 install && \
    update-alternatives --install /usr/bin/gcc gcc /opt/gcc-15/bin/gcc 150 --slave /usr/bin/g++ g++ /opt/gcc-15/bin/g++ && \
    cd /root && rm -rf gcc-15

# GCC master
RUN mkdir -p /root/gcc-master/obj && \
    cd /root/gcc-master && \
    curl -L https://github.com/gcc-mirror/gcc/archive/master.tar.gz \
    | tar -xz --strip-components=1 && \
    cd /root/gcc-master/obj && ../configure --enable-languages=c,c++,lto --prefix=/opt/gcc-master --disable-checking --disable-bootstrap && \
    make -j3 && \
    make -j3 install && \
    cd /root && rm -rf gcc-master

CMD ["/bin/bash"]

