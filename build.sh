#!/bin/bash

rm -rf /tmp/sdk2-agents /tmp/ivs-device-sdk2 /tmp/libs.tar /ivs-build/*
mkdir -p /tmp/sdk2-agents
mkdir -p /tmp/ivs-device-sdk2
mkdir -p /ivs-build/libs

cd /tmp/sdk2-agents
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=/ivs/sdk2-agents/toolchain.txt /ivs/sdk2-agents && make

if [[ ! -z "${BUILD_IFLYOS_SDK}" ]]; then
  cd /tmp/ivs-device-sdk2
  cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=/ivs/sdk2-agents/toolchain.txt -DBUILD_TESTING=OFF -DBLUETOOTH_BLUEZ=OFF -DOPUS=ON /ivs/ivs-device-sdk2
  make VERBOSE=1 -j6 && make DESTDIR=$PWD/out install
fi

#collect artifacts for runtime
cd /rpxc/sysroot/lib
tar -cvf /tmp/libs.tar libnghttp2.so* libcurl.so* libportaudio.so*

if [[ ! -z "${BUILD_IFLYOS_SDK}" ]]; then
  cd /tmp/ivs-device-sdk2/out/usr/local/lib
  tar --append --file /tmp/libs.tar *.so
  cp /tmp/ivs-device-sdk2/out/usr/bin/* /ivs-build/
fi

cd  /ivs-build/libs/
tar -xvf /tmp/libs.tar
cp /tmp/sdk2-agents/MicAgent/MicAgent /ivs-build
cp /tmp/sdk2-agents/IFLYOSUmbrella/IFLYOSUmbrella /ivs-build
cp -r /ivs/sdk2-agents/patchFiles/* /ivs-build

if [[ ! -z "${BUILD_IFLYOS_SDK}" ]]; then
  cd /ivs-build
  tar -czvf ivs-release.tar.gz *
fi

