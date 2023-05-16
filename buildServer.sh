#!/bin/bash -l
currDir=`pwd`
arch=`uname -p`

buildType="Debug"
if [ $# -eq 0 ]
then
	buildType="Debug"
else
	buildType="Release"
fi

set -x 
if [ $arch = "arm" ]
then
	cmake CMakeLists.txt -B`pwd`/build/ -DCMAKE_BUILD_TYPE=$buildType -DONNX_EXECUTOR=1 -D CMAKE_CXX_COMPILER=g++ -DMACOS=1
	ln -sf $currDir/libLinux/onnx/lib/libonnxruntime.dylib $currDir/libLinux/onnx/lib/libonnxruntime.1.13.1.dylib 
	install_name_tool -change @rpath/libonnxruntime.1.13.1.dylib $currDir/libLinux/onnx/lib/libonnxruntime.dylib nimbleclient
elif [ $arch = "x86_64" ]
then
    cmake CMakeLists.txt -B`pwd`/build/ -DCMAKE_BUILD_TYPE=$buildType -DONNX_EXECUTOR=1 -D CMAKE_CXX_COMPILER=g++
else
	cmake CMakeLists.txt -B`pwd`/build/ -DCMAKE_BUILD_TYPE=$buildType -DONNX_EXECUTOR=1 -D CMAKE_CXX_COMPILER=g++ -DMACOS=1
fi
cd build/
sudo apt-get install libcurl4-openssl-dev
make
cd ..
