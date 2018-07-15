#!/bin/bash

function emit-llvm {
    clang -S -emit-llvm -O1 -Xclang -disable-llvm-passes $@
}

emit-llvm $@