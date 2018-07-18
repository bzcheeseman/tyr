#!/usr/bin/env bash

../scripts/emit-llvm.sh example.cpp
opt -load ../cmake-build-debug/src/compiler/libFindMarkedFunctionsPass.so -find-marked example.ll -S > /dev/null
