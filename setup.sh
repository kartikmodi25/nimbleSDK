#!/bin/bash -l

set -x

arch=`uname -p`
os=`uname -s`

if [ $arch = "arm" ]
then 
    mkdir onnxlib
    wget https://www.nuget.org/api/v2/package/Microsoft.ML.OnnxRuntime/1.13.1
    unzip 1.13.1 -d onnxlib
    mkdir -p libLinux/onnx/include
    mkdir -p libLinux/onnx/lib 
    mv onnxlib/build/native/include/* libLinux/onnx/include/
    mv onnxlib/runtimes/osx.10.14-arm64/native/* libLinux/onnx/lib/
    rm -rf onnxlib/ 
    rm -rf 1.13.1
elif [ $arch = "x86_64" ]
then
    mkdir onnxlib
    wget https://github.com/microsoft/onnxruntime/releases/download/v1.13.1/onnxruntime-linux-x64-1.13.1.tgz
    tar -xvf onnxruntime-linux-x64-1.13.1.tgz
    mkdir -p libLinux
    mv onnxruntime-linux-x64-1.13.1 libLinux/onnx
    rm -f onnxruntime-linux-x64-1.13.1.tgz
else 
    mkdir onnxlib
    wget https://github.com/microsoft/onnxruntime/releases/download/v1.13.1/onnxruntime-osx-x86_64-1.13.1.tgz
    tar -xvf onnxruntime-osx-x86_64-1.13.1.tgz
    mkdir -p libLinux
    mv onnxruntime-osx-x86_64-1.13.1 libLinux/onnx
    rm -f onnxruntime-osx-x86_64-1.13.1.tgz
fi
